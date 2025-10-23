/**
 * @file buzzer_led.c
 * @brief Реализация индикации
 */

#include "buzzer_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *TAG = "buzzer_led";

static gpio_num_t s_led_gpio = GPIO_NUM_NC;
static gpio_num_t s_buzzer_gpio = GPIO_NUM_NC;
static gpio_num_t s_button_gpio = GPIO_NUM_NC;

static led_mode_t s_current_mode = LED_MODE_OFF;
static TaskHandle_t s_led_task = NULL;
static TaskHandle_t s_button_task = NULL;
static button_press_callback_t s_button_cb = NULL;

// Forward declarations
static void led_task(void *arg);
static void button_task(void *arg);

esp_err_t buzzer_led_init(gpio_num_t led_gpio, gpio_num_t buzzer_gpio, gpio_num_t button_gpio) {
    s_led_gpio = led_gpio;
    s_buzzer_gpio = buzzer_gpio;
    s_button_gpio = button_gpio;

    // Инициализация GPIO для LED
    if (led_gpio != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << led_gpio),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(led_gpio, 0);
    }

    // Инициализация GPIO для Buzzer
    if (buzzer_gpio != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << buzzer_gpio),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(buzzer_gpio, 0);
    }

    // Инициализация GPIO для Button
    if (button_gpio != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << button_gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        
        // Запуск задачи обработки кнопки
        xTaskCreate(button_task, "button_task", 2048, NULL, 5, &s_button_task);
    }

    // Запуск задачи LED
    xTaskCreate(led_task, "led_task", 2048, NULL, 5, &s_led_task);

    ESP_LOGI(TAG, "Buzzer/LED/Button initialized (LED=%d, Buzzer=%d, Button=%d)",
             led_gpio, buzzer_gpio, button_gpio);
    
    return ESP_OK;
}

void buzzer_led_set_mode(led_mode_t mode) {
    s_current_mode = mode;
    ESP_LOGI(TAG, "LED mode set to: %d", mode);
}

void buzzer_beep(int count, uint32_t duration_ms, uint32_t interval_ms) {
    if (s_buzzer_gpio == GPIO_NUM_NC) {
        return;
    }

    for (int i = 0; i < count; i++) {
        gpio_set_level(s_buzzer_gpio, 1);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(s_buzzer_gpio, 0);
        
        if (i < count - 1) {
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
        }
    }
}

void buzzer_alarm(void) {
    buzzer_beep(3, 100, 100);  // 3 коротких сигнала
}

void buzzer_stop(void) {
    if (s_buzzer_gpio != GPIO_NUM_NC) {
        gpio_set_level(s_buzzer_gpio, 0);
    }
}

void buzzer_led_register_button_cb(button_press_callback_t cb) {
    s_button_cb = cb;
}

// Задача управления LED
static void led_task(void *arg) {
    if (s_led_gpio == GPIO_NUM_NC) {
        vTaskDelete(NULL);
        return;
    }

    bool led_state = false;
    
    while (1) {
        switch (s_current_mode) {
            case LED_MODE_OFF:
                gpio_set_level(s_led_gpio, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;

            case LED_MODE_GREEN:
                gpio_set_level(s_led_gpio, 1);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;

            case LED_MODE_YELLOW_BLINK:
                led_state = !led_state;
                gpio_set_level(s_led_gpio, led_state);
                vTaskDelay(pdMS_TO_TICKS(500));  // Мигание каждые 500 мс
                break;

            case LED_MODE_RED_BLINK_SLOW:
                led_state = !led_state;
                gpio_set_level(s_led_gpio, led_state);
                vTaskDelay(pdMS_TO_TICKS(1000));  // Мигание каждую секунду
                break;

            case LED_MODE_RED_BLINK_FAST:
                led_state = !led_state;
                gpio_set_level(s_led_gpio, led_state);
                vTaskDelay(pdMS_TO_TICKS(200));  // Быстрое мигание
                break;
        }
    }
}

// Задача обработки кнопки
static void button_task(void *arg) {
    if (s_button_gpio == GPIO_NUM_NC) {
        vTaskDelete(NULL);
        return;
    }

    uint32_t press_start = 0;
    bool pressed = false;

    while (1) {
        bool current = (gpio_get_level(s_button_gpio) == 0);  // LOW = нажата (pull-up)

        if (current && !pressed) {
            // Начало нажатия
            press_start = xTaskGetTickCount() * portTICK_PERIOD_MS;
            pressed = true;
            ESP_LOGI(TAG, "Button pressed");

        } else if (!current && pressed) {
            // Отпускание кнопки
            uint32_t duration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - press_start;
            pressed = false;
            
            ESP_LOGI(TAG, "Button released (duration: %d ms)", duration);
            
            // Вызов callback
            if (s_button_cb) {
                s_button_cb(duration);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // Опрос каждые 50 мс
    }
}

