#!/bin/bash

# ========================================
# MESH HYDRO V2 - DOCKER BUILD SCRIPT
# ========================================

set -e

echo "🚀 Начинаем сборку Mesh Hydro System V2..."

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

required_files=(
    "docker-compose.yml"
    "backend/Dockerfile"
    "frontend/Dockerfile"
    "nginx/conf.d/production.conf"
)

for file in "${required_files[@]}"; do
    if [ ! -f "$file" ]; then
        error "Файл $file не найден!"
    fi
done

success "Все необходимые файлы найдены"

# Очистка старых образов
log "Очищаем старые образы..."
docker system prune -f || true

# Сборка образов
log "Собираем Docker образы..."

# Backend
log "Собираем backend образ..."
docker-compose build backend --no-cache

# Frontend
log "Собираем frontend образ..."
docker-compose build frontend --no-cache

# MQTT Listener
log "Собираем mqtt_listener образ..."
docker-compose build mqtt_listener --no-cache

# Scheduler
log "Собираем scheduler образ..."
docker-compose build scheduler --no-cache

# Reverb
log "Собираем reverb образ..."
docker-compose build reverb --no-cache

success "Все образы успешно собраны!"

# Проверка образов
log "Проверяем созданные образы..."
docker images | grep hydro

# Создание .env файла если не существует
if [ ! -f ".env" ]; then
    log "Создаем .env файл..."
    cat > .env << EOF
# Mesh Hydro System V2 Environment
APP_NAME="Mesh Hydro System"
APP_ENV=production
APP_KEY=base64:$(openssl rand -base64 32)
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
else
    log ".env файл уже существует"
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

# Финальная проверка
log "Проверяем готовность к запуску..."

# Проверка портов
ports=(80 443 8000 8080 1883 5432)
for port in "${ports[@]}"; do
    if lsof -Pi :$port -sTCP:LISTEN -t >/dev/null 2>&1; then
        warning "Порт $port уже используется. Убедитесь, что это не конфликтует с другими сервисами."
    fi
done

success "Система готова к запуску!"

echo ""
echo "🎉 Сборка завершена успешно!"
echo ""
echo "📋 Следующие шаги:"
echo "1. Запустите систему: docker-compose up -d"
echo "2. Проверьте статус: docker-compose ps"
echo "3. Просмотрите логи: docker-compose logs -f"
echo "4. Откройте браузер: http://localhost"
echo ""
echo "🔧 Полезные команды:"
echo "- Остановить: docker-compose down"
echo "- Перезапустить: docker-compose restart"
echo "- Очистить все: docker-compose down -v --remove-orphans"
echo ""
echo "📊 Мониторинг:"
echo "- Логи всех сервисов: docker-compose logs -f"
echo "- Логи конкретного сервиса: docker-compose logs -f backend"
echo "- Статус контейнеров: docker-compose ps"
echo ""
echo "🚀 Готово к запуску!"
