#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

//miscellaneous
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" //delay,mutexx,semphr i rtos
#include "freertos/task.h"
#include "nvs_flash.h" //non volatile storage

//biblioteki esp
#include "esp_system.h"
#include "esp_wifi.h" //wifi functions and operations
#include "esp_log.h" //pokazywanie logów
#include "esp_event.h" //wifi event
//do zad2 lab1
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_tls.h"

//light weight ip (TCP IP)
#include "lwip/sockets.h" //sockets
#include "lwip/netdb.h" 
#include "lwip/err.h" //error handling
#include "lwip/sys.h" //system applications

#include "mqtt_utils.h"
#include "wifi.h"
#include "credentials.h"

#include <time.h>
#include "esp_sntp.h"
#include <esp_netif_sntp.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "esp_bt.h"
#include "utils.h"

TaskHandle_t mqtt_task_handle = NULL;

//oled lib
#include "my_ssd1306.h"
#define tag "SSD1306"

void buzz(){
	gpio_set_level(GPIO_NUM_19,1);
    vTaskDelay(10);
    gpio_set_level(GPIO_NUM_19, 0);
}

void oled_start(){
	const ssd1306_config_t config = create_config();//config dla OLED //config values can be changed config vals via sdkconfig file
    i2c_handler_t i2c_handler; //handler struct for i2c
    oled_display_t oled_display; // oled display handler
	i2c_master_init(&config, &i2c_handler, &oled_display);//init for master bus and device
	#if CONFIG_FLIP
	oled_display.flip_display = true;
	#endif
	oled_cmd_init(&oled_display, &config, &i2c_handler); //init of oled commands
	clear_oled_display_struct(&oled_display); //clear data in struct that represents oled screen
	clear_screen(&config, &oled_display, &i2c_handler);
	set_brightness(&config, &i2c_handler, 132);


    show_text(&config,  &oled_display,  &i2c_handler, 0, "Hello new day !");
    vTaskDelay(10000/ config.ticks_to_wait);
    clear_page(&config, &oled_display, &i2c_handler, 0);

    show_text_large(&config, &oled_display, &i2c_handler, 3, "Hello");
	vTaskDelay(10000/ config.ticks_to_wait);
    clear_screen(&config, &oled_display, &i2c_handler);

    show_text(&config,  &oled_display,  &i2c_handler, 0, "Current Count:");

	gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT); //buzzer

	char buffer[10];
    for (int i = 0 ; i < 50 ; i++){
		snprintf(buffer, sizeof(buffer), "  %d ", i);
		show_text_large(&config, &oled_display, &i2c_handler, 3, buffer);
		if (i==15){
			set_brightness(&config, &i2c_handler, 255);
		}
		//buzz(); //before uncommenting check if your buzzer is on GPIO 19 !!! 
		vTaskDelay(10000/ config.ticks_to_wait);
	}
	clear_large_page(&config, &oled_display, &i2c_handler, 3); //clearing count value
	clear_page(&config, &oled_display, &i2c_handler, 0);  //clearing "Current Count"
	vTaskDelay(50000/ config.ticks_to_wait);
	show_text(&config,  &oled_display,  &i2c_handler, 0, "Goodbye   ");
	show_text(&config,  &oled_display,  &i2c_handler, 3, "   Goodbye   ");
	show_text(&config,  &oled_display,  &i2c_handler, 7, "      Goodbye");
	vTaskDelay(30000/ config.ticks_to_wait);
	clear_screen(&config, &oled_display, &i2c_handler);
}


 /*
 * @brief Function that configures time settings using SNTP.
 * It sets timezone to CEST. 
 * @warning it's a blocking call. It waits for 10s for SNTP response.
 */
void configure_time(){
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
	esp_netif_sntp_init(&config);
	if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
    printf("Failed to update system time within 10s timeout");
	}
	setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
	tzset();
}

int call_count = 0;

void mqtt_pub_test(esp_mqtt_client_handle_t client){
	EventType event = call_count % 2 == 0 ? CAR_PARKED : CAR_LEFT;
	StatusType status = call_count % 2 == 0 ? OCCUPIED : FREE;
	mqtt_publish_event(client, event, time(NULL));
	mqtt_publish_status(client, status);
	mqtt_publish_healthcheck(client);
	call_count++;
}

void mqtt_task(void *pvParameters) {
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) pvParameters;
    while (1) {
        mqtt_pub_test(client);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 1 second
    }
}


void nvs_init(){
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ESP_ERROR_CHECK(nvs_flash_init());
	}
}

void init_hw_services(void){
	ESP_LOGI(LOG_HW, "Initializing hardware services");
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
	nvs_init();
	ESP_LOGI(LOG_HW, "Hardware services initialized");
}



void app_main(void)
{
	init_hw_services();
	init_wifi();
	configure_time();
	esp_mqtt_client_handle_t client = mqtt_connect(MQTT_BROKER_URI, MQTT_USERNAME, MQTT_PASSWORD);
	if (mqtt_task_handle == NULL){
		xTaskCreate(&mqtt_task, "mqtt_task", 4096, client, 5, &mqtt_task_handle);
	}
	//oled_start(); //showcase of oled library
}
