# 🔌 GPIO Summary - NODE pH

## ✅ Текущая конфигурация

| GPIO | Назначение | Режим | Статус |
|------|------------|-------|--------|
| **12** | Pump 0 (pH UP) | PWM Output | ✅ Настроено |
| **13** | Pump 1 (pH DOWN) | PWM Output | ✅ Настроено |
| **21** | I2C SDA | I2C | ✅ Настроено |
| **22** | I2C SCL | I2C | ✅ Настроено |

## 🧪 Тестирование

### Быстрый тест с LED:
```bash
# Подключение:
GPIO 12 → LED (+ 220Ω) → GND
GPIO 13 → LED (+ 220Ω) → GND

# Запуск теста:
test_pumps.bat
```

### Команды тестирования:
```json
// Тест Pump 0 (GPIO 12) - 2 секунды
{"command": "run_pump_manual", "params": {"pump_id": 0, "duration_sec": 2}}

// Тест Pump 1 (GPIO 13) - 2 секунды  
{"command": "run_pump_manual", "params": {"pump_id": 1, "duration_sec": 2}}

// Проверка статуса
{"command": "get_sensor_status", "params": {}}
```

## 📋 Файлы конфигурации

- `components/pump_controller/pump_controller.c:30` - GPIO пины насосов
- `main/app_main.c:36-37` - I2C пины
- `GPIO_CONFIG.md` - Полная документация

## ⚡ Схема подключения насосов

```
ESP32 GPIO 12 → MOSFET/Реле → Pump pH UP (12V)
ESP32 GPIO 13 → MOSFET/Реле → Pump pH DOWN (12V)
```

**ВАЖНО:** Для управления насосами 12V нужен MOSFET (IRLZ44N) или реле!

## 🎯 Готово к использованию!

Все GPIO правильно настроены и готовы к работе.
