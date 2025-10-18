/**
 * @file lux_sensor.h
 * @brief Драйвер датчика освещенности Trema Lux
 * 
 * Trema Module Light Sensor
 * I2C адрес: 0x12
 */

#ifndef LUX_SENSOR_H
#define LUX_SENSOR_H

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUX_SENSOR_I2C_ADDR  0x12

/**
 * @brief Инициализация датчика освещенности
 * 
 * @param i2c_port I2C порт
 * @return ESP_OK при успехе
 */
esp_err_t lux_sensor_init(i2c_port_t i2c_port);

/**
 * @brief Чтение освещенности
 * 
 * @param lux Указатель для значения освещенности (lux)
 * @return ESP_OK при успехе
 */
esp_err_t lux_sensor_read(uint16_t *lux);

#ifdef __cplusplus
}
#endif

#endif // LUX_SENSOR_H

