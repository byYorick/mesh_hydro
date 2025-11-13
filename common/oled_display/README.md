# oled_display — SSD1306 128x64

Общий компонент для OLED дисплеев 0.96" на контроллере SSD1306. Реализует базовый драйвер I2C, шаблонный вывод строк и отображение индикатора heartbeat.

## Возможности
- Инициализация дисплея по I2C (адрес по умолчанию `0x3C`, 128x64)
- Шаблоны строк с плейсхолдерами `{key}`
- Очередь обновлений и фоновая задача
- Указание таймаута для индикатора heartbeat (`<3`)

## Быстрый старт
```c
#include "oled_display.h"

void init_display(void) {
    oled_display_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_pin = GPIO_NUM_18,
        .scl_pin = GPIO_NUM_17,
        .clk_speed_hz = 400000,
        .i2c_address = 0x3C,
        .width = 128,
        .height = 64,
        .line_count = 4,
    };
    oled_display_init(&cfg);

    oled_display_task_config_t task_cfg = {
        .stack_size = 4096,
        .priority = 4,
        .queue_depth = 6,
        .heartbeat_timeout_ticks = pdMS_TO_TICKS(1500),
    };
    oled_display_start_task(&task_cfg);

    oled_display_set_template(0, "Zone: {zone}");
    oled_display_set_template(1, "Temp: {temp}");
    oled_display_set_template(2, "Humidity: {hum}");
    oled_display_set_template(3, "Status: {state}");
}

void update_display(void) {
    oled_display_kv_t values[] = {
        {.key = "zone", .value = "Greenhouse"},
        {.key = "temp", .value = "24.5C"},
        {.key = "hum", .value = "52%"},
        {.key = "state", .value = "ONLINE"},
    };
    oled_display_queue_render(values, 4, 0);
}
```

### Heartbeat
При отправке heartbeat на сервер:
```c
oled_display_show_heartbeat(true);
```
Индикатор `<3` автоматически скрывается после таймаута из конфигурации задачи.

## Ограничения
- До 6 строк (по одному текстовому ряду на страницу SSD1306)
- Максимальная длина шаблона — 63 символа
- До 12 пар ключ/значение за одно обновление
- Значения строк — ASCII (шрифт 5x7)

## Очистка
```c
oled_display_shutdown();
```
Удаляет задачу и очередь, деинициализирует I2C (если установлен драйвер компонентом).

