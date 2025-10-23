# 🚀 Настройка Production окружения

## ✅ Выполненные изменения

### 1. База данных очищена от тестовых данных
- ❌ Удалены все тестовые узлы (7 узлов)
- ❌ Удалена вся тестовая телеметрия (864 записи)
- ❌ Удалены все тестовые события (10 событий)
- ✅ Сохранены PID пресеты (6 настроек)

### 2. Настроены Production параметры
```yaml
APP_ENV=production
APP_DEBUG=false
LOG_LEVEL=info
```

### 3. Отключены тестовые сиды
Файл `database/seeders/DatabaseSeeder.php` теперь загружает только:
- ✅ PID пресеты (для реальной работы)
- ❌ Тестовые узлы (отключено)
- ❌ Тестовая телеметрия (отключено)
- ❌ Тестовые события (отключено)

## 📡 Как работает система сейчас

### Автоматическое добавление узлов
Узлы будут **автоматически добавляться** при первом подключении через MQTT:

1. ESP32 узел подключается к MQTT брокеру
2. Отправляет сообщение в топик `hydro/discovery`
3. MQTT Listener автоматически создаёт запись в базе данных
4. Узел появляется в веб-интерфейсе

### MQTT топики (прослушиваются автоматически)
```
hydro/telemetry/#    - Телеметрия с узлов
hydro/event/#        - События и оповещения
hydro/heartbeat/#    - Проверка жизнеспособности узлов
hydro/response/#     - Ответы на команды
hydro/discovery      - Регистрация новых узлов
hydro/error/#        - Ошибки узлов
```

## 🔧 Управление данными

### Очистка базы данных
Если нужно снова очистить БД от данных:
```bash
docker exec hydro_backend php artisan db:clean-test-data --force
```

### Проверка состояния
```bash
docker ps --filter "name=hydro_"
docker logs hydro_mqtt_listener --tail 20
docker logs hydro_backend --tail 20
```

### Перезапуск с новыми настройками
```bash
cd server
docker-compose down
docker-compose up -d
```

## 📊 Текущий статус базы данных
```
Nodes:        0  (будут добавлены автоматически)
Telemetry:    0  (будет поступать от узлов)
Events:       0  (будут генерироваться системой)
Commands:     0  (будут создаваться через API)
PID Presets:  6  (готовые настройки)
```

## 🎯 Следующие шаги

1. **Подключите ESP32 узлы:**
   - Прошейте узлы используя `idf.py flash`
   - Настройте WiFi и MQTT параметры
   - Узлы автоматически зарегистрируются в системе

2. **Проверьте подключение:**
   - Откройте веб-интерфейс: http://localhost:3000
   - Узлы должны появиться автоматически после подключения
   - Телеметрия начнёт поступать сразу

3. **Настройте параметры:**
   - pH/EC целевые значения
   - PID параметры (можно использовать готовые пресеты)
   - Калибровки датчиков

## ⚠️ Важные замечания

- **База данных:** PostgreSQL (постоянное хранение)
- **MQTT брокер:** Mosquitto на порту 1883
- **Backend API:** http://localhost:8000
- **Frontend:** http://localhost:3000
- **WebSocket:** ws://localhost:8080

- **Логи:** `docker logs <container_name>`
- **Конфигурация:** `docker-compose.yml`
- **Миграции:** Применяются автоматически при старте

## 🔐 Production рекомендации

Для реального production окружения рекомендуется:

1. Изменить пароль PostgreSQL (`POSTGRES_PASSWORD`)
2. Настроить SSL для MQTT (TLS)
3. Добавить аутентификацию для MQTT
4. Настроить backup базы данных
5. Использовать Nginx с SSL для frontend/backend
6. Настроить мониторинг (Prometheus + Grafana)

## 📝 Команды для отладки

```bash
# Проверка статуса всех контейнеров
docker-compose ps

# Логи конкретного сервиса
docker logs hydro_mqtt_listener -f
docker logs hydro_backend -f

# Вход в контейнер
docker exec -it hydro_backend bash

# Очистка кэша Laravel
docker exec hydro_backend php artisan cache:clear
docker exec hydro_backend php artisan config:clear

# База данных
docker exec hydro_backend php artisan migrate:status
docker exec hydro_backend php artisan db:show
```

---
**Статус:** ✅ Система настроена и готова к работе с реальными устройствами
**Дата настройки:** 22 октября 2025

