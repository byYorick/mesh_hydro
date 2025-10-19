# 🔧 Устранение неполадок Frontend

## Проблема: "MQTT: offline" в интерфейсе

### Быстрое решение:

1. **Обновите страницу** (F5 или Ctrl+R)
2. Или нажмите кнопку **"Обновить"** в правом верхнем углу

### Проверка статуса:

#### 1. Проверьте Backend API:
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
test-api.bat
```

Должно показать:
```json
{
  "status": "running",
  "database": "ok",
  "mqtt": "connected",
  ...
}
```

#### 2. Проверьте MQTT Listener:
- Окно "MQTT Listener" должно показывать:
```
✅ Connected successfully!
🎧 MQTT Listener is running...
```

#### 3. Проверьте MQTT Broker:
```cmd
netstat -an | findstr :1883
```

Должно показать:
```
TCP    0.0.0.0:1883           0.0.0.0:0              LISTENING
TCP    192.168.1.100:1883     192.168.1.191:xxxx     ESTABLISHED
```

### Как работает определение статуса MQTT:

1. **Frontend** каждые N секунд вызывает `/api/status`
2. **Backend** проверяет:
   - Есть ли телеметрия за последние 2 минуты?
   - Если нет - проверяет socket соединение с MQTT брокером
3. **Результат** передается во frontend:
   ```javascript
   this.mqttConnected = this.systemStatus.mqtt === 'connected'
   ```

### Автообновление

По умолчанию данные обновляются каждые 30 секунд (настраивается в Settings).

Чтобы изменить:
1. Откройте **Settings** (⚙️)
2. Вкладка **"UI Preferences"**
3. Измените **"Refresh Interval"**

### Ручное обновление

Нажмите кнопку **"🔄 Обновить"** в правом верхнем углу Dashboard.

Это выполнит:
```javascript
await appStore.fetchSystemStatus()
```

И обновит статус MQTT.

### Проверка в консоли браузера:

Откройте DevTools (F12) → Console:

```javascript
// Проверить текущий статус
console.log('MQTT Status:', window.appStore.mqttConnected)

// Проверить полный статус системы
console.log('System Status:', window.appStore.systemStatus)

// Принудительно обновить статус
await window.appStore.fetchSystemStatus()
```

### Проблема: Статус не обновляется автоматически

**Решение 1:** Проверьте настройки автообновления
- Settings → UI Preferences
- "Enable Auto-refresh" должно быть включено
- "Refresh Interval" > 0

**Решение 2:** Перезапустите frontend
```cmd
# Ctrl+C в терминале frontend
cd C:\esp\hydro\mesh\mesh_hydro\server\frontend
npm run dev
```

### Проблема: API возвращает 500 или 404

**Проверка 1:** Backend запущен?
```cmd
curl http://localhost:8000/api/health
```

Должно вернуть:
```json
{"status":"ok","timestamp":"..."}
```

**Проверка 2:** Проверьте логи Laravel
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
type storage\logs\laravel.log
```

**Проверка 3:** Очистите кэш
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
C:\PHP\php.exe artisan cache:clear
C:\PHP\php.exe artisan config:clear
```

### Проблема: CORS ошибки

Если в консоли браузера видите ошибки CORS:

**Решение:** Проверьте `backend/config/cors.php`

Должно быть:
```php
'paths' => ['api/*', 'sanctum/csrf-cookie'],
'allowed_origins' => ['http://localhost:5173'],
```

### Отладка в реальном времени:

#### В App.vue добавьте:
```javascript
watch(() => appStore.mqttConnected, (newVal, oldVal) => {
  console.log('MQTT status changed:', oldVal, '→', newVal)
})
```

#### В браузере:
1. Откройте DevTools (F12)
2. Вкладка Network
3. Фильтр: `/api/status`
4. Смотрите ответы от сервера

### Типичные причины и решения:

| Симптом | Причина | Решение |
|---------|---------|---------|
| "MQTT: offline" сразу после запуска | Страница еще не обновилась | Подождите 30 сек или обновите |
| "MQTT: offline" постоянно | MQTT Listener не запущен | Запустите start-mqtt.bat |
| "Нет связи" | Backend не запущен | Запустите start-dev.bat |
| API возвращает "disconnected" | Mosquitto не запущен | net start mosquitto |
| API возвращает "error" | Порт 1883 закрыт | Проверьте firewall |

### Проверка всей цепочки:

```cmd
# 1. MQTT Broker
netstat -an | findstr :1883

# 2. Backend Laravel
curl http://localhost:8000/api/health

# 3. MQTT Listener
# Смотрим окно "MQTT Listener"

# 4. API Status
curl http://localhost:8000/api/status

# 5. Frontend
# Открываем http://localhost:5173 и проверяем
```

### Логи для диагностики:

#### Backend Laravel:
```cmd
cd backend
type storage\logs\laravel.log | findstr MQTT
```

#### MQTT Listener:
Смотрите окно "MQTT Listener" - там выводятся все сообщения.

#### Frontend (Browser Console):
```javascript
// Включить подробные логи
localStorage.setItem('debug', 'true')
location.reload()
```

### Контакты для помощи:

Если проблема не решается:
1. Сделайте скриншот ошибки
2. Скопируйте вывод test-api.bat
3. Скопируйте логи из Laravel
4. Опишите, что именно не работает

---

**Последнее обновление:** 18.10.2025

