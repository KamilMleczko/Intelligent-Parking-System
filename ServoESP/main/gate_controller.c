// gate_controller.c

#include "gate_controller.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "GATE_CTRL";

typedef enum {
    GATE_CLOSED = 0,
    GATE_OPEN
} gate_state_t;

struct GateController {
    gate_state_t          state;
    TimerHandle_t         timer;      // one-shot 10s
    const ssd1306_config_t *oled_cfg;
    i2c_handler_t         *i2c_h;
    oled_display_t        *oled_disp;
};

/// Timer callback—automatically closes the gate
static void _timer_cb(TimerHandle_t t) {
    GateController *gc = pvTimerGetTimerID(t);
    ESP_LOGI(TAG, "Timer expired: closing gate");
    servo_close_gate();
    clear_screen(gc->oled_cfg, gc->oled_disp, gc->i2c_h);
    show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 1, "Gate Closed");
    gc->state = GATE_CLOSED;
}

GateController *gate_controller_create(
    const ssd1306_config_t *oled_cfg,
    i2c_handler_t          *i2c_h,
    oled_display_t         *oled_disp)
{
    GateController *gc = pvPortMalloc(sizeof(*gc));
    if (!gc) return NULL;
    gc->state     = GATE_CLOSED;
    gc->oled_cfg  = oled_cfg;
    gc->i2c_h     = i2c_h;
    gc->oled_disp = oled_disp;

    // Create a one-shot 10s timer, storing 'gc' as its ID
    gc->timer = xTimerCreate(
        "gate_tmr",
        pdMS_TO_TICKS(10000),
        pdFALSE,
        (void*)gc,
        _timer_cb);
    if (!gc->timer) {
        vPortFree(gc);
        return NULL;
    }

    // On startup, show “Gate Closed”
    clear_screen(oled_cfg, oled_disp, i2c_h);
    show_text(oled_cfg, oled_disp, i2c_h, 1, "Gate Closed");
    return gc;
}

void gate_controller_destroy(GateController *gc) {
    if (!gc) return;
    if (gc->timer) {
        xTimerDelete(gc->timer, 0);
    }
    vPortFree(gc);
}

void gate_controller_handle_message(GateController *gc, const char *json_msg) {
    cJSON *root  = cJSON_Parse(json_msg);
    if (!root) {
        ESP_LOGW(TAG, "Invalid JSON: %s", json_msg);
        return;
    }
    cJSON *act   = cJSON_GetObjectItem(root, "action");
    cJSON *plate = cJSON_GetObjectItem(root, "car_plate");
    const char *action    = cJSON_IsString(act)   ? act->valuestring   : "";
    const char *plate_str = cJSON_IsString(plate) ? plate->valuestring : "";

    // Only handle when gate is closed
    if (gc->state == GATE_CLOSED) {
        clear_screen(gc->oled_cfg, gc->oled_disp, gc->i2c_h);

        if (strcmp(action, "open") == 0) {
            ESP_LOGI(TAG, "OPEN for %s", plate_str);
            show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 1, "Authorized");
            show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 3, (char*)plate_str);
            servo_open_gate();
            gc->state = GATE_OPEN;
            xTimerStart(gc->timer, 0);

        } else if (strcmp(action, "close") == 0) {
            ESP_LOGI(TAG, "CLOSE for %s", plate_str);
            show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 1, "Gate Closed");
            show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 3, (char*)plate_str);

        } else {
            ESP_LOGW(TAG, "UNAUTH for %s", plate_str);
            show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 1, "Unauthorized");
            show_text(gc->oled_cfg, gc->oled_disp, gc->i2c_h, 3, (char*)plate_str);
        }
    } else {
        ESP_LOGI(TAG, "Gate already open, ignoring %s", action);
    }

    cJSON_Delete(root);
}
