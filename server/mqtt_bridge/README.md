# MQTT Bridge

PHP демоны для работы с MQTT и Telegram

## Файлы

- `mqtt_listener.php` - Подписка на MQTT топики
- `telegram_bot.php` - Telegram бот

## Запуск

### Через Supervisor (рекомендуется)

```bash
sudo supervisorctl start mqtt-listener
sudo supervisorctl start telegram-bot
```

### Через Laravel Artisan

```bash
php artisan mqtt:listen
php artisan telegram:bot
```

### Вручную

```bash
php mqtt_listener.php
php telegram_bot.php
```

## MQTT Listener

Подписывается на топики:
- `hydro/telemetry` - Телеметрия от узлов
- `hydro/event` - События от узлов
- `hydro/heartbeat` - Heartbeat от узлов

Сохраняет данные в PostgreSQL и отправляет через WebSocket.

## Telegram Bot

Обрабатывает команды:
- `/status` - Общий статус
- `/ph` - pH всех зон
- `/ec` - EC всех зон
- `/climate` - Климат
- `/nodes` - Список узлов
- `/alerts` - Тревоги

Отправляет уведомления при критичных событиях.

