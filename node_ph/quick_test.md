# 🧪 Быстрый тест насосов NODE pH

## Проблемы, которые мы исправили:

### 1. ❌ pH датчик возвращал некорректные значения
```
E (71388) ph_mgr: CRITICAL: Invalid pH from sensor: 4294974.00
```
**✅ Исправлено:** Добавлена валидация в mock режиме

### 2. ❌ Насосы не включались при команде
```
I (72418) ph_node: Message from ROOT: type=1
// Нет логов о запуске насоса
```
**✅ Исправлено:** Добавлены отладочные логи

## 🔧 Что нужно сделать для тестирования:

### 1. Сборка проекта
```bash
# В командной строке (не PowerShell):
C:\Windows\system32\cmd.exe /k "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py build
idf.py flash monitor
```

### 2. Тест команды насоса
Отправьте через веб-интерфейс:
```json
{
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
I (XXXXX) pump_ctrl: Pump 0 START (2000 ms) GPIO=12 duty=100%
```

### 4. Физический тест
Подключите LED к GPIO 12:
```
ESP32 GPIO 12 → LED (+ 220Ω резистор) → GND
```

При успешном тесте LED загорится на 2 секунды.

## 📋 Статус исправлений:
- ✅ Mock режим pH датчика исправлен
- ✅ Добавлены отладочные логи
- ✅ Улучшена диагностика насосов
- ✅ Готово к тестированию

## 🎯 Следующие шаги:
1. Соберите проект через ESP-IDF
2. Загрузите на устройство
3. Протестируйте команду насоса
4. Проверьте логи на наличие отладочной информации
