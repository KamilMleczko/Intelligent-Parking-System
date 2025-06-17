/* ws_listener.c
 * Passive WebSocket client: listens to JSON commands at /ws/gate
 * Displays car plate on OLED and opens/closes gate via servo.
 * Assumes OLED context (oled_cfg, oled_i2c, oled_disp) are defined globally
 * elsewhere.
 */

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_context.h"
#include "cJSON.h"
#include "credentials.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "my_ssd1306.h"  // show_text, clear_screen
#include "sg90.h"        // servo_open_gate, servo_close_gate
#include "gate_controller.h"

static const char *TAG = "WS_LISTENER";

static GateController *gate_controller = NULL;

// WebSocket event handler
static void ws_event_handler(void *arg, esp_event_base_t base, int32_t event_id,
                             void *event_data) {
  switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      ESP_LOGI(TAG, "WebSocket connected");
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
      ESP_LOGW(TAG, "WebSocket disconnected");
      break;
    case WEBSOCKET_EVENT_DATA: {
      esp_websocket_event_data_t *evt =
          (esp_websocket_event_data_t *)event_data;
      if (evt->op_code == 0x1 && evt->data_len > 0) {
        if (!gate_controller) {
          gate_controller = gate_controller_create(
              &app_ctx.oled_cfg, &app_ctx.oled_i2c, &app_ctx.oled_disp);
        }
        gate_controller_handle_message(
            gate_controller, evt->data_ptr);
      }
      break;
    }
    case WEBSOCKET_EVENT_ERROR:
      ESP_LOGE(TAG, "WebSocket error");
      break;
    default:
      break;
  }
}

/**
 * @brief Start the passive WebSocket gate listener
 * @return true on successful start
 */
bool start_ws_listener(void) {
    // If already initialized, nothing to do
    if (app_ctx.ws_client) {
        return true;
    }

    // Build the URI
    char uri[128];
    snprintf(uri, sizeof(uri), "ws://%s:%d/ws/gate",
             STREAM_SERVER_HOST, STREAM_SERVER_PORT);

    // Configure the client
    esp_websocket_client_config_t cfg = {
        .uri                    = uri,
        .disable_auto_reconnect = false,
        .reconnect_timeout_ms   = 5000
    };

    // Init
    app_ctx.ws_client = esp_websocket_client_init(&cfg);
    if (!app_ctx.ws_client) {
        ESP_LOGE(TAG, "esp_websocket_client_init failed");
        return false;
    }

    // Register events
    esp_websocket_register_events(
        app_ctx.ws_client,
        WEBSOCKET_EVENT_ANY,
        ws_event_handler,
        NULL
    );

    // Start
    if (esp_websocket_client_start(app_ctx.ws_client) != ESP_OK) {
        ESP_LOGE(TAG, "esp_websocket_client_start failed");
        esp_websocket_client_destroy(app_ctx.ws_client);
        app_ctx.ws_client = NULL;
        return false;
    }

    ESP_LOGI(TAG, "WebSocket listener started at %s", uri);
    return true;
}