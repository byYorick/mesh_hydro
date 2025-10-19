# 🚀 НАЧНИТЕ ЗДЕСЬ

**Mesh Hydro System V2 - Быстрый старт**

---

## 📋 ЧТО У ВАС ЕСТЬ

### 🐳 Docker сервер (готов):
```
✅ Mosquitto MQTT Broker (порт 1883)
✅ Backend API (порт 8000)
✅ Frontend Web (порт 3000)
✅ SQLite Database (пустая, без тестовых данных)
```

### 🔌 ESP32 узлы (нужно прошить):
```
⏳ ROOT Node (ESP32-S3 #1)
⏳ NODE Climate (ESP32)
⏳ NODE pH/EC (ESP32-S3 #2)
⏳ NODE Relay (ESP32)
⏳ NODE Display (ESP32-S3 #3)
⏳ NODE Water (ESP32-C3)
```

---

## ⚡ БЫСТРЫЙ СТАРТ

### 1️⃣ Запустите сервер:
```cmd
cd server
docker-start.bat
```

**Откроется:** http://localhost:3000

**Ожидаемый результат:**
```
Dashboard пустой
Total Nodes: 0
"Connect ESP32 to start..."
```

### 2️⃣ Прошейте ROOT Node:
```bash
cd root_node
idf.py set-target esp32s3
idf.py build flash monitor
```

**В логах должно быть:**
```
I (12345) mesh_manager: Mesh started
I (12350) mqtt_manager: Connecting to broker...
I (12500) mqtt_manager: MQTT connected ✅
I (12505) mqtt_manager: Published discovery
```

### 3️⃣ Проверьте дашборд:
```
http://localhost:3000
```

**Узел появится автоматически!**
```
✅ ROOT Node (online)
   MAC: AA:BB:CC:DD:EE:FF
   Status: Connected
```

### 4️⃣ Прошейте другие узлы:
```bash
cd node_climate
idf.py build flash

cd node_ph_ec  
idf.py build flash
```

**Они появятся на дашборде автоматически!**

---

## 📚 ДОКУМЕНТАЦИЯ

### Основные файлы:
| Файл | Описание |
|------|----------|
| `README.md` | Полная документация |
| `DOCKER_QUICK_START.md` | Docker инструкция |
| `REAL_DATA_ONLY.md` | О реальных данных |
| `DOCKER_TROUBLESHOOTING.md` | Решение проблем |

### Если проблемы:
```
DOCKER_TROUBLESHOOTING.md
```

---

## 🎯 ЧТО ДАЛЬШЕ

### После того как ESP32 подключились:

**1. Мониторинг:**
- Dashboard → Реальные данные от ESP32
- Telemetry → Графики
- Events → Алерты

**2. Управление:**
- Nodes → Управление узлами
- Commands → Отправка команд
- Settings → Конфигурация

**3. Production:**
- Настройте WiFi на роутер
- Настройте IP адрес MQTT в ROOT
- Установите ESP32 в теплице

---

## ✅ CHECKLIST

- [ ] Docker запущен (`docker-start.bat`)
- [ ] Dashboard открыт (http://localhost:3000)
- [ ] Dashboard пустой (Total Nodes: 0)
- [ ] ROOT Node прошит
- [ ] ROOT подключился к MQTT
- [ ] ROOT появился на дашборде
- [ ] NODE Climate прошит
- [ ] NODE Climate появился
- [ ] Телеметрия идёт

---

## 🔧 КОМАНДЫ

### Docker:
```cmd
docker-start.bat     # Запуск
docker-stop.bat      # Остановка
docker-logs.bat      # Логи
docker-rebuild.bat   # Пересборка
```

### ESP32:
```bash
idf.py build         # Сборка
idf.py flash         # Прошивка
idf.py monitor       # Логи
idf.py flash monitor # Прошивка + логи
```

---

## 🎉 ВСЁ ГОТОВО!

**Система настроена и ждёт подключения ESP32!**

Прошейте узлы и они автоматически появятся на дашборде.

---

**Дата:** 2025-10-18  
**Версия:** 1.0

