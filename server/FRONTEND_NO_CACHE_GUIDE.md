# Отключение кеширования фронтенда

## Что было изменено

### 1. Nginx конфигурация (`server/frontend/nginx.conf`)
- Отключено кеширование для всех статических файлов (CSS, JS, изображения)
- Добавлены заголовки `Cache-Control: no-cache, no-store, must-revalidate`
- Отключено кеширование HTML файлов

### 2. Vite конфигурация (`server/frontend/vite.config.js`)
- Настроена генерация уникальных хешей для файлов
- Добавлены настройки для предотвращения кеширования в development режиме

### 3. Docker Compose (`server/docker-compose.yml`)
- Добавлен флаг `no_cache: true` для принудительной пересборки
- Настроена пересборка без использования кеша Docker

## Как использовать

### Автоматическая пересборка
```bash
# Запустите скрипт для полной пересборки без кеша
./rebuild_frontend_no_cache.bat
```

### Ручная пересборка
```bash
# Остановка сервисов
docker-compose down

# Удаление старых образов
docker rmi hydro_frontend

# Пересборка без кеша
docker-compose build --no-cache frontend

# Запуск сервисов
docker-compose up -d
```

## Проверка отключения кеширования

1. Откройте браузер и перейдите на `http://localhost:3000`
2. Откройте Developer Tools (F12)
3. Перейдите на вкладку Network
4. Обновите страницу (Ctrl+F5)
5. Проверьте заголовки ответа - должны быть:
   - `Cache-Control: no-cache, no-store, must-revalidate`
   - `Pragma: no-cache`
   - `Expires: 0`

## Восстановление кеширования

Если нужно вернуть кеширование обратно, замените в `nginx.conf`:

```nginx
# Вместо отключения кеширования
location ~* \.(jpg|jpeg|png|gif|ico|css|js|svg|woff|woff2|ttf|eot)$ {
    expires 1y;
    add_header Cache-Control "public, immutable";
}
```

И удалите заголовки из location / блоков.
