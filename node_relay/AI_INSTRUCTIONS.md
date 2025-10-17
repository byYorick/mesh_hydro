# 🤖 AI INSTRUCTIONS - NODE Relay

## 🎯 Назначение узла

**NODE Relay (ESP32)** - Управление климатом (форточки, вентиляция, свет)

### Основные функции:
- 🪟 Линейные актуаторы форточек (2 шт, 12V)
- 💨 Вентилятор (реле)
- 💡 LED свет с **PWM диммированием** (0-100%)
- 📥 Получение команд от ROOT
- ❌ **БЕЗ датчиков** - только исполнительные устройства!

### Критичные правила:
- ✅ Команды приходят от **ROOT** (не самостоятельные решения)
- ✅ PWM для света (плавная регулировка яркости)
- ✅ Safety: таймауты на актуаторы (макс 30 сек)
- ✅ Концевики встроены в актуаторы

---

## 🔌 Распиновка

| GPIO | Назначение | Управление |
|------|------------|------------|
| 1 | Актуатор форточки 1 | HIGH=открыто, LOW=закрыто |
| 2 | Актуатор форточки 2 | HIGH=открыто, LOW=закрыто |
| 3 | Вентилятор | HIGH=вкл, LOW=выкл |
| 7 | LED свет (PWM) | 0-255 (0-100%) |

---

## 💻 Примеры кода

### Линейные актуаторы

```c
void actuator_open_window(int window_num, uint32_t timeout_ms) {
    gpio_set_level(actuator_gpio[window_num], 1);  // HIGH = открыть
    
    // Таймер безопасности
    xTimerStart(safety_timer, timeout_ms);
    
    // Актуатор имеет встроенный концевик
    // Остановится автоматически при полном открытии
}

void actuator_close_window(int window_num, uint32_t timeout_ms) {
    gpio_set_level(actuator_gpio[window_num], 0);  // LOW = закрыть
    
    xTimerStart(safety_timer, timeout_ms);
}
```

### PWM диммирование света

```c
#define LED_PWM_GPIO 7
#define LED_PWM_FREQ 5000
#define LED_PWM_RESOLUTION LEDC_TIMER_8_BIT

void light_controller_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LED_PWM_RESOLUTION,
        .freq_hz = LED_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);
    
    ledc_channel_config_t channel = {
        .gpio_num = LED_PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
}

// Установка яркости 0-100%
void light_set_brightness(uint8_t percent) {
    uint32_t duty = (percent * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
    ESP_LOGI(TAG, "Light brightness set to %d%%", percent);
}
```

### Обработка команд от ROOT

```c
void on_command_received(mesh_message_t *msg) {
    const char *command = cJSON_GetObjectItem(msg->data, "command")->valuestring;
    
    if (strcmp(command, "open_windows") == 0) {
        actuator_open_window(0, 30000);
        actuator_open_window(1, 30000);
        
    } else if (strcmp(command, "start_fan") == 0) {
        gpio_set_level(GPIO_FAN, 1);
        
    } else if (strcmp(command, "set_light_brightness") == 0) {
        int brightness = cJSON_GetObjectItem(msg->data, "brightness")->valueint;
        light_set_brightness(brightness);
    }
}
```

---

## 📚 Документация

См. `node_relay/README.md` и `doc/MESH_HYDRO_V2_FINAL_PLAN.md`

**Фаза 7 реализации (3-4 дня)**

---

## 🔧 Важно

- Логика управления на **ROOT** (climate_logic)
- ROOT анализирует данные от Climate node
- ROOT отправляет команды на Relay node
- Relay node просто исполняет команды

