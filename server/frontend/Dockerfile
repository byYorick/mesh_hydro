# ========================================
# Vue.js Frontend Dockerfile (Multi-stage)
# ========================================

# Stage 1: Build
FROM node:18-alpine AS builder

WORKDIR /app

# Копирование package files
COPY package*.json ./

# Установка зависимостей (используем npm install вместо ci)
RUN npm install --legacy-peer-deps

# Копирование исходников
COPY . .

# Аргументы сборки
ARG VITE_API_URL=http://localhost:8000
ENV VITE_API_URL=$VITE_API_URL

# Сборка production
RUN npm run build

# Stage 2: Production
FROM nginx:alpine

# Копирование собранных файлов
COPY --from=builder /app/dist /usr/share/nginx/html

# Копирование nginx конфигурации
COPY nginx.conf /etc/nginx/conf.d/default.conf

# Expose порт
EXPOSE 80

# Запуск nginx
CMD ["nginx", "-g", "daemon off;"]

