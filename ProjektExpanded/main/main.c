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
// Camera
//#include "esp_camera.h"

#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

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

  // camera_config_t config = {
  //   .pin_pwdn = PWDN_GPIO_NUM,
  //   .pin_reset = RESET_GPIO_NUM,
  //   .pin_xclk = XCLK_GPIO_NUM,
  //   .pin_sscb_sda = SIOD_GPIO_NUM,
  //   .pin_sscb_scl = SIOC_GPIO_NUM,

  //   .pin_d7 = Y9_GPIO_NUM,
  //   .pin_d6 = Y8_GPIO_NUM,
  //   .pin_d5 = Y7_GPIO_NUM,
  //   .pin_d4 = Y6_GPIO_NUM,
  //   .pin_d3 = Y5_GPIO_NUM,
  //   .pin_d2 = Y4_GPIO_NUM,
  //   .pin_d1 = Y3_GPIO_NUM,
  //   .pin_d0 = Y2_GPIO_NUM,
  //   .pin_vsync = VSYNC_GPIO_NUM,
  //   .pin_href = HREF_GPIO_NUM,
  //   .pin_pclk = PCLK_GPIO_NUM,

  //   .xclk_freq_hz = 20000000,
  //   .ledc_timer = LEDC_TIMER_0,
  //   .ledc_channel = LEDC_CHANNEL_0,

  //   .pixel_format = PIXFORMAT_JPEG, // For testing; can use RGB565 or GRAYSCALE
  //   .frame_size = FRAMESIZE_QVGA,   // Small frame size for quick test
  //   .jpeg_quality = 12,             // Lower is better quality
  //   .fb_count = 1
  // };
  // esp_err_t err = esp_camera_init(&config);
  // if (err != ESP_OK) {
  //     ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
  //     return;
  // }

  //   // Capture a frame
  // camera_fb_t *fb = esp_camera_fb_get();
  // if (!fb) {
  //     ESP_LOGE(TAG, "Camera capture failed");
  // } else {
  //     ESP_LOGI(TAG, "Captured a frame! Size: %zu bytes", fb->len);
  //     esp_camera_fb_return(fb);
  // }
}
