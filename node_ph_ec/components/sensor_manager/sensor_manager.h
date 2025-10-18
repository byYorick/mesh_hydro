/**
 * @file sensor_manager.h
 * @brief Управление pH и EC датчиками (Trema I2C)
 * 
 * TODO: Портировать из hydro1.0/components/sensor_manager/
 * 
 * Функции:
 * - Чтение pH датчика (I2C 0x0A)
 * - Чтение EC датчика (I2C 0x08)  
 * - Чтение температуры
 * - Калибровка датчиков
 * - Retry логика (3 попытки)
 * - Валидация данных
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация менеджера датчиков
 * 
 * @return ESP_OK при успехе
 */
esp_err_t sensor_manager_init(void);

/**
 * @brief Чтение pH датчика
 * 
 * @param ph Указатель для значения pH
 * @return ESP_OK при успехе
 */
esp_err_t sensor_manager_read_ph(float *ph);

/**
 * @brief Чтение EC датчика
 * 
 * @param ec Указатель для значения EC (мСм/см)
 * @return ESP_OK при успехе
 */
esp_err_t sensor_manager_read_ec(float *ec);

/**
 * @brief Чтение температуры
 * 
 * @param temp Указатель для значения температуры (°C)
 * @return ESP_OK при успехе
 */
esp_err_t sensor_manager_read_temp(float *temp);

/**
 * @brief Калибровка pH датчика
 * 
 * @param calibration_value Калибровочное значение
 * @return ESP_OK при успехе
 */
esp_err_t sensor_manager_calibrate_ph(float calibration_value);

/**
 * @brief Калибровка EC датчика
 * 
 * @param calibration_value Калибровочное значение
 * @return ESP_OK при успехе
 */
esp_err_t sensor_manager_calibrate_ec(float calibration_value);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_MANAGER_H

