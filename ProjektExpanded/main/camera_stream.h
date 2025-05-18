#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

esp_err_t init_camera(void);
void stream_camera_task(void *pvParameters);
bool send_frame_over_websocket(const uint8_t *frame_data, size_t frame_len);
bool connect_socket(void);

#endif // CAMERA_STREAM_H