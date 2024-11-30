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



/**
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
// void wifi_task(void *pvParameters) {
//     while (1) {
//         // Simulate WiFi connection check
//         wifi_connection(); // mutates is_connected_to_wifi
//         if (is_connected_to_wifi) {

//             xSemaphoreGive(wifi_semaphore);
//         }
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }
// }

void initial_wifi_connection(void){
	setup_wifi();
	printf("Won't proceed further without wifi Connection\n");
	try_connecting_to_wifi();
}

void app_main(void)
{
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT); //zewnetrzna dioda
	nvs_flash_init(); 
	initial_wifi_connection();
	configure_time();
	esp_mqtt_client_handle_t client = mqtt_connect(MQTT_BROKER_URI, MQTT_USERNAME, MQTT_PASSWORD);
	xTaskCreate(&mqtt_task, "mqtt_task", 4096, client, 5, NULL);
}
