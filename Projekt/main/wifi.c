#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// miscellaneous
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
// do zad2 lab1
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_tls.h"

// light weight ip (TCP IP)
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "credentials.h"
#include "lwip/err.h"  //error handling
#include "lwip/netdb.h"
#include "lwip/sockets.h"  //sockets
#include "lwip/sys.h"      //system applications
#include "mqtt_utils.h"
#include "ultrasonic.h"
#include "utils.h"
#include "wifi.h"

#define WIFI_DIODE 17

bool is_connected_to_wifi = false;
bool wifi_setup_done = false;
bool got_new_wifi_credentials = false;

TaskHandle_t wifi_reconnect_task_handle = NULL;

void wifi_diode_blink(void) {
  gpio_set_level(WIFI_DIODE, 1);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(WIFI_DIODE, 0);
  vTaskDelay(100);
}

void init_sta(void) {
  ESP_LOGI(LOG_WIFI, "Initializing station mode");
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_provisioning_event_handler(void* arg,
                                            esp_event_base_t event_base,
                                            int event_id, void* event_data) {
  if (event_base == WIFI_PROV_EVENT) {
    switch (event_id) {
      case WIFI_PROV_START:
        ESP_LOGI(LOG_PROV, "Provisioning started");
        break;
      case WIFI_PROV_CRED_RECV: {
        wifi_sta_config_t* wifi_sta_cfg = (wifi_sta_config_t*)event_data;
        ESP_LOGI(LOG_PROV,
                 "Received Wi-Fi credentials"
                 "\n\tSSID     : %s\n\tPassword : %s",
                 (const char*)wifi_sta_cfg->ssid,
                 (const char*)wifi_sta_cfg->password);
        break;
      }
      case WIFI_PROV_CRED_FAIL: {
        wifi_prov_sta_fail_reason_t* reason =
            (wifi_prov_sta_fail_reason_t*)event_data;
        ESP_LOGE(LOG_PROV,
                 "Provisioning failed!\n\tReason : %s"
                 "\n\tPlease reset to factory and retry provisioning",
                 (*reason == WIFI_PROV_STA_AUTH_ERROR)
                     ? "Wi-Fi station authentication failed"
                     : "Wi-Fi access-point not found");
        break;
      }
      case WIFI_PROV_CRED_SUCCESS:
        ESP_LOGI(LOG_PROV, "Provisioning successful");
        got_new_wifi_credentials = true;
        esp_wifi_connect();
        break;
      case WIFI_PROV_END:
        // ESP_LOGI(LOG_PROV, "Provisioning ended but service will continue
        // running");
        wifi_prov_mgr_deinit();
        break;
      default:
        break;
    }
  }
}

esp_err_t max_people_endpoint_handler(uint32_t session_id, const uint8_t* inbuf,
                                      ssize_t inlen, uint8_t** outbuf,
                                      ssize_t* outlen, void* priv_data) {
  ESP_LOGI(LOG_PROV, "max_people_endpoint_handler");

  if (inbuf) {
    ESP_LOGI(LOG_PROV, "Received data: %.*s", inlen, (char*)inbuf);
    int read_ultrasonic_max_people = atoi((char*)inbuf);
    if (read_ultrasonic_max_people > 0) {
      save_max_people(read_ultrasonic_max_people);
    }
  }
  char response[] = "SUCCESS";
  *outbuf = (uint8_t*)strdup(response);
  if (*outbuf == NULL) {
    ESP_LOGE(TAG, "System out of memory");
    return ESP_ERR_NO_MEM;
  }
  *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

  return ESP_OK;
}

void start_wifi_provisioning(void) {
  ESP_LOGI(LOG_WIFI, "Starting WiFi provisioning");
  const char* pop = WIFI_PROV_POP;

  wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
  uint8_t custom_service_uuid[] = {
      /* LSB <---------------------------------------
       * ---------------------------------------> MSB */
      0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
      0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
  };

  wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
  ESP_ERROR_CHECK(
      wifi_prov_mgr_start_provisioning(security, pop, DEVICE_NAME, NULL));

  wifi_prov_mgr_endpoint_register("maxPeople", max_people_endpoint_handler,
                                  NULL);

  esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                             &wifi_provisioning_event_handler, NULL);

  ESP_LOGI(LOG_WIFI, "WiFi provisioning started");
}

void wifi_event_handler(void* event_handler_arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START:
        ESP_LOGI(LOG_WIFI, "WIFI_EVENT_STA_START");
        esp_wifi_connect();
        if (wifi_reconnect_task_handle != NULL) {
          vTaskDelete(wifi_reconnect_task_handle);
          wifi_reconnect_task_handle = NULL;
        }
        break;
      case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(LOG_WIFI, "WIFI_EVENT_STA_CONNECTED");

        break;
      case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(LOG_WIFI, "WIFI_EVENT_STA_DISCONNECTED");
        is_connected_to_wifi = false;
        if (wifi_reconnect_task_handle == NULL) {
          xTaskCreate(&wifi_reconnect_task, "wifi_reconnect_task", 4096, NULL,
                      5, &wifi_reconnect_task_handle);
        }
        break;
      default:
        break;
    }
  } else if (event_base == IP_EVENT) {
    switch (event_id) {
      case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(LOG_WIFI, "IP_EVENT_STA_GOT_IP");
        is_connected_to_wifi = true;
        break;
      default:
        break;
    }
  }
}

void wifi_reconnect_task(void* pvParameters) {
  esp_wifi_connect();
  while (true) {
    if (!is_connected_to_wifi) {
      ESP_LOGI(LOG_WIFI, "Reconnecting to WiFi");
      esp_wifi_connect();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL);
}

void init_wifi_services(void) {
  ESP_LOGI(LOG_WIFI, "Initializing WiFi and TCP");
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_initiation));
  ESP_LOGI(LOG_WIFI, "WiFi initialized");
}

void initialize_wifi_event_handlers(void) {
  ESP_LOGI(LOG_WIFI, "Initializing event handlers");
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));
  // Duplicate event handler registration removed
  ESP_LOGI(LOG_WIFI, "Event handlers initialized");
}

void initialize_wifi_provider_manager(void) {
  ESP_LOGI(LOG_PROV, "Initializing WiFi provisioning manager");
  wifi_prov_mgr_config_t config = {
      .scheme = wifi_prov_scheme_ble,
      .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
  };
  ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
  wifi_prov_mgr_endpoint_create("maxPeople");

  ESP_LOGI(LOG_PROV, "WiFi provisioning manager initialized");
}

void init_wifi(void) {
  init_wifi_services();
  initialize_wifi_event_handlers();
  initialize_wifi_provider_manager();
  init_device_name();
  init_mqtt_topics();
  bool provisioned = false;

  wifi_config_t wifi_config;
  esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
  ESP_LOGI(LOG_WIFI, "Current WiFi configuration: SSID: %s, Password: %s",
           (char*)wifi_config.sta.ssid, (char*)wifi_config.sta.password);

  start_wifi_provisioning();
  vTaskDelay(pdMS_TO_TICKS(60 * 1000));  // wait for 1.5 min for provisioning.
  if (!got_new_wifi_credentials) {
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    init_sta();
    esp_wifi_connect();
  }
}
