# 🔧 Руководство по мониторингу и эксплуатации

## Мониторинг системы

### Проверка состояния сервисов

```bash
# Статус всех контейнеров
docker-compose ps

# Использование ресурсов
docker stats

# Логи в реальном времени
docker-compose logs -f

# Логи конкретного сервиса
docker logs hydro_backend -f
docker logs hydro_mqtt_listener -f
docker logs hydro_postgres -f
```

### Проверка работы MQTT

```bash
# Логи MQTT брокера
docker logs hydro_mosquitto

# Проверка подключения MQTT Listener
docker logs hydro_mqtt_listener | grep "Connected successfully"

# Подписка на все топики (тест)
docker exec -it hydro_mosquitto mosquitto_sub -t "hydro/#" -v
```

### Мониторинг базы данных

```bash
# Подключение к PostgreSQL
docker exec -it hydro_postgres psql -U hydro -d hydro_system

# Проверка размера БД
docker exec hydro_postgres psql -U hydro -d hydro_system -c "\l+"

# Статистика таблиц
docker exec hydro_postgres psql -U hydro -d hydro_system -c "
SELECT 
  schemaname, 
  tablename, 
  pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) AS size,
  n_live_tup AS rows
FROM pg_stat_user_tables
ORDER BY pg_total_relation_size(schemaname||'.'||tablename) DESC;"
```

---

## Backup и восстановление

### Backup базы данных

```bash
# Полный бэкап
docker exec hydro_postgres pg_dump -U hydro hydro_system > backup_$(date +%Y%m%d_%H%M%S).sql

# Бэкап только структуры
docker exec hydro_postgres pg_dump -U hydro --schema-only hydro_system > schema.sql

# Бэкап только данных
docker exec hydro_postgres pg_dump -U hydro --data-only hydro_system > data.sql
```

### Восстановление

```bash
# Восстановление из бэкапа
cat backup.sql | docker exec -i hydro_postgres psql -U hydro hydro_system

# Или через docker cp
docker cp backup.sql hydro_postgres:/tmp/backup.sql
docker exec hydro_postgres psql -U hydro hydro_system -f /tmp/backup.sql
```

### Автоматический backup (cron)

**Linux/WSL:**
```bash
# Добавить в crontab
0 3 * * * cd /path/to/mesh_hydro/server && docker exec hydro_postgres pg_dump -U hydro hydro_system | gzip > /backups/hydro_$(date +\%Y\%m\%d).sql.gz
```

**Windows (Task Scheduler):**
```powershell
# backup.ps1
$date = Get-Date -Format "yyyyMMdd_HHmmss"
docker exec hydro_postgres pg_dump -U hydro hydro_system | Out-File -Encoding UTF8 "D:\backups\hydro_$date.sql"
```

---

## Обслуживание и очистка

### Очистка старых данных

```bash
# Очистка телеметрии старше 365 дней
docker exec hydro_backend php artisan telemetry:cleanup --days=365

# Очистка завершённых событий
docker exec hydro_backend php artisan db:seed --class=CleanupSeeder
```

### Очистка Docker кэша

```bash
# Удаление неиспользуемых образов
docker image prune -a

# Удаление неиспользуемых volume
docker volume prune

# Полная очистка (осторожно!)
docker system prune -a --volumes
```

### Пересоздание контейнеров

```bash
# Остановка всех сервисов
docker-compose down

# Пересоздание с чистыми образами
docker-compose up -d --build --force-recreate

# Пересоздание только backend
docker-compose up -d --build --force-recreate backend
```

---

## Масштабирование

### Увеличение узлов

Система поддерживает автообнаружение узлов:
1. Прошейте новый ESP32 узел
2. Настройте WiFi и MQTT параметры
3. Узел автоматически появится в интерфейсе

**Лимиты:**
- До 50 узлов на один MQTT брокер
- До 100 узлов с кластерным MQTT

### Оптимизация базы данных

При большом объёме телеметрии (>1M записей):

```sql
-- Партиционирование таблицы telemetry по дате
CREATE TABLE telemetry_2025_10 PARTITION OF telemetry
FOR VALUES FROM ('2025-10-01') TO ('2025-11-01');

-- Агрегация данных
CREATE MATERIALIZED VIEW telemetry_hourly AS
SELECT 
  node_id,
  date_trunc('hour', received_at) AS hour,
  AVG((data->>'temperature')::float) AS avg_temp,
  AVG((data->>'humidity')::float) AS avg_humidity
FROM telemetry
GROUP BY node_id, hour;
```

### Репликация PostgreSQL

Для высокой доступности:

```yaml
# docker-compose.yml
postgres_replica:
  image: postgres:15.4-alpine
  environment:
    POSTGRES_PASSWORD: hydro_secure_pass_2025
    PGUSER: replicator
    PGPASSWORD: replicator_pass
  command: |
    -c wal_level=replica
    -c hot_standby=on
    -c max_wal_senders=10
```

---

## Безопасность

### Рекомендации

1. **Смените пароли по умолчанию:**
   - PostgreSQL: `POSTGRES_PASSWORD`
   - MQTT: добавьте авторизацию в `mosquitto.conf`
   - Pusher: `PUSHER_APP_SECRET`

2. **Используйте HTTPS в production:**
   - Настройте nginx с SSL сертификатами
   - Let's Encrypt для бесплатных сертификатов

