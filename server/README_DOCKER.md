# 🐳 **MESH HYDRO V2 - DOCKER SETUP**

## 📋 **Обзор системы**

Mesh Hydro System V2 - это комплексная система мониторинга и управления гидропонными установками с использованием ESP32 узлов и централизованного сервера.

### **🏗️ Архитектура Docker**

```
┌─────────────────────────────────────────────────────────────┐
│                    MESH HYDRO V2 DOCKER                    │
├─────────────────────────────────────────────────────────────┤
│  Frontend (Vue.js)     │  Backend (Laravel)  │  Database    │
│  Port: 3000           │  Port: 8000         │  Port: 5432  │
├─────────────────────────────────────────────────────────────┤
│  MQTT Broker          │  WebSocket          │  Scheduler   │
│  Port: 1883           │  Port: 8080         │  Background │
├─────────────────────────────────────────────────────────────┤
│  Nginx (Reverse Proxy) │  SSL/TLS            │  Monitoring │
│  Port: 80/443         │  Security           │  Logs       │
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 **Быстрый старт**

### **1. Подготовка системы**

```bash
# Клонирование репозитория
git clone <repository-url>
cd mesh_hydro/server

# Установка прав на выполнение (Linux/Mac)
chmod +x *.sh
```

### **2. Сборка и запуск**

#### **Linux/Mac:**
```bash
# Сборка образов
./build.sh

# Запуск системы
./start.sh

# Проверка статуса
./status.sh

# Остановка
./stop.sh
```

#### **Windows:**
```cmd
# Сборка образов
build.bat

# Запуск системы
start.bat

# Проверка статуса
status.bat

# Остановка
stop.bat
```

---

## 📊 **Сервисы системы**

### **🗄️ PostgreSQL Database**
- **Порт**: 5432
- **База данных**: hydro_system
- **Пользователь**: hydro
- **Пароль**: hydro_secure_pass_2025

### **📡 MQTT Broker (Mosquitto)**
- **Порт**: 1883
- **WebSocket**: 9001
- **Конфигурация**: `mosquitto/config/mosquitto.conf`

### **🔧 Backend (Laravel 10)**
- **Порт**: 8000
- **API**: http://localhost:8000/api
- **Health Check**: http://localhost:8000/health

### **🌐 Frontend (Vue.js 3)**
- **Порт**: 3000
- **URL**: http://localhost:3000
- **Nginx**: Встроенный

### **🔄 WebSocket (Laravel Reverb)**
- **Порт**: 8080
- **URL**: ws://localhost:8080

### **⏰ Scheduler**
- **Функция**: Автоматические задачи
- **Команды**: `php artisan schedule:work`

### **📨 MQTT Listener**
- **Функция**: Обработка MQTT сообщений
- **Команда**: `php artisan mqtt:listen`

---

## 🔧 **Управление системой**

### **Основные команды**

```bash
# Запуск всех сервисов
docker-compose up -d

# Остановка всех сервисов
docker-compose down

# Перезапуск сервиса
docker-compose restart [service_name]

# Просмотр логов
docker-compose logs -f [service_name]

# Проверка статуса
docker-compose ps

# Очистка системы
docker-compose down -v --remove-orphans
```

### **Полезные команды**

```bash
# Подключение к базе данных
docker-compose exec postgres psql -U hydro -d hydro_system

# Подключение к backend контейнеру
docker-compose exec backend bash

# Просмотр логов MQTT
docker-compose logs -f mosquitto

# Проверка здоровья системы
curl http://localhost:8000/health
```

---

## 📁 **Структура файлов**

```
server/
├── docker-compose.yml          # Основная конфигурация
├── .env                        # Переменные окружения
├── build.sh / build.bat        # Скрипты сборки
├── start.sh / start.bat        # Скрипты запуска
├── stop.sh / stop.bat          # Скрипты остановки
├── status.sh / status.bat      # Скрипты проверки статуса
├── backend/
│   ├── Dockerfile              # Backend образ
│   └── ...                     # Laravel приложение
├── frontend/
│   ├── Dockerfile              # Frontend образ
│   ├── nginx.conf              # Nginx конфигурация
│   └── ...                     # Vue.js приложение
├── nginx/
│   ├── conf.d/
│   │   └── production.conf      # Production конфигурация
│   └── ssl/                    # SSL сертификаты
└── mosquitto/
    ├── config/
    │   └── mosquitto.conf      # MQTT конфигурация
    ├── data/                   # MQTT данные
    └── log/                    # MQTT логи
