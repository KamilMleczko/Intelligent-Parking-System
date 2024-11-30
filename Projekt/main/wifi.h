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
#include "lwip/err.h"  //error handling
#include "lwip/netdb.h"
#include "lwip/sockets.h"  //sockets
#include "lwip/sys.h"      //system applications

#include "credentials.h"

#include <time.h>
#ifndef WIFI_H
#define WIFI_H
extern bool is_connected_to_wifi;
#endif  // WIFI_H

#define TAG "HTTP_GET"

void wifi_diode_blink(void);

void try_connecting_to_wifi(void);

void wifi_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void wifi_reconnect_task(void *pvParameters);

void setup_wifi();
void get_site_html(const char *server_name);