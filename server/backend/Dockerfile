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
    sqlite-dev \
    autoconf \
    g++ \
    make

# Установка PHP расширений
RUN docker-php-ext-install \
    pdo \
    pdo_sqlite \
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
COPY composer.json composer.lock ./

# Установка зависимостей PHP
RUN composer install --no-dev --optimize-autoloader --no-scripts

# Копирование остальных файлов
COPY . .

# Создание директории для SQLite
RUN mkdir -p /var/www/html/database && chmod 777 /var/www/html/database

# Права доступа
RUN chown -R www-data:www-data /var/www/html \
    && chmod -R 775 /var/www/html/storage \
    && chmod -R 775 /var/www/html/bootstrap/cache

# Генерация ключа приложения
RUN php artisan key:generate --force || true

# Оптимизация
RUN php artisan config:cache && \
    php artisan route:cache && \
    php artisan view:cache

# Expose порт
EXPOSE 8000

# Точка входа
CMD ["php", "artisan", "serve", "--host=0.0.0.0", "--port=8000"]