3. **Firewall:**
   - Закройте прямой доступ к PostgreSQL (5432) извне
   - Разрешите только MQTT (1883) для ESP32 узлов

4. **Обновления:**
   - Регулярно обновляйте Docker образы
   - Следите за обновлениями Laravel и зависимостей

### Аудит логов

```bash
# Критичные события за последние 24 часа
docker exec hydro_backend php artisan events:report --level=critical --hours=24

# Ошибки узлов
docker exec hydro_backend php artisan errors:report --severity=high

# Активность дозирования
docker logs hydro_backend | grep "Dosing"
```

---

## Производительность

### Метрики для мониторинга

**Backend:**
- Время ответа API (<200ms желательно)
- Memory usage (<512MB при 50 узлах)
- CPU usage (<50% среднее)

**MQTT:**
- Сообщений в секунду
- Задержка доставки (<100ms)
- Количество подключённых клиентов

**PostgreSQL:**
- Размер БД
- Количество подключений (<20 обычно)
- Медленные запросы (>1 сек)

### Оптимизация Laravel

```bash
# Кэширование конфигурации
docker exec hydro_backend php artisan config:cache

# Кэширование роутов
docker exec hydro_backend php artisan route:cache

# Оптимизация автозагрузки
docker exec hydro_backend composer dump-autoload --optimize

# Очистка логов
docker exec hydro_backend php artisan log:clear
```

---

## Диагностика проблем

### Checklist при сбоях

1. **Проверить статус контейнеров:**
   ```bash
   docker-compose ps
   ```

2. **Проверить логи:**
   ```bash
   docker-compose logs --tail=100
   ```

3. **Проверить сеть:**
   ```bash
   docker network inspect server_hydro_network
   ```

4. **Проверить volumes:**
   ```bash
   docker volume ls
   docker volume inspect server_postgres_data
   ```

5. **Проверить ресурсы хоста:**
   ```bash
   # Место на диске
   df -h
   
   # Память
   free -h
   
   # Загрузка CPU
   top
   ```

### Типичные ошибки

**"Connection refused" для PostgreSQL:**
- Проверьте, что контейнер запущен: `docker ps | grep postgres`
- Проверьте healthcheck: `docker inspect hydro_postgres`
- Очистите config cache: `docker exec hydro_backend php artisan config:clear`

**"MQTT connection failed":**
- Проверьте mosquitto: `docker logs hydro_mosquitto`
- Убедитесь, что порт 1883 доступен
- Проверьте firewall/антивирус

**"WebSocket connection error":**
- Система автоматически переключится на polling
- Проверьте Reverb: `docker logs hydro_backend | grep reverb`
- Порт 6001 должен быть доступен

---

## Команды Laravel Artisan

### Управление данными

```bash
# Миграции
docker exec hydro_backend php artisan migrate
docker exec hydro_backend php artisan migrate:fresh --seed

# Сидеры
docker exec hydro_backend php artisan db:seed
docker exec hydro_backend php artisan db:seed --class=NodeSeeder

# Очистка
docker exec hydro_backend php artisan telemetry:cleanup --days=365
docker exec hydro_backend php artisan commands:process-timeouts
```

### Проверка статуса

```bash
# Статус узлов
docker exec hydro_backend php artisan nodes:check-status

# Статистика системы
docker exec hydro_backend php artisan tinker
>>> App\Models\Node::count()
>>> App\Models\Telemetry::count()
>>> App\Models\Event::where('resolved_at', null)->count()
```

### Telegram бот

```bash
# Запуск бота
docker exec hydro_backend php artisan telegram:bot

# Отправка тестового сообщения
docker exec hydro_backend php artisan telegram:test
```

---

## Профили запуска

### Production (основной)
```bash
docker-compose up -d
```
Запускает: postgres, mosquitto, backend, mqtt_listener, frontend

### Development (с Socket.IO)
```bash
docker-compose --profile dev up -d
```
Дополнительно: websocket (Socket.IO на порту 6002)

### Production с Nginx
```bash
docker-compose --profile production up -d
```
Дополнительно: nginx reverse proxy (порты 80/443)

---

## Мониторинг производительности

### Prometheus + Grafana (опционально)

Для продакшена рекомендуется добавить метрики:

```yaml
# docker-compose.yml
prometheus:
  image: prom/prometheus
  volumes:
    - ./prometheus.yml:/etc/prometheus/prometheus.yml
  ports:
    - "9090:9090"

grafana:
  image: grafana/grafana
  ports:
    - "3001:3000"
```

### Ключевые метрики

- Uptime узлов
- Частота heartbeat сообщений
- Задержка MQTT сообщений
- Количество алертов в час
- Использование памяти backend
- Размер БД

---

## Обновление системы

### Обновление Docker образов

```bash
# Остановка системы
docker-compose down

# Обновление образов
docker-compose pull

# Пересборка
docker-compose build --no-cache

# Запуск
docker-compose up -d
```

### Обновление прошивок ESP32

```bash
# OTA обновление через интерфейс
# Узлы → Выбрать узел → OTA Update

# Или вручную:
cd node_ph_ec
idf.py build
idf.py flash
```

---

**Версия:** 2.0.0  
**Дата обновления:** 20.10.2025

