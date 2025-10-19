# 🚀 Production Deployment Guide

## Запуск в Production режиме

### Быстрый старт:

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend

REM 1. Настроить production окружение
setup-production.bat

REM 2. Очистить данные и запустить
clean-and-start-prod.bat

REM 3. Запустить frontend (в новом терминале)
cd ..\frontend
npm run dev
```

---

## 📋 Пошаговая инструкция

### Шаг 1: Подготовка к Production

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
setup-production.bat
```

Это действие:
- ✅ Копирует `.env.production` в `.env`
- ✅ Генерирует `APP_KEY`
- ✅ Создаёт необходимые директории
- ✅ Кэширует конфигурацию и роуты

### Шаг 2: Очистка данных и запуск

```cmd
clean-and-start-prod.bat
```

Это действие:
- ✅ Останавливает все PHP процессы
- ✅ Удаляет старую БД с тестовыми данными
- ✅ Создаёт новую пустую БД
- ✅ Применяет миграции (с оптимизированными индексами)
- ✅ Очищает кэш
- ✅ Запускает Backend + MQTT Listener

### Шаг 3: Запуск Frontend

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\frontend
npm run dev
```

Откройте браузер: **http://localhost:5173**

---

## 🎯 Production настройки

### .env.production

```env
APP_ENV=production           # Production режим
APP_DEBUG=false             # Отключить debug
LOG_LEVEL=warning           # Только warnings и errors

MQTT_HOST=192.168.1.100     # IP вашего MQTT брокера
TELEMETRY_RETENTION_DAYS=365 # Хранить данные 1 год
NODE_OFFLINE_TIMEOUT=30     # Таймаут offline (секунды)
```

### Отличия от Development:

| Параметр | Development | Production |
|----------|-------------|------------|
| APP_DEBUG | true | **false** |
| LOG_LEVEL | debug | **warning** |
| Тестовые данные | ✅ Есть | ❌ **Нет** |
| Кэширование | Отключено | **Включено** |
| Оптимизации | Нет | **Да** |

---

## 📊 Пустая база данных

После запуска `clean-and-start-prod.bat` база данных **пустая**.

### Как добавить узлы:

#### Вариант 1: Автоматически (Рекомендуется)
Root Node автоматически зарегистрируется при первом подключении:
```
1. Root Node подключается к MQTT
2. Отправляет телеметрию
3. Backend автоматически создаёт запись в БД
4. Узел появляется в Dashboard
```

#### Вариант 2: Вручную через Web UI
1. Откройте Dashboard (http://localhost:5173)
2. Нажмите кнопку **"+ Добавить узел"**
3. Заполните форму:
   - Node ID: `root_001`
   - Type: `root`
   - Zone: `Main`
   - MAC: `AA:BB:CC:DD:EE:00`
4. Нажмите **"Создать"**

#### Вариант 3: Через API
```bash
curl -X POST http://localhost:8000/api/nodes \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": "root_001",
    "node_type": "root",
    "zone": "Main",
    "mac_address": "AA:BB:CC:DD:EE:00",
    "config": {
      "mqtt_broker": "192.168.1.100",
      "mqtt_port": 1883
    }
  }'
```

---

## 🔍 Проверка работы

### 1. Проверка Backend:
```cmd
curl http://localhost:8000/api/health
```
Ответ: `{"status":"ok","timestamp":"..."}`

### 2. Проверка MQTT:
```cmd
curl http://localhost:8000/api/status
```
Ответ: `{"mqtt":"connected",...}`

### 3. Проверка Dashboard:
```cmd
curl http://localhost:8000/api/dashboard/summary
```
Должно вернуть пустую статистику:
```json
{
  "nodes": {
    "total": 0,
    "online": 0,
    "offline": 0
  }
}
```

### 4. Проверка Frontend:
Откройте http://localhost:5173
- Dashboard должен быть пуст
- Кнопка "+ Добавить узел" работает
- MQTT Status: **connected** ✅

---

## 📈 Мониторинг

### Логи Backend:
```cmd
type backend\storage\logs\laravel.log
```

### Логи MQTT Listener:
Смотрите окно **"MQTT Listener (Production)"**

### Системный статус:
```cmd
curl http://localhost:8000/api/status
```

---

## 🛡️ Production Security

### Что включено:
- ✅ **Валидация данных** (Form Requests)
- ✅ **Rate Limiting** (60 req/min read, 30 req/min write)
- ✅ **Error handling** (не показывает stack traces)
- ✅ **CSRF Protection** (Laravel default)
- ✅ **Input sanitization** (валидация)

### Что нужно добавить (опционально):
- 🔒 **Authentication** (Laravel Sanctum)
- 🔒 **HTTPS** (Nginx + Let's Encrypt)
- 🔒 **Firewall** (только разрешённые IP)
- 🔒 **MQTT Authentication** (username/password)

---

## 📦 Backup

### Ручной backup БД:
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
copy database\database.sqlite database\backup_%date:~-4,4%%date:~-10,2%%date:~-7,2%.sqlite
```

