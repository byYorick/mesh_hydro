/**
 * @file ec_sensor.h
 * @brief Trema EC sensor driver для NODE pH/EC
 */

#ifndef EC_SENSOR_H
#define EC_SENSOR_H

#include "esp_err.h"
#include "driver/i2c.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C адрес Trema EC sensor
#define EC_SENSOR_ADDR 0x64

/**
 * @brief Инициализация EC сенсора
 * 
 * @param i2c_port Порт I2C
 * @return ESP_OK при успехе
 */
esp_err_t ec_sensor_init(i2c_port_t i2c_port);

/**
 * @brief Чтение значения EC (электропроводности)
 * 
 * @param ec Указатель для сохранения значения (mS/cm)
 * @return ESP_OK при успехе
 */
esp_err_t ec_sensor_read(float *ec);

/**
 * @brief Калибровка EC сенсора
 * 
 * @param reference_tds Эталонное значение TDS (ppm)
 * @return ESP_OK при успехе
 */
esp_err_t ec_sensor_calibrate(uint16_t reference_tds);

/**
 * @brief Установка температуры для компенсации
 * 
 * @param temperature Температура в °C
 * @return ESP_OK при успехе
 */
esp_err_t ec_sensor_set_temperature(float temperature);

/**
 * @brief Проверка подключения датчика
 * 
 * @return true если датчик подключен
 */
bool ec_sensor_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // EC_SENSOR_H

