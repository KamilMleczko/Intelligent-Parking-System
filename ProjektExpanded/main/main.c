#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// miscellaneous
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"  //delay,mutexx,semphr i rtos
#include "freertos/task.h"
#include "nvs_flash.h"  //non volatile storage

// biblioteki esp
#include "esp_event.h"  //wifi event
#include "esp_log.h"    //pokazywanie logów
#include "esp_system.h"
#include "esp_wifi.h"  //wifi functions and operations
// do zad2 lab1
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_tls.h"

// light weight ip (TCP IP)
#include <esp_netif_sntp.h>
#include <time.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "credentials.h"
#include "esp_bt.h"
#include "esp_sntp.h"
#include "lwip/err.h"  //error handling
#include "lwip/netdb.h"
#include "lwip/sockets.h"  //sockets
#include "lwip/sys.h"      //system applications
#include "mqtt_utils.h"
#include "utils.h"
#include "wifi.h"
#include "esp_err.h"
// oled lib
#include "my_ssd1306.h"
#define tag "SSD1306"
// HC-SR04 Sensor

TaskHandle_t mqtt_task_handle = NULL;


/*
 * @brief Function that configures time settings using SNTP.
 * It sets timezone to CEST.
 * @warning it's a blocking call. It waits for 10s for SNTP response.
 */
void configure_time() {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);
  if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
    printf("Failed to update system time within 10s timeout");
  }
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();
}

void nvs_init() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
}

void init_hw_services(void) {
  ESP_LOGI(LOG_HW, "Initializing hardware services");
  gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
  nvs_init();
  ESP_LOGI(LOG_HW, "Hardware services initialized");
}

void app_main(void) {

  ESP_LOGI(LOG_HW, "Starting application main function");
}
