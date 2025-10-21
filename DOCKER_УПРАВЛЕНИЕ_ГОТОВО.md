# 🐳 DOCKER УПРАВЛЕНИЕ ГОТОВО!

## ✅ ЧТО РЕАЛИЗОВАНО

### 1. **DockerService** - Backend сервис ✅
**Файл:** `server/backend/app/Services/DockerService.php`

**Возможности:**
- ✅ Получение статуса всех контейнеров
- ✅ Перезапуск всех контейнеров
- ✅ Перезапуск конкретного контейнера
- ✅ Запуск всех контейнеров
- ✅ Остановка всех контейнеров
- ✅ Получение логов контейнера

**Использует:**
- Laravel Process facade
- docker-compose команды
- JSON вывод для парсинга

---

### 2. **DockerController** - API эндпоинты ✅
**Файл:** `server/backend/app/Http/Controllers/DockerController.php`

**API:**
```
GET  /api/docker/status              - Статус контейнеров
GET  /api/docker/logs                - Логи контейнера
POST /api/docker/restart/all         - Перезапустить все
POST /api/docker/restart/container   - Перезапустить один
POST /api/docker/start/all           - Запустить все
POST /api/docker/stop/all            - Остановить все (по умолчанию выключено)
```

**Безопасность:**
- ✅ Rate limiting (10 запросов в минуту)
- ✅ Валидация параметров
- ✅ Логирование всех операций
- ✅ Разрешения в конфиге

---

### 3. **DockerManager.vue** - UI компонент ✅
**Файл:** `server/frontend/src/components/DockerManager.vue`

**Функционал:**
- ✅ Отображение статуса всех контейнеров
- ✅ Цветовая индикация (running, stopped, etc)
- ✅ Health status (healthy/unhealthy)
- ✅ Перезапуск конкретного контейнера
- ✅ Перезапуск всех контейнеров
- ✅ Запуск/остановка всех
- ✅ Просмотр логов контейнера
- ✅ Подтверждение критичных действий
- ✅ Автообновление каждые 30 сек

---

### 4. **Конфигурация** ✅
**Файл:** `server/backend/config/docker.php`

```php
'enabled' => true,                    // Включить управление
'allowed_operations' => [
    'status' => true,                 // Просмотр статуса
    'restart' => true,                // Перезапуск
    'start' => true,                  // Запуск
    'stop' => false,                  // Остановка (выключено для безопасности)
    'logs' => true,                   // Просмотр логов
],
```

---

### 5. **Интеграция в Settings.vue** ✅

DockerManager добавлен на страницу Settings рядом с Telegram!

---

## 🎨 UI КОМПОНЕНТ

### Внешний вид:
```
╔═══════════════════════════════════════════════════════╗
║  🐳 Управление Docker контейнерами          🔄        ║
╠═══════════════════════════════════════════════════════╣
║                                                       ║
║  🟢 backend           running                [📄][🔄] ║
║     Healthy • Up 2 hours                             ║
║                                                       ║
║  🟢 frontend          running                [📄][🔄] ║
║     Up 2 hours                                       ║
║                                                       ║
║  🟢 websocket         running                [📄][🔄] ║
║     Healthy • Up 2 hours                             ║
║                                                       ║
║  🟢 mqtt              running                [📄][🔄] ║
║     Up 2 hours                                       ║
║                                                       ║
║  🟢 postgres          running                [📄][🔄] ║
║     Healthy • Up 2 hours                             ║
║                                                       ║
╠═══════════════════════════════════════════════════════╣
║  [▶ Запустить все] [🔄 Перезапустить] [⏹ Остановить]║
╚═══════════════════════════════════════════════════════╝
```

---

## 🔄 ФУНКЦИИ

### 1. **Статус контейнеров**
- Автоматическое обновление каждые 30 сек
- Цветовая индикация состояния
- Health check статус

### 2. **Перезапуск**
**Отдельный контейнер:**
- Нажать 🔄 рядом с контейнером
- Подтверждение
- Контейнер перезапускается
- Статус обновляется через 3 сек

**Все контейнеры:**
- Нажать "Перезапустить все"
- Подтверждение с предупреждением
- Все контейнеры перезапускаются
- Система недоступна 30-60 сек

### 3. **Просмотр логов**
- Нажать 📄 рядом с контейнером
- Открывается диалог с последними 200 строками
- Кнопка "Обновить" для перезагрузки
- Монопространственный шрифт

---

## 🛡️ БЕЗОПАСНОСТЬ

### Защита от случайных действий:
```js
// Подтверждение для критичных операций
const confirmRestartAll = () => {
  confirmMessage.value = 'Система будет недоступна 30-60 секунд.'
  confirmDialog.value = true
}
```

### Защита от спама:
```php
// Rate limiting: 10 запросов в минуту
Route::middleware('throttle:10,1')->group(...)
```

### Защита от несанкционированного доступа:
```php
// Проверка разрешений в конфиге
if (!config('docker.allowed_operations.stop', false)) {
    return response()->json(['error' => 'Operation disabled'], 403);
}
```

