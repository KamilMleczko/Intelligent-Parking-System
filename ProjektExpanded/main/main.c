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

//servo
#include "sg90.h"
//oled
#include "my_ssd1306.h"
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

typedef struct {
  const ssd1306_config_t *config;  // Pointer to the SSD1306 configuration
  i2c_handler_t *i2c_handler;      // Pointer to the I2C handler
  oled_display_t *oled_display;    // Pointer to the OLED display
} TaskParameters;
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


void main_loop(void *pvParameters) {
  TaskParameters *params = (TaskParameters *)pvParameters;

  const ssd1306_config_t *config = params->config;
  i2c_handler_t *i2c_handler = params->i2c_handler;
  oled_display_t *oled_display = params->oled_display;
  
  show_text_large(config, oled_display, i2c_handler, 3, "Hello");
  vTaskDelay(10000 / config->ticks_to_wait);
  clear_screen(config, oled_display, i2c_handler);
  while (1) {
    //gate is initially closed
    show_text_large(config, oled_display, i2c_handler, 3, "CLOSE");
    vTaskDelay(pdMS_TO_TICKS(5000));

    //open gate
    clear_screen(config, oled_display, i2c_handler);
    show_text(config, oled_display, i2c_handler, 2, "OPENING");
    show_text(config, oled_display, i2c_handler, 3, "GATE");
    servo_open_gate();
    clear_screen(config, oled_display, i2c_handler);
    show_text_large(config, oled_display, i2c_handler, 3, "OPEN");

    //hold gate open for 5 sec
    vTaskDelay(pdMS_TO_TICKS(5000));

    //close gate
    clear_screen(config, oled_display, i2c_handler);
    show_text(config, oled_display, i2c_handler, 2, "CLOSING");
    show_text(config, oled_display, i2c_handler, 3, "GATE");
    servo_close_gate();
    clear_screen(config, oled_display, i2c_handler);
  }
}
void app_main(void) {
  init_hw_services();
  nvs_init();
  servo_init();

    // OLED initialization
  ssd1306_config_t *config = malloc(sizeof(ssd1306_config_t));
  i2c_handler_t *i2c_handler = malloc(sizeof(i2c_handler_t));
  oled_display_t *oled_display = malloc(sizeof(oled_display_t));
  *config = create_config();
  i2c_master_init(config, i2c_handler, oled_display);
#if CONFIG_FLIP
  oled_display->flip_display = true;
#endif
  oled_cmd_init(oled_display, config, i2c_handler);
  clear_oled_display_struct(oled_display);
  clear_screen(config, oled_display, i2c_handler);
  set_brightness(config, i2c_handler, 200);
  TaskParameters *taskParameters = malloc(sizeof(TaskParameters));
  taskParameters->config = config;
  taskParameters->i2c_handler = i2c_handler;
  taskParameters->oled_display = oled_display;

  // Start main loop task
  if (true ){//TODO: check if wifi is connected) {
    xTaskCreate(main_loop, "main_loop", configMINIMAL_STACK_SIZE * 6,
                (void *)taskParameters, 5, NULL);
  }


  //   init_wifi();
  //   configure_time();
  //   ESP_LOGI(LOG_HW, "Starting application main function");

  // #if ESP_CAMERA_SUPPORTED
  //     if (init_camera() != ESP_OK) {
  //         ESP_LOGE("CAMERA", "Failed to initialize camera!");
  //         return;
  //     }

  //     xTaskCreatePinnedToCore(stream_camera_task,"stream_camera_task", 4096, NULL, 8, NULL, 1);
  //     ESP_LOGI("CAMERA", "Camera streaming started");

  // #else
  //     ESP_LOGE("CAMERA", "Camera not supported");
  // #endif

}
   

