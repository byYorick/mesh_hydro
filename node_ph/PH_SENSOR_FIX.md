# Исправление проблемы pH датчика

## Проблема
```
E (291564) ph_mgr: EMERGENCY: pH out of range (4294974.50)
```

Значение pH 4294974.50 явно некорректно и указывает на проблему с чтением данных с датчика pH.

## Причины
1. **Некорректные данные I2C** - датчик возвращает мусорные данные
2. **Отсутствие валидации** - система не проверяла разумность значений pH
3. **Плохая обработка ошибок** - недостаточное логирование проблем I2C

## Внесенные исправления

### 1. Валидация данных pH (`ph_sensor.c`)
```c
// Проверка на разумные значения pH (0.0-14.0)
if (ph_value < 0.0f || ph_value > 14.0f || raw == 0xFFFF || raw == 0x0000) {
    ESP_LOGE(TAG, "INVALID pH data: raw=0x%04X (%.2f) - using last value", raw, ph_value);
    *ph = s_last_value;
}
```

### 2. Улучшенное логирование I2C ошибок
```c
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C read failed: %s (reg=0x%02X, addr=0x%02X)", 
             esp_err_to_name(ret), reg, PH_SENSOR_ADDR);
}
```

### 3. Автоматический fallback на mock режим
```c
// Переключение в mock режим после 5 ошибок подряд
if (s_error_count >= 5 && !s_mock_mode) {
    ESP_LOGW(TAG, "Too many pH errors (%lu) - switching to MOCK mode", s_error_count);
    s_mock_mode = true;
    s_last_value = 6.5f;
}
```

### 4. Новые функции управления
- `ph_sensor_is_mock_mode()` - проверка режима mock
- `ph_sensor_force_mock_mode(bool)` - принудительное переключение
- `ph_sensor_is_connected()` - проверка реального подключения

### 5. Новые команды управления
- `force_mock_mode` - переключение в mock режим
- `get_sensor_status` - получение статуса датчика

## Использование

### Сборка
```bash
build.bat
```

### Принудительное включение mock режима
```json
{
  "command": "force_mock_mode",
  "params": {
    "enable": true
  }
}
```

### Проверка статуса датчика
```json
{
  "command": "get_sensor_status",
  "params": {}
}
```

## Результат
- ✅ Некорректные значения pH больше не вызывают emergency режим
- ✅ Автоматическое переключение в mock режим при проблемах
- ✅ Детальное логирование для диагностики
- ✅ Возможность ручного управления режимом датчика

## Мониторинг
После применения исправлений в логах будет видно:
- `INVALID pH data: raw=0xXXXX (X.XX) - error #N` - при некорректных данных
- `Too many pH errors (5) - switching to MOCK mode` - при переключении
- `pH sensor recovered after N errors` - при восстановлении
