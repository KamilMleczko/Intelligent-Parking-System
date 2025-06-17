#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
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

// light weight ip (TCP IP)
#include <esp_netif_sntp.h>
#include <time.h>
//other
#include "credentials.h"
#include "esp_sntp.h"
#include "lwip/err.h"  //error handling
#include "lwip/netdb.h"
#include "lwip/sockets.h"  //sockets
#include "lwip/sys.h"      //system applications
#include "utils.h"
#include "wifi.h"
#include "esp_err.h"
#include "ws_listener.h"
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "sg90.h"
#include "my_ssd1306.h"
#include "app_context.h"


typedef struct {
  const ssd1306_config_t *config;  // Pointer to the SSD1306 configuration
  i2c_handler_t *i2c_handler;      // Pointer to the I2C handler
  oled_display_t *oled_display;    // Pointer to the OLED displayAdd commentMore actions
} TaskParameters;




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
  nvs_init();
  ESP_LOGI(LOG_HW, "Hardware services initialized");
}


void app_main(void) {
    // 1) Initialize hardware services (NVS, logging)
    ESP_LOGI(TAG, "Initializing hardware services");
    nvs_flash_init();
    init_hw_services();

    // 2) Start Wi-Fi (AP+STA provisioning UI into SoftAP)
    ESP_LOGI(TAG, "Starting Wi-Fi provisioning");
    init_wifi();

    // 3) Synchronize time via SNTP
    ESP_LOGI(TAG, "Configuring system time");
    configure_time();

    // 4) Initialize shared application context (OLED, servo, etc.)
    ESP_LOGI(TAG, "Initializing application context");
    app_context_init();

    // Display initial closed state
    clear_screen(&app_ctx.oled_cfg, &app_ctx.oled_disp, &app_ctx.oled_i2c);
    show_text(&app_ctx.oled_cfg, &app_ctx.oled_disp, &app_ctx.oled_i2c, 3, (char*)"Gate Closed");

    // 5) Start passive WebSocket listener for gate commands
    ESP_LOGI(TAG, "Starting WebSocket gate listener");
    if (!start_ws_listener()) {
        ESP_LOGE(TAG, "Failed to start WebSocket listener");
        // Optionally blink LED or reset
    }

    // 6) Main task can sleep forever; WS events drive the rest
    ESP_LOGI(TAG, "Setup complete. Entering idle loop.");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}