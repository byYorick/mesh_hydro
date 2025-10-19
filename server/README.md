# 🌐 Mesh Hydro System - Server (Production Ready v2.0)

**Полная серверная система для мониторинга и управления гидропонической mesh-сетью на ESP32.**

[![Laravel](https://img.shields.io/badge/Laravel-10-red)](https://laravel.com)
[![Vue.js](https://img.shields.io/badge/Vue.js-3-green)](https://vuejs.org)
[![Vuetify](https://img.shields.io/badge/Vuetify-3-blue)](https://vuetifyjs.com)
[![MQTT](https://img.shields.io/badge/MQTT-Mosquitto-purple)](https://mosquitto.org)

---

## ⚠️ ВАЖНО: ЧТО В DOCKER

**Docker содержит ТОЛЬКО серверные компоненты:**
- ✅ MQTT Broker (Mosquitto) - прием данных от ESP32
- ✅ Backend API (Laravel) - обработка
- ✅ Database (SQLite) - файловая БД в volume
- ✅ Frontend (Vue.js) - веб-интерфейс

**ESP32 узлы - физическое железо!** (прошиваются отдельно)
- 🔌 ROOT Node (ESP32-S3) → `cd root_node && idf.py flash`
- 🔌 NODE Climate (ESP32) → `cd node_climate && idf.py flash`
- 🔌 NODE pH/EC (ESP32-S3) → `cd node_ph_ec && idf.py flash`

📖 **См. подробнее:** [SIMPLE_ARCHITECTURE.md](./SIMPLE_ARCHITECTURE.md)

---

## 🎯 Возможности

### 💻 Backend (Laravel 10)
- ✅ REST API для управления всей системой
- ✅ SQLite/PostgreSQL база данных
- ✅ MQTT Listener для приема данных от ROOT узла
- ✅ Real-time Broadcasting (WebSocket events)
- ✅ Telegram Bot для уведомлений
- ✅ SMS Gateway для критичных алертов
- ✅ Автоматическая очистка старых данных
- ✅ Полная система команд для узлов

### 🎨 Frontend (Vue.js 3 + Vuetify 3)
- ✅ Современный Material Design интерфейс
- ✅ Real-time обновления (каждые 30 сек)
- ✅ Интерактивные графики с zoom/pan
- ✅ Управление узлами (калибровка, насосы, реле)
- ✅ Редактор конфигурации (визуальный + JSON)
- ✅ Аналитика и статистика
- ✅ Экспорт данных (CSV, JSON, PNG)
- ✅ Адаптивный дизайн (mobile/tablet/desktop)
- ✅ Светлая/темная тема

### 📡 MQTT Integration
- ✅ Mosquitto MQTT Broker
- ✅ Топики: telemetry, events, heartbeat, commands
- ✅ QoS 0/1 поддержка
- ✅ Retained messages

---

## ⚡ Быстрый старт

### 🐳 Docker (РЕКОМЕНДУЕТСЯ!)

**Запуск сервера:**
```cmd
cd server
docker-start.bat
```

**Откройте:** http://localhost:3000

**⚠️ Dashboard будет пустой - это нормально!**
- Тестовые данные отключены
- Узлы появятся после прошивки ESP32
- См. `START_HERE.md` для инструкций

🎉 **Сервер готов! Теперь прошейте ESP32!**

**Требуется только:**
- [Docker Desktop](https://www.docker.com/products/docker-desktop/)

**См. подробнее:** `DOCKER_QUICK_START.md`

---

### 💻 Вариант 2: Нативная установка (Windows)

**Установите зависимости:**
- [PHP 8.2+](https://windows.php.net/download/)
- [Composer](https://getcomposer.org/download/)
- [Node.js 18+](https://nodejs.org/)
- [Mosquitto MQTT](https://mosquitto.org/download/)

**Запустите:**
```cmd
START_SYSTEM.bat
```

**Затем frontend:**
```cmd
cd frontend
npm install
npm run dev
```

**Откройте:** http://localhost:3000

🎉 **Готово! Система работает!**

---

## 📦 Структура проекта

```
server/
├── backend/                 # Laravel 10 API
│   ├── app/
│   │   ├── Models/          # Node, Telemetry, Event, Command
│   │   ├── Services/        # MqttService, TelegramService, SmsService
│   │   ├── Console/         # mqtt:listen, telegram:bot, etc
│   │   ├── Http/Controllers/# NodeController, TelemetryController, etc
│   │   └── Events/          # Broadcasting events
│   ├── database/
│   │   ├── migrations/      # Database schema
│   │   └── seeders/         # Test data (6 nodes, 576 telemetry)
│   ├── routes/api.php       # API endpoints
│   ├── .env                 # Configuration
│   └── *.bat                # Windows scripts
│
├── frontend/                # Vue.js 3 Dashboard
│   ├── src/
│   │   ├── components/      # NodeCard, AdvancedChart, EventLog, etc
│   │   ├── views/           # Dashboard, Nodes, Analytics, etc
│   │   ├── stores/          # Pinia stores (app, nodes, telemetry, events)
│   │   ├── services/        # API client
│   │   └── router/          # Vue Router
│   ├── package.json
│   └── vite.config.js
│
├── mqtt_bridge/             # MQTT демоны (для будущего)
├── nginx/                   # Nginx конфигурация
├── supervisor/              # Supervisor конфигурация (Linux)
│
├── START_SYSTEM.bat         # 🚀 Запуск всей системы
├── DEPLOYMENT_GUIDE.md      # 📖 Production deployment
└── README.md                # Этот файл
```

---

## 🎮 Управление узлами

### pH/EC Node:
- 🧪 Калибровка pH (4.0, 7.0, 10.0)
- ⚡ Калибровка EC (1.413 mS/cm)
- 💧 Насосы: pH Up, pH Down, EC Up, Water
- 🔄 Сброс ошибок

### Climate Node:
- 🔄 Обновить датчики (DHT22, MHZ19)
- ⚙️ Настройка пороговых значений

### Relay Node:
- 🪟 Открыть/Закрыть окна (индивидуально или все)
- 💨 Управление вентилятором
- 🔥 Управление обогревом

### Water Node:
- 📏 Мониторинг уровня
- 🌡️ Температура воды

### Все узлы:
- ℹ️ Получить статус
- 🔄 Перезагрузка
- ⚙️ Редактирование конфигурации

---

## 📊 API Endpoints

```
Dashboard:
  GET  /api/dashboard/summary
  GET  /api/status
  GET  /api/health

Nodes:
  GET    /api/nodes
  POST   /api/nodes
  GET    /api/nodes/{id}
  POST   /api/nodes/{id}/command
  PUT    /api/nodes/{id}/config
  DELETE /api/nodes/{id}

Telemetry:
  GET  /api/telemetry
  GET  /api/telemetry/latest
  GET  /api/telemetry/aggregate
  GET  /api/telemetry/export

Events:
  GET    /api/events
  POST   /api/events/{id}/resolve
  POST   /api/events/resolve-bulk
  GET    /api/events/statistics
  DELETE /api/events/{id}
```

---

## 🎨 Screenshots

### Dashboard
- Карточки узлов в реальном времени
- Статистика: online/offline, события
- Последняя телеметрия

### Node Detail
- Информация об узле
- Интерактивные графики (zoom/pan)
- Панель быстрых действий
- История событий

### Analytics
- Множественные графики для каждого параметра
- Корреляция между параметрами
- Статистика (среднее, мин, макс)
- Экспорт (CSV, JSON)

---

## 📝 Документация

### 🎯 Начните здесь:
| Документ | Описание |
|----------|----------|
| **`START_HERE.md`** | **⚡ НАЧНИТЕ ОТСЮДА!** |
| **`SIMPLE_ARCHITECTURE.md`** | **📐 Простая схема системы** |
| **`DOCKER_QUICK_START.md`** | **🚀 Быстрый старт с Docker** |
| **`REAL_DATA_ONLY.md`** | **📊 О реальных данных (без тестов)** |

### 🔧 Если что-то не работает:
| Документ | Описание |
|----------|----------|
| **`DOCKER_TROUBLESHOOTING.md`** | **🛠️ Все проблемы и решения** |

### 🐳 Docker:
| Документ | Описание |
|----------|----------|
| `DOCKER_README.md` | Полная документация Docker |
| `ARCHITECTURE_DOCKER.md` | Архитектура и схемы |
| `DOCKER_DEPLOY.md` | Деплой на сервер |

### 💻 Native:
| Документ | Описание |
|----------|----------|
| `DEPLOYMENT_GUIDE.md` | Production развертывание |
| `backend/README.md` | Backend API документация |
| `backend/WINDOWS_SETUP.md` | Установка на Windows |
| `frontend/README.md` | Frontend документация |

---

## 🔧 Обслуживание

### Backup базы данных:
```cmd
cd backend
backup.bat
```

### Очистка старой телеметрии:
```cmd
php artisan telemetry:cleanup --days=90
```

### Проверка статуса узлов:
```cmd
php artisan nodes:check-status --notify
```

### Просмотр логов:
```cmd
type backend\storage\logs\laravel.log
type C:\HydroLogs\mqtt.log
```

---

## 📞 Поддержка

**Документация:**
- Backend: `backend/README.md`
- Frontend: `frontend/README.md`
- Deployment: `DEPLOYMENT_GUIDE.md`

**Проверка системы:**
```cmd
curl http://127.0.0.1:8000/api/status
```

---

## 🏆 Полнота реализации

✅ **Backend:** 100% (Models, Services, Controllers, Commands, Events, Broadcasting)  
✅ **Frontend:** 100% (Components, Views, Stores, Router, API integration)  
✅ **Database:** 100% (Migrations, Seeders, Relations)  
✅ **MQTT:** 100% (Listener, Publisher, Topics)  
✅ **Документация:** 100% (READMEs, Guides, Scripts)  
✅ **Windows Support:** 100% (BAT скрипты, Services, Troubleshooting)

---

**Версия:** 2.0.0 Production  
**Статус:** ✅ Ready for Production  
**Платформа:** Windows 10/11, Windows Server  
**Лицензия:** MIT
