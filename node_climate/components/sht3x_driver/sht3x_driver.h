/**
 * @file sht3x_driver.h
 * @brief Драйвер датчика SHT3x (температура + влажность)
 * 
 * Sensirion SHT3x I2C датчик
 * I2C адрес: 0x44 (по умолчанию)
 */

#ifndef SHT3X_DRIVER_H
#define SHT3X_DRIVER_H

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHT3X_I2C_ADDR  0x44

/**
 * @brief Инициализация SHT3x датчика
 * 
 * @param i2c_port I2C порт (I2C_NUM_0 или I2C_NUM_1)
 * @return ESP_OK при успехе
 */
esp_err_t sht3x_init(i2c_port_t i2c_port);

/**
 * @brief Чтение температуры и влажности
 * 
 * @param temperature Указатель для температуры (°C)
 * @param humidity Указатель для влажности (%)
 * @return ESP_OK при успехе
 */
esp_err_t sht3x_read(float *temperature, float *humidity);

/**
 * @brief Программный сброс датчика
 * 
 * @return ESP_OK при успехе
 */
esp_err_t sht3x_soft_reset(void);

#ifdef __cplusplus
}
#endif

#endif // SHT3X_DRIVER_H

