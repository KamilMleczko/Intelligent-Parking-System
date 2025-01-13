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

TaskHandle_t mqtt_task_handle = NULL;

// oled lib
#include "my_ssd1306.h"
#define tag "SSD1306"
// HC-SR04 Sensor
#include "esp_err.h"
#include "ultrasonic.h"
#define MAX_DISTANCE_CM 200  // 2m max
#define TRIGGER_GPIO 4
#define ECHO_GPIO 2
#define TRIGGER_GPIO_SECOND 32
#define ECHO_GPIO_SECOND 33

void buzz() {
  gpio_set_level(GPIO_NUM_19, 1);
  vTaskDelay(10);
  gpio_set_level(GPIO_NUM_19, 0);
}

void oled_start() {
  const ssd1306_config_t config =
      create_config();  // config dla OLED //config values can be changed config
                        // vals via sdkconfig file
  i2c_handler_t i2c_handler;    // handler struct for i2c
  oled_display_t oled_display;  // oled display handler
  i2c_master_init(&config, &i2c_handler,
                  &oled_display);  // init for master bus and device
#if CONFIG_FLIP
  oled_display.flip_display = true;
#endif
  oled_cmd_init(&oled_display, &config, &i2c_handler);  // init of oled commands
  clear_oled_display_struct(
      &oled_display);  // clear data in struct that represents oled screen
  clear_screen(&config, &oled_display, &i2c_handler);
  set_brightness(&config, &i2c_handler, 132);

  show_text(&config, &oled_display, &i2c_handler, 0, "Hello new day !");
  vTaskDelay(10000 / config.ticks_to_wait);
  clear_page(&config, &oled_display, &i2c_handler, 0);

  show_text_large(&config, &oled_display, &i2c_handler, 3, "Hello");
  vTaskDelay(10000 / config.ticks_to_wait);
  clear_screen(&config, &oled_display, &i2c_handler);

  show_text(&config, &oled_display, &i2c_handler, 0, "Current Count:");

  gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);  // buzzer

  char buffer[10];
  for (int i = 0; i < 50; i++) {
    snprintf(buffer, sizeof(buffer), "  %d ", i);
    show_text_large(&config, &oled_display, &i2c_handler, 3, buffer);
    if (i == 15) {
      set_brightness(&config, &i2c_handler, 255);
    }
    buzz();  // before uncommenting check if your buzzer is on GPIO 19 !!!
    vTaskDelay(10000 / config.ticks_to_wait);
  }
  clear_large_page(&config, &oled_display, &i2c_handler,
                   3);  // clearing count value
  clear_page(&config, &oled_display, &i2c_handler,
             0);  // clearing "Current Count"
  vTaskDelay(50000 / config.ticks_to_wait);
  show_text(&config, &oled_display, &i2c_handler, 0, "Goodbye   ");
  show_text(&config, &oled_display, &i2c_handler, 3, "   Goodbye   ");
  show_text(&config, &oled_display, &i2c_handler, 7, "      Goodbye");
  vTaskDelay(30000 / config.ticks_to_wait);
  clear_screen(&config, &oled_display, &i2c_handler);
}

float get_sensor_dist(const ultrasonic_sensor_t* sensor,
                      const ssd1306_config_t* config,
                      oled_display_t* oled_display,
                      i2c_handler_t* i2c_handler) {
  show_text(config, oled_display, i2c_handler, 1, "Measuring(cm)...");
  char buffer[16];
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
    snprintf(buffer, sizeof(buffer), "%0.04f", distance * 100);
    show_text(config, oled_display, i2c_handler, 4,
              buffer);  // 3 is display page number
    if ((distance * 100) > MAX_DISTANCE_CM) {
      return distance * 100;
    }
    distance_array[i] = distance * 100;
    vTaskDelay(10000 / config->ticks_to_wait);
  }
  clear_screen(config, oled_display, i2c_handler);

  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum = sum + distance_array[i];
  }
  float avg = sum / 10;
  show_text(config, oled_display, i2c_handler, 1, "Average Distance");
  printf("Average distance measured: %.2f cm\n", avg);

  snprintf(buffer, sizeof(buffer), "%.2f", avg);
  show_text_large(config, oled_display, i2c_handler, 3, buffer);
  vTaskDelay(20000 / config->ticks_to_wait);
  clear_screen(config, oled_display, i2c_handler);
  return avg;
}

