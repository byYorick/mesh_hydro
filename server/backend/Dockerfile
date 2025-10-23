# ========================================
# Laravel Backend Dockerfile
# ========================================

FROM php:8.2-fpm-alpine

# Установка зависимостей
RUN apk add --no-cache \
    git \
    curl \
    libpng-dev \
    oniguruma-dev \
    libxml2-dev \
    zip \
    unzip \
    postgresql-dev \
    autoconf \
    g++ \
    make

# Установка PHP расширений
RUN docker-php-ext-install \
    pdo \
    pdo_pgsql \
    pgsql \
    mbstring \
    exif \
    pcntl \
    bcmath \
    opcache

# Установка Composer
COPY --from=composer:latest /usr/bin/composer /usr/bin/composer

# Рабочая директория
WORKDIR /var/www/html

# Копирование composer файлов
COPY composer.json ./

# Установка зависимостей PHP (обновление lock файла)
RUN composer update --no-dev --optimize-autoloader --no-scripts --prefer-dist

# Копирование остальных файлов
COPY . .

# Права доступа
RUN chown -R www-data:www-data /var/www/html \
    && chmod -R 775 /var/www/html/storage \
    && chmod -R 775 /var/www/html/bootstrap/cache

# Генерация ключа приложения
RUN php artisan key:generate --force || true

# Оптимизация (без config:cache - он будет выполнен в runtime с переменными окружения)
RUN php artisan route:cache

# Expose порт
EXPOSE 8000

# Точка входа
CMD ["php", "artisan", "serve", "--host=0.0.0.0", "--port=8000"]

