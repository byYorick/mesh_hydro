# ⚡ Быстрый старт Backend (Windows)

## 🚀 За 3 шага до работающего backend!

### Шаг 1: Остановите текущий сервер

Если у вас запущен `php artisan serve` - нажмите **Ctrl+C**

---

### Шаг 2: Запустите скрипт исправления

Откройте **CMD** (не PowerShell!) в директории backend:

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
fix-and-start.bat
```

Скрипт автоматически:
- ✅ Очистит все кэши Laravel
- ✅ Оптимизирует конфигурацию
- ✅ Покажет список API routes
- ✅ Запустит сервер на http://127.0.0.1:8000

---

### Шаг 3: Проверьте что работает

Откройте в браузере:
- ✅ http://localhost:8000/api/health
- ✅ http://localhost:3000 (frontend)

Должны увидеть:
```json
{"status":"ok","timestamp":"..."}
```

---

## 🎯 Что уже настроено:

### ✅ API Endpoints (моковые данные):
- `GET /api/health` - Health check
- `GET /api/status` - Статус системы
- `GET /api/dashboard/summary` - Dashboard данные
- `GET /api/nodes` - Список узлов (3 тестовых)
- `GET /api/events` - События (2 тестовых)
- `POST /api/nodes/{id}/command` - Отправка команд

### ✅ Моковые узлы:
1. **ph_ec_001** - pH/EC сенсор (Online)
   - pH: 6.5
   - EC: 1.8
   - Temp: 24.3°C

2. **climate_001** - Климат сенсор (Online)
   - Temp: 25.5°C
   - Humidity: 65%
   - CO₂: 850 ppm

3. **water_001** - Датчик воды (Offline)

---

## 📝 Текущее состояние:

- ✅ **Laravel 10** настроен
- ✅ **API Routes** зарегистрированы
- ✅ **Контроллеры** работают с моковыми данными
- ⏳ **PostgreSQL** не подключена (работает без БД)
- ⏳ **MQTT** не настроен (будет позже)

---

## 🔄 Следующий этап (когда будет нужно):

### Подключение PostgreSQL:

1. Установите PostgreSQL 15
2. Создайте базу данных:
   ```sql
   CREATE DATABASE hydro_system;
   CREATE USER hydro WITH PASSWORD 'password';
   GRANT ALL PRIVILEGES ON DATABASE hydro_system TO hydro;
   ```
3. Обновите `.env`:
   ```env
   DB_DATABASE=hydro_system
   DB_USERNAME=hydro
   DB_PASSWORD=password
   ```
4. Выполните миграции:
   ```cmd
   php artisan migrate
   ```
5. Замените Simple контроллеры на полные в `routes/api.php`

---

## 🐛 Если что-то не работает:

### Проблема: Ошибка 500
```cmd
php artisan config:clear
php artisan route:clear
fix-and-start.bat
```

### Проблема: Routes не найдены (404)
```cmd
php artisan route:list
# Проверьте что маршруты показываются
```

### Проблема: CORS ошибки
Frontend уже настроен с proxy, должно работать.

---

## ✨ Готово к работе!

После запуска `fix-and-start.bat` ваш frontend на http://localhost:3000 заработает с моковыми данными! 🎉

**Сейчас можно протестировать UI без настройки БД!**

