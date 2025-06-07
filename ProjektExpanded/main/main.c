#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
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
//other
#include "credentials.h"
#include "esp_sntp.h"
#include "lwip/err.h"  //error handling
#include "lwip/netdb.h"
#include "lwip/sockets.h"  //sockets
#include "lwip/sys.h"      //system applications
#include "utils.h"
#include "wifi.h"
#include "esp_err.h"

//camera and webscoket streaming
#include "esp_camera.h"
#include "camera_stream.h"
#include "ultrasonic.h"
#include "sg90.h"
#include "my_ssd1306.h"
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#define MAX_DISTANCE_CM 200
// Task handles
TaskHandle_t streamCameraTaskHandle = NULL;
TaskHandle_t ultrasonicTaskHandle = NULL;
TaskHandle_t servoTaskHandle = NULL;
TaskHandle_t mainControlTaskHandle = NULL;

// Synchronization primitives
SemaphoreHandle_t systemMutex;
QueueHandle_t servoCommandQueue;
QueueHandle_t ultrasonicDataQueue;

// System state
typedef struct {
    float current_distance;
    bool distance_valid;
    TickType_t last_measurement_time;
} ultrasonic_data_t;

typedef enum {
    SERVO_CMD_INIT,
    SERVO_CMD_OPEN_GATE,
    SERVO_CMD_CLOSE_GATE
} servo_command_t;

typedef struct {
    float baseline_distance;
    bool gate_is_closed;
    bool camera_running;
    bool system_initialized;
    TickType_t plate_recognition_start_time;
} system_state_t;

system_state_t system_state = {
    .baseline_distance = 0.0,
    .gate_is_closed = true,
    .camera_running = false,
    .system_initialized = false,
    .plate_recognition_start_time = 0
};

typedef struct {
  const ssd1306_config_t *config;  // Pointer to the SSD1306 configuration
  i2c_handler_t *i2c_handler;      // Pointer to the I2C handler
  oled_display_t *oled_display;    // Pointer to the OLED display
} TaskParameters;

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

// Suspend all non-critical tasks
void suspend_background_tasks(void) {
    if (ultrasonicTaskHandle != NULL) {
        vTaskSuspend(ultrasonicTaskHandle);
    }
    if (servoTaskHandle != NULL) {
        vTaskSuspend(servoTaskHandle);
    }
    ESP_LOGI("SYSTEM", "Background tasks suspended for camera operation");
}

// Resume all background tasks
void resume_background_tasks(void) {
    if (ultrasonicTaskHandle != NULL) {
        vTaskResume(ultrasonicTaskHandle);
    }
    if (servoTaskHandle != NULL) {
        vTaskResume(servoTaskHandle);
    }
    ESP_LOGI("SYSTEM", "Background tasks resumed");
}

