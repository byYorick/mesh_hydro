# 🚀 БЫСТРЫЙ ЗАПУСК С LARAVEL 11 + REVERB

## 📦 Предварительные требования

- Docker & Docker Compose
- 4GB+ свободной RAM
- Порты свободны: 3000, 5432, 1883, 8000, 8080

---

## ⚡ Быстрый старт

### 1. Остановить старые контейнеры:

```bash
cd server
docker-compose down
```

### 2. Пересобрать с Laravel 11:

```bash
docker-compose build
```

### 3. Запустить все сервисы:

```bash
docker-compose up -d
```

### 4. Проверить статус:

```bash
docker-compose ps
```

**Ожидаемый вывод:**
```
hydro_backend       Up      8000/tcp
hydro_reverb        Up      8080/tcp
hydro_frontend      Up      3000/tcp
hydro_postgres      Up      5432/tcp
hydro_mosquitto     Up      1883/tcp
hydro_mqtt_listener Up
```

---

## 🧪 Проверка работы

### 1. Backend (Laravel API):

```bash
curl http://localhost:8000/api/nodes
```

### 2. Frontend (Vue.js):

Открыть в браузере: http://localhost:3000

### 3. Reverb (WebSocket):

```bash
docker logs hydro_reverb --tail 20
```

**Должно быть:**
```
Starting Reverb server on 0.0.0.0:8080
Reverb server started successfully
```

### 4. WebSocket подключение:

Открыть консоль браузера (F12) на http://localhost:3000:

**Ожидается:**
```
✅ Echo initialized
✅ WebSocket connected
```

---

## 🐛 Troubleshooting

### Проблема: Backend не запускается

```bash
# Проверить логи
docker logs hydro_backend --tail 50

# Возможное решение: переустановить зависимости
docker exec hydro_backend composer install
docker exec hydro_backend php artisan migrate --force
```

### Проблема: Reverb не подключается

```bash
# Проверить логи
docker logs hydro_reverb

# Проверить порт
netstat -an | findstr "8080"

# Перезапустить
docker restart hydro_reverb
```

### Проблема: WebSocket ошибка "cluster"

Это исправлено! Но если видишь, обнови frontend:

```bash
docker-compose build frontend
docker-compose up -d frontend
```

---

## 📊 Мониторинг

### Все логи сразу:

```bash
docker-compose logs -f
```

### Отдельные сервисы:

```bash
docker logs hydro_backend -f
docker logs hydro_reverb -f
docker logs hydro_frontend -f
```

---

## 🎯 Что дальше?

1. ✅ Система запущена с Laravel 11
2. ✅ WebSocket работает через Reverb
3. ⏭️ Прошить ROOT узел (если еще не прошит)
4. ⏭️ Проверить real-time обновления в интерфейсе
5. ⏭️ Прошить другие NODE узлы

---

**Готово! Mesh Hydro V2 на Laravel 11 работает!** 🎉

