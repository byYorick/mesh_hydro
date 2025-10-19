# 🔧 Docker Troubleshooting

**Решение проблем с Docker**

---

## ❌ Проблема: "Frontend не видит сервер"

### Симптомы:
```
Console Error:
- Failed to fetch
- Network error
- CORS error
- ERR_CONNECTION_REFUSED
```

### Причина:
Frontend пытается подключиться к `http://localhost:8000`, но в Docker это не работает.

### ✅ Решение:

**1. Используется Nginx proxy:**

Frontend делает запросы на `/api` (относительный путь), Nginx перенаправляет на backend:

```nginx
# frontend/nginx.conf
location /api {
    proxy_pass http://backend:8000/api;
}
```

**2. VITE_API_URL изменён:**

В `docker-compose.yml`:
```yaml
frontend:
  build:
    args:
      - VITE_API_URL=/api  # Было: http://localhost:8000
```

**3. CORS настроен в Laravel:**

```php
// backend/config/cors.php
'allowed_origins' => ['*'],
'allowed_methods' => ['*'],
```

### Как исправить:

```bash
# Остановить
docker compose down

# Пересобрать с новыми настройками
docker compose build --no-cache frontend backend

# Запустить
docker compose up -d

# Проверить
curl http://localhost:3000/api/status
```

---

## ❌ Проблема: "Database locked"

### Симптомы:
```
SQLSTATE[HY000] [5] database is locked
```

### Причина:
SQLite не может работать с множественными параллельными записями.

### ✅ Решение:

**1. Проверить права доступа:**
```bash
docker compose exec backend ls -la /var/www/html/database/
docker compose exec backend chmod 666 /var/www/html/database/hydro_system.sqlite
```

**2. Настроить Laravel для SQLite:**

В `backend/.env`:
```env
DB_CONNECTION=sqlite
DB_DATABASE=/var/www/html/database/hydro_system.sqlite
```

В `backend/config/database.php`:
```php
'sqlite' => [
    'database' => env('DB_DATABASE'),
    'foreign_key_constraints' => true,
    'busy_timeout' => 5000,  // Добавить!
],
```

---

## ❌ Проблема: "Port already in use"

### Симптомы:
```
Error: bind: address already in use
Ports are not available: port is already allocated
```

### Причина:
Порты 1883, 3000 или 8000 заняты другими приложениями.

### ✅ Решение:

**1. Найти процесс на порту:**
```cmd
netstat -ano | findstr :3000
netstat -ano | findstr :8000
netstat -ano | findstr :1883
```

**2. Убить процесс:**
```cmd
taskkill /PID <PID> /F
```

**3. Или изменить порты в docker-compose.yml:**
```yaml
ports:
  - "3001:80"    # Было 3000:80
  - "8001:8000"  # Было 8000:8000
```

---

## ❌ Проблема: "Container keeps restarting"

### Симптомы:
```
docker compose ps
# Status: Restarting
```

### Причина:
Ошибка в приложении или неправильная конфигурация.

### ✅ Решение:

**1. Смотреть логи:**
```bash
docker compose logs backend
docker compose logs frontend
docker compose logs mosquitto
```

**2. Войти в контейнер:**
```bash
docker compose exec backend sh
php artisan config:clear
php artisan cache:clear
```

**3. Проверить environment:**
```bash
docker compose exec backend env | grep DB_
```

---

## ❌ Проблема: "No such file or directory (SQLite)"

### Симптомы:
```
SQLSTATE[HY000] [14] unable to open database file
```

### Причина:
SQLite файл не создан или нет прав доступа.

### ✅ Решение:

```bash
# Войти в контейнер
docker compose exec backend sh

# Создать файл
mkdir -p /var/www/html/database
touch /var/www/html/database/hydro_system.sqlite
chmod 666 /var/www/html/database/hydro_system.sqlite

# Выполнить миграции
php artisan migrate --force
```

---

## ❌ Проблема: "MQTT connection failed"

### Симптомы:
```
ESP32 logs:
E (12345) MQTT: Connection refused
```

### Причина:
ESP32 не может подключиться к MQTT broker в Docker.

### ✅ Решение:

**1. Проверить IP ПК:**
```cmd
ipconfig
# Найти IPv4 Address: 192.168.1.XXX
```

**2. В ROOT Node указать IP:**

`root_node/components/mqtt_client/mqtt_client_manager.c`:
```c
#define MQTT_BROKER_URI "mqtt://192.168.1.100:1883"
//                              ↑↑↑
//                         IP вашего ПК!
```

**3. Проверить firewall:**
```cmd
netsh advfirewall firewall add rule name="MQTT Docker" dir=in action=allow protocol=TCP localport=1883
```

**4. Проверить что Mosquitto запущен:**
```bash
docker compose ps mosquitto
docker compose logs mosquitto
```

---

## ❌ Проблема: "Docker build failed"

### Симптомы:
```
ERROR [internal] load metadata for...
failed to solve: failed to resolve source metadata
```

### Причина:
Нет интернета или Docker registry недоступен.

### ✅ Решение:

**1. Проверить интернет:**
```cmd
ping google.com
```

**2. Перезапустить Docker Desktop**

**3. Очистить кэш:**
```bash
docker system prune -a
```

**4. Попробовать снова:**
```bash
docker compose build --no-cache
```

---

## ❌ Проблема: "Volume permission denied"

### Симптомы:
```
Error: permission denied
Cannot write to /var/www/html/storage
```

### Причина:
Неправильные права доступа в контейнере.

### ✅ Решение:

```bash
# Войти в контейнер
docker compose exec backend sh

# Исправить права
chown -R www-data:www-data /var/www/html
chmod -R 775 /var/www/html/storage
chmod -R 775 /var/www/html/bootstrap/cache
chmod 666 /var/www/html/database/hydro_system.sqlite
```

---

## 🔍 ПОЛЕЗНЫЕ КОМАНДЫ

### Просмотр логов:
```bash
docker compose logs -f                    # Все логи
docker compose logs -f backend            # Только backend
docker compose logs -f --tail=100 backend # Последние 100 строк
```

### Перезапуск:
```bash
docker compose restart backend
docker compose restart frontend
docker compose restart
```

### Вход в контейнер:
```bash
docker compose exec backend sh
docker compose exec frontend sh
docker compose exec mosquitto sh
```

### Проверка статуса:
```bash
docker compose ps
docker compose top
```

### Очистка:
```bash
docker compose down                    # Остановить
docker compose down -v                 # Остановить + удалить volumes
docker system prune -a                # Очистить всё
```

### Пересборка:
```bash
docker compose build --no-cache       # Без кэша
docker compose up -d --build          # Пересобрать и запустить
```

---

## ✅ CHECKLIST ПРИ ПРОБЛЕМАХ

- [ ] Docker Desktop запущен?
- [ ] Порты свободны (3000, 8000, 1883)?
- [ ] Интернет работает?
- [ ] Firewall не блокирует?
- [ ] Логи проверены (`docker compose logs`)?
- [ ] Environment переменные правильные?
- [ ] SQLite файл создан и доступен?
- [ ] CORS настроен в Laravel?
- [ ] Nginx proxy работает?
- [ ] ESP32 подключается к правильному IP?

---

## 📞 ПОДДЕРЖКА

Если проблема не решена:

1. Соберите логи:
```bash
docker compose logs > logs.txt
```

2. Проверьте конфигурацию:
```bash
docker compose config
```

3. Проверьте сеть:
```bash
docker network inspect mesh_hydro_hydro_network
```

---

**Дата:** 2025-10-18  
**Версия:** 1.0

