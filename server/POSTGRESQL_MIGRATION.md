# 🐘 Миграция на PostgreSQL

**Дата:** 2025-10-20  
**Версия:** 2.0  
**Статус:** ✅ Готово к использованию

---

## 🎯 Что изменилось

### **Было (SQLite):**
```yaml
- DB_CONNECTION=sqlite
- DB_DATABASE=/var/www/html/database/hydro_system.sqlite
```

### **Стало (PostgreSQL):**
```yaml
postgres:
  image: postgres:15-alpine
  environment:
    POSTGRES_DB: hydro_system
    POSTGRES_USER: hydro
    POSTGRES_PASSWORD: hydro_secure_pass_2025

backend:
  environment:
    - DB_CONNECTION=pgsql
    - DB_HOST=postgres
    - DB_PORT=5432
    - DB_DATABASE=hydro_system
    - DB_USERNAME=hydro
    - DB_PASSWORD=hydro_secure_pass_2025
```

---

## ✨ Преимущества PostgreSQL

### **1. Профессиональная СУБД**
- ✅ Поддержка JSONB (нативный JSON с индексами)
- ✅ Полноценные транзакции (ACID)
- ✅ Параллельная работа без блокировок
- ✅ Репликация и бэкапы

### **2. Производительность**
- ✅ GIN индексы для быстрого поиска по JSON
- ✅ Эффективная работа с большими объёмами данных
- ✅ Партиционирование таблиц
- ✅ Оптимизатор запросов

### **3. Масштабируемость**
- ✅ Множественные подключения без блокировок
- ✅ Connection pooling
- ✅ Read replicas для чтения

---

## 📊 Сравнение

| Функция | SQLite | PostgreSQL |
|---------|--------|------------|
| **Тип БД** | Файловая | Серверная |
| **Параллельные записи** | ❌ Блокировки | ✅ Множественные |
| **JSONB индексы** | ❌ Нет | ✅ GIN индексы |
| **Производительность** | Средняя | Высокая |
| **Масштабируемость** | Ограничена | Отличная |
| **Backup** | Копия файла | pg_dump/pg_restore |
| **Сложность** | Простая | Средняя |

---

## 🚀 Запуск системы

### **1. Остановить старые контейнеры**
```bash
cd server
docker compose down -v  # -v удалит старые SQLite данные
```

### **2. Пересобрать с PostgreSQL**
```bash
docker compose build --no-cache
```

### **3. Запустить систему**
```bash
docker compose up -d
```

### **4. Проверить статус**
```bash
docker compose ps
```

**Ожидаемый результат:**
```
NAME                      STATUS         PORTS
hydro_postgres            Up (healthy)   0.0.0.0:5432->5432/tcp
hydro_mosquitto           Up             0.0.0.0:1883->1883/tcp
hydro_backend             Up             0.0.0.0:8000->8000/tcp
hydro_mqtt_listener       Up
hydro_frontend            Up             0.0.0.0:3000->80/tcp
```

### **5. Проверить логи**
```bash
# PostgreSQL
docker compose logs postgres

# Backend (должны быть миграции)
docker compose logs backend | grep -i "migrat"

# Должно быть:
# Running migrations...
# Migration table created successfully.
# Migrating: 2024_01_01_000001_create_nodes_table
# Migrated:  2024_01_01_000001_create_nodes_table
```

---

## 🗄️ Структура БД

### **Таблицы:**

1. **nodes** - Узлы mesh-сети
   - Поля: id, node_id, node_type, zone, mac_address, online, last_seen_at
   - JSONB: config, metadata
   - GIN индексы: config, metadata

2. **telemetry** - Телеметрия от узлов
   - Поля: id, node_id, node_type, received_at
   - JSONB: data
   - GIN индекс: data

3. **events** - События и алерты
   - Поля: id, node_id, level, message, resolved_at
   - JSONB: data
   - GIN индекс: data

4. **commands** - История команд
   - Поля: id, node_id, command, status, sent_at, completed_at
   - JSONB: params, response
   - GIN индексы: params, response

---

## 🔍 Проверка индексов

### **Подключение к PostgreSQL:**
```bash
docker compose exec postgres psql -U hydro -d hydro_system
```

### **Просмотр таблиц:**
```sql
\dt
```

**Вывод:**
```
 Schema |   Name    | Type  | Owner 
--------+-----------+-------+-------
 public | commands  | table | hydro
 public | events    | table | hydro
 public | nodes     | table | hydro
 public | telemetry | table | hydro
```

### **Просмотр индексов:**
```sql
\di
```

**Должны быть GIN индексы:**
```
 public | nodes_config_gin        | index | hydro | nodes
 public | nodes_metadata_gin      | index | hydro | nodes
 public | telemetry_data_gin      | index | hydro | telemetry
 public | events_data_gin         | index | hydro | events
 public | commands_params_gin     | index | hydro | commands
 public | commands_response_gin   | index | hydro | commands
```

### **Проверка JSONB:**
```sql
-- Посмотреть структуру таблицы nodes
\d+ nodes

-- Должно быть:
-- config   | jsonb | nullable
-- metadata | jsonb | nullable
```

---

## 📈 Использование JSONB индексов

### **Быстрый поиск по JSON полям:**