```

---

## 🔒 **Безопасность**

### **Переменные окружения**

```bash
# Основные настройки
APP_ENV=production
APP_DEBUG=false
APP_KEY=base64:your_app_key

# База данных
DB_PASSWORD=hydro_secure_pass_2025

# MQTT
MQTT_USERNAME=your_mqtt_user
MQTT_PASSWORD=your_mqtt_password

# Telegram Bot
TELEGRAM_BOT_TOKEN=your_bot_token
TELEGRAM_CHAT_ID=your_chat_id
```

### **SSL/TLS (Production)**

```bash
# Генерация SSL сертификатов
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout nginx/ssl/key.pem \
  -out nginx/ssl/cert.pem

# Включение HTTPS в nginx/conf.d/production.conf
```

---

## 📈 **Мониторинг и логи**

### **Просмотр логов**

```bash
# Все сервисы
docker-compose logs -f

# Конкретный сервис
docker-compose logs -f backend
docker-compose logs -f frontend
docker-compose logs -f postgres
docker-compose logs -f mosquitto

# Последние 100 строк
docker-compose logs --tail=100 backend
```

### **Мониторинг ресурсов**

```bash
# Использование ресурсов
docker stats

# Информация о контейнерах
docker-compose ps

# Проверка здоровья
curl http://localhost:8000/health
```

---

## 🚨 **Устранение неполадок**

### **Частые проблемы**

1. **Порт уже используется**
   ```bash
   # Проверка портов
   netstat -tulpn | grep :80
   netstat -tulpn | grep :8000
   
   # Остановка конфликтующих сервисов
   sudo systemctl stop apache2
   sudo systemctl stop nginx
   ```

2. **Ошибки базы данных**
   ```bash
   # Перезапуск PostgreSQL
   docker-compose restart postgres
   
   # Проверка подключения
   docker-compose exec postgres pg_isready -U hydro
   ```

3. **Проблемы с MQTT**
   ```bash
   # Проверка MQTT
   docker-compose exec mosquitto mosquitto_pub -h localhost -t test -m "test"
   ```

4. **Очистка системы**
   ```bash
   # Полная очистка
   docker-compose down -v --remove-orphans
   docker system prune -f
   ```

### **Логи ошибок**

```bash
# Backend ошибки
docker-compose logs backend | grep ERROR

# MQTT ошибки
docker-compose logs mosquitto | grep ERROR

# Frontend ошибки
docker-compose logs frontend | grep ERROR
```

---

## 🔄 **Обновление системы**

### **Обновление кода**

```bash
# Остановка системы
./stop.sh

# Обновление кода
git pull origin main

# Пересборка образов
./build.sh

# Запуск системы
./start.sh
```

### **Обновление базы данных**

```bash
# Миграции
docker-compose exec backend php artisan migrate

# Очистка кэша
docker-compose exec backend php artisan cache:clear
docker-compose exec backend php artisan config:clear
```

---

## 📞 **Поддержка**

### **Полезные ссылки**

- **Frontend**: http://localhost:3000
- **Backend API**: http://localhost:8000/api
- **Health Check**: http://localhost:8000/health
- **MQTT Broker**: localhost:1883

### **Команды диагностики**

```bash
# Полная диагностика
./status.sh

# Проверка здоровья
curl http://localhost:8000/health

# Тест MQTT
docker-compose exec mosquitto mosquitto_pub -h localhost -t test -m "test"

# Проверка базы данных
docker-compose exec postgres pg_isready -U hydro -d hydro_system
```

---

## 🎯 **Production Deployment**

### **Подготовка к production**

1. **Настройка SSL сертификатов**
2. **Изменение паролей по умолчанию**
3. **Настройка firewall**
4. **Настройка мониторинга**
5. **Настройка backup**

### **Команды production**

```bash
# Запуск с production профилем
docker-compose --profile production up -d

# Настройка SSL
# (см. nginx/conf.d/production.conf)

# Настройка backup
# (см. backup/scripts/)
```

---

## 🚀 **Готово к работе!**

Система Mesh Hydro V2 готова к использованию. Все сервисы настроены и оптимизированы для production использования.

**Следующие шаги:**
1. Настройте ESP32 узлы
2. Подключите к MQTT брокеру
3. Настройте мониторинг
4. Настройте уведомления

**Удачи с вашей гидропонной системой! 🌱**
