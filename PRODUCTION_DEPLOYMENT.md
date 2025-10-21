# 🚀 PRODUCTION DEPLOYMENT - Mesh Hydro V2

**Дата:** 20 октября 2025  
**Версия:** Production v1.0  
**Laravel:** 11.46.1

---

## 📋 ПРЕДВАРИТЕЛЬНЫЕ ТРЕБОВАНИЯ

### Сервер:

- **OS:** Linux (Ubuntu 20.04+, Debian 11+, CentOS 8+)
- **RAM:** 4GB минимум, 8GB рекомендуется
- **Disk:** 20GB минимум, SSD рекомендуется
- **CPU:** 2 ядра минимум, 4 ядра рекомендуется

### Установленное ПО:

```bash
# Docker & Docker Compose
docker --version  # ≥ 20.10
docker-compose --version  # ≥ 2.0

# Git
git --version

# Опционально: certbot для SSL
```

---

## 🔧 ПОДГОТОВКА СЕРВЕРА

### 1. Обновление системы

```bash
# Ubuntu/Debian
sudo apt update && sudo apt upgrade -y

# CentOS
sudo yum update -y
```

---

### 2. Установка Docker (если нет)

```bash
# Ubuntu/Debian
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER

# Перелогиниться для применения группы
newgrp docker
```

---

### 3. Клонирование репозитория

```bash
cd /opt
sudo git clone https://github.com/your-repo/mesh_hydro.git
cd mesh_hydro/server
sudo chown -R $USER:$USER .
```

---

## 🔑 НАСТРОЙКА ПЕРЕМЕННЫХ ОКРУЖЕНИЯ

### 1. Создай .env файл

```bash
cd server
cp .env.production.example .env
nano .env  # или vim .env
```

---

### 2. Заполни ОБЯЗАТЕЛЬНЫЕ переменные

```env
# Database Password (ВАЖНО!)
DB_PASSWORD=ваш_очень_сложный_пароль_2025

# Reverb Keys (сгенерируй случайные строки)
REVERB_APP_KEY=$(openssl rand -hex 16)
REVERB_APP_SECRET=$(openssl rand -hex 16)

# Domain (если есть)
DOMAIN=hydro.your-domain.com
```

---

### 3. Генерация паролей

```bash
# Генерация случайного пароля для БД
openssl rand -base64 32

# Генерация Reverb ключей
echo "REVERB_APP_KEY=$(openssl rand -hex 16)"
echo "REVERB_APP_SECRET=$(openssl rand -hex 16)"
```

**Скопируй результаты в .env файл!**

---

## 🐳 ЗАПУСК PRODUCTION

### 1. Первый запуск

```bash
cd /opt/mesh_hydro/server

# Сборка образов
docker-compose -f docker-compose.prod.yml build

# Запуск системы
docker-compose -f docker-compose.prod.yml up -d

# Проверка статуса
docker-compose -f docker-compose.prod.yml ps
```

---

### 2. Генерация APP_KEY

```bash
# Генерация ключа Laravel
docker exec hydro_backend_prod php artisan key:generate --show

# Скопируй вывод и добавь в .env:
# APP_KEY=base64:xxxxxxxxxxxxxxxxx
```

**После добавления APP_KEY перезапусти:**

```bash
docker-compose -f docker-compose.prod.yml restart backend
```

---

### 3. Инициализация базы данных

```bash
# Миграции
docker exec hydro_backend_prod php artisan migrate --force

# Опционально: сиды (тестовые данные)
# docker exec hydro_backend_prod php artisan db:seed --force
```

---

### 4. Оптимизация Laravel

```bash
# Кэширование конфигурации
docker exec hydro_backend_prod php artisan config:cache

# Кэширование роутов
docker exec hydro_backend_prod php artisan route:cache

# Кэширование views
docker exec hydro_backend_prod php artisan view:cache

# Оптимизация autoload
docker exec hydro_backend_prod composer dump-autoload --optimize
```

---

## 🔐 НАСТРОЙКА SSL (HTTPS)

### Вариант 1: Let's Encrypt (бесплатно) ⭐

