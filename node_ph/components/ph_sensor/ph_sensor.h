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

// Режимы работы датчика pH
typedef enum {
    PH_SENSOR_MODE_REAL = 0,              ///< Реальный датчик
    PH_SENSOR_MODE_MOCK_REACTIVE = 1,     ///< Mock с реакцией на дозирование
    PH_SENSOR_MODE_MOCK_NON_REACTIVE = 2  ///< Mock без реакции (предопределенная последовательность)
} ph_sensor_mode_t;

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

/**
 * @brief Обновление mock значения pH на основе дозирования
 * 
 * Имитирует изменение pH при дозировании в mock режиме
 * 
 * @param pump_id ID насоса (PUMP_PH_UP или PUMP_PH_DOWN)
 * @param dose_ml Доза в мл
 * @return ESP_OK при успехе
 */
esp_err_t ph_sensor_update_mock_for_dosing(uint8_t pump_id, float dose_ml);

/**
 * @brief Установка режима работы датчика pH
 * 
 * @param mode Режим работы (PH_SENSOR_MODE_REAL, PH_SENSOR_MODE_MOCK_REACTIVE, PH_SENSOR_MODE_MOCK_NON_REACTIVE)
 * @return ESP_OK при успехе
 */
esp_err_t ph_sensor_set_mode(ph_sensor_mode_t mode);

/**
 * @brief Получение текущего режима работы датчика pH
 * 
 * @return Текущий режим работы
 */
ph_sensor_mode_t ph_sensor_get_mode(void);

#ifdef __cplusplus
}
#endif

#endif // PH_SENSOR_H

