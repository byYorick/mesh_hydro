#!/bin/bash

# ========================================
# MESH HYDRO V2 - DOCKER START SCRIPT
# ========================================

set -e

echo "🚀 Запускаем Mesh Hydro System V2..."

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функция для логирования
log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Проверка Docker
if ! command -v docker &> /dev/null; then
    error "Docker не установлен. Установите Docker и попробуйте снова."
fi

if ! command -v docker-compose &> /dev/null; then
    error "Docker Compose не установлен. Установите Docker Compose и попробуйте снова."
fi

# Проверка файлов
log "Проверяем наличие необходимых файлов..."

if [ ! -f "docker-compose.yml" ]; then
    error "Файл docker-compose.yml не найден!"
fi

if [ ! -f ".env" ]; then
    warning ".env файл не найден. Создаем базовый .env файл..."
    cat > .env << EOF
# Mesh Hydro System V2 Environment
APP_NAME="Mesh Hydro System"
APP_ENV=production
APP_KEY=base64:hydro_app_key_2025
APP_DEBUG=false
APP_URL=http://localhost

LOG_CHANNEL=stack
LOG_DEPRECATIONS_CHANNEL=null
LOG_LEVEL=info

DB_CONNECTION=pgsql
DB_HOST=postgres
DB_PORT=5432
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=hydro_secure_pass_2025

BROADCAST_DRIVER=reverb
CACHE_DRIVER=file
FILESYSTEM_DISK=local
QUEUE_CONNECTION=sync
SESSION_DRIVER=file
SESSION_LIFETIME=120

MEMCACHED_HOST=127.0.0.1

REDIS_HOST=127.0.0.1
REDIS_PASSWORD=null
REDIS_PORT=6379

MAIL_MAILER=smtp
MAIL_HOST=mailpit
MAIL_PORT=1025
MAIL_USERNAME=null
MAIL_PASSWORD=null
MAIL_ENCRYPTION=null
MAIL_FROM_ADDRESS="hello@example.com"
MAIL_FROM_NAME="\${APP_NAME}"

AWS_ACCESS_KEY_ID=
AWS_SECRET_ACCESS_KEY=
AWS_DEFAULT_REGION=us-east-1
AWS_BUCKET=
AWS_USE_PATH_STYLE_ENDPOINT=false

PUSHER_APP_ID=hydro-app
PUSHER_APP_KEY=hydro-app-key
PUSHER_APP_SECRET=hydro-app-secret
PUSHER_HOST=reverb
PUSHER_PORT=8080
PUSHER_SCHEME=http
PUSHER_APP_CLUSTER=mt1

VITE_PUSHER_APP_KEY="\${PUSHER_APP_KEY}"
VITE_PUSHER_HOST="\${PUSHER_HOST}"
VITE_PUSHER_PORT="\${PUSHER_PORT}"
VITE_PUSHER_SCHEME="\${PUSHER_SCHEME}"
VITE_PUSHER_APP_CLUSTER="\${PUSHER_APP_CLUSTER}"

# MQTT Settings
MQTT_HOST=mosquitto
MQTT_PORT=1883
MQTT_USERNAME=
MQTT_PASSWORD=
MQTT_CLIENT_ID=hydro_backend
MQTT_TOPIC_PREFIX=hydro

# Telegram Bot
TELEGRAM_BOT_TOKEN=
TELEGRAM_CHAT_ID=

# Node Settings
NODE_OFFLINE_TIMEOUT=300
NODE_HEARTBEAT_INTERVAL=60
EOF
    success ".env файл создан"
fi

# Создание директорий для volumes
log "Создаем необходимые директории..."
mkdir -p mosquitto/{config,data,log}
mkdir -p nginx/ssl

# Создание конфигурации Mosquitto
if [ ! -f "mosquitto/config/mosquitto.conf" ]; then
    log "Создаем конфигурацию Mosquitto..."
    cat > mosquitto/config/mosquitto.conf << EOF
