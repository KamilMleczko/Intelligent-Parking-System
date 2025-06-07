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


void app_main(void) {
    init_hw_services();
    nvs_init();
    init_wifi();
    configure_time();
    ESP_LOGI(LOG_HW, "Starting application main function");

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

    while (true) {
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
      }
    vTaskDelay(100);
    }


    

  

  }
