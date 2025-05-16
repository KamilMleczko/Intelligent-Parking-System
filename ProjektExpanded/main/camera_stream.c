#include "camera_stream.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "credentials.h"

#define STREAM_FPS 1
#define IMAGE_QUALITY 10
#define CAMERA_BRIGHTNESS 500

static const char *TAG = "CAMERA_STREAM";

camera_fb_t *current_frame = NULL;
int sock = -1;
bool socket_connected = false;

#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET -1
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

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = IMAGE_QUALITY,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

esp_err_t init_camera(void)
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_brightness(s, 2);
    s->set_contrast(s, 1);
    s->set_saturation(s, 1);
    s->set_aec2(s, 0);
    s->set_gainceiling(s, (gainceiling_t)6);
    s->set_exposure_ctrl(s, 1);
    s->set_aec_value(s, CAMERA_BRIGHTNESS);

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

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return false;
    }

    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket connect failed, errno=%d", errno);
        close(sock);
        sock = -1;
        return false;
    }

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
        ESP_LOGE(TAG, "Failed to send WebSocket upgrade request");
        close(sock);
        sock = -1;
        return false;
    }

    char rx_buffer[512];
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(TAG, "Connection failed, recv error");
        close(sock);
        sock = -1;
        return false;
    }

    rx_buffer[len] = 0;
    if (strstr(rx_buffer, "HTTP/1.1 101") != NULL) {
        ESP_LOGI(TAG, "WebSocket handshake successful");
        socket_connected = true;
        return true;
    } else {
        ESP_LOGE(TAG, "WebSocket handshake failed: %s", rx_buffer);
        close(sock);
        sock = -1;
        return false;
    }
}


bool send_frame_over_websocket(const uint8_t *frame_data, size_t frame_len) {
    if (!socket_connected && !connect_socket()) {
        return false;
    }

    uint8_t header[14];
    size_t header_len = 0;
    uint8_t mask_key[4] = {0x12, 0x34, 0x56, 0x78};  // Ideally random
    const size_t chunk_size = 1024;
    uint8_t chunk_buf[chunk_size];

    // Construct WebSocket frame header
    header[0] = 0x82;  // FIN + binary frame
    if (frame_len < 126) {
        header[1] = 0x80 | frame_len;
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
    if (send(sock, header, header_len, 0) < 0) goto drop;

    // Send mask key
    if (send(sock, mask_key, 4, 0) < 0) goto drop;

    // Send payload in masked chunks
    for (size_t i = 0; i < frame_len; i += chunk_size) {
        size_t remaining = frame_len - i;
        size_t send_len = remaining > chunk_size ? chunk_size : remaining;

        for (size_t j = 0; j < send_len; ++j) {
            chunk_buf[j] = frame_data[i + j] ^ mask_key[(i + j) % 4];
        }

        if (send(sock, chunk_buf, send_len, 0) < 0) goto drop;
    }

    ESP_LOGI("SOCKET", "Frame sent: %d bytes", frame_len);
    return true;

drop:
    // Drop this frame and disconnect socket
    ESP_LOGE("SOCKET", "Dropping frame due to send failure");
    socket_connected = false;
    close(sock);
    sock = -1;
    return false;
}


void stream_camera_task(void *pvParameters) {
    const TickType_t delay_time = 1000 / STREAM_FPS / portTICK_PERIOD_MS;
    int consecutive_failures = 0;

    while (true) {
        if (!socket_connected && !connect_socket()) {
            ESP_LOGW(TAG, "Failed to connect to server, retrying...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            vTaskDelay(delay_time);
            continue;
        }

        if (!send_frame_over_websocket(fb->buf, fb->len)) {
            consecutive_failures++;
            ESP_LOGE(TAG, "Failed to send frame (failure #%d)", consecutive_failures);
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