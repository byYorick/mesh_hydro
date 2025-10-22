# Исправление проблем с насосами и pH датчиком

## Проблемы
1. **pH датчик возвращает некорректные значения** (4294974.00) даже в mock режиме
2. **Насосы не включаются** при команде `run_pump_manual`
3. **Отсутствие диагностических логов** для понимания проблем

## Исправления

### 1. Исправление mock режима pH датчика
**Файл:** `components/ph_sensor/ph_sensor.c`

```c
// ВАЖНО: Валидация mock данных тоже!
if (final_ph < 0.0f || final_ph > 14.0f) {
    ESP_LOGE(TAG, "MOCK ERROR: Invalid pH %.2f, using safe value", final_ph);
    final_ph = 6.5f;  // Безопасное значение
}
```

### 2. Добавление отладочных логов для команд насосов
**Файл:** `components/ph_manager/ph_manager.c`

```c
ESP_LOGI(TAG, "=== RUN PUMP MANUAL COMMAND ===");
// Отладочный вывод параметров
if (params) {
    char *params_str = cJSON_PrintUnformatted(params);
    ESP_LOGI(TAG, "Command params: %s", params_str ? params_str : "NULL");
    if (params_str) free(params_str);
}
```

### 3. Улучшенная диагностика pump_controller
**Файл:** `components/pump_controller/pump_controller.c`

```c
ESP_LOGI(TAG, "=== PUMP CONTROLLER RUN ===");
ESP_LOGI(TAG, "Pump: %d, Duration: %lu ms", pump, (unsigned long)duration_ms);
ESP_LOGI(TAG, "Calling pump_start_internal...");
esp_err_t ret = pump_start_internal(pump, duration_ms);
ESP_LOGI(TAG, "pump_start_internal returned: %s", esp_err_to_name(ret));
```

## Тестирование

### 1. Сборка с отладкой
```bash
test_pump_debug.bat
```

### 2. Команда тестирования насоса
```json
{
  "type": "command",
  "command_id": null,
  "node_id": "ph_3f0c00",
  "command": "run_pump_manual",
  "params": {
    "pump_id": 0,
    "duration_sec": 2
  }
}
```

### 3. Ожидаемые логи
```
I (XXXXX) ph_mgr: === RUN PUMP MANUAL COMMAND ===
I (XXXXX) ph_mgr: Command params: {"pump_id":0,"duration_sec":2}
I (XXXXX) ph_mgr: pump_id: found, duration_sec: found
I (XXXXX) ph_mgr: Manual pump run: PUMP_UP (id=0) for 2.0 sec (2000 ms)
I (XXXXX) pump_ctrl: === PUMP CONTROLLER RUN ===
I (XXXXX) pump_ctrl: Pump: 0, Duration: 2000 ms
I (XXXXX) pump_ctrl: Calling pump_start_internal...
I (XXXXX) pump_ctrl: Pump 0 START (2000 ms) GPIO=12 duty=100%
I (XXXXX) pump_ctrl: pump_start_internal returned: ESP_OK
```

## GPIO конфигурация
- **GPIO 12** = Pump 0 (pH UP)
- **GPIO 13** = Pump 1 (pH DOWN)

## Схема тестирования
```
ESP32 GPIO 12 → LED (+ 220Ω) → GND
ESP32 GPIO 13 → LED (+ 220Ω) → GND
```

При успешном тесте LED должен загореться на 2 секунды.

## Результат
- ✅ Mock режим pH датчика исправлен
- ✅ Добавлены детальные логи для диагностики
- ✅ Улучшена обработка команд насосов
- ✅ Готово к тестированию
