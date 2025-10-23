FROM node:20-alpine

WORKDIR /app

# Копировать package файлы
COPY package*.json ./

# Установить зависимости
RUN npm install --production

# Копировать исходники
COPY server.js ./

# Экспортировать порт
EXPOSE 6001

# Запуск сервера
CMD ["node", "server.js"]

