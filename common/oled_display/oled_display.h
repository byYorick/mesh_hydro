#ifndef COMMON_OLED_DISPLAY_H
#define COMMON_OLED_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(ESP_PLATFORM) && (defined(__XTENSA__) || defined(__riscv))
#define OLED_PLATFORM_AVAILABLE 1
#endif

#ifdef OLED_PLATFORM_AVAILABLE
#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#else
typedef int i2c_port_t;
typedef int gpio_num_t;
typedef int esp_err_t;
typedef uint32_t TickType_t;
#endif

/**
 * @brief Конфигурация OLED дисплея.
 */
typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t clk_speed_hz;
    uint8_t i2c_address;
    uint8_t width;
    uint8_t height;
    size_t line_count;
} oled_display_config_t;

/**
 * @brief Пара ключ/значение для шаблонов.
 */
typedef struct {
    const char *key;
    const char *value;
} oled_display_kv_t;

/**
 * @brief Конфигурация фоновой задачи.
 */
typedef struct {
    uint32_t stack_size;
    UBaseType_t priority;
    size_t queue_depth;
    TickType_t heartbeat_timeout_ticks;
} oled_display_task_config_t;

esp_err_t oled_display_init(const oled_display_config_t *config);
esp_err_t oled_display_set_template(size_t line_index, const char *template_str);
esp_err_t oled_display_start_task(const oled_display_task_config_t *task_cfg);
esp_err_t oled_display_queue_render(const oled_display_kv_t *items, size_t count, TickType_t timeout_ticks);
esp_err_t oled_display_show_heartbeat(bool active);
void oled_display_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_OLED_DISPLAY_H */

