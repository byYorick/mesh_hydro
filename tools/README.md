# Tools

Утилиты для работы с Mesh Hydro System

## Файлы

- `flash_all.bat` / `flash_all.sh` - Прошивка всех узлов
- `create_symlinks.bat` / `create_symlinks.sh` - Создание симлинков на common
- `monitor_mesh.py` - Мониторинг mesh-сети
- `mqtt_tester.py` - Тестирование MQTT
- `node_configurator.py` - Конфигурация узлов
- `backup_restore.sh` - Backup/Restore БД

## create_symlinks - Создание символических ссылок

Создает симлинки на папку `common` в каждом `node_*` и `root_node`.

**⚠️ Не обязательно!** Проект уже настроен через `EXTRA_COMPONENT_DIRS`.

**Когда использовать:**
- Если хочешь видеть `common` в файловом менеджере каждого узла
- Если твоя IDE плохо находит файлы из `EXTRA_COMPONENT_DIRS`

**Windows (требуется администратор):**
```batch
create_symlinks.bat
```

**Linux/Mac:**
```bash
chmod +x create_symlinks.sh
./create_symlinks.sh
```

После этого в каждом `node_*/` появится папка `common/` (символическая ссылка).

---

## flash_all - Прошивка всех узлов

Автоматическая прошивка всех узлов.

**Windows:**
```batch
flash_all.bat
```

**Linux/Mac:**
```bash
chmod +x flash_all.sh
./flash_all.sh
```

**⚠️ Перед использованием:**
1. Подключи все ESP32 к компьютеру
2. Проверь COM порты в скрипте
3. Убедись что все проекты собраны (`idf.py build` в каждом)

---

## monitor_mesh.py - Мониторинг mesh

Мониторинг состояния mesh-сети в реальном времени через serial.

```bash
python monitor_mesh.py --port COM3
```

**Опции:**
- `--port, -p` - Serial порт (обязательно)
- `--baud, -b` - Baudrate (по умолчанию 115200)

**Пример:**
```bash
# Windows
python monitor_mesh.py -p COM3

# Linux
python monitor_mesh.py -p /dev/ttyUSB0

# Mac
python monitor_mesh.py -p /dev/cu.usbserial-0001
```

**Что показывает:**
- Mesh события (зеленым)
- Ошибки (красным)
- Предупреждения (желтым)
- Все логи с timestamp

---

## mqtt_tester.py - Тестирование MQTT

Тестирование MQTT подключения и отправка тестовых сообщений.

```bash
python mqtt_tester.py --broker 192.168.1.100 --topic hydro/test
```

**Опции:**
- `--broker, -b` - MQTT broker адрес (по умолчанию localhost)
- `--port, -p` - MQTT порт (по умолчанию 1883)
- `--topic, -t` - Topic для теста (по умолчанию hydro/test)

**Пример:**
```bash
# Подключение к локальному брокеру
python mqtt_tester.py

# Подключение к удаленному брокеру
python mqtt_tester.py -b 192.168.1.100 -p 1883

# Свой topic
python mqtt_tester.py -t hydro/sensors/ph_ec_001
```

**Что делает:**
1. Подключается к MQTT брокеру
2. Подписывается на `hydro/#`
3. Отправляет тестовое сообщение
4. Показывает все входящие сообщения

---

## node_configurator.py - Конфигурация узлов

GUI для настройки параметров узлов через serial.

```bash
python node_configurator.py
```

*TODO: Реализовать в Фазе 11*

---

## backup_restore.sh - Backup/Restore

Резервное копирование и восстановление PostgreSQL БД.

**Создать backup:**
```bash
./backup_restore.sh backup
```

Создаст файл `backups/hydro_backup_YYYYMMDD_HHMMSS.sql.gz`

**Восстановить из backup:**
```bash
./backup_restore.sh restore backups/hydro_backup_20251017_143000.sql.gz
```

⚠️ **Внимание:** Это удалит текущую БД!

**Список backups:**
```bash
./backup_restore.sh list
```

---

## 📝 Требования

### Python скрипты:

```bash
pip install pyserial paho-mqtt
```

Или:
```bash
pip install -r requirements.txt
```

*(TODO: Создать requirements.txt)*

### Bash скрипты:

**Linux/Mac:**
- bash
- PostgreSQL клиент (`psql`, `pg_dump`)

**Windows:**
- PowerShell (встроен)
- Git Bash или WSL (для .sh скриптов)

---

## 🔧 Настройка

### Порты в flash_all

**Windows (flash_all.bat):**
```batch
REM Измени COM порты на свои
idf.py -p COM3 flash  # ROOT
idf.py -p COM4 flash  # Display
idf.py -p COM5 flash  # pH/EC
...
```

**Linux/Mac (flash_all.sh):**
```bash
# Измени порты на свои
idf.py -p /dev/ttyUSB0 flash  # ROOT
idf.py -p /dev/ttyUSB1 flash  # Display
...
```

### БД в backup_restore.sh

```bash
DB_NAME="hydro_system"
DB_USER="postgres"
BACKUP_DIR="./backups"
```

---

## ⚠️ Частые проблемы

### Symlinks не работают на Windows

**Решение:**
1. Запусти `create_symlinks.bat` от администратора
2. Или включи Developer Mode:
   - Settings → Update & Security → For developers → Developer Mode ON

### Python скрипты не работают

**Решение:**
```bash
# Установи зависимости
pip install pyserial paho-mqtt

# Проверь Python
python --version  # Должен быть 3.7+
```

### Права на порт (Linux)

**Решение:**
```bash
# Добавь пользователя в группу dialout
sudo usermod -a -G dialout $USER

# Перелогинься
```

---

## 💡 Советы

1. **Используй monitor_mesh.py** для отладки mesh-сети
2. **Проверяй MQTT** через mqtt_tester.py перед запуском системы
3. **Делай backup БД** перед major изменениями
4. **Symlinks нужны только для удобства**, проект работает без них
