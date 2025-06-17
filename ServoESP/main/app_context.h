#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "my_ssd1306.h"       // for ssd1306_config_t, i2c_handler_t, oled_display_t
#include "esp_websocket_client.h" // for esp_websocket_client_handle_t

/**
 * @brief Global application context holding shared peripherals and state.
 */
typedef struct {
    // OLED display context
    ssd1306_config_t            oled_cfg;
    i2c_handler_t               oled_i2c;
    oled_display_t              oled_disp;

    // WebSocket client handle for gate listener
    esp_websocket_client_handle_t ws_client;
} AppContext;

/**
 * @brief The single, global application context instance.
 */
extern AppContext app_ctx;

/**
 * @brief Initialize the global application context.
 *        Sets up OLED configuration, I2C, display startup, and resets WS client.
 */
void app_context_init(void);


#endif // APP_CONTEXT_H
