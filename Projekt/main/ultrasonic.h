#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_ERR_ULTRASONIC_PING         0x200
#define ESP_ERR_ULTRASONIC_PING_TIMEOUT 0x201
#define ESP_ERR_ULTRASONIC_ECHO_TIMEOUT 0x202

/**
 * Device descriptor
 */
typedef struct
{
    gpio_num_t trigger_pin; //!< GPIO output pin for trigger
    gpio_num_t echo_pin;    //!< GPIO input pin for echo
} ultrasonic_sensor_t;

/**
 * @brief Init ranging module
 *
 * @param dev Pointer to the device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t ultrasonic_init(const ultrasonic_sensor_t *dev);

/**
 * @brief Measure time between ping and echo
 *
 * @param dev Pointer to the device descriptor
 * @param max_time_us Maximal time to wait for echo
 * @param[out] time_us Time, us
 * @return `ESP_OK` on success, otherwise:
 *         - ::ESP_ERR_ULTRASONIC_PING         - Invalid state (previous ping is not ended)
 *         - ::ESP_ERR_ULTRASONIC_PING_TIMEOUT - Device is not responding
 *         - ::ESP_ERR_ULTRASONIC_ECHO_TIMEOUT - Distance is too big or wave is scattered
 */
esp_err_t ultrasonic_measure_raw(const ultrasonic_sensor_t *dev, uint32_t max_time_us, uint32_t *time_us);

/**
 * @brief Measure distance in meters
 *
 * @param dev Pointer to the device descriptor
 * @param max_distance Maximal distance to measure, meters
 * @param[out] distance Distance in meters
 * @return `ESP_OK` on success, otherwise:
 *         - ::ESP_ERR_ULTRASONIC_PING         - Invalid state (previous ping is not ended)
 *         - ::ESP_ERR_ULTRASONIC_PING_TIMEOUT - Device is not responding
 *         - ::ESP_ERR_ULTRASONIC_ECHO_TIMEOUT - Distance is too big or wave is scattered
 */
esp_err_t ultrasonic_measure(const ultrasonic_sensor_t *dev, float max_distance, float *distance);

/**
 * @brief Measure distance in centimeters
 *
 * @param dev Pointer to the device descriptor
 * @param max_distance Maximal distance to measure, centimeters
 * @param[out] distance Distance in centimeters
 * @return `ESP_OK` on success, otherwise:
 *         - ::ESP_ERR_ULTRASONIC_PING         - Invalid state (previous ping is not ended)
 *         - ::ESP_ERR_ULTRASONIC_PING_TIMEOUT - Device is not responding
 *         - ::ESP_ERR_ULTRASONIC_ECHO_TIMEOUT - Distance is too big or wave is scattered
 */
esp_err_t ultrasonic_measure_cm(const ultrasonic_sensor_t *dev, uint32_t max_distance, uint32_t *distance);

/**
 * @brief Measure distance in meters with temperature compensation
 *
 * This function measures the distance by taking into account the temperature of the air,
 * which affects the speed of sound. This method improves the accuracy of measurements
 * in various environmental conditions.
 *
 * @param dev Pointer to the device descriptor
 * @param max_distance Maximal distance to measure, meters
 * @param[out] distance Distance in meters
 * @param temperature_c Current air temperature in degrees Celsius
 * @return `ESP_OK` on success, otherwise:
 *         - ::ESP_ERR_ULTRASONIC_PING         - Invalid state (previous ping is not ended)
 *         - ::ESP_ERR_ULTRASONIC_PING_TIMEOUT - Device is not responding
 *         - ::ESP_ERR_ULTRASONIC_ECHO_TIMEOUT - Distance is too big or wave is scattered
 */
esp_err_t ultrasonic_measure_temp_compensated(const ultrasonic_sensor_t *dev, float max_distance, float *distance, float temperature_c);

/**
 * @brief Measure distance in centimeters with temperature compensation
 *
 * Similar to ultrasonic_measure_temp_compensated but provides the distance in centimeters.
 * It factors in the temperature of the air for more accurate measurements across a range of environmental conditions.
 *
 * @param dev Pointer to the device descriptor
 * @param max_distance Maximal distance to measure, centimeters
 * @param[out] distance Distance in centimeters
 * @param temperature_c Current air temperature in degrees Celsius
 * @return `ESP_OK` on success, otherwise:
 *         - ::ESP_ERR_ULTRASONIC_PING         - Invalid state (previous ping is not ended)
 *         - ::ESP_ERR_ULTRASONIC_PING_TIMEOUT - Device is not responding
 *         - ::ESP_ERR_ULTRASONIC_ECHO_TIMEOUT - Distance is too big or wave is scattered
 */
esp_err_t ultrasonic_measure_cm_temp_compensated(const ultrasonic_sensor_t *dev, uint32_t max_distance, uint32_t *distance, float temperature_c);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __ULTRASONIC_H__ */
