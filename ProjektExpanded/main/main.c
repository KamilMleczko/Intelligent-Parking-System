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
TaskHandle_t mqtt_task_handle = NULL;
#define DEFAULT_MAX_PEOPLE 30
// oled lib
#include "my_ssd1306.h"
#define tag "SSD1306"
// Camera
#include "esp_camera.h"
#include "esp_http_client.h"

camera_fb_t *current_frame = NULL;

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 20, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};
static esp_err_t init_camera(void)
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_HW, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}
#endif
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
  nvs_init();
  ESP_LOGI(LOG_HW, "Hardware services initialized");
}


void camera_task(void *pvParameters) {
  while (true) {
      // Free the previous frame if it exists
      if (current_frame) {
          esp_camera_fb_return(current_frame);
          ESP_LOGI("CAMERA", "freeing last frame");
      }

      // Grab a new frame
      current_frame = esp_camera_fb_get();
      if (!current_frame) {
          ESP_LOGE("CAMERA", "Camera capture failed");
      }
      else{
        ESP_LOGI("CAMERA", "Camera capture succesfull");
      }

      // Sleep a bit (adjust based on your needs)
      vTaskDelay(5000 / portTICK_RATE_MS);
  }
}

void init_camera_capture() {
  if (init_camera() == ESP_OK) {
      // Take first frame to avoid VSYNC overflow spam
      camera_fb_t *pic = esp_camera_fb_get();
      esp_camera_fb_return(pic);
      vTaskDelay(5000 / portTICK_RATE_MS);
      // Start the camera grabbing task
      xTaskCreatePinnedToCore(camera_task, "camera_task", 4096, NULL, 5, NULL, 0);
      ESP_LOGI("CAMERA", "Camera capture task started");
  } else {
      ESP_LOGE("CAMERA", "Camera init failed!");
  }
}

void take_photo_and_upload(void) {
  if (!current_frame) {
      ESP_LOGE("UPLOAD", "No frame available to upload");
      return;
  }

  esp_http_client_config_t config = {
      .url = SERVER_URL,
      .timeout_ms = 10000, // Optional: timeout for connection
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL) {
      ESP_LOGE("UPLOAD", "Failed to initialize HTTP client");
      return;
  }

  const char *boundary = "----ESP32Boundary";
  char start_form[512];
  snprintf(start_form, sizeof(start_form),
           "--%s\r\n"
           "Content-Disposition: form-data; name=\"file\"; filename=\"picture.jpg\"\r\n"
           "Content-Type: image/jpeg\r\n\r\n",
           boundary);

  char end_form[128];
  snprintf(end_form, sizeof(end_form), "\r\n--%s--\r\n", boundary);

  char content_type[128];
  snprintf(content_type, sizeof(content_type), "multipart/form-data; boundary=%s", boundary);
  esp_http_client_set_header(client, "Content-Type", content_type);

  int total_len = strlen(start_form) + current_frame->len + strlen(end_form);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  esp_err_t err = esp_http_client_open(client, total_len);
  if (err != ESP_OK) {
      ESP_LOGE("UPLOAD", "Failed to open HTTP connection: %s", esp_err_to_name(err));
      esp_http_client_cleanup(client); // Important to cleanup here
      return;
  }

  // Write start of multipart
  int wlen = esp_http_client_write(client, start_form, strlen(start_form));
  if (wlen < 0) {
      ESP_LOGE("UPLOAD", "Failed to write start form");
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      return;
  }

  // Write image data
  wlen = esp_http_client_write(client, (const char *)current_frame->buf, current_frame->len);
  if (wlen < 0) {
      ESP_LOGE("UPLOAD", "Failed to write image data");
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      return;
  }

  // Write end of multipart
  wlen = esp_http_client_write(client, end_form, strlen(end_form));
  if (wlen < 0) {
      ESP_LOGE("UPLOAD", "Failed to write end form");
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      return;
  }

  // Now fetch server response
  int http_status = esp_http_client_fetch_headers(client);
  if (http_status > 0) {
      int status_code = esp_http_client_get_status_code(client);
      ESP_LOGI("UPLOAD", "HTTP POST Status = %d", status_code);
  } else {
      ESP_LOGE("UPLOAD", "HTTP POST request failed: %s", esp_err_to_name(http_status));
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}



void app_main(void) {
  

  init_hw_services();
  init_wifi();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  //configure_time();



  #if ESP_CAMERA_SUPPORTED
    while (1){
    init_camera_capture();
      int i = 0;
      while (1) {
        if (i == 10 || i ==5 || i==3) {
          take_photo_and_upload();
        }
        vTaskDelay(5000 / portTICK_RATE_MS);
        i++;
      }
    }
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    return;
#endif
    
}