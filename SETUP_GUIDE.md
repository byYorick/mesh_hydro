# 🔧 Руководство по установке Mesh Hydro System V2

## Требования

### Оборудование
- 3× ESP32-S3 (ROOT + Display + pH/EC #1)
- 1× ESP32 (Climate)
- 1× ESP32 (Relay)
- 1× ESP32-C3 (Water)
- Датчики, дисплеи (см. shopping list в плане)

### Программное обеспечение
- ESP-IDF v5.1+
- PHP 8.1+
- Composer
- Node.js 18+
- PostgreSQL 15+
- Mosquitto MQTT Broker

## Установка

### 1. Клонирование репозитория

```bash
cd D:\mesh_hydro
# Структура уже создана
```

### 2. Установка ESP-IDF

```bash
# Следуйте официальной документации ESP-IDF
# https://docs.espressif.com/projects/esp-idf/en/latest/get-started/
```

### 3. Настройка сервера

#### PostgreSQL
```bash
# Создать БД
createdb hydro_system
```

#### Mosquitto
```bash
# Установить Mosquitto
# Windows: https://mosquitto.org/download/
# Linux: sudo apt install mosquitto
```

#### Laravel Backend
```bash
cd server/backend
composer install
cp .env.example .env
php artisan key:generate
php artisan migrate
php artisan db:seed
```

#### Vue.js Frontend
```bash
cd server/frontend
npm install
npm run build
```

### 4. Прошивка узлов

#### ROOT NODE
```bash
cd root_node
idf.py set-target esp32s3
idf.py menuconfig  # Настроить WiFi SSID/Password
idf.py build
idf.py -p COM3 flash monitor
```

#### NODE Display
```bash
cd node_display
idf.py set-target esp32s3
idf.py build
idf.py -p COM4 flash monitor
```

#### NODE pH/EC
```bash
cd node_ph_ec
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

#### NODE Climate
```bash
cd node_climate
idf.py set-target esp32
idf.py build
idf.py -p COM6 flash monitor
```

#### NODE Relay
```bash
cd node_relay
idf.py set-target esp32
idf.py build
idf.py -p COM7 flash monitor
```

#### NODE Water
```bash
cd node_water
idf.py set-target esp32c3
idf.py build
idf.py -p COM8 flash monitor
```

### 5. Запуск сервера

#### MQTT Listener
```bash
php artisan mqtt:listen
```

#### Telegram Bot (опционально)
```bash
php artisan telegram:bot
```

#### Web Server
```bash
php artisan serve
# или настроить Nginx
```

#### Frontend Dev Server
```bash
cd server/frontend
npm run dev
```

## Проверка работы

1. **Mesh сеть:** Все узлы должны подключиться к ROOT
2. **MQTT:** Данные поступают на сервер
3. **Web UI:** Открыть http://localhost:8000 и войти
4. **Display:** TFT показывает данные всех узлов

## Troubleshooting

См. `doc/TROUBLESHOOTING.md`

---

**Готово!** Система запущена 🚀

