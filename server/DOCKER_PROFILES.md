# Docker Compose профили

## Профили запуска

### Базовый запуск (production)
```bash
docker-compose up
```
Запускает только основные сервисы:
- postgres
- mosquitto  
- backend
- mqtt_listener
- frontend

### Разработка (dev)
```bash
docker-compose --profile dev up
```
Дополнительно запускает:
- websocket (Socket.IO на порту 6002)

### Полный production
```bash
docker-compose --profile production up
```
Дополнительно запускает:
- nginx (reverse proxy)

## Команды по профилям

### Только основные сервисы
```bash
docker-compose up postgres mosquitto backend mqtt_listener frontend
```

### С Socket.IO для разработки
```bash
docker-compose --profile dev up
```

### С Nginx для production
```bash
docker-compose --profile production up
```

### Остановка всех сервисов
```bash
docker-compose down
```

## Конфигурация WebSocket

### Reverb (основной)
- Порт: 6001
- Протокол: Pusher
- Используется в production

### Socket.IO (dev)
- Порт: 6002  
- Протокол: Socket.IO
- Используется только в dev режиме

## Переменные окружения

### WebSocket (Reverb)
```env
PUSHER_APP_ID=hydro-app
PUSHER_APP_KEY=hydro-app-key
PUSHER_APP_SECRET=hydro-app-secret
PUSHER_HOST=localhost
PUSHER_PORT=6001
PUSHER_SCHEME=http
```

### WebSocket (Socket.IO)
```env
PORT=6001
APP_ID=hydro-app
APP_KEY=hydro-app-key
APP_SECRET=hydro-app-secret
```

## Рекомендации

1. **Разработка**: используйте `--profile dev` для доступа к Socket.IO
2. **Production**: используйте `--profile production` с Nginx
3. **Тестирование**: используйте базовый запуск без дополнительных сервисов