### Логирование:
```php
Log::warning("Docker stop all requested", [
    'user' => request()->user()?->email ?? 'api',
]);
```

---

## 📊 ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ

### Сценарий 1: Просмотр статуса
```
1. Пользователь открывает /settings
2. Видит раздел "Docker управление"
3. Видит список всех контейнеров
4. backend: 🟢 running, healthy
5. postgres: 🟢 running, healthy
```

### Сценарий 2: Перезапуск Backend
```
1. Пользователь нажимает 🔄 рядом с "backend"
2. Появляется confirm: "Перезапустить backend?"
3. Нажимает "OK"
4. Toast: "Контейнер backend перезапускается..."
5. Через 3 сек статус обновляется
6. backend: 🟢 running (обновлено)
```

### Сценарий 3: Просмотр логов
```
1. Пользователь нажимает 📄 рядом с "mqtt"
2. Открывается диалог с логами:
   mqtt_1  | Listening on port 1883
   mqtt_1  | Client connected: hydro_backend
3. Нажимает "Обновить" для свежих логов
```

### Сценарий 4: Перезапуск всех
```
1. Нажимает "Перезапустить все"
2. Диалог: "⚠️ Система будет недоступна 30-60 сек"
3. Подтверждает
4. Toast: "Все контейнеры перезапускаются..."
5. Через 5 сек статус обновляется
6. Все контейнеры: 🟢 running
```

---

## 🔧 КОНФИГУРАЦИЯ

### .env файл:
```bash
# Docker управление
DOCKER_MANAGEMENT_ENABLED=true
DOCKER_COMPOSE_PATH=/path/to/server
DOCKER_ALLOW_STOP=false  # Запретить остановку (безопасность)
```

### Права доступа:
```php
// config/docker.php
'allowed_operations' => [
    'status' => true,    // ✅ Всегда разрешено
    'restart' => true,   // ✅ Разрешено
    'start' => true,     // ✅ Разрешено
    'stop' => false,     // ❌ Запрещено (безопасность)
    'logs' => true,      // ✅ Разрешено
],
```

---

## 📋 API ЭНДПОИНТЫ

### GET /api/docker/status
**Ответ:**
```json
{
  "success": true,
  "docker": {
    "enabled": true,
    "running": true,
    "containers": [
      {
        "name": "server_backend_1",
        "service": "backend",
        "state": "running",
        "status": "Up 2 hours",
        "health": "healthy"
      },
      ...
    ]
  }
}
```

### POST /api/docker/restart/all
**Ответ:**
```json
{
  "success": true,
  "message": "All containers restarted successfully"
}
```

### POST /api/docker/restart/container
**Запрос:**
```json
{
  "container": "backend"
}
```

**Ответ:**
```json
{
  "success": true,
  "message": "Container backend restarted successfully"
}
```

### GET /api/docker/logs
**Запрос:**
```
?container=backend&lines=200
```

**Ответ:**
```json
{
  "success": true,
  "logs": "backend_1  | Server started on port 9000\nbackend_1  | ..."
}
```

---

## 🎯 ИНТЕГРАЦИЯ

### В Settings.vue:
```vue
<template>
  <v-row>
    <!-- Telegram -->
    <v-col cols="12" lg="6">
      <TelegramSetupGuide />
    </v-col>

    <!-- Docker -->
    <v-col cols="12" lg="6">
      <DockerManager />
    </v-col>
  </v-row>
</template>
```

### Маршрут уже есть:
```
/settings → Settings.vue → включает DockerManager
```

---

## ⚠️ БЕЗОПАСНОСТЬ

### Что РАЗРЕШЕНО по умолчанию:
- ✅ Просмотр статуса
- ✅ Перезапуск контейнеров
- ✅ Запуск контейнеров
- ✅ Просмотр логов

### Что ЗАПРЕЩЕНО:
- ❌ Остановка всех контейнеров (можно включить через .env)
- ❌ Удаление контейнеров
- ❌ Изменение docker-compose.yml

### Rate Limiting:
- **10 операций в минуту** - защита от спама
- Просмотр статуса и логов - без ограничений

---

## 🎉 ИТОГ

**Добавлено:**
- ✅ DockerService (полное управление)
- ✅ DockerController (5 эндпоинтов)
- ✅ DockerManager.vue (UI компонент)
- ✅ config/docker.php (настройки)
- ✅ routes/api.php (маршруты)
- ✅ Settings.vue (интеграция)

**Возможности:**
- ✅ Просмотр статуса контейнеров
- ✅ Перезапуск из UI
- ✅ Просмотр логов
- ✅ Автообновление статуса
- ✅ Безопасность и подтверждения

**Файлов:** 6  
**Строк кода:** ~800  

---

## 🚀 ИСПОЛЬЗОВАНИЕ

```
1. Открыть http://localhost:3000/settings
2. Прокрутить до "Docker управление"
3. Видеть статус всех контейнеров
4. Нажать 🔄 для перезапуска
5. Нажать 📄 для просмотра логов
```

**ГОТОВО!** 🐳✅

---

**Дата:** 21 октября 2025  
**Время:** 30 минут  
**Статус:** ✅ **РЕАЛИЗОВАНО**