```bash
# Установка certbot
sudo apt install certbot python3-certbot-nginx -y

# Получение сертификата
sudo certbot certonly --standalone -d hydro.your-domain.com

# Сертификаты будут в:
# /etc/letsencrypt/live/hydro.your-domain.com/fullchain.pem
# /etc/letsencrypt/live/hydro.your-domain.com/privkey.pem

# Копирование в проект
sudo cp /etc/letsencrypt/live/hydro.your-domain.com/fullchain.pem server/nginx/ssl/cert.pem
sudo cp /etc/letsencrypt/live/hydro.your-domain.com/privkey.pem server/nginx/ssl/key.pem
sudo chown $USER:$USER server/nginx/ssl/*
```

---

### Вариант 2: Самоподписанный сертификат (для тестов)

```bash
# Создание директории
mkdir -p server/nginx/ssl

# Генерация сертификата
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout server/nginx/ssl/key.pem \
  -out server/nginx/ssl/cert.pem \
  -subj "/CN=hydro.local"
```

---

### 3. Раскомментируй HTTPS в nginx

```bash
nano server/nginx/conf.d/production.conf

# Найди и раскомментируй:
# listen 443 ssl http2;
# ssl_certificate /etc/nginx/ssl/cert.pem;
# ssl_certificate_key /etc/nginx/ssl/key.pem;
```

---

### 4. Перезапусти nginx

```bash
docker-compose -f docker-compose.prod.yml restart nginx
```

---

## 📊 МОНИТОРИНГ

### Проверка статуса контейнеров

```bash
docker-compose -f docker-compose.prod.yml ps

# Должно быть все "Up":
# hydro_backend_prod     Up
# hydro_frontend_prod    Up
# hydro_postgres_prod    Up (healthy)
# hydro_mosquitto_prod   Up
# hydro_mqtt_listener_prod  Up
# hydro_scheduler_prod   Up
# hydro_reverb_prod      Up
# hydro_nginx_prod       Up
```

---

### Просмотр логов

```bash
# Все логи
docker-compose -f docker-compose.prod.yml logs -f

# Отдельные сервисы
docker logs hydro_backend_prod -f --tail 50
docker logs hydro_scheduler_prod -f --tail 20
docker logs hydro_mqtt_listener_prod -f --tail 50
docker logs hydro_nginx_prod -f --tail 30
```

---

### Health Checks

```bash
# Backend API
curl http://localhost/up

# Dashboard
curl http://localhost/api/dashboard/summary

# Nodes
curl http://localhost/api/nodes
```

---

## 🔄 ОБНОВЛЕНИЕ СИСТЕМЫ

### 1. Остановка системы

```bash
cd /opt/mesh_hydro/server
docker-compose -f docker-compose.prod.yml down
```

---

### 2. Получение обновлений

```bash
git pull origin main  # или ваша ветка
```

---

### 3. Пересборка и запуск

```bash
docker-compose -f docker-compose.prod.yml build
docker-compose -f docker-compose.prod.yml up -d
```

---

### 4. Миграции (если есть)

```bash
docker exec hydro_backend_prod php artisan migrate --force
```

---

### 5. Очистка кэша

```bash
docker exec hydro_backend_prod php artisan cache:clear
docker exec hydro_backend_prod php artisan config:cache
docker exec hydro_backend_prod php artisan route:cache
docker exec hydro_backend_prod php artisan view:cache
```

---

## 💾 БЭКАП И ВОССТАНОВЛЕНИЕ

### Бэкап базы данных

```bash
# Создание бэкапа
docker exec hydro_postgres_prod pg_dump -U hydro hydro_system > backup_$(date +%Y%m%d_%H%M%S).sql

# Или с gzip сжатием
docker exec hydro_postgres_prod pg_dump -U hydro hydro_system | gzip > backup_$(date +%Y%m%d_%H%M%S).sql.gz
```

---

### Восстановление из бэкапа

```bash
# Из обычного SQL
docker exec -i hydro_postgres_prod psql -U hydro hydro_system < backup_20251020_230000.sql

# Из gzip
gunzip < backup_20251020_230000.sql.gz | docker exec -i hydro_postgres_prod psql -U hydro hydro_system
```

---

### Автоматический бэкап (cron)

```bash
# Добавить в crontab
crontab -e

# Бэкап каждый день в 3:00 AM
0 3 * * * cd /opt/mesh_hydro/server && docker exec hydro_postgres_prod pg_dump -U hydro hydro_system | gzip > /opt/backups/hydro_$(date +\%Y\%m\%d).sql.gz
```

---

## 🛡️ БЕЗОПАСНОСТЬ

### 1. Firewall настройки