# Mosquitto Configuration for Mesh Hydro System
listener 1883
allow_anonymous true
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
log_type error
log_type warning
log_type notice
log_type information
connection_messages true
log_timestamp true
EOF
    success "Конфигурация Mosquitto создана"
fi

# Остановка существующих контейнеров
log "Останавливаем существующие контейнеры..."
docker-compose down --remove-orphans || true

# Запуск сервисов
log "Запускаем сервисы..."

# Запуск базовых сервисов
log "Запускаем PostgreSQL и Mosquitto..."
docker-compose up -d postgres mosquitto

# Ожидание готовности PostgreSQL
log "Ожидаем готовности PostgreSQL..."
timeout=60
while [ $timeout -gt 0 ]; do
    if docker-compose exec -T postgres pg_isready -U hydro -d hydro_system >/dev/null 2>&1; then
        success "PostgreSQL готов"
        break
    fi
    sleep 2
    timeout=$((timeout - 2))
done

if [ $timeout -le 0 ]; then
    error "PostgreSQL не запустился в течение 60 секунд"
fi

# Запуск backend
log "Запускаем backend..."
docker-compose up -d backend

# Ожидание готовности backend
log "Ожидаем готовности backend..."
timeout=60
while [ $timeout -gt 0 ]; do
    if curl -s http://localhost:8000/health >/dev/null 2>&1; then
        success "Backend готов"
        break
    fi
    sleep 2
    timeout=$((timeout - 2))
done

if [ $timeout -le 0 ]; then
    warning "Backend не отвечает на health check, но продолжаем..."
fi

# Запуск остальных сервисов
log "Запускаем остальные сервисы..."
docker-compose up -d mqtt_listener scheduler reverb frontend

# Ожидание готовности frontend
log "Ожидаем готовности frontend..."
timeout=60
while [ $timeout -gt 0 ]; do
    if curl -s http://localhost:3000 >/dev/null 2>&1; then
        success "Frontend готов"
        break
    fi
    sleep 2
    timeout=$((timeout - 2))
done

if [ $timeout -le 0 ]; then
    warning "Frontend не отвечает, но продолжаем..."
fi

# Проверка статуса
log "Проверяем статус сервисов..."
docker-compose ps

# Финальная проверка
log "Проверяем доступность сервисов..."

# Проверка PostgreSQL
if docker-compose exec -T postgres pg_isready -U hydro -d hydro_system >/dev/null 2>&1; then
    success "✅ PostgreSQL доступен"
else
    error "❌ PostgreSQL недоступен"
fi

# Проверка Backend
if curl -s http://localhost:8000/health >/dev/null 2>&1; then
    success "✅ Backend API доступен"
else
    warning "⚠️ Backend API недоступен"
fi

# Проверка Frontend
if curl -s http://localhost:3000 >/dev/null 2>&1; then
    success "✅ Frontend доступен"
else
    warning "⚠️ Frontend недоступен"
fi

# Проверка MQTT
if docker-compose exec -T mosquitto mosquitto_pub -h localhost -t test -m "test" >/dev/null 2>&1; then
    success "✅ MQTT Broker доступен"
else
    warning "⚠️ MQTT Broker недоступен"
fi

success "Система запущена!"

echo ""
echo "🎉 Mesh Hydro System V2 запущена!"
echo ""
echo "📊 Доступные сервисы:"
echo "- Frontend: http://localhost:3000"
echo "- Backend API: http://localhost:8000"
echo "- MQTT Broker: localhost:1883"
echo "- PostgreSQL: localhost:5432"
echo "- WebSocket: ws://localhost:8080"
echo ""
echo "🔧 Полезные команды:"
echo "- Статус: docker-compose ps"
echo "- Логи: docker-compose logs -f"
echo "- Остановить: docker-compose down"
echo "- Перезапустить: docker-compose restart"
echo ""
echo "📱 Откройте браузер и перейдите на http://localhost:3000"
echo ""
echo "🚀 Система готова к работе!"
