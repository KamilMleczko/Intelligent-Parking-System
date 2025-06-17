#ifndef GATE_CONTROLLER_H
#define GATE_CONTROLLER_H

#include "my_ssd1306.h"
#include "sg90.h"

/**
 * Opaque handle for gate controller.
 * You never touch its internals from outside.
 */
typedef struct GateController GateController;

/**
 * Create and initialize a controller.
 * Pass in pointers to your already-configured OLED and servo functions.
 */
GateController *gate_controller_create(
    const ssd1306_config_t *oled_cfg,
    i2c_handler_t          *i2c_h,
    oled_display_t         *oled_disp);

/**
 * Clean up when you’re done (if ever).
 */
void gate_controller_destroy(GateController *gc);

/**
 * Pass each incoming JSON message here.
 * The controller will:
 *  - parse action/car_plate
 *  - update its internal state machine
 *  - display text via OLED
 *  - open/close servo
 *  - (re)start its own 10s timer when opening
 */
void gate_controller_handle_message(GateController *gc, const char *json_msg);

#endif // GATE_CONTROLLER_H