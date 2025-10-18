/**
 * @file ccs811_driver.h
 * @brief Драйвер датчика CCS811 (CO2 + TVOC)
 * 
 * AMS CCS811 I2C датчик качества воздуха
 * I2C адрес: 0x5A (по умолчанию)
 */

#ifndef CCS811_DRIVER_H
#define CCS811_DRIVER_H

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCS811_I2C_ADDR  0x5A

/**
 * @brief Инициализация CCS811 датчика
 * 
 * @param i2c_port I2C порт
 * @return ESP_OK при успехе
 */
esp_err_t ccs811_init(i2c_port_t i2c_port);

/**
 * @brief Чтение CO2 и TVOC
 * 
 * @param co2 Указатель для CO2 (ppm)
 * @param tvoc Указатель для TVOC (ppb) - опционально, может быть NULL
 * @return ESP_OK при успехе
 */
esp_err_t ccs811_read(uint16_t *co2, uint16_t *tvoc);

/**
 * @brief Установка температуры и влажности для компенсации
 * 
 * @param temperature Температура (°C)
 * @param humidity Влажность (%)
 * @return ESP_OK при успехе
 */
esp_err_t ccs811_set_environment(float temperature, float humidity);

#ifdef __cplusplus
}
#endif

#endif // CCS811_DRIVER_H