```bash
# UFW (Ubuntu/Debian)
sudo ufw allow 80/tcp    # HTTP
sudo ufw allow 443/tcp   # HTTPS
sudo ufw allow 1883/tcp  # MQTT (для ESP32)
sudo ufw enable
```

---

### 2. Docker security

```bash
# Ограничить доступ к Docker socket
sudo chmod 660 /var/run/docker.sock

# Использовать non-root user для контейнеров (уже настроено)
```

---

### 3. Database security

```bash
# Убедись что PostgreSQL доступен только изнутри Docker
# Порт проброшен только на localhost: 127.0.0.1:5432
```

---

### 4. MQTT security (опционально)

Если нужна аутентификация MQTT:

```bash
# Создать пароль файл для Mosquitto
docker exec hydro_mosquitto_prod mosquitto_passwd -c /mosquitto/config/passwd mqtt_user

# Обновить mosquitto.conf:
# allow_anonymous false
# password_file /mosquitto/config/passwd
```

---

## 📈 ОПТИМИЗАЦИЯ ПРОИЗВОДИТЕЛЬНОСТИ

### 1. PostgreSQL

Уже настроено в docker-compose.prod.yml:
```yaml
POSTGRES_SHARED_BUFFERS: "256MB"
POSTGRES_WORK_MEM: "16MB"
POSTGRES_EFFECTIVE_CACHE_SIZE: "1GB"
```

---

### 2. Laravel Caching

```bash
# После каждого обновления кода:
docker exec hydro_backend_prod php artisan optimize

# Включает:
# - config:cache
# - route:cache  
# - view:cache
```

---

### 3. Nginx Caching

Уже настроено в `nginx/conf.d/production.conf`:
- ✅ Gzip compression
- ✅ Static files caching (1 year)
- ✅ API response caching (10 seconds)
- ✅ Rate limiting

---

## 🔍 TROUBLESHOOTING

### Backend не запускается

```bash
# Проверить логи
docker logs hydro_backend_prod --tail 100

# Проверить переменные окружения
docker exec hydro_backend_prod env | grep APP

# Проверить подключение к БД
docker exec hydro_backend_prod php artisan db:show
```

---

### Scheduler не работает

```bash
# Проверить что контейнер запущен
docker ps | grep scheduler

# Проверить логи
docker logs hydro_scheduler_prod -f

# Проверить расписание
docker exec hydro_backend_prod php artisan schedule:list
```

---

### MQTT не принимает данные

```bash
# Проверить Mosquitto
docker logs hydro_mosquitto_prod --tail 50

# Тест подключения
mosquitto_pub -h localhost -p 1883 -t "test/topic" -m "test message"

# Проверить MQTT listener
docker logs hydro_mqtt_listener_prod -f
```

---

### Frontend не загружается

```bash
# Проверить nginx
docker logs hydro_nginx_prod --tail 50

# Проверить frontend
docker exec hydro_frontend_prod ls -la /usr/share/nginx/html

# Проверить доступность
curl -I http://localhost
```

---

## 📊 МОНИТОРИНГ В PRODUCTION

### Prometheus + Grafana (опционально)

Добавь в docker-compose.prod.yml:

```yaml
prometheus:
  image: prom/prometheus:latest
  container_name: hydro_prometheus
  volumes:
    - ./monitoring/prometheus.yml:/etc/prometheus/prometheus.yml
    - prometheus_data:/prometheus
  ports:
    - "9090:9090"

grafana:
  image: grafana/grafana:latest
  container_name: hydro_grafana
  volumes:
    - grafana_data:/var/lib/grafana
  ports:
    - "3001:3000"
  environment:
    - GF_SECURITY_ADMIN_PASSWORD=${GRAFANA_PASSWORD}
```

---

### Логирование

```bash
# Просмотр логов с фильтрацией
docker logs hydro_backend_prod 2>&1 | grep ERROR

# Экспорт логов
docker logs hydro_backend_prod > backend_logs_$(date +%Y%m%d).log
```

---

## 🔄 АВТОМАТИЗАЦИЯ

### Systemd service для автозапуска

Создай `/etc/systemd/system/hydro.service`:

```ini
[Unit]
Description=Mesh Hydro System
Requires=docker.service
After=docker.service

[Service]
Type=oneshot
RemainAfterExit=yes
WorkingDirectory=/opt/mesh_hydro/server
ExecStart=/usr/bin/docker-compose -f docker-compose.prod.yml up -d
ExecStop=/usr/bin/docker-compose -f docker-compose.prod.yml down
TimeoutStartSec=0

[Install]
WantedBy=multi-user.target
```

