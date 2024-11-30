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

#include "credentials.h"

#include "wifi.h"

#define TAG "HTTP_GET"
#define WIFI_DIODE 17

bool is_connected_to_wifi = false;
bool wifi_setup_done = false;

void wifi_diode_blink(void){
    gpio_set_level(GPIO_NUM_17,1); 
    vTaskDelay(100);
    gpio_set_level(GPIO_NUM_17, 0);
}

void try_connecting_to_wifi(void){
    assert (wifi_setup_done);
    esp_wifi_connect();
    while (!is_connected_to_wifi) {
        wifi_diode_blink();
        esp_wifi_connect();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void wifi_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	if(event_id == WIFI_EVENT_STA_START){
	  printf("WIFI CONNECTING....\n");
	}
	
	else if (event_id == WIFI_EVENT_STA_CONNECTED){
	  is_connected_to_wifi = true;
	}
	
	else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
    is_connected_to_wifi = false;
    printf("WiFi lost connection\n");
    xTaskCreate(&wifi_reconnect_task, "wifi_reconnect_task", 2048, NULL, 5, NULL);
	}
	else if (event_id == IP_EVENT_STA_GOT_IP)
	{
	  printf("Wifi got IP...\n\n");
	}
}

void wifi_reconnect_task(void *pvParameters) {
    try_connecting_to_wifi();
    vTaskDelete(NULL);
}


void setup_wifi(){
	esp_netif_init(); //network interface initialization
	esp_event_loop_create_default(); //responsible for handling and dispatching events
	esp_netif_create_default_wifi_sta(); //necessary data structs for wifi station interface
	wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();//wifi_init_config struct (defaultowe wartosci)
	esp_wifi_init(&wifi_initiation); //wifi initialised 
	
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);//register event handler register dla wifi event
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);//register event handler dla ip event
	wifi_config_t wifi_configuration = { //struct wifi_config_t var wifi_configuration
		.sta= {
		    .ssid = "",
		    .password= "", 
		  }
	};
	strcpy((char*)wifi_configuration.sta.ssid,WIFI_SSID); // kopiowanie do wifi_configuration struct
	strcpy((char*)wifi_configuration.sta.password,WIFI_PASS); //ssid i password zadeklarowane narazie w kodzie 
	
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);// configi dla eventu ESP_IF_WIFI_STA
	esp_wifi_start();//start connection 
	esp_wifi_set_mode(WIFI_MODE_STA);//station mode selected
    wifi_setup_done = true;
}

void get_site_html(const char *server_name) {
    const char *REQUEST = "GET / HTTP/1.0\r\n"
                          "Host: %s\r\n"
                          "User-Agent: esp-idf/1.0 esp32\r\n"
                          "\r\n";

    char request_buffer[256];
    snprintf(request_buffer, sizeof(request_buffer), REQUEST, server_name); //tworzy pełne zapytanie

    struct addrinfo hints; //do funckji getaddrinfo() (zawiera preferencje dotyczące tworzenia połączenia sieciowego.)
    struct addrinfo *res; //wsakznik na strukture getaddrinfo (do zwalniania pamięci)
    int sockfd; //deskryptor gniazda(socketu)
    char recv_buffer[1024]; //bufor na dane

    //Konfiguracja adresu serwera
    memset(&hints, 0, sizeof(hints)); // Inicjalizacja struktury
    hints.ai_family = AF_INET; // Uzycie IPv4
    hints.ai_socktype = SOCK_STREAM; // Uzycie TCP

    int err = getaddrinfo(server_name, "80", &hints, &res);
    if (err != 0 || res == NULL) {

        ESP_LOGE(TAG, "Błąd przy rozwiązywaniu nazwy serwera: %s", strerror(err));
        return;
    }

    //Nawiązanie połączenia TCP
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

    //Zwolnienie struktury adresu
    freeaddrinfo(res);

    //Wysłanie zapytania GET
    if (write(sockfd, request_buffer, strlen(request_buffer)) < 0) {
        ESP_LOGE(TAG, "Błąd przy wysyłaniu zapytania");
        close(sockfd);
        return;
    }

    //Odbiór i wypisanie odpowiedzi
    int len;
    while ((len = read(sockfd, recv_buffer, sizeof(recv_buffer) - 1)) > 0) {
        recv_buffer[len] = 0;  // Dodanie znaku końca stringa (ten sam efekt co \0)
        printf("%s", recv_buffer);  // Wypisanie odebranej treści
    }

    if (len < 0) {
        ESP_LOGE(TAG, "Błąd przy odbiorze danych");
    }

    //Zamknięcie połączenia
    close(sockfd);
}