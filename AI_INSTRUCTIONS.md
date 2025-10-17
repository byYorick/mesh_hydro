# 🤖 AI INSTRUCTIONS для Mesh Hydro System V2

## Критичные правила

### Правило #1: NODE pH/EC - СВЯТОЕ!
- ✅ ВСЕГДА автономная работа при потере связи
- ✅ ВСЕГДА сохранение конфигурации в NVS
- ✅ ВСЕГДА локальный ring buffer (1000 записей)
- ✅ ВСЕГДА watchdog reset
- ❌ НИКОГДА не блокировать PID контроллер при потере связи с ROOT

### Правило #2: JSON размер < 1 KB
- Mesh пакет: максимум 1456 байт
- Используй краткие ключи в JSON
- Батчи данные при синхронизации (10 записей за раз)

### Правило #3: Используй существующий код
- Портируй компоненты из `hydro1.0`, не пиши с нуля:
  - `sensor_manager` (pH, EC)
  - `pump_manager` (5 насосов)
  - `adaptive_pid` (AI PID)
  - `lvgl_ui` (LVGL экраны)
  - `lcd_ili9341` (TFT дисплей)
  - `encoder` (ротационный энкодер)

### Правило #4: NVS для ВСЕХ узлов
- Каждый узел хранит свою конфигурацию в NVS
- Обновление через команду `config` от ROOT
- Fallback на NVS при потере связи с ROOT
- Структуры конфигурации: см. `common/node_config/node_config.h`

### Правило #5: PWM для света
- Использовать `ledc_timer` ESP-IDF
- Частота: 5000 Hz
- Разрешение: 8 бит (0-255)
- Плавные переходы (fade)

## Структура проекта

### Общие компоненты (common/)
Используются ВСЕМИ узлами через симлинки или копирование:
- `mesh_manager/` - ESP-WIFI-MESH управление
- `mesh_protocol/` - JSON протокол
- `node_config/` - NVS конфигурация
- `ota_manager/` - OTA обновления
- `sensor_base/` - Базовые функции датчиков
- `actuator_base/` - Базовые функции исполнителей

### ROOT NODE (root_node/)
Координатор mesh + MQTT:
- `node_registry/` - реестр всех узлов
- `mqtt_client/` - MQTT клиент
- `data_router/` - маршрутизация данных
- `climate_logic/` - резервная логика климата
- `ota_coordinator/` - координация OTA

### NODE узлы
Каждый NODE в отдельной папке:
- `node_display/` - TFT дисплей
- `node_ph_ec/` - pH/EC контроль (критичный!)
- `node_climate/` - датчики климата
- `node_relay/` - форточки + вентиляция + свет
- `node_water/` - насосы + клапаны

### Сервер (server/)
- `backend/` - Laravel 10 API
- `frontend/` - Vue.js 3 + Vuetify 3
- `mqtt_bridge/` - MQTT демоны

## Протокол mesh обмена

Формат: JSON (cJSON библиотека)

### Типы сообщений:

**1. Телеметрия (NODE → ROOT):**
```json
{
  "type": "telemetry",
  "node_id": "ph_ec_001",
  "timestamp": 1697548800,
  "data": {"ph": 6.5, "ec": 1.8}
}
```

**2. Команда (ROOT → NODE):**
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "run_pump",
  "params": {"pump": "ph_up", "duration_ms": 5000}
}
```

**3. Конфигурация (ROOT → NODE):**
```json
{
  "type": "config",
  "node_id": "ph_ec_001",
  "config": {"ph_target": 6.8, "ec_target": 2.0}
}
```

**4. Событие (NODE → ROOT):**
```json
{
  "type": "event",
  "node_id": "ph_ec_001",
  "level": "critical",
  "message": "pH критично низкий: 4.8"
}
```

**5. Heartbeat (каждые 10 сек):**
```json
{
  "type": "heartbeat",
  "node_id": "ph_ec_001",
  "uptime": 36000
}
```

## Команды для разработки

### Сборка узла
```bash
cd node_ph_ec
idf.py build
idf.py -p COM3 flash monitor
```

### Очистка
```bash
idf.py fullclean
```

### Menuconfig
```bash
idf.py menuconfig
```

### Мониторинг
```bash
idf.py monitor
```

## Приоритеты разработки

См. фазы реализации в `doc/MESH_HYDRO_V2_FINAL_PLAN.md`:
1. Базовая mesh-сеть (3-4 дня)
2. ROOT узел (3-4 дня)
3. Сервер базовый (3-4 дня)
4. **NODE pH/EC (4-5 дней) ⚠️ КРИТИЧНО!**
5. Vue Dashboard (3-4 дня)
6. NODE Display (3-4 дня)
7. Climate + Relay (3-4 дня)
8. Water (2-3 дня)
9. Telegram + SMS (2-3 дня)
10. OTA система (3-4 дня)
11. Аутентификация (1-2 дня)
12. Backup + Export/Import (1-2 дня)
13. Документация (2-3 дня)
14. Тестирование (3-5 дней)

**ИТОГО:** 36-51 день

## Полезные ссылки

- ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/latest/
- ESP-WIFI-MESH: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/mesh.html
- LVGL: https://docs.lvgl.io/
- Laravel: https://laravel.com/docs/10.x
- Vue.js 3: https://vuejs.org/guide/

---

**Следуй этим правилам при разработке!**

