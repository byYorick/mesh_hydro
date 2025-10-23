/**
 * @file buzzer_led.h
 * @brief Управление индикацией (LED + Buzzer + Button)
 * 
 * LED индикация состояния системы:
 * - Зеленый = ONLINE
 * - Желтый мигающий = DEGRADED/AUTONOMOUS
 * - Красный мигающий = EMERGENCY
 * 
 * Buzzer для аварийных сигналов
 * Button для ручного управления
 */

#ifndef BUZZER_LED_H
#define BUZZER_LED_H

#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED режимы
 */
typedef enum {
    LED_MODE_OFF = 0,           ///< Выключен
    LED_MODE_GREEN,             ///< Зеленый постоянный
    LED_MODE_YELLOW_BLINK,      ///< Желтый мигающий
    LED_MODE_RED_BLINK_SLOW,    ///< Красный мигающий медленно
    LED_MODE_RED_BLINK_FAST     ///< Красный мигающий быстро
} led_mode_t;

/**
 * @brief Callback при нажатии кнопки
 * 
 * @param duration_ms Длительность нажатия в миллисекундах
 */
typedef void (*button_press_callback_t)(uint32_t duration_ms);

/**
 * @brief Инициализация LED, Buzzer и Button
 * 
 * @param led_gpio GPIO для LED
 * @param buzzer_gpio GPIO для Buzzer
 * @param button_gpio GPIO для Button
 * @return ESP_OK при успехе
 */
esp_err_t buzzer_led_init(gpio_num_t led_gpio, gpio_num_t buzzer_gpio, gpio_num_t button_gpio);

/**
 * @brief Установка режима LED
 * 
 * @param mode Режим LED
 */
void buzzer_led_set_mode(led_mode_t mode);

/**
 * @brief Воспроизведение buzzer сигнала
 * 
 * @param count Количество сигналов
 * @param duration_ms Длительность каждого сигнала
 * @param interval_ms Интервал между сигналами
 */
void buzzer_beep(int count, uint32_t duration_ms, uint32_t interval_ms);

/**
 * @brief Аварийный сигнал (3 коротких сигнала)
 */
void buzzer_alarm(void);

/**
 * @brief Остановка buzzer
 */
void buzzer_stop(void);

/**
 * @brief Регистрация callback при нажатии кнопки
 * 
 * @param cb Callback функция
 */
void buzzer_led_register_button_cb(button_press_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_LED_H

