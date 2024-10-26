#include <stdbool.h>
#include <unistd.h>
#include <stdio.h> //for basic printf commands
#include <stdlib.h>
#include <string.h> //for handling strings
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "freertos/task.h"
#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_log.h" //for showing logs
#include "esp_event.h" //for wifi event
#include "nvs_flash.h" //non volatile storage
#include "lwip/err.h" //light weight ip packets error handling
#include "lwip/sys.h" //system applications for light weight ip apps


#include "esp_err.h"
//#include "protocol_examples_common.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

const char *your_ssid = "Logiczna Sieć";
const char *your_pass = "srzj6042";
bool is_connected_to_wifi = false;
#define TAG "HTTP_GET"

static void wifi_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	if(event_id == WIFI_EVENT_STA_START){
	  printf("WIFI CONNECTING....\n");
	}
	
	else if (event_id == WIFI_EVENT_STA_CONNECTED){
	  printf("WiFi CONNECTED\n");
	  is_connected_to_wifi = true;
	}
	
	else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
	  is_connected_to_wifi = false;
	  printf("WiFi lost connection\n");
	  gpio_set_level(GPIO_NUM_17,1);
      vTaskDelay(100);
      gpio_set_level(GPIO_NUM_17, 0);
      vTaskDelay(100);
	  esp_wifi_connect();
	  printf("Retrying to Connect...\n");
	}
	else if (event_id == IP_EVENT_STA_GOT_IP)
	{
	  printf("Wifi got IP...\n\n");
	}
}


void wifi_connection(){
	esp_netif_init(); //network interdace initialization
	esp_event_loop_create_default(); //responsible for handling and dispatching events
	esp_netif_create_default_wifi_sta(); //sets up necessary data structs for wifi station interface
	wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();//sets up wifi wifi_init_config struct with default values
	esp_wifi_init(&wifi_initiation); //wifi initialised with dafault wifi_initiation
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);//creating event handler register for wifi
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);//creating event handler register for ip event
	wifi_config_t wifi_configuration = { //struct wifi_config_t var wifi_configuration
		.sta= {
		    .ssid = "",
		    .password= "", /*we are sending a const char of ssid and password which we will strcpy in following line so leaving it blank*/ 
		  }//also this part is used if you donot want to use Kconfig.projbuild
	};
	strcpy((char*)wifi_configuration.sta.ssid,your_ssid); // copy chars from hardcoded configs to struct
	strcpy((char*)wifi_configuration.sta.password,your_pass);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);//setting up configs when event ESP_IF_WIFI_STA
	esp_wifi_start();//start connection with configurations provided in funtion
	esp_wifi_set_mode(WIFI_MODE_STA);//station mode selected
	esp_wifi_connect(); //connect with saved ssid and pass
	printf( "wifi_init_softap finished. SSID:%s  password:%s",your_ssid,your_pass);
}

void get_site_html(const char *server_name) {
	printf(server_name);
    const char *REQUEST = "GET / HTTP/1.0\r\n"
                          "Host: %s\r\n"
                          "User-Agent: esp-idf/1.0 esp32\r\n"
                          "\r\n";

    char request_buffer[256];
    snprintf(request_buffer, sizeof(request_buffer), REQUEST, server_name);

    struct addrinfo hints;
    struct addrinfo *res;
    int sockfd;
    char recv_buffer[1024];

    // Konfiguracja adresu serwera
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(server_name, "80", &hints, &res);
    if (err != 0 || res == NULL) {

        ESP_LOGE(TAG, "Błąd przy rozwiązywaniu nazwy serwera: %s", strerror(err));
        return;
    }

    // Nawiązanie połączenia TCP
    sockfd = socket(res->ai_family, res->ai_socktype, 0);
    if (sockfd < 0) {
        ESP_LOGE(TAG, "Błąd przy tworzeniu socketu");
        freeaddrinfo(res);
        return;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "Błąd przy łączeniu się z serwerem");
        close(sockfd);
        freeaddrinfo(res);
        return;
    }

    // Zwolnienie struktury adresu
    freeaddrinfo(res);

    // Wysłanie zapytania GET
    if (write(sockfd, request_buffer, strlen(request_buffer)) < 0) {
        ESP_LOGE(TAG, "Błąd przy wysyłaniu zapytania");
        close(sockfd);
        return;
    }

    // Odbiór i wypisanie odpowiedzi
    int len;
    while ((len = read(sockfd, recv_buffer, sizeof(recv_buffer) - 1)) > 0) {
        recv_buffer[len] = 0;  // Dodanie znaku końca stringa
        printf("%s", recv_buffer);  // Wypisanie odebranej treści
    }

    if (len < 0) {
        ESP_LOGE(TAG, "Błąd przy odbiorze danych");
    }

    // Zamknięcie połączenia
    close(sockfd);
}

void app_main(void)
{
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);

	nvs_flash_init(); 
	wifi_connection(); //zad1
	vTaskDelay(200);
 	get_site_html("httpforever.com");    //zad2
}
