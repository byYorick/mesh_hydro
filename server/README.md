# Server

Веб-сервер для Mesh Hydro System V2

## Компоненты

- **backend/** - Laravel 10 API + PostgreSQL
- **frontend/** - Vue.js 3 + Vuetify 3 Dashboard
- **mqtt_bridge/** - MQTT демоны (PHP)
- **nginx/** - Конфигурация Nginx
- **supervisor/** - Конфигурация Supervisor

## Требования

- PHP 8.1+
- Composer
- Node.js 18+
- npm или yarn
- PostgreSQL 15+
- Mosquitto MQTT Broker

## Установка

### Backend (Laravel)

```bash
cd server/backend
composer install
cp .env.example .env
# Настроить .env (DB, MQTT)
php artisan key:generate
php artisan migrate
php artisan db:seed
```

### Frontend (Vue.js)

```bash
cd server/frontend
npm install
npm run dev  # Development
npm run build  # Production
```

### MQTT Listener

```bash
php artisan mqtt:listen
```

### Telegram Bot

```bash
php artisan telegram:bot
```

## Структура

```
server/
├── backend/          # Laravel API
│   ├── app/
│   ├── database/
│   ├── routes/
│   └── config/
├── frontend/         # Vue.js Dashboard
│   ├── src/
│   ├── public/
│   └── package.json
├── mqtt_bridge/      # MQTT демоны
├── nginx/            # Nginx config
└── supervisor/       # Supervisor config
```

