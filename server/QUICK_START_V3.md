# 🚀 Быстрый запуск Mesh Hydro v3.0

**Дата:** 2025-10-20  
**Версия:** 3.0 Beta  
**Новое:** Real-time WebSocket + Система ошибок + Современный дизайн

---

## ⚡ Запуск за 3 шага

### 1. Установить frontend зависимости

```bash
cd d:\mesh\mesh_hydro\server\frontend
npm install
```

**Установятся:**
- `laravel-echo` - WebSocket клиент
- `socket.io-client` - Socket.io
- `@vueuse/core` - Vue утилиты
- `vitest` - тестирование
- `@vue/test-utils` - Vue тесты

### 2. Пересобрать Docker

```bash
cd d:\mesh\mesh_hydro\server
docker compose down -v
docker compose build --no-cache
docker compose up -d
```

**Запустится:**
- ✅ PostgreSQL 15 (порт 5432)
- ✅ Mosquitto MQTT (порт 1883)
- ✅ **Soketi WebSocket (порт 6001)** ← НОВОЕ!
- ✅ Backend API (порт 8000)
- ✅ Frontend (порт 3000)
- ✅ MQTT Listener

### 3. Проверить систему

**Откройте:** http://localhost:3000

**В консоли браузера (F12) должно быть:**
```
✅ WebSocket connected
✅ WebSocket listeners configured
```

**Проверьте логи:**
```bash
# Soketi
docker compose logs -f soketi

# Backend
docker compose logs -f backend

# MQTT Listener
docker compose logs -f mqtt_listener
```

---

## 🎯 Что нового в v3.0

### ⚡ Real-time обновления
- **Было:** Polling каждые 30 сек
- **Стало:** WebSocket с мгновенными обновлениями
- **Задержка:** <1 секунда

**Что обновляется real-time:**
- 📊 Телеметрия от узлов
- 🔄 Статус узлов (online/offline)
- 🔍 Обнаружение новых узлов
- 🔔 События
- ❌ Ошибки узлов

### 🐛 Система ошибок
**Новая страница:** http://localhost:3000/errors

**Возможности:**
- Детальная информация об ошибках ESP32
- Error code, type, severity
- Диагностика (heap, uptime, GPIO)
- Stack trace если доступен
- Статистика по ошибкам
- Фильтры и поиск
- Резолвение ошибок с заметками

**Health Score узлов:**
- 0-100% на основе ошибок, памяти, uptime, WiFi
- Цветовая индикация (зеленый/желтый/красный)

### 🎨 Современный дизайн
**Улучшения:**
- Градиенты и glassmorphism эффекты
- Pulse анимация для online узлов
- Анимированные счетчики
- Улучшенная темная тема
- Stagger-fade transitions
- Skeleton loaders
- Улучшенные empty states

**Компоненты:**
- `GradientCard` - карточки с градиентами
- `StatCard` - статистические карточки с анимацией
- `GlassCard` - glassmorphism
- `AnimatedNumber` - анимированные цифры

### 📱 Адаптивная разметка
- Responsive на mobile/tablet/desktop
- Touch-friendly кнопки (44x44px)
- Адаптивные заголовки и spacing
- Mobile-optimized padding

### 🧪 Unit тесты
**Новое:**
- Vitest настроен
- 20+ тестов написано
- Coverage ~40%

**Запуск:**
```bash
cd frontend
npm run test
npm run test:ui
npm run test:coverage
```

---

## 🔍 Проверка компонентов

### WebSocket:
```bash
# Проверить что Soketi запущен
docker compose ps soketi

# Статус: Up
# Ports: 0.0.0.0:6001->6001/tcp
```

### PostgreSQL:
```bash
# Проверить таблицу ошибок
docker compose exec postgres psql -U hydro -d hydro_system -c "\d node_errors"

# Должна быть таблица node_errors
```

### Frontend:
```
1. Откройте http://localhost:3000
2. F12 → Console
3. Должно быть: "✅ WebSocket connected"
```

### Backend Broadcasting:
```bash
# Проверить env
docker compose exec backend php artisan tinker
>>> config('broadcasting.default')
=> "pusher"

>>> config('pusher.app_id')
=> "hydro-app"
```

---

## 📡 MQTT топики (обновлено)

### Существующие:
- `hydro/telemetry/#` - телеметрия
- `hydro/event/#` - события
- `hydro/heartbeat/#` - живые сигналы
- `hydro/discovery` - обнаружение узлов
- `hydro/command/#` - команды узлам
- `hydro/response/#` - ответы от узлов

### НОВЫЙ:
- `hydro/error/#` - ошибки узлов ⭐

**Формат сообщения:**
```json
{
  "type": "error",
  "node_id": "climate_001",
  "error_code": "SENSOR_READ_FAIL",
  "error_type": "hardware",
  "severity": "high",
  "message": "DHT22 sensor read timeout",
  "diagnostics": {
    "heap_free": 45000,
    "heap_min": 32000,
    "uptime": 86400,
    "retry_count": 3
  },
  "timestamp": 1697548805
}
```

---

## 🎮 Новые возможности UI

### Dashboard:
- Hero section с градиентом
- Анимированные stat cards
- System status indicator
- Улучшенный empty state

### Страница ошибок (/errors):
- Таблица всех ошибок
- Фильтры: узел, severity, type, status
- Статистика
- Массовое резолвение
- ErrorDetailsDialog для деталей

### NodeCard:
- Pulse анимация для online
- Gradient border
- Health indicator (будет добавлен)
- Error badge (будет добавлен)

### NodeDetail:
- Health Score карточка
- Error timeline (будет добавлен)

---

## 🛠️ Troubleshooting

### WebSocket не подключается

**Проблема:** "WebSocket connection error" в консоли

**Решение:**
```bash
# Проверить Soketi
docker compose logs soketi

# Перезапустить
docker compose restart soketi

# Проверить порт
netstat -ano | findstr :6001
```

### Ошибки не появляются

**Проблема:** Ошибки от ESP32 не сохраняются

**Решение:**
```bash
# Проверить MQTT Listener
docker compose logs mqtt_listener | grep "ERROR"

# Должно быть:
# 📡 Subscribing to: hydro/error/#
```

### Тесты не запускаются

**Проблема:** `npm run test` не работает

**Решение:**
```bash
cd frontend

# Переустановить зависимости
rm -rf node_modules package-lock.json
npm install

# Запустить снова
npm run test
```

---

## 📚 Документация

- `FRONTEND_IMPROVEMENTS_DONE.md` - Детальный отчет
- `POSTGRESQL_ONLY.md` - PostgreSQL конфигурация
- `docker-compose.yml` - Docker services
- `frontend/vitest.config.js` - Test configuration

---

## 🎉 Готово!

**Система обновлена до версии 3.0!**

### Что работает:
✅ Real-time WebSocket  
✅ Детализация ошибок  
✅ Современный дизайн  
✅ Адаптивная разметка  
✅ Unit тесты (40%)  

### Что осталось:
⏳ Mobile Bottom Navigation  
⏳ Error Timeline  
⏳ Больше тестов (60%+)  
⏳ ESP32 error reporting  

**Наслаждайтесь улучшенной системой!** 🚀

---

**Создано:** 2025-10-20  
**Автор:** AI Assistant  
**Версия:** 3.0 Beta  
**Статус:** ✅ Ready to Use