// Ultrasonic sensor task
void ultrasonic_task(void *pvParameters) {
    ultrasonic_sensor_t *sensor = (ultrasonic_sensor_t *)pvParameters;
    ultrasonic_data_t data;
    
    ESP_LOGI("ULTRASONIC", "Ultrasonic task started");
    
    while (1) {
        float distance;
        esp_err_t res = ultrasonic_measure(sensor, MAX_DISTANCE_CM, &distance);
        
        if (res == ESP_OK) {
            data.current_distance = distance * 100; // Convert to cm
            data.distance_valid = true;
            data.last_measurement_time = xTaskGetTickCount();
            
            // Send data to queue (non-blocking)
            if (xQueueSend(ultrasonicDataQueue, &data, 0) != pdTRUE) {
                ESP_LOGW("ULTRASONIC", "Failed to send distance data to queue");
            }
            
            ESP_LOGD("ULTRASONIC", "Distance measured: %.2f cm", data.current_distance);
        } else {
            data.distance_valid = false;
            ESP_LOGW("ULTRASONIC", "Distance measurement failed: %s", esp_err_to_name(res));
        }
        
        // Normal measurement interval
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Servo control task
void servo_task(void *pvParameters) {
    servo_command_t command;
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(5000); // 5 second timeout
    
    ESP_LOGI("SERVO", "Servo task started");
    
    while (1) {
        // Wait for servo commands with timeout
        if (xQueueReceive(servoCommandQueue, &command, xMaxBlockTime) == pdTRUE) {
            xSemaphoreTake(systemMutex, portMAX_DELAY);
            
            switch (command) {
                case SERVO_CMD_INIT:
                    ESP_LOGI("SERVO", "Initializing servo");
                    servo_init();
                    break;
                    
                case SERVO_CMD_OPEN_GATE:
                    ESP_LOGI("SERVO", "Opening gate");
                    servo_open_gate();
                    system_state.gate_is_closed = false;
                    break;
                    
                case SERVO_CMD_CLOSE_GATE:
                    ESP_LOGI("SERVO", "Closing gate");
                    servo_close_gate();
                    system_state.gate_is_closed = true;
                    break;
                    
                default:
                    ESP_LOGW("SERVO", "Unknown servo command: %d", command);
                    break;
            }
            
            xSemaphoreGive(systemMutex);
        } else {
            // Timeout occurred - do health check
            ESP_LOGI("SERVO", "No commands received in 5 seconds, checking system state");
            
            if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Reset to known good state if needed
                if (!system_state.gate_is_closed && !system_state.camera_running) {
                    ESP_LOGW("SERVO", "Gate appears stuck open, attempting to close");
                    servo_close_gate();
                    system_state.gate_is_closed = true;
                }
                xSemaphoreGive(systemMutex);
            }
        }
    }
}

// Get baseline distance measurement
float get_baseline_distance(ultrasonic_sensor_t *sensor) {
    float distance_array[5];
    int valid_measurements = 0;
    
    ESP_LOGI("CALIBRATION", "Starting baseline distance calibration");
    
    for (int i = 0; i < 5; i++) {
        float distance;
        esp_err_t res = ultrasonic_measure(sensor, MAX_DISTANCE_CM, &distance);
        
        if (res == ESP_OK && (distance * 100) <= MAX_DISTANCE_CM) {
            distance_array[valid_measurements] = distance * 100;
            valid_measurements++;
            ESP_LOGI("CALIBRATION", "Valid measurement %d: %.2f cm", valid_measurements, distance * 100);
        } else {
            ESP_LOGW("CALIBRATION", "Invalid measurement %d", i + 1);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    if (valid_measurements == 0) {
        ESP_LOGE("CALIBRATION", "No valid baseline measurements obtained");
        return -1;
    }
    
    // Calculate average
    float sum = 0;
    for (int i = 0; i < valid_measurements; i++) {
        sum += distance_array[i];
    }
    
    float avg = sum / valid_measurements;
    ESP_LOGI("CALIBRATION", "Baseline distance calibrated: %.2f cm", avg);
    
    return avg;
}

// Main control logic task
void main_control_task(void *pvParameters) {
    ultrasonic_data_t ultrasonic_data;
    servo_command_t servo_cmd;
    bool plate_recognition_complete = false;
    TickType_t current_time;
    
    ESP_LOGI("MAIN_CONTROL", "Main control task started");
    
    // Initialize servo
    servo_cmd = SERVO_CMD_INIT;
    xQueueSend(servoCommandQueue, &servo_cmd, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for servo init
    
    while (1) {
        // Check if plate recognition timer has elapsed
        if (system_state.camera_running) {
            current_time = xTaskGetTickCount();
            TickType_t elapsed = current_time - system_state.plate_recognition_start_time;
            
            if (elapsed >= pdMS_TO_TICKS(20000)) { // 20 seconds elapsed
                plate_recognition_complete = true;
                ESP_LOGI("MAIN_CONTROL", "Plate recognition timer elapsed: %d ticks", (int)elapsed);
            }
        }
        
        // Process plate recognition completion
        if (plate_recognition_complete) {
            xSemaphoreTake(systemMutex, portMAX_DELAY);
            
            if (system_state.camera_running) {
                // Stop camera and resume normal operation
                ESP_LOGI("MAIN_CONTROL", "Stopping camera and resuming normal tasks");
                vTaskSuspend(streamCameraTaskHandle);
                system_state.camera_running = false;
                resume_background_tasks();
                
                ESP_LOGI("MAIN_CONTROL", "Plate recognition completed, opening gate");
                
                // Open gate
                servo_cmd = SERVO_CMD_OPEN_GATE;
                if (xQueueSend(servoCommandQueue, &servo_cmd, pdMS_TO_TICKS(500)) != pdTRUE) {
                    ESP_LOGE("MAIN_CONTROL", "Failed to send open gate command to servo queue");
                }
                
                xSemaphoreGive(systemMutex);
                
                // Wait 5 seconds then close gate
                vTaskDelay(pdMS_TO_TICKS(5000));
                
                xSemaphoreTake(systemMutex, portMAX_DELAY);
                servo_cmd = SERVO_CMD_CLOSE_GATE;
                if (xQueueSend(servoCommandQueue, &servo_cmd, pdMS_TO_TICKS(500)) != pdTRUE) {
                    ESP_LOGE("MAIN_CONTROL", "Failed to send close gate command to servo queue");
                    // Force the state to closed since we couldn't send the command
                    system_state.gate_is_closed = true;
                }
                
                ESP_LOGI("MAIN_CONTROL", "Gate operation completed");
                plate_recognition_complete = false;
            }
            
            xSemaphoreGive(systemMutex);
        }
        
        // Get latest ultrasonic data
        if (xQueueReceive(ultrasonicDataQueue, &ultrasonic_data, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (!ultrasonic_data.distance_valid) {
                continue;
            }
            
            xSemaphoreTake(systemMutex, portMAX_DELAY);
            
            // Skip distance checking while camera is running
            if (system_state.camera_running) {
                xSemaphoreGive(systemMutex);
                continue;
            }
            
            // Normal operation - check for vehicle detection
            if (system_state.system_initialized && system_state.gate_is_closed) {
                float error_margin = system_state.baseline_distance * 0.2;
                float diff = fabs(system_state.baseline_distance - ultrasonic_data.current_distance);
                
                if (diff > error_margin) {
                    ESP_LOGI("MAIN_CONTROL", "Vehicle detected! Distance change: %.2f cm", diff);
                    
                    // Start camera for plate recognition
                    suspend_background_tasks();
                    
                    // Boost camera task priority temporarily
                    vTaskPrioritySet(streamCameraTaskHandle, 15);
                    vTaskResume(streamCameraTaskHandle);
                    
                    system_state.camera_running = true;
                    system_state.plate_recognition_start_time = xTaskGetTickCount();
                    
                    ESP_LOGI("MAIN_CONTROL", "Camera started for plate recognition, start time: %d", (int)system_state.plate_recognition_start_time);
                }
            }
            
            xSemaphoreGive(systemMutex);
        }
        
        // Small delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void) {
    init_hw_services();
    nvs_init();
    init_wifi();
    configure_time();

    // Create synchronization primitives
    systemMutex = xSemaphoreCreateMutex();
    servoCommandQueue = xQueueCreate(5, sizeof(servo_command_t));
    ultrasonicDataQueue = xQueueCreate(10, sizeof(ultrasonic_data_t));
    
    if (systemMutex == NULL || servoCommandQueue == NULL || ultrasonicDataQueue == NULL) {
        ESP_LOGE("SYSTEM", "Failed to create synchronization primitives");
        return;
    }

    // Initialize camera
    #if ESP_CAMERA_SUPPORTED
        if (init_camera() != ESP_OK) {
            ESP_LOGE("CAMERA", "Failed to initialize camera!");
            return;
        }
        
        // Create camera task (suspended initially)
        xTaskCreatePinnedToCore(stream_camera_task, "stream_camera_task", 
                              4096, NULL, 8, &streamCameraTaskHandle, 1);
        vTaskSuspend(streamCameraTaskHandle);
        ESP_LOGI("CAMERA", "Camera task created and suspended");
    #else
        ESP_LOGE("CAMERA", "Camera not supported");
        return;
    #endif

    // Initialize ultrasonic sensor
    static ultrasonic_sensor_t sensor = {
        .trigger_pin = 32,
        .echo_pin = 33
    };
    
    if (ultrasonic_init(&sensor) != ESP_OK) {
        ESP_LOGE("ULTRASONIC", "Failed to initialize ultrasonic sensor");
        return;
    }

    // Get baseline distance
    system_state.baseline_distance = get_baseline_distance(&sensor);
    if (system_state.baseline_distance < 0) {
        ESP_LOGE("SYSTEM", "Failed to calibrate baseline distance");
        return;
    }
    
    system_state.system_initialized = true;
    ESP_LOGI("SYSTEM", "System initialized with baseline distance: %.2f cm", 
             system_state.baseline_distance);

    // Create tasks
    xTaskCreatePinnedToCore(ultrasonic_task, "ultrasonic_task", 
                          2048, &sensor, 6, &ultrasonicTaskHandle, 0);
    
    xTaskCreatePinnedToCore(servo_task, "servo_task", 
                          2048, NULL, 5, &servoTaskHandle, 0);
    
    xTaskCreatePinnedToCore(main_control_task, "main_control_task", 
                          3072, NULL, 10, &mainControlTaskHandle, 0);

    ESP_LOGI("SYSTEM", "All tasks created successfully");
    
    // Main task can now idle or be deleted
    vTaskDelete(NULL);
}