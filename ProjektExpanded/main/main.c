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

//camera and webscoket streaming
#include "esp_camera.h"
#include "camera_stream.h"
#include "ultrasonic.h"
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#define MAX_DISTANCE_CM 200  // 2m max
#define TRIGGER_GPIO 32
#define ECHO_GPIO 33

// Added camera streaming parameters
#define STREAM_FPS 0.5
#define IMAGE_QUALITY 10
#define CAMERA_BRIGHTNESS 500

// External variables needed for camera streaming
extern camera_fb_t *current_frame;
extern int sock;
extern bool socket_connected;


float get_sensor_dist(const ultrasonic_sensor_t *sensor) {
 
  float distance_array[10];
  for (int i = 0; i < 10; i++) {
    float distance;
    esp_err_t res = ultrasonic_measure(sensor, MAX_DISTANCE_CM, &distance);
    if (res != ESP_OK) {
      printf("Error %d: ", res);
      switch (res) {
        case ESP_ERR_ULTRASONIC_PING:
          printf("Cannot ping (device is in invalid state)\n");
          break;
        case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
          printf("Ping timeout (no device found)\n");
          break;
        case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
          printf("Echo timeout (i.e. distance too big)\n");
          break;
        default:
          printf("%s\n", esp_err_to_name(res));
      }
    } else
      printf("Distance measured: %0.04f cm\n", distance * 100);
    if ((distance * 100) > MAX_DISTANCE_CM) {
      return distance * 100;
    }
    distance_array[i] = distance * 100;
    vTaskDelay(100);
  }


  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum = sum + distance_array[i];
  }
  float avg = sum / 10;
  printf("Average distance measured: %.2f cm\n", avg);

  return avg;
}

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



// Camera streaming task moved from camera_stream.c
void stream_camera_task(void *pvParameters) {

   ultrasonic_sensor_t sensor = {// FIRST SENSOR
                                  .trigger_pin = TRIGGER_GPIO,
                                  .echo_pin = ECHO_GPIO};
    ultrasonic_init(&sensor);
    float dist_first = get_sensor_dist(&sensor);
    printf("distance first: %0.04f cm\n", dist_first);
    while (dist_first > MAX_DISTANCE_CM) {
      // try again until measurements are correct
      dist_first = get_sensor_dist(&sensor);
      printf("distance first: %0.04f cm\n", dist_first);
    }



    const TickType_t delay_time = 1000 / STREAM_FPS / portTICK_PERIOD_MS;
    int consecutive_failures = 0;
    bool object_detected = false;
    while (true) {

      // Ultrasonic sensor
      float distance1;
      esp_err_t res1 = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance1);
      if (res1 != ESP_OK) {
        printf("Error %d: ", res1);
        switch (res1) {
          case ESP_ERR_ULTRASONIC_PING:
            printf("Cannot ping (device is in invalid state)\n");
            break;
          case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
            printf("Ping timeout (no device found)\n");
            break;
          case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
            printf("Echo timeout (i.e. distance too big)\n");
            break;
          default:
            printf("%s\n", esp_err_to_name(res1));
        }
      } else printf("Distance1: %0.04f cm\n", distance1 * 100);
      float error_margin_first = dist_first * 0.2;
      float diff_first = fabs(dist_first - distance1 * 100);


      if (diff_first > error_margin_first) {
        ESP_LOGE("ULTRASONIC", "Car detected");
        object_detected = true;
      }
      else {
        object_detected = false;
      }


        if (!socket_connected && !connect_socket()) {
            ESP_LOGW("CAMERA_STREAM", "Failed to connect to server, retrying...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE("CAMERA_STREAM", "Camera capture failed");
            vTaskDelay(delay_time);
            continue;
        }

        if (!send_frame_over_websocket(fb->buf, fb->len, object_detected)) {
            consecutive_failures++;
            ESP_LOGE("CAMERA_STREAM", "Failed to send frame (failure #%d)", consecutive_failures);
            if (consecutive_failures > 5) {
                vTaskDelay(5000 / portTICK_PERIOD_MS);
            }
        } else {
            consecutive_failures = 0;
        }

        esp_camera_fb_return(fb);
        vTaskDelay(delay_time);
    }
}

void app_main(void) {
    init_hw_services();
    nvs_init();
    init_wifi();
    configure_time();
    ESP_LOGI(LOG_HW, "Starting application main function");

   

    
    #if ESP_CAMERA_SUPPORTED
        if (init_camera() != ESP_OK) {
            ESP_LOGE("CAMERA", "Failed to initialize camera!");
            return;
        }
        xTaskCreatePinnedToCore(stream_camera_task,"stream_camera_task", 4096, NULL, 8, NULL, 1);
        ESP_LOGI("CAMERA", "Camera streaming started");
    #else
        ESP_LOGE("CAMERA", "Camera not supported");
    #endif

  }
