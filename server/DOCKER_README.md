# 🐳 Hydro System - Docker Edition

**Вся система в контейнерах - запуск одной командой!**

---

## ⚡ БЫСТРЫЙ СТАРТ

### 1. Установите Docker Desktop

**Windows/Mac:**  
https://www.docker.com/products/docker-desktop/

**Linux:**
```bash
sudo apt-get install docker.io docker-compose-plugin
```

### 2. Запустите систему

```cmd
cd server
docker-start.bat      # Windows
```

```bash
docker compose up -d  # Linux/Mac
```

### 3. Откройте браузер

**http://localhost:3000**

🎉 **Готово! Вся система работает!**

---

## 📦 Что включено в Docker

### ⚠️ ВАЖНО: В Docker ТОЛЬКО серверные компоненты!

**ESP32 узлы (ROOT, NODE Climate, NODE pH/EC) - это физическое железо!**  
Они прошиваются отдельно и подключаются к MQTT broker в Docker.

### 4 серверных контейнера:

1. **mosquitto** - MQTT Broker (Eclipse Mosquitto 2.0)
   - Порт: 1883 (MQTT) - для подключения ESP32
   - Порт: 9001 (WebSocket)

2. **backend** - Laravel API (PHP 8.2) + SQLite
   - Порт: 8000
   - REST API
   - MQTT обработка

3. **mqtt_listener** - MQTT демон
   - Фоновый процесс
   - Прием данных от ESP32 через MQTT

4. **frontend** - Vue.js Dashboard
   - Порт: 3000
   - Веб-интерфейс
   - Визуализация данных

---

## 🎮 Управление

### Запуск:
```bash
docker-start.bat
```

### Остановка:
```bash
docker-stop.bat
```

### Логи:
```bash
docker-logs.bat
```

### Пересборка:
```bash
docker-rebuild.bat
```

### Статус:
```bash
docker compose ps
```

---

## 🔧 Конфигурация

### MQTT Broker

**Файл:** `mosquitto/config/mosquitto.conf`

```conf
listener 1883
allow_anonymous true  # Dev режим

# Для production:
# allow_anonymous false
# password_file /mosquitto/config/passwd
```

### Backend (.env)

**Файл:** `backend/.env`

```env
DB_HOST=postgres          # Имя контейнера!
DB_PORT=5432
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=hydro_secure_pass_2025

MQTT_HOST=mosquitto       # Имя контейнера!
MQTT_PORT=1883
```

### Frontend API URL

**Файл:** `docker-compose.yml`

```yaml
frontend:
  build:
    args:
      - VITE_API_URL=http://localhost:8000
```

---

## 🌐 Подключение ROOT узла

### В ROOT узле укажите IP вашего ПК:

**Файл:** `root_node/components/mqtt_client/mqtt_client_manager.c`

```c
// Замените на IP вашего ПК (где запущен Docker)
#define MQTT_BROKER_URI "mqtt://192.168.1.100:1883"
```

**Как узнать IP:**
```cmd
ipconfig           # Windows
ifconfig           # Linux
ip addr show       # Linux альтернатива
```

---

## 🧪 Тестирование

### 1. Проверка контейнеров:
```bash
docker compose ps
```

Все 5 контейнеров должны быть "Up".

### 2. Проверка MQTT:
```bash
docker compose exec mosquitto mosquitto_sub -t "hydro/#" -v
```

### 3. Проверка Backend:
```bash
curl http://localhost:8000/api/status
```

### 4. Проверка Frontend:
Откройте: http://localhost:3000

---

## 💾 Данные

### Volumes (постоянное хранилище):

```
postgres_data        - База данных PostgreSQL
backend_storage      - Laravel storage (logs, cache)
mosquitto/data       - MQTT persistence
```

**Данные сохраняются** при остановке контейнеров!

### Backup БД:
```bash
docker compose exec postgres pg_dump -U hydro hydro_system > backup.sql
```

### Восстановление БД:
```bash
docker compose exec -T postgres psql -U hydro hydro_system < backup.sql
```

---

## 🔄 Обновление кода

### После изменений в коде:

**Backend:**
```bash
docker compose restart backend
```

**Frontend:**
```bash
docker compose build frontend
docker compose up -d frontend
```

**Оба:**
```bash
docker-rebuild.bat
```

---

## 🐛 Troubleshooting

### Проблема: Контейнер не запускается

```bash
# Просмотр логов
docker compose logs <имя_контейнера>

# Пример
docker compose logs backend
docker compose logs postgres
```

### Проблема: Backend не видит базу данных

```bash
# Проверка PostgreSQL
docker compose exec postgres psql -U hydro -d hydro_system -c "\dt"

# Ручной запуск миграций
docker compose exec backend php artisan migrate --force
```

### Проблема: MQTT не работает

```bash
# Проверка Mosquitto
docker compose exec mosquitto mosquitto -c /mosquitto/config/mosquitto.conf -v

# Проверка портов
netstat -an | findstr 1883  # Windows
ss -tulpn | grep 1883       # Linux
```

### Проблема: Frontend показывает ошибку

Проверьте `VITE_API_URL` в `docker-compose.yml` и пересоберите:
```bash
docker compose build frontend --no-cache
docker compose up -d frontend
```

---

## 🚀 Production

### Для production используйте:

```bash
# С nginx reverse proxy
docker compose --profile production up -d

# Или отдельно настройте nginx на хосте
```

### Рекомендации:

1. **Измените пароли** в `docker-compose.yml`
2. **Включите MQTT auth** в `mosquitto.conf`
3. **Настройте SSL** через Nginx
4. **Настройте backup** (cron job)
5. **Мониторинг** (Prometheus + Grafana)

---

## 📊 Размеры

| Компонент | Размер образа |
|-----------|---------------|
| mosquitto | ~20 MB |
| postgres | ~200 MB |
| backend | ~400 MB |
| frontend | ~50 MB |
| **ИТОГО** | **~670 MB** |

**Первая сборка:** 5-10 минут  
**Следующие запуски:** 30 секунд

---

## 🎯 Итого

**Docker делает развертывание ТРИВИАЛЬНЫМ!**

- ✅ Одна команда для запуска
- ✅ Все зависимости в контейнерах
- ✅ Изолированное окружение
- ✅ Легко масштабировать
- ✅ Production-ready

**Рекомендуется для всех!** 🐳

---

## 📚 Дополнительно

- **DOCKER_DEPLOY.md** - детальная инструкция
- **DOCKER_QUICK_START.md** - этот файл
- **docker-compose.yml** - основная конфигурация
- **docker-compose.dev.yml** - для разработки

---

**Создано:** 2025-10-18  
**Версия:** 1.0  
**Docker Compose:** v3.8

