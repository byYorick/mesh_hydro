# Laravel Backend

Laravel 10 API для Mesh Hydro System

## Установка

```bash
composer install
cp .env.example .env
php artisan key:generate
php artisan migrate
php artisan db:seed
```

## API Endpoints

### Аутентификация
- `POST /api/login` - Вход
- `POST /api/logout` - Выход
- `GET /api/user` - Текущий пользователь

### Узлы
- `GET /api/nodes` - Список узлов
- `GET /api/nodes/{id}` - Детали узла
- `POST /api/nodes/{id}/command` - Отправить команду

### Телеметрия
- `GET /api/telemetry` - История данных
- `GET /api/telemetry/{node_id}` - Данные узла

### События
- `GET /api/events` - Список событий
- `POST /api/events/{id}/acknowledge` - Подтвердить событие

### OTA
- `GET /api/ota/updates` - Список обновлений
- `POST /api/ota/upload` - Загрузить прошивку
- `POST /api/ota/deploy` - Применить обновление

## Команды Artisan

```bash
php artisan mqtt:listen     # MQTT слушатель
php artisan telegram:bot    # Telegram бот
php artisan data:aggregate  # Агрегация данных
```

## Модели

- `Node` - Узлы mesh-сети
- `Telemetry` - Телеметрия
- `Event` - События
- `Command` - Команды
- `OtaUpdate` - OTA обновления
- `User` - Пользователи (Laravel Sanctum)