void main_loop(void* pvParameters) {
  esp_mqtt_client_handle_t client =
      mqtt_connect(MQTT_BROKER_URI, MQTT_USERNAME, MQTT_PASSWORD);
  // SENSOR INIT
  ultrasonic_sensor_t sensor = {// FIRST SENSOR
                                .trigger_pin = TRIGGER_GPIO,
                                .echo_pin = ECHO_GPIO};
  ultrasonic_init(&sensor);

  ultrasonic_sensor_t sensor2 = {// SECOND SENSOR
                                 .trigger_pin = TRIGGER_GPIO_SECOND,
                                 .echo_pin = ECHO_GPIO_SECOND};
  ultrasonic_init(&sensor2);

  // OLED INIT
  const ssd1306_config_t config =
      create_config();  // config dla OLED //config values can be changed config
                        // vals via sdkconfig file
  i2c_handler_t i2c_handler;    // handler struct for i2c
  oled_display_t oled_display;  // oled display handler
  i2c_master_init(&config, &i2c_handler,
                  &oled_display);  // init for master bus and device
#if CONFIG_FLIP
  oled_display.flip_display = true;
#endif
  oled_cmd_init(&oled_display, &config, &i2c_handler);  // init of oled commands
  clear_oled_display_struct(
      &oled_display);  // clear data in struct that represents oled screen
  clear_screen(&config, &oled_display, &i2c_handler);
  set_brightness(&config, &i2c_handler, 200);

  show_text_large(&config, &oled_display, &i2c_handler, 3, "Hello");
  vTaskDelay(10000 / config.ticks_to_wait);
  clear_screen(&config, &oled_display, &i2c_handler);

  // first sensor distance to the wall
  float dist_first =
      get_sensor_dist(&sensor, &config, &oled_display, &i2c_handler);
  printf("distance first: %0.04f cm\n", dist_first);

  // second sensor distance to the wall
  float dist_second =
      get_sensor_dist(&sensor2, &config, &oled_display, &i2c_handler);
  printf("distance second: %0.04f cm\n", dist_second);

  int ultrasonic_max_people = read_max_people();

  while (dist_first > MAX_DISTANCE_CM || dist_second > MAX_DISTANCE_CM) {
    show_text(&config, &oled_display, &i2c_handler, 0, "Sensor wasn't");
    show_text(&config, &oled_display, &i2c_handler, 1, "able to get");
    show_text(&config, &oled_display, &i2c_handler, 2, "correct");
    show_text(&config, &oled_display, &i2c_handler, 3, "measurements");
    show_text(&config, &oled_display, &i2c_handler, 4, "please try again");
    show_text(&config, &oled_display, &i2c_handler, 5, "in 5 seconds");
    vTaskDelay(50000 / config.ticks_to_wait);
    clear_screen(&config, &oled_display, &i2c_handler);

    // try again until measurements are correct
    dist_first = get_sensor_dist(&sensor, &config, &oled_display, &i2c_handler);
    printf("distance first: %0.04f cm\n", dist_first);

    dist_second =
        get_sensor_dist(&sensor2, &config, &oled_display, &i2c_handler);
    printf("distance second: %0.04f cm\n", dist_second);
  }

  // now that distances are measured we can start checking for anomalies in
  // measurements
  clear_screen(&config, &oled_display, &i2c_handler);

  show_text_large(&config, &oled_display, &i2c_handler, 3, "Start");
  vTaskDelay(10000 / config.ticks_to_wait);
  clear_screen(&config, &oled_display, &i2c_handler);

  show_text(&config, &oled_display, &i2c_handler, 0, "Current Count:");
  char buffer1[16];
  char buffer2[16];
  char buffer_count[16];
  int count = 0;
  bool was_counted_first = false;
  bool was_counted_second = false;

  bool is_first_counting = true;

  // enables buzzer.
  // gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);  // before uncommenting
  // check if your buzzer is on GPIO 19 !!!
  while (true) {
    snprintf(buffer_count, sizeof(buffer_count), "  %d ", count);
    if (is_first_counting) {
      show_text_large(&config, &oled_display, &i2c_handler, 2, buffer_count);
    }
    float distance1;
    float distance2;
    esp_err_t res1 = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance1);
    esp_err_t res2 = ultrasonic_measure(&sensor2, MAX_DISTANCE_CM, &distance2);
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
    } else
      printf("Distance1: %0.04f cm\n", distance1 * 100);

    if (res2 != ESP_OK) {
      printf("Error %d: ", res2);
      switch (res2) {
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
          printf("%s\n", esp_err_to_name(res2));
      }
    } else
      printf("Distance2: %0.04f cm\n", distance2 * 100);

    float error_margin_first = dist_first * 0.2;
    float error_margin_second = dist_second * 0.2;

    float diff_first = fabs(dist_first - distance1 * 100);
    float diff_second = fabs(dist_second - distance2 * 100);

    if (diff_first > error_margin_first && is_first_counting) {
      if (!was_counted_first) {
        buzz();  // before uncommenting check if your buzzer is on GPIO 19 !!!
        count += 1;
        EventType event = PERSON_ENTERED;
        mqtt_publish_event(client, event, time(NULL));
        snprintf(buffer_count, sizeof(buffer_count), "  %d ", count);
        clear_large_page(&config, &oled_display, &i2c_handler, 2);  // clearing
        show_text_large(&config, &oled_display, &i2c_handler, 2, buffer_count);
        was_counted_first = true;

        if (count == ultrasonic_max_people) {
          is_first_counting = false;
          clear_page(&config, &oled_display, &i2c_handler, 0);
          show_text(&config, &oled_display, &i2c_handler, 0,
                    "Too many visitors");
          clear_large_page(&config, &oled_display, &i2c_handler,
                           2);  // clearing
          show_text_large(&config, &oled_display, &i2c_handler, 2, "STOP");
        }
      }
    } else {
      was_counted_first = false;
    }

    if (diff_second > error_margin_second) {
      if (!was_counted_second) {
        if (count == ultrasonic_max_people) {
          is_first_counting = true;
          clear_page(&config, &oled_display, &i2c_handler, 0);
          show_text(&config, &oled_display, &i2c_handler, 0, "Current Count:");
        }
        buzz();  // before uncommenting check if your buzzer is on GPIO 19 !!
        count -= 1;
        EventType event = PERSON_LEFT;
        mqtt_publish_event(client, event, time(NULL));
        snprintf(buffer_count, sizeof(buffer_count), "  %d ", count);
        clear_large_page(&config, &oled_display, &i2c_handler, 2);  // clearing
        show_text_large(&config, &oled_display, &i2c_handler, 2, buffer_count);
        was_counted_second = true;
      }
    } else {
      was_counted_second = false;
    }

    snprintf(buffer1, sizeof(buffer1), "%0.04f cm", distance1 * 100);
    snprintf(buffer2, sizeof(buffer2), "%0.04f cm", distance2 * 100);
    show_text(&config, &oled_display, &i2c_handler, 6, buffer1);
    show_text(&config, &oled_display, &i2c_handler, 7, buffer2);

    vTaskDelay(3500 / config.ticks_to_wait);
  }

  clear_screen(&config, &oled_display, &i2c_handler);
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

