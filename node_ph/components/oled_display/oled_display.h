/**
 * @file oled_display.h
 * @brief Управление OLED дисплеем SSD1306 128x64
 * 
 * Отображает:
 * - pH и EC значения
 * - Статус подключения (ONLINE/AUTONOMOUS/EMERGENCY)
 * - Статистика насосов
 * - Алерты и предупреждения
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "esp_err.h"
#include "connection_monitor.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация OLED дисплея
 * 
 * @param i2c_port I2C порт (I2C_NUM_0 или I2C_NUM_1)
 * @param sda_gpio GPIO для SDA
 * @param scl_gpio GPIO для SCL
 * @return ESP_OK при успехе
 */
esp_err_t oled_display_init(int i2c_port, int sda_gpio, int scl_gpio);

/**
 * @brief Обновление основного экрана с данными
 * 
 * @param ph Значение pH
 * @param ec Значение EC
 * @param temp Температура
 * @param state Состояние подключения
 */
void oled_display_update_main(float ph, float ec, float temp, connection_state_t state);

/**
 * @brief Показ статистики насосов
 * 
 * @param ph_up_ml мл за час (pH UP)
 * @param ph_down_ml мл за час (pH DOWN)
 * @param ec_ml мл за час (EC total)
 */
void oled_display_show_pump_stats(uint32_t ph_up_ml, uint32_t ph_down_ml, uint32_t ec_ml);

/**
 * @brief Показ сообщения об ошибке/аварии
 * 
 * @param message Текст сообщения
 * @param value Значение для отображения
 */
void oled_display_show_emergency(const char *message, float value);

/**
 * @brief Показ краткого сообщения (3 секунды)
 * 
 * @param message Текст сообщения
 */
void oled_display_show_message(const char *message);

/**
 * @brief Очистка дисплея
 */
void oled_display_clear(void);

#ifdef __cplusplus
}
#endif

#endif // OLED_DISPLAY_H

