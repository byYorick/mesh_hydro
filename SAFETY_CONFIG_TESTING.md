# 🧪 Тестирование параметров безопасности

## ✅ Что проверено

### 1. **Структуры конфигурации**
- ✅ Добавлены новые поля в `node_config.h`
- ✅ Инициализация по умолчанию в `node_config.c`
- ✅ JSON парсинг в `node_config_update_from_json`

### 2. **Обработка команд**
- ✅ `update_safety_config` в `ph_manager.c`
- ✅ `update_safety_config` в `ec_manager.c`
- ✅ `update_safety_config` в `ph_ec_manager.c`
- ✅ Сохранение в NVS после изменения

### 3. **Emergency пороги**
- ✅ Использование настраиваемых порогов в `check_emergency_conditions`
- ✅ Логирование с новыми значениями

## 🧪 Тестирование

### 1. **Тест через MQTT**

```bash
# Отправка команды на pH ноду
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ph_001" -m '{
  "type": "command",
  "node_id": "ph_001",
  "command": "update_safety_config",
  "params": {
    "max_pump_time_ms": 15000,
    "cooldown_ms": 90000,
    "max_daily_volume_ml": 1500,
    "ph_emergency_low": 5.0,
    "ph_emergency_high": 8.0
  }
}'
```

### 2. **Тест через веб-интерфейс**

1. Откройте Dashboard
2. Выберите узел pH/EC
3. Перейдите в "Настройки безопасности"
4. Измените параметры:
   - Emergency пороги: pH 5.0-8.0
   - Максимальное время насоса: 15 сек
   - Cooldown: 90 сек
   - Дневной лимит: 1.5 л
5. Нажмите "Сохранить"

### 3. **Проверка логов**

После отправки команды должны появиться логи:
```
I ph_mgr: Safety settings updated: max_pump=15000 ms, cooldown=90000 ms, daily_vol=1500 ml, ph_emergency=5.00-8.00
```

### 4. **Проверка Emergency**

Симулируйте критичные значения:
```c
// В коде ноды (временно)
s_current_ph = 4.5f; // Ниже ph_emergency_low (5.0)
```

Должен сработать emergency stop:
```
E ph_mgr: EMERGENCY: pH out of range (4.50) - limits: 5.00-8.00
```

## 📋 Тестовые команды

### Для pH ноды:
```json
{
  "type": "command",
  "node_id": "ph_001",
  "command": "update_safety_config",
  "params": {
    "max_pump_time_ms": 15000,
    "cooldown_ms": 90000,
    "max_daily_volume_ml": 1500,
    "ph_emergency_low": 5.0,
    "ph_emergency_high": 8.0
  }
}
```

### Для EC ноды:
```json
{
  "type": "command",
  "node_id": "ec_001",
  "command": "update_safety_config",
  "params": {
    "max_pump_time_ms": 12000,
    "cooldown_ms": 120000,
    "max_daily_volume_ml": 2000,
    "ec_emergency_high": 3.5
  }
}
```

### Для pH/EC ноды:
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "update_safety_config",
  "params": {
    "max_pump_time_ms": 20000,
    "cooldown_ms": 60000,
    "max_daily_volume_ml": 3000,
    "ph_emergency_low": 4.5,
    "ph_emergency_high": 8.5,
    "ec_emergency_high": 4.0
  }
}
```

## 🔍 Проверка NVS

### 1. **Проверка сохранения**
После изменения параметров проверьте:
- Логи: "Configuration saved to NVS"
- Перезагрузка ноды - параметры должны сохраниться

### 2. **Проверка загрузки**
При старте ноды должны загружаться новые параметры:
```
I ph_mgr: pH emergency: 6.50 (limits: 5.00-8.00)
```

## ⚠️ Важные моменты

1. **Валидация параметров** - проверьте разумность значений
2. **Emergency пороги** должны быть строже рабочих диапазонов
3. **Сохранение в NVS** происходит автоматически
4. **Изменения применяются немедленно** без перезагрузки

## 🎯 Ожидаемый результат

- ✅ Команды парсятся корректно
- ✅ Параметры сохраняются в NVS
- ✅ Emergency пороги работают с новыми значениями
- ✅ Логи показывают обновленные параметры
- ✅ Изменения сохраняются после перезагрузки

## 🚀 Готово к использованию!

Все параметры безопасности теперь настраиваются удаленно и сохраняются в NVS!
