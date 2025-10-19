# 🛠️ TOOLS - Утилиты для работы с проектом

**Все скрипты для сборки, прошивки и управления проектом**

---

## 📋 СОДЕРЖАНИЕ

- [Windows Scripts (.bat)](#windows-scripts-bat)
- [Python Scripts](#python-scripts)
- [Shell Scripts (Linux/Mac)](#shell-scripts-linuxmac)

---

## 🪟 WINDOWS SCRIPTS (.bat)

### 🔥 Прошивка узлов:

| Скрипт | Узел | COM порт | Описание |
|--------|------|----------|----------|
| **`flash_root.bat`** | ROOT NODE | COM7 | Прошивка ROOT + monitor |
| **`flash_climate.bat`** | NODE Climate | COM10 | Прошивка Climate + monitor |
| **`flash_ph_ec.bat`** | NODE pH/EC | COM9 | Прошивка pH/EC + monitor |

**Использование:**
```batch
tools\flash_root.bat
```

**Что делает:**
- Инициализирует ESP-IDF окружение
- Прошивает узел на указанный COM порт
- Запускает monitor для просмотра логов

**Выход из monitor:** `Ctrl+]`

---

### 🔨 Пересборка:

| Скрипт | Описание |
|--------|----------|
| **`rebuild_all.bat`** | Полная пересборка ROOT + Climate + pH/EC |

**Использование:**
```batch
tools\rebuild_all.bat
```

**Что делает:**
- `idf.py fullclean` для каждого узла
- `idf.py build` для каждого узла
- Время: ~15-20 минут

**Когда использовать:**
- После изменения `common/mesh_config/mesh_config.h`
- После изменения общих компонентов (`mesh_manager`, `mesh_protocol`)
- После обновления ESP-IDF

---

### 🐳 Docker Backend:

| Скрипт | Описание |
|--------|----------|
| **`server_start.bat`** | Запуск Backend + MQTT + Mosquitto + Frontend |
| **`server_stop.bat`** | Остановка всех контейнеров |
| **`server_logs.bat`** | Live логи MQTT Listener |

**Использование:**
```batch
tools\server_start.bat
```

**Что запускается:**
- `hydro_mosquitto` - MQTT Broker (192.168.1.100:1883)
- `hydro_backend` - Laravel API (localhost:8000)
- `hydro_mqtt_listener` - обработчик сообщений от узлов
- `hydro_frontend` - Vue.js Dashboard (localhost:3000)

---

## 🐍 PYTHON SCRIPTS

### monitor_mesh.py

**Назначение:** Мониторинг mesh-сети через MQTT

**Использование:**
```bash
python tools/monitor_mesh.py
```

**Что показывает:**
- Все узлы в реальном времени
- Heartbeat от каждого узла
- Telemetry данные
- Events и ошибки

**Требования:**
```bash
pip install paho-mqtt
```

---

### mqtt_tester.py

**Назначение:** Тестирование отправки команд через MQTT

**Использование:**
```bash
python tools/mqtt_tester.py
```

**Примеры команд:**
```python
# Изменить интервал чтения Climate
send_command("climate_001", "set_read_interval", {"interval_ms": 30000})

# Открыть форточку
send_command("relay_001", "set_window", {"window": 1, "action": "open"})

# Запустить насос
send_command("water_001", "run_pump", {"duration_sec": 60})
```

---

## 🐧 SHELL SCRIPTS (Linux/Mac)

### backup_restore.sh

**Назначение:** Резервное копирование конфигураций и БД

**Использование:**
```bash
# Создание backup:
./tools/backup_restore.sh backup

# Восстановление:
./tools/backup_restore.sh restore backup_2025-10-19.tar.gz
```

**Что бэкапится:**
- SQLite database
- .env файлы
- NVS конфигурации узлов
- Docker volumes

---

### create_symlinks.sh / create_symlinks.bat

**Назначение:** Создание симлинков для общих компонентов

**НЕ ИСПОЛЬЗУЕТСЯ** в текущей версии (компоненты в `common/`)

---

### flash_all.sh / flash_all.bat

**Назначение:** Прошивка всех узлов автоматически

**НЕ РЕАЛИЗОВАНО** (используй `flash_root.bat`, затем `flash_climate.bat`)

---

## 📊 СТАТУС УТИЛИТ

| Утилита | Статус | Приоритет |
|---------|--------|-----------|
| `flash_root.bat` | ✅ Готов | HIGH |
| `flash_climate.bat` | ✅ Готов | HIGH |
| `flash_ph_ec.bat` | ✅ Готов | MEDIUM |
| `rebuild_all.bat` | ✅ Готов | HIGH |
| `server_start.bat` | ✅ Готов | HIGH |
| `server_stop.bat` | ✅ Готов | HIGH |
| `server_logs.bat` | ✅ Готов | HIGH |
| `monitor_mesh.py` | ✅ Готов | MEDIUM |
| `mqtt_tester.py` | ✅ Готов | MEDIUM |
| `backup_restore.sh` | 🟡 Базовый | LOW |
| `flash_all.bat` | ❌ Не реализован | LOW |

---

## 🔧 СОЗДАНИЕ НОВОГО СКРИПТА

### Пример: Скрипт для node_relay

**tools/flash_relay.bat:**
```batch
@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПРОШИВКА NODE RELAY (COM11)
echo ════════════════════════════════════════════════════════
echo.

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005"

cd /d %~dp0\..\node_relay

C:\Windows\system32\cmd.exe /k "%IDF_INIT% && idf.py -p COM11 flash monitor"
```

---

## 🐛 ОТЛАДОЧНЫЕ УТИЛИТЫ

### Проверка MQTT сообщений на хосте:

```bash
# Подписка на все топики:
mosquitto_sub -h 192.168.1.100 -t "hydro/#" -v

# Только heartbeat:
mosquitto_sub -h 192.168.1.100 -t "hydro/heartbeat/#" -v

# Только telemetry:
mosquitto_sub -h 192.168.1.100 -t "hydro/telemetry/#" -v
```

### Отправка тестовой команды:

```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/climate_001 -m '{
  "type": "command",
  "node_id": "climate_001",
  "command": "set_read_interval",
  "params": {"interval_ms": 15000}
}'
```

---

## 📋 CHECKLIST ПЕРЕД ПРОШИВКОЙ

Перед прошивкой узлов проверь:

- [ ] Backend запущен (`tools\server_start.bat`)
- [ ] MQTT Broker доступен (192.168.1.100:1883)
- [ ] Wi-Fi роутер включён и доступен
- [ ] В `mesh_config.h` правильные SSID/пароль
- [ ] Канал роутера совпадает с `MESH_NETWORK_CHANNEL`
- [ ] COM порты правильные (COM7=ROOT, COM10=Climate)
- [ ] ESP32 подключены к USB
- [ ] Закрыты другие программы на COM портах

---

## 🆘 TROUBLESHOOTING

### "idf.py not found":
```batch
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
```

### "COM port busy":
- Закрой все окна `idf.py monitor`
- Закрой Arduino IDE, PlatformIO
- Перезагрузи ESP32

### "Docker not running":
1. Запусти Docker Desktop
2. Подожди 2 минуты
3. Повтори `tools\server_start.bat`

---

**ВСЕ УТИЛИТЫ В ОДНОМ МЕСТЕ!** 🛠️
