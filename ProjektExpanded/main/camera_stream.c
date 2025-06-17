#include "camera_stream.h"
#include <string.h>
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "credentials.h"

static const char *TAG = "CAMERA_STREAM";
static esp_websocket_client_handle_t ws_client = NULL;
bool websocket_connected = false;

#define STREAM_FPS 0.2
#define IMAGE_QUALITY 10

camera_fb_t *current_frame = NULL;

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
// Camera pin configuration (ESP32-CAM)
static camera_config_t camera_config = {
    .pin_pwdn       = CAM_PIN_PWDN,
    .pin_reset      = CAM_PIN_RESET,
    .pin_xclk       = CAM_PIN_XCLK,
    .pin_sccb_sda   = CAM_PIN_SIOD,
    .pin_sccb_scl   = CAM_PIN_SIOC,
    .pin_d7         = CAM_PIN_D7,
    .pin_d6         = CAM_PIN_D6,
    .pin_d5         = CAM_PIN_D5,
    .pin_d4         = CAM_PIN_D4,
    .pin_d3         = CAM_PIN_D3,
    .pin_d2         = CAM_PIN_D2,
    .pin_d1         = CAM_PIN_D1,
    .pin_d0         = CAM_PIN_D0,
    .pin_vsync      = CAM_PIN_VSYNC,
    .pin_href       = CAM_PIN_HREF,
    .pin_pclk       = CAM_PIN_PCLK,
    .xclk_freq_hz   = 20000000,
    .ledc_timer     = LEDC_TIMER_0,
    .ledc_channel   = LEDC_CHANNEL_0,
    .pixel_format   = PIXFORMAT_JPEG,
    .frame_size     = FRAMESIZE_SXGA,
    .jpeg_quality   = IMAGE_QUALITY,
    .fb_count       = 1,
    .fb_location    = CAMERA_FB_IN_PSRAM,
    .grab_mode      = CAMERA_GRAB_WHEN_EMPTY,
};

esp_err_t init_camera(void)
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed: %d", err);
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    s->set_brightness(s, 1);         // Helps shadow detail
    s->set_contrast(s, 1);           // Mild boost to edge clarity
    s->set_saturation(s, 0);         // Neutral color
    s->set_gainceiling(s, (gainceiling_t)5); // Prevent over-amplified noise
    s->set_exposure_ctrl(s, 1);      // Auto exposure ON
    s->set_aec2(s, 0);               // Disable AEC2 for better consistency
    s->set_aec_value(s, 800);        // Tweak depending on ambient light
    s->set_whitebal(s, 1);           // Auto WB on
    s->set_awb_gain(s, 1);           // Better accuracy
    s->set_denoise(s, 1);            // Reduces JPEG noise

    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}
#endif // ESP_CAMERA_SUPPORTED

// WebSocket event handler
static void websocket_event_handler(void *handler_args, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            websocket_connected = true;
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WebSocket disconnected");
            websocket_connected = false;
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "WebSocket data received: len=%d", data->data_len);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error");
            websocket_connected = false;
            break;
        default:
            break;
    }
}

bool init_websocket_client(void)
{
    if (ws_client) {
        return true;
    }

    char uri[128];
    snprintf(uri, sizeof(uri), "ws://%s:%d%s", STREAM_SERVER_HOST, STREAM_SERVER_PORT, STREAM_SERVER_PATH);

    esp_websocket_client_config_t config = {
        .uri = uri,
        .disable_auto_reconnect = false,
        .reconnect_timeout_ms = 5000,
        // .ping_interval_sec = 5,
        // .pingpong_timeout_sec = 5,
    };

    ws_client = esp_websocket_client_init(&config);
    if (!ws_client) {
        ESP_LOGE(TAG, "Failed to init WebSocket client");
        return false;
    }

    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY,
                                  websocket_event_handler, NULL);

    if (esp_websocket_client_start(ws_client) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket client");
        esp_websocket_client_destroy(ws_client);
        ws_client = NULL;
        return false;
    }

    ESP_LOGI(TAG, "WebSocket client started: %s", uri);
    return true;
}

bool send_frame_over_websocket(const uint8_t *frame, size_t len, bool detected) {
    if (!websocket_connected) return false;
    // build payload: 1 byte flag + image data
    size_t total = len + 1;
    ESP_LOGI(TAG, "Sending frame: detected=%d, size=%zu bytes", detected, total);
    uint8_t *buf = malloc(total);
    if (!buf) return false;
    buf[0] = detected ? 1 : 0;
    memcpy(buf+1, frame, len);
    int r = esp_websocket_client_send_bin(ws_client, (const char*)buf, total, portMAX_DELAY);
    free(buf);
    if (r < 0) {
        websocket_connected = false;
        return false;
    }
    return true;
}
// Cleanup WebSocket client
void deinit_websocket_client(void)
{
    if (ws_client) {
        esp_websocket_client_stop(ws_client);
        esp_websocket_client_destroy(ws_client);
        ws_client = NULL;
        websocket_connected = false;
    }
}