### Автоматический backup (Task Scheduler):
```cmd
schtasks /create /tn "Hydro DB Backup" /tr "C:\esp\hydro\mesh\mesh_hydro\server\backend\backup-database.bat" /sc daily /st 03:00
```

Создайте `backup-database.bat`:
```bat
@echo off
cd /d "C:\esp\hydro\mesh\mesh_hydro\server\backend"
copy database\database.sqlite database\backup\db_%date:~-4,4%%date:~-10,2%%date:~-7,2%.sqlite
forfiles /p database\backup /m *.sqlite /d -30 /c "cmd /c del @path"
```

---

## 🔄 Обновление системы

### Обновление Backend:
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend

REM Backup БД
copy database\database.sqlite database\database.backup.sqlite

REM Остановить систему
taskkill /F /IM php.exe

REM Обновить код (git pull или копирование файлов)

REM Применить новые миграции
C:\PHP\php.exe artisan migrate --force

REM Очистить кэш
C:\PHP\php.exe artisan cache:clear
C:\PHP\php.exe artisan config:clear

REM Запустить
START_ALL.bat
```

### Обновление Frontend:
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\frontend

REM Обновить зависимости
npm install

REM Пересобрать
npm run build

REM Для dev сервера:
npm run dev
```

---

## 🚨 Troubleshooting

### Проблема: "Database is locked"
```cmd
# Остановить все PHP процессы
taskkill /F /IM php.exe

# Подождать 5 секунд
timeout /t 5

# Запустить заново
START_ALL.bat
```

### Проблема: "MQTT: offline"
```cmd
# Проверить Mosquitto
netstat -an | findstr :1883

# Если не запущен:
net start mosquitto

# Или запустить вручную
mosquitto -v
```

### Проблема: "Too Many Requests (429)"
Вы достигли rate limit. Подождите 1 минуту или увеличьте лимит в `routes/api.php`:
```php
Route::middleware('throttle:120,1')  // Было 60
```

---

## 📊 Performance Tips

### 1. Используйте PostgreSQL для больших объёмов:
```env
DB_CONNECTION=pgsql
DB_HOST=127.0.0.1
DB_DATABASE=hydro_system
```

### 2. Включите Redis для кэширования:
```env
CACHE_DRIVER=redis
SESSION_DRIVER=redis
QUEUE_CONNECTION=redis
```

### 3. Настройте Nginx как reverse proxy:
```nginx
server {
    listen 80;
    server_name hydro.local;

    location / {
        proxy_pass http://localhost:5173;
    }

    location /api {
        proxy_pass http://localhost:8000;
    }
}
```

### 4. Настройте автоматическую очистку:
В `app/Console/Kernel.php`:
```php
protected function schedule(Schedule $schedule)
{
    $schedule->command('telemetry:cleanup --days=365')
             ->dailyAt('02:00');
}
```

---

## ✅ Production Checklist

Перед запуском в production:

- [ ] ✅ Применены все миграции
- [ ] ✅ База данных очищена от тестовых данных
- [ ] ✅ APP_DEBUG=false
- [ ] ✅ LOG_LEVEL=warning
- [ ] ✅ Настроен MQTT_HOST
- [ ] ✅ Rate limiting включен
- [ ] ✅ Валидация работает
- [ ] ✅ Frontend собран
- [ ] ✅ Backend работает
- [ ] ✅ MQTT Listener работает
- [ ] ✅ Root Node подключается
- [ ] ✅ Настроен backup
- [ ] ✅ Протестирована работа

---

**Production готов! 🚀**

**Версия:** 1.0.0  
**Дата:** 18.10.2025  
**Автор:** AI Assistant

