/**
 * @file ph_sensor.h
 * @brief Trema pH sensor driver для NODE pH/EC
 * 
 * Упрощённая версия для mesh узла
 */

#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "esp_err.h"
#include "driver/i2c.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C адрес Trema pH sensor
#define PH_SENSOR_ADDR 0x4D

/**
 * @brief Инициализация pH сенсора
 * 
 * @param i2c_port Порт I2C (I2C_NUM_0 или I2C_NUM_1)
 * @return ESP_OK при успехе
 */
esp_err_t ph_sensor_init(i2c_port_t i2c_port);

/**
 * @brief Чтение значения pH
 * 
 * @param ph Указатель для сохранения значения (0.0-14.0)
 * @return ESP_OK при успехе
 */
esp_err_t ph_sensor_read(float *ph);

/**
 * @brief Калибровка pH сенсора
 * 
 * @param reference_ph Эталонное значение pH (обычно 4.0, 7.0 или 10.0)
 * @return ESP_OK при успехе
 */
esp_err_t ph_sensor_calibrate(float reference_ph);

/**
 * @brief Проверка стабильности измерения
 * 
 * @return true если измерение стабильно
 */
bool ph_sensor_is_stable(void);

/**
 * @brief Проверка подключения датчика
 * 
 * @return true если датчик подключен и отвечает
 */
bool ph_sensor_is_connected(void);

/**
 * @brief Проверка режима mock
 * 
 * @return true если датчик работает в mock режиме
 */
bool ph_sensor_is_mock_mode(void);

/**
 * @brief Принудительное переключение в mock режим
 * 
 * @param enable true - включить mock, false - отключить
 * @return ESP_OK при успехе
 */
esp_err_t ph_sensor_force_mock_mode(bool enable);

#ifdef __cplusplus
}
#endif

#endif // PH_SENSOR_H