**Активация:**

```bash
sudo systemctl enable hydro.service
sudo systemctl start hydro.service
sudo systemctl status hydro.service
```

---

### Автоматическое обновление (опционально)

Создай `/opt/mesh_hydro/update.sh`:

```bash
#!/bin/bash
cd /opt/mesh_hydro/server

echo "🔄 Pulling latest changes..."
git pull origin main

echo "🐳 Rebuilding containers..."
docker-compose -f docker-compose.prod.yml build

echo "⬇️ Stopping services..."
docker-compose -f docker-compose.prod.yml down

echo "⬆️ Starting services..."
docker-compose -f docker-compose.prod.yml up -d

echo "💾 Running migrations..."
docker exec hydro_backend_prod php artisan migrate --force

echo "🧹 Clearing cache..."
docker exec hydro_backend_prod php artisan optimize

echo "✅ Update complete!"
```

```bash
chmod +x /opt/mesh_hydro/update.sh
```

---

## 🌐 NGINX КОНФИГУРАЦИЯ

### Проверка конфигурации

```bash
docker exec hydro_nginx_prod nginx -t
```

---

### Перезагрузка nginx

```bash
docker exec hydro_nginx_prod nginx -s reload
```

---

## 📋 ЧЕКЛИСТ PRODUCTION DEPLOYMENT

### Перед запуском:

- [ ] .env файл создан и заполнен
- [ ] DB_PASSWORD установлен (сложный!)
- [ ] REVERB_APP_KEY сгенерирован
- [ ] REVERB_APP_SECRET сгенерирован
- [ ] APP_KEY сгенерирован
- [ ] Firewall настроен
- [ ] SSL сертификаты получены (опционально)

### После запуска:

- [ ] Все контейнеры запущены (8 шт)
- [ ] Backend отвечает: `curl http://localhost/up`
- [ ] Frontend загружается: `curl http://localhost/`
- [ ] API работает: `curl http://localhost/api/nodes`
- [ ] MQTT принимает данные от ESP32
- [ ] Scheduler выполняет задачи
- [ ] Логи без ошибок

---

## 🎯 КРАТКАЯ ИНСТРУКЦИЯ (TL;DR)

```bash
# 1. Подготовка
cd /opt/mesh_hydro/server
cp .env.production.example .env
nano .env  # Заполни DB_PASSWORD и REVERB_* ключи

# 2. Генерация ключей
openssl rand -base64 32  # DB_PASSWORD
openssl rand -hex 16     # REVERB_APP_KEY
openssl rand -hex 16     # REVERB_APP_SECRET

# 3. Запуск
docker-compose -f docker-compose.prod.yml build
docker-compose -f docker-compose.prod.yml up -d

# 4. Инициализация
docker exec hydro_backend_prod php artisan key:generate --show  # Добавь в .env
docker-compose -f docker-compose.prod.yml restart backend
docker exec hydro_backend_prod php artisan migrate --force
docker exec hydro_backend_prod php artisan optimize

# 5. Проверка
docker-compose -f docker-compose.prod.yml ps
curl http://localhost/up
curl http://localhost/api/nodes

# ✅ Готово!
```

---

## 📞 ДОСТУП К СИСТЕМЕ

### Development:
- Frontend: http://localhost:3000
- Backend API: http://localhost:8000
- WebSocket: ws://localhost:8080

### Production:
- Frontend: http://your-domain.com (или http://server-ip)
- Backend API: http://your-domain.com/api
- WebSocket: ws://your-domain.com/app
- Все через Nginx ✅

---

## 🎉 ИТОГ

**Production версия включает:**

- ✅ Laravel 11.46.1 с оптимизацией
- ✅ PostgreSQL с настройками производительности
- ✅ Nginx reverse proxy с кэшированием
- ✅ SSL/TLS поддержка
- ✅ Rate limiting
- ✅ Security headers
- ✅ Автоматический рестарт
- ✅ Логирование с ротацией
- ✅ Health checks
- ✅ Systemd integration

**Система готова к боевому использованию!** 🚀

---

**Автор:** AI Assistant (Claude Sonnet 4.5)  
**Дата:** 20.10.2025 23:10  
**Версия:** Production Deployment Guide v1.0

