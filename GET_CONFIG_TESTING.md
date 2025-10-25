# 🔍 Тестирование команды get_config

## ✅ Обновления

### 1. **pH Manager (ph_manager.c)**
- ✅ Добавлены новые safety параметры в `get_config`
- ✅ Emergency пороги: `ph_emergency_low`, `ph_emergency_high`
- ✅ Дневной лимит: `max_daily_volume_ml`
- ✅ Автономия: `mesh_timeout_ms`

### 2. **EC Manager (ec_manager.c)**
- ✅ Добавлены новые safety параметры в `get_config`
- ✅ Emergency пороги: `ec_emergency_high`
- ✅ Дневной лимит: `max_daily_volume_ml`
- ✅ Автономия: `mesh_timeout_ms`

### 3. **pH/EC Manager (ph_ec_manager.c)**
- ✅ Добавлена полная команда `get_config`
- ✅ Все pH и EC параметры
- ✅ Все safety параметры
- ✅ Все emergency пороги

## 🧪 Тестирование

### 1. **Отправка команды через MQTT**

```bash
# Тест pH ноды
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ph_001" -m '{
  "type": "command",
  "node_id": "ph_001",
  "command": "get_config",
  "params": {}
}'

# Тест EC ноды
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ec_001" -m '{
  "type": "command",
  "node_id": "ec_001", 
  "command": "get_config",
  "params": {}
}'

# Тест pH/EC ноды
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ph_ec_001" -m '{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "get_config", 
  "params": {}
}'
```

### 2. **Ожидаемый ответ от pH ноды**

```json
{
  "type": "config_response",
  "node_id": "ph_001",
  "timestamp": 1703123456,
  "config": {
    "node_id": "ph_001",
    "node_type": "ph",
    "zone": "Zone A",
    "ph_target": 6.5,
    "ph_min": 5.5,
    "ph_max": 7.5,
    "ph_cal_offset": 0.0,
    "pumps_calibration": [...],
    "pumps_pid": [...],
    "max_pump_time_ms": 10000,
    "cooldown_ms": 60000,
    "max_daily_volume_ml": 1000,
    "ph_emergency_low": 5.5,
    "ph_emergency_high": 7.5,
    "autonomous_enabled": true,
    "mesh_timeout_ms": 30000
  }
}
```

### 3. **Ожидаемый ответ от EC ноды**

```json
{
  "type": "config_response",
  "node_id": "ec_001",
  "timestamp": 1703123456,
  "config": {
    "node_id": "ec_001",
    "node_type": "ec",
    "zone": "Zone A",
    "ec_target": 1.8,
    "ec_min": 0.5,
    "ec_max": 3.0,
    "ec_cal_offset": 0.0,
    "pumps_calibration": [...],
    "pumps_pid": [...],
    "max_pump_time_ms": 10000,
    "cooldown_ms": 60000,
    "max_daily_volume_ml": 1000,
    "ec_emergency_high": 3.0,
    "autonomous_enabled": true,
    "mesh_timeout_ms": 30000
  }
}
```

### 4. **Ожидаемый ответ от pH/EC ноды**

```json
{
  "type": "config_response",
  "node_id": "ph_ec_001",
  "timestamp": 1703123456,
  "config": {
    "node_id": "ph_ec_001",
    "node_type": "ph_ec",
    "zone": "Zone A",
    "ph_target": 6.5,
    "ph_min": 5.5,
    "ph_max": 7.5,
    "ph_cal_offset": 0.0,
    "ec_target": 1.8,
    "ec_min": 0.5,
    "ec_max": 3.0,
    "ec_cal_offset": 0.0,
    "max_pump_time_ms": 10000,
    "cooldown_ms": 60000,
    "max_daily_volume_ml": 1000,
    "ph_emergency_low": 5.5,
    "ph_emergency_high": 7.5,
    "ec_emergency_high": 3.0,
    "autonomous_enabled": true,
    "mesh_timeout_ms": 30000
  }
}
```

## 🔍 Проверка логов

### 1. **Успешная отправка**
```
I ph_mgr: Config sent to server
I ec_mgr: Config sent to server  
I ph_ec_mgr: Config sent to server
```

### 2. **Ошибка mesh**
```
W ph_mgr: Cannot send config: mesh offline
```

### 3. **Ошибка JSON**
```
E ph_mgr: Failed to create root JSON object
E ph_mgr: Config JSON too large
```

## 📋 Тестовые сценарии

### 1. **Нормальная работа**
1. Отправьте команду `get_config`
2. Проверьте получение ответа на сервере
3. Убедитесь, что все параметры присутствуют

### 2. **Mesh offline**
1. Отключите mesh
2. Отправьте команду `get_config`
3. Проверьте лог "Cannot send config: mesh offline"

### 3. **Большой JSON**
1. Увеличьте размер конфигурации
2. Проверьте обработку ошибки "Config JSON too large"

## 🎯 Критерии успеха

- ✅ Команда `get_config` работает для всех типов нод
- ✅ Все safety параметры включены в ответ
- ✅ Все emergency пороги включены в ответ
- ✅ JSON корректно формируется и отправляется
- ✅ Ошибки обрабатываются gracefully
- ✅ Логи показывают статус операции

## 🚀 Готово!

Команда `get_config` теперь передает все настройки NVS на сервер, включая новые параметры безопасности!