```sql
-- Найти узлы с определённой версией прошивки
SELECT * FROM nodes 
WHERE metadata->>'firmware' = 'v1.0.0';

-- Найти телеметрию с pH < 6.0
SELECT * FROM telemetry 
WHERE data->>'ph' < '6.0'::numeric;

-- Найти узлы с определённой конфигурацией
SELECT * FROM nodes 
WHERE config @> '{"enabled": true}'::jsonb;
```

**GIN индексы делают эти запросы БЫСТРЫМИ!** ⚡

---

## 🔧 Администрирование

### **Backup базы данных:**
```bash
# Создать backup
docker compose exec postgres pg_dump -U hydro hydro_system > backup.sql

# Или с компрессией
docker compose exec postgres pg_dump -U hydro hydro_system | gzip > backup.sql.gz
```

### **Restore базы данных:**
```bash
# Из SQL файла
cat backup.sql | docker compose exec -T postgres psql -U hydro hydro_system

# Из архива
gunzip -c backup.sql.gz | docker compose exec -T postgres psql -U hydro hydro_system
```

### **Очистка старых данных:**
```bash
# Через Laravel команду
docker compose exec backend php artisan telemetry:cleanup --days=90

# Или напрямую через SQL
docker compose exec postgres psql -U hydro -d hydro_system -c \
  "DELETE FROM telemetry WHERE received_at < NOW() - INTERVAL '90 days'"
```

### **Размер БД:**
```bash
docker compose exec postgres psql -U hydro -d hydro_system -c \
  "SELECT pg_size_pretty(pg_database_size('hydro_system'))"
```

### **Статистика таблиц:**
```sql
SELECT 
  schemaname,
  tablename,
  pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) AS size,
  n_live_tup AS rows
FROM pg_stat_user_tables
ORDER BY pg_total_relation_size(schemaname||'.'||tablename) DESC;
```

---

## ⚠️ Troubleshooting

### **Проблема: "Connection refused"**

**Решение:**
```bash
# Проверить что PostgreSQL запущен
docker compose ps postgres

# Посмотреть логи
docker compose logs postgres

# Перезапустить
docker compose restart postgres
```

### **Проблема: "Too many connections"**

**Решение:**
```bash
# Посмотреть количество подключений
docker compose exec postgres psql -U hydro -d hydro_system -c \
  "SELECT count(*) FROM pg_stat_activity"

# Увеличить max_connections (в docker-compose.yml)
postgres:
  command: postgres -c max_connections=200
```

### **Проблема: "Database is locked"**

**Это не PostgreSQL проблема!** PostgreSQL не блокируется как SQLite. 🎉

---

## 🎯 Оптимизация производительности

### **1. Connection Pooling (для production):**

Добавьте PgBouncer:
```yaml
pgbouncer:
  image: pgbouncer/pgbouncer
  environment:
    DATABASES_HOST: postgres
    DATABASES_PORT: 5432
    DATABASES_USER: hydro
    DATABASES_PASSWORD: hydro_secure_pass_2025
    DATABASES_DBNAME: hydro_system
    PGBOUNCER_POOL_MODE: transaction
    PGBOUNCER_MAX_CLIENT_CONN: 1000
    PGBOUNCER_DEFAULT_POOL_SIZE: 25
```

### **2. Партиционирование телеметрии:**

Для больших объёмов данных (>1M записей):
```sql
-- Конвертировать в партиционированную таблицу
ALTER TABLE telemetry 
  PARTITION BY RANGE (received_at);

-- Создать партиции по месяцам
CREATE TABLE telemetry_2025_01 PARTITION OF telemetry
  FOR VALUES FROM ('2025-01-01') TO ('2025-02-01');

CREATE TABLE telemetry_2025_02 PARTITION OF telemetry
  FOR VALUES FROM ('2025-02-01') TO ('2025-03-01');
```

### **3. Автоматическая очистка (VACUUM):**

PostgreSQL автоматически очищает удалённые записи, но можно настроить:
```sql
-- Ручная очистка
VACUUM ANALYZE telemetry;

-- Агрессивная очистка (освобождает место на диске)
VACUUM FULL telemetry;
```

---

## 📊 Мониторинг

### **Slow queries:**
```sql
SELECT 
  query,
  calls,
  total_time,
  mean_time,
  max_time
FROM pg_stat_statements
ORDER BY mean_time DESC
LIMIT 10;
```

### **Размер индексов:**
```sql
SELECT
  schemaname,
  tablename,
  indexname,
  pg_size_pretty(pg_relation_size(indexrelid)) AS index_size
FROM pg_stat_user_indexes
ORDER BY pg_relation_size(indexrelid) DESC;
```

---

## ✅ Checklist

- [x] Docker Compose настроен с PostgreSQL
- [x] Dockerfile обновлён (pdo_pgsql расширение)
- [x] Миграции совместимы с PostgreSQL
- [x] GIN индексы для JSONB полей
- [x] Backend использует PostgreSQL по умолчанию
- [x] Healthcheck для PostgreSQL
- [x] Volume для постоянства данных

---

## 🎉 Готово!

**Система теперь работает на PostgreSQL!**

Преимущества:
- ✅ Быстрее SQLite
- ✅ GIN индексы для JSON
- ✅ Параллельная работа
- ✅ Профессиональная СУБД

**Запустите и проверьте:**
```bash
docker compose up -d
docker compose logs -f backend

# Открыть дашборд
http://localhost:3000
```

---

**Дата создания:** 2025-10-20  
**Версия:** 2.0  
**Автор:** AI Assistant