int call_count = 0;

void mqtt_pub_test(esp_mqtt_client_handle_t client) {
  EventType event = call_count % 2 == 0 ? PERSON_ENTERED : PERSON_LEFT;
  StatusType status = call_count % 2 == 0 ? OCCUPIED : FREE;
  mqtt_publish_event(client, event, time(NULL));
  mqtt_publish_status(client, status);
  mqtt_publish_healthcheck(client);
  call_count++;
}

void mqtt_task(void* pvParameters) {
  esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
  while (1) {
    mqtt_pub_test(client);
    vTaskDelay(pdMS_TO_TICKS(5000));  // Delay for 1 second
  }
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

void clear_oled(void) {
  const ssd1306_config_t config = create_config();
  i2c_handler_t i2c_handler;
  oled_display_t oled_display;
  i2c_master_init(&config, &i2c_handler, &oled_display);
#if CONFIG_FLIP
  oled_display.flip_display = true;
#endif
  oled_cmd_init(&oled_display, &config, &i2c_handler);  // init of oled commands
  clear_oled_display_struct(
      &oled_display);  // clear data in struct that represents oled screen
  clear_screen(&config, &oled_display, &i2c_handler);
}

void app_main(void) {
  init_hw_services();
  init_mqtt_topics();
  // clear_oled();
  init_wifi();
  configure_time();
  if (mqtt_task_handle == NULL) {
    xTaskCreate(main_loop, "main_loop", configMINIMAL_STACK_SIZE * 6, NULL, 5,
                NULL);
  }
}
