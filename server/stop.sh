#!/bin/bash

# ========================================
# MESH HYDRO V2 - DOCKER STOP SCRIPT
# ========================================

set -e

echo "🛑 Останавливаем Mesh Hydro System V2..."

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
log "Проверяем наличие docker-compose.yml..."

if [ ! -f "docker-compose.yml" ]; then
    error "Файл docker-compose.yml не найден!"
fi

# Остановка сервисов
log "Останавливаем все сервисы..."
docker-compose down

# Очистка (опционально)
if [ "$1" = "--clean" ] || [ "$1" = "-c" ]; then
    log "Очищаем volumes и orphaned контейнеры..."
    docker-compose down -v --remove-orphans
    
    log "Очищаем неиспользуемые образы..."
    docker system prune -f
    
    success "Система полностью очищена"
else
    success "Сервисы остановлены (данные сохранены)"
fi

# Проверка статуса
log "Проверяем статус контейнеров..."
docker-compose ps

success "Система остановлена!"

echo ""
echo "🛑 Mesh Hydro System V2 остановлена!"
echo ""
echo "📊 Статус:"
echo "- Все контейнеры остановлены"
echo "- Данные сохранены в volumes"
echo "- Система готова к перезапуску"
echo ""
echo "🔧 Полезные команды:"
echo "- Запустить: ./start.sh"
echo "- Полная очистка: ./stop.sh --clean"
echo "- Статус: docker-compose ps"
echo "- Логи: docker-compose logs -f"
echo ""
echo "🚀 Для запуска выполните: ./start.sh"
