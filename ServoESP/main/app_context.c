#include "app_context.h"

AppContext app_ctx;

void app_context_init(void) {
    app_ctx.oled_cfg    = create_config();
    i2c_master_init(&app_ctx.oled_cfg, &app_ctx.oled_i2c, &app_ctx.oled_disp);
    oled_cmd_init(&app_ctx.oled_disp, &app_ctx.oled_cfg, &app_ctx.oled_i2c);
    clear_oled_display_struct(&app_ctx.oled_disp);
    clear_screen(&app_ctx.oled_cfg, &app_ctx.oled_disp, &app_ctx.oled_i2c);
    set_brightness(&app_ctx.oled_cfg, &app_ctx.oled_i2c, 200);

    app_ctx.ws_client = NULL;
}
