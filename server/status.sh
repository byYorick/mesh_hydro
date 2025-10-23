#!/bin/bash

# ========================================
# MESH HYDRO V2 - DOCKER STATUS SCRIPT
# ========================================

set -e

echo "📊 Статус Mesh Hydro System V2..."

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
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    error "Docker Compose не установлен. Установите Docker Compose и попробуйте снова."
    exit 1
fi

# Проверка файлов
log "Проверяем наличие docker-compose.yml..."

if [ ! -f "docker-compose.yml" ]; then
    error "Файл docker-compose.yml не найден!"
    exit 1
fi

# Статус контейнеров
log "Проверяем статус контейнеров..."
echo ""
echo "📋 Статус контейнеров:"
echo "===================="
docker-compose ps

echo ""
echo "📊 Детальная информация:"
echo "========================"

# Проверка каждого сервиса
services=("postgres" "mosquitto" "backend" "mqtt_listener" "scheduler" "reverb" "frontend")

for service in "${services[@]}"; do
    status=$(docker-compose ps -q $service)
    if [ -n "$status" ]; then
        container_status=$(docker inspect --format='{{.State.Status}}' $status 2>/dev/null || echo "not found")
        if [ "$container_status" = "running" ]; then
            success "✅ $service: Запущен"
        else
            warning "⚠️ $service: $container_status"
        fi
    else
        error "❌ $service: Не найден"
    fi
done

echo ""
echo "🔍 Проверка доступности сервисов:"
echo "================================="

# Проверка PostgreSQL
if docker-compose exec -T postgres pg_isready -U hydro -d hydro_system >/dev/null 2>&1; then
    success "✅ PostgreSQL: Доступен"
else
    error "❌ PostgreSQL: Недоступен"
fi

# Проверка Backend
if curl -s http://localhost:8000/health >/dev/null 2>&1; then
    success "✅ Backend API: Доступен"
else
    warning "⚠️ Backend API: Недоступен"
fi

# Проверка Frontend
if curl -s http://localhost:3000 >/dev/null 2>&1; then
    success "✅ Frontend: Доступен"
else
    warning "⚠️ Frontend: Недоступен"
fi

# Проверка MQTT
if docker-compose exec -T mosquitto mosquitto_pub -h localhost -t test -m "test" >/dev/null 2>&1; then
    success "✅ MQTT Broker: Доступен"
else
    warning "⚠️ MQTT Broker: Недоступен"
fi

# Проверка WebSocket
if curl -s http://localhost:8080 >/dev/null 2>&1; then
    success "✅ WebSocket: Доступен"
else
    warning "⚠️ WebSocket: Недоступен"
fi

echo ""
echo "📈 Использование ресурсов:"
echo "========================="
docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}\t{{.BlockIO}}" | head -10

echo ""
echo "🔧 Полезные команды:"
echo "==================="
echo "- Запустить: ./start.sh"
echo "- Остановить: ./stop.sh"
echo "- Перезапустить: docker-compose restart"
echo "- Логи: docker-compose logs -f"
echo "- Логи сервиса: docker-compose logs -f [service_name]"
echo "- Очистить все: ./stop.sh --clean"
echo ""
echo "📱 Доступные URL:"
echo "================="
echo "- Frontend: http://localhost:3000"
echo "- Backend API: http://localhost:8000"
echo "- MQTT Broker: localhost:1883"
echo "- PostgreSQL: localhost:5432"
echo "- WebSocket: ws://localhost:8080"
echo ""
echo "🚀 Система готова к работе!"
