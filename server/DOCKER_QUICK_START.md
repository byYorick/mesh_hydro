# 🐳 DOCKER - БЫСТРЫЙ СТАРТ (1 минута!)

**Самый простой способ запустить Hydro System**

---

## ⚡ ОДНА КОМАНДА

### Windows:

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server
docker-start.bat
```

### Linux/Mac:

```bash
cd /path/to/mesh_hydro/server
docker compose up -d
```

**Готово!** Откройте: http://localhost:3000

---

## 🎯 Что будет запущено в Docker

### ⚠️ ВАЖНО: Только СЕРВЕРНЫЕ компоненты!

Docker автоматически создаст и запустит:

1. **Mosquitto MQTT Broker** → порт 1883 (для ESP32)
2. **Laravel Backend API** → порт 8000 (REST API + SQLite БД)
3. **MQTT Listener** → фоновый (прием от ESP32)
4. **Vue.js Frontend** → порт 3000 (веб-интерфейс)

**ESP32 узлы (ROOT, NODE Climate, и т.д.) - это физическое железо!**  
Они прошиваются отдельно и подключаются к MQTT в Docker.

**Все серверное в изолированных контейнерах!** 🐳

---

## 📦 Требования

- **Docker Desktop** (Windows/Mac)
- **Docker + Docker Compose** (Linux)

**Скачать:** https://www.docker.com/products/docker-desktop/

---

## 🔧 Управление

### Запуск:
```bash
docker-start.bat         # Windows
docker compose up -d     # Linux/Mac
```

### Остановка:
```bash
docker-stop.bat          # Windows
docker compose down      # Linux/Mac
```

### Логи:
```bash
docker-logs.bat          # Windows
docker compose logs -f   # Linux/Mac
```

### Перезапуск после изменений:
```bash
docker-rebuild.bat       # Windows
docker compose build && docker compose up -d  # Linux
```

---

## 📊 Проверка статуса

```bash
docker compose ps
```

**Должно быть 5 контейнеров со статусом "Up":**

```
NAME                   STATUS              PORTS
hydro_mosquitto        Up 2 minutes        1883/tcp, 9001/tcp
hydro_postgres         Up 2 minutes        5432/tcp
hydro_backend          Up 2 minutes        8000/tcp
hydro_mqtt_listener    Up 2 minutes        
hydro_frontend         Up 2 minutes        0.0.0.0:3000->80/tcp
```

---

## 🌐 Доступ к сервисам

| Сервис | URL | Описание |
|--------|-----|----------|
| **Frontend** | http://localhost:3000 | Веб-интерфейс |
| **Backend API** | http://localhost:8000/api/status | REST API |
| **MQTT Broker** | mqtt://localhost:1883 | MQTT подключение для ROOT |
| **PostgreSQL** | localhost:5432 | База данных |

---

## 🔍 Тестирование

### 1. Проверка Frontend:
Откройте: http://localhost:3000

Должен появиться Dashboard с узлами.

### 2. Проверка Backend API:
```bash
curl http://localhost:8000/api/status
```

Ответ: `{"status":"ok"}`

### 3. Проверка MQTT:
```bash
# Подписка
docker compose exec mosquitto mosquitto_sub -t "hydro/#" -v

# Публикация (в другом терминале)
docker compose exec mosquitto mosquitto_pub -t "hydro/test" -m "Hello Docker!"
```

---

## 🛠️ Настройка ROOT узла

### В ROOT узле измените MQTT broker адрес:

**Файл:** `root_node/components/mqtt_client/mqtt_client_manager.c`

```c
// ДО (localhost):
#define MQTT_BROKER_URI "mqtt://192.168.1.100:1883"

// ПОСЛЕ (ваш IP):
#define MQTT_BROKER_URI "mqtt://192.168.1.XXX:1883"  
// Замените XXX на IP вашего ПК с Docker
```

**Как узнать IP:**
```cmd
ipconfig
# Найдите IPv4 Address вашей WiFi/Ethernet
```

---

## 💡 Преимущества Docker

### ✅ Простота:
- Одна команда для запуска всего
- Не нужно устанавливать PHP, PostgreSQL, Mosquitto вручную
- Работает одинаково на Windows/Linux/Mac

### ✅ Изоляция:
- Каждый сервис в своем контейнере
- Не конфликтует с другими программами
- Легко удалить (docker compose down -v)

### ✅ Масштабируемость:
- Легко добавить новые сервисы
- Horizontal scaling
- Load balancing

### ✅ Production-ready:
- Те же контейнеры в dev и production
- Воспроизводимое окружение
- CI/CD integration

---

## 📝 Файлы Docker

Созданные файлы:

```
server/
├── docker-compose.yml          ← Основная конфигурация
├── docker-compose.dev.yml      ← Для разработки (hot reload)
├── backend/
│   ├── Dockerfile              ← Laravel образ
│   ├── .dockerignore           ← Исключения
│   └── .env.docker             ← Шаблон конфигурации
├── frontend/
│   ├── Dockerfile              ← Vue.js multi-stage
│   ├── nginx.conf              ← Nginx для SPA
│   └── .dockerignore           ← Исключения
├── mosquitto/
│   └── config/
│       └── mosquitto.conf      ← MQTT конфигурация
├── docker-start.bat            ← Запуск (Windows)
├── docker-stop.bat             ← Остановка (Windows)
├── docker-logs.bat             ← Логи (Windows)
├── docker-rebuild.bat          ← Пересборка (Windows)
└── DOCKER_DEPLOY.md            ← Полная инструкция
```

---

## 🚀 ГОТОВО!

**Docker конфигурация полностью готова!**

**Запуск одной командой:**

```cmd
docker-start.bat
```

**Вся система (Backend + Frontend + MQTT + Database) запустится автоматически!** 🎉

---

## 📚 Документация

- **DOCKER_DEPLOY.md** - полная инструкция (250+ строк)
- **DOCKER_QUICK_START.md** - этот файл
- **docker-compose.yml** - основная конфигурация
- **docker-compose.dev.yml** - для разработки

---

**Все в Docker! Portable, reproducible, production-ready!** 🐳🚀

