#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include "esp_err.h"
#include "esp_camera.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

extern camera_fb_t *current_frame;
extern bool websocket_connected;

esp_err_t init_camera(void);
// stream_camera_task moved to main.c
bool send_frame_over_websocket(const uint8_t *frame_data, size_t frame_len, bool object_detected);
bool init_websocket_client(void);
void deinit_websocket_client(void);
#endif // CAMERA_STREAM_H