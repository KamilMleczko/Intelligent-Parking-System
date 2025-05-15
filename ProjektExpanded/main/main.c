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

// For websocket streaming
#define STREAM_SERVER_HOST "192.168.103.77"
#define STREAM_SERVER_PORT 8000
#define STREAM_SERVER_PATH "/ws_stream/esp32cam1"  // Include a unique device ID
#define STREAM_FPS 1  // frames per second to stream
#define STREAM_QUALITY 20  // JPEG quality (0-63, lower is better quality)

// TCP socket implementation constants
camera_fb_t *current_frame = NULL;
int sock = -1;  // Socket for TCP streaming
bool socket_connected = false;


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
bool connect_socket() {
    if (socket_connected) {
        return true;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(STREAM_SERVER_HOST);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(STREAM_SERVER_PORT);

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE("SOCKET", "Unable to create socket: errno %d", errno);
        return false;
    }

    // Set socket option to timeout if connection takes too long
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // Connect to the server
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
    if (err != 0) {
        ESP_LOGE("SOCKET", "Socket connect failed, errno=%d", errno);
        close(sock);
        sock = -1;
        return false;
    }

    // Send HTTP WebSocket upgrade request
    char upgrade_request[256];
    snprintf(upgrade_request, sizeof(upgrade_request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n",
        STREAM_SERVER_PATH, STREAM_SERVER_HOST, STREAM_SERVER_PORT);

    if (send(sock, upgrade_request, strlen(upgrade_request), 0) < 0) {
        ESP_LOGE("SOCKET", "Failed to send WebSocket upgrade request");
        close(sock);
        sock = -1;
        return false;
    }

    // Read response to verify connection
    char rx_buffer[512];
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE("SOCKET", "Connection failed, recv error");
        close(sock);
        sock = -1;
        return false;
    }
    rx_buffer[len] = 0; // Null-terminate to make it a valid string
    
    // Check if we got "HTTP/1.1 101" (WebSocket Switching Protocols)
    if (strstr(rx_buffer, "HTTP/1.1 101") != NULL) {
        ESP_LOGI("SOCKET", "WebSocket handshake successful");
        socket_connected = true;
        return true;
    } else {
        ESP_LOGE("SOCKET", "WebSocket handshake failed: %s", rx_buffer);
        close(sock);
        sock = -1;
        return false;
    }
}

// Function to send frame over WebSocket
bool send_frame_over_websocket(const uint8_t *frame_data, size_t frame_len) {
    if (!socket_connected && !connect_socket()) {
        return false;
    }

    uint8_t header[14];
    size_t header_len = 0;
    uint8_t mask_key[4] = {0x12, 0x34, 0x56, 0x78};  // Ideally, randomize this
    size_t payload_offset = 0;

    header[0] = 0x82;  // FIN + binary
    if (frame_len < 126) {
        header[1] = 0x80 | frame_len;  // MASK bit set
        header_len = 2;
    } else if (frame_len <= 0xFFFF) {
        header[1] = 0x80 | 126;
        header[2] = (frame_len >> 8) & 0xFF;
        header[3] = frame_len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 0x80 | 127;
        for (int i = 0; i < 8; i++) {
            header[2 + i] = (frame_len >> ((7 - i) * 8)) & 0xFF;
        }
        header_len = 10;
    }

    // Send header
    if (send(sock, header, header_len, 0) < 0) goto err;

    // Send masking key
    if (send(sock, mask_key, 4, 0) < 0) goto err;

    // Mask and send payload
    for (size_t i = 0; i < frame_len; i++) {
        uint8_t masked = frame_data[i] ^ mask_key[i % 4];
        if (send(sock, &masked, 1, 0) < 0) goto err;
    }

    ESP_LOGI("SOCKET", "Frame sent: %d bytes", frame_len);
    return true;

err:
    ESP_LOGE("SOCKET", "Failed during WebSocket frame send");
    socket_connected = false;
    close(sock);
    sock = -1;
    return false;
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

void stream_camera_task(void *pvParameters) {
    // Calculate delay between frames based on desired FPS
    const TickType_t delay_time = 1000 / STREAM_FPS / portTICK_RATE_MS;
    int consecutive_failures = 0;
    
    while (true) {
        // Try to connect if not connected
        if (!socket_connected) {
            if (connect_socket()) {
                consecutive_failures = 0;
            } else {
                ESP_LOGW("CAMERA", "Failed to connect to server, will retry");
                vTaskDelay(5000 / portTICK_RATE_MS); // Wait 5 seconds before retrying
                continue;
            }
        }
        
        // Capture a frame
        camera_fb_t *fb = esp_camera_fb_get();
        
        if (!fb) {
            ESP_LOGE("CAMERA", "Camera capture failed");
            vTaskDelay(delay_time);
            continue;
        }

        // Send the frame
        if (!send_frame_over_websocket(fb->buf, fb->len)) {
            consecutive_failures++;
            ESP_LOGE("CAMERA", "Failed to send frame (failure #%d)", consecutive_failures);
            
            // If we've failed multiple times, sleep longer to prevent thrashing
            if (consecutive_failures > 5) {
                vTaskDelay(5000 / portTICK_RATE_MS);
            }
        } else {
            consecutive_failures = 0;
        }

        // Return the frame buffer to be reused
        esp_camera_fb_return(fb);
        
        // Delay to achieve target FPS
        vTaskDelay(delay_time);
    }
}


void app_main(void) {
   init_hw_services();
    init_wifi();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    #if ESP_CAMERA_SUPPORTED
        // Initialize camera
        if (init_camera() != ESP_OK) {
            ESP_LOGE("CAMERA", "Failed to initialize camera!");
            return;
        }
        
        // Start camera streaming task
        xTaskCreatePinnedToCore(
            stream_camera_task,      // Task function
            "stream_camera_task",    // Task name
            4096,                   // Stack size (bytes)
            NULL,                   // Task parameters
            5,                      // Task priority
            NULL,                   // Task handle
            0                       // Core ID (0 for PRO CPU)
        );
        
        ESP_LOGI("CAMERA", "Camera streaming started");
        
        // Keep the main task alive
        while (1) {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    #else
        ESP_LOGE("CAMERA", "Camera support is not available for this chip");
        return;
    #endif
    
}