# Mesh Hydro System

**Версия:** 2.0  
**Статус:** Production Ready  

IoT система управления гидропонной установкой с mesh-сетью ESP32 узлов.

## 📚 Документация

### 🤖 Для AI
- **[AI_DOCUMENTATION.md](AI_DOCUMENTATION.md)** - Полная документация для AI

### 📖 Основная документация
- **[START_HERE.md](START_HERE.md)** - Быстрый старт
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Архитектура системы
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - Структура проекта
- **[TECH_STACK.md](TECH_STACK.md)** - Технологии
- **[PROJECT_INDEX.md](PROJECT_INDEX.md)** - Индекс проекта

### 🛠️ Руководства
- **[SETUP_GUIDE.md](SETUP_GUIDE.md)** - Установка и настройка
- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - Быстрая справка

### 📊 Отчеты и логи
- **[PRODUCTION_CHANGES_LOG.md](PRODUCTION_CHANGES_LOG.md)** - История изменений
- **[CODE_AUDIT_REPORT.md](CODE_AUDIT_REPORT.md)** - Аудит кода
- **[MQTT_FIX_LOG.md](MQTT_FIX_LOG.md)** - Исправления MQTT
- **[GPIO_UPDATE_SUMMARY.md](GPIO_UPDATE_SUMMARY.md)** - Обновления GPIO

### 💡 Frontend
- **[server/frontend/DIALOG_SYSTEM.md](server/frontend/DIALOG_SYSTEM.md)** - Система диалогов
- **[server/frontend/package.json](server/frontend/package.json)** - Зависимости

## 🚀 Быстрый старт

### Docker (Рекомендуется)
```bash
cd server
docker-compose up -d
```

### ESP32 Node сборка
```bash
cd root_node
idf.py build flash
```

## 🌐 Основные возможности

- 🌿 Управление гидропоникой (pH, EC, температура, влажность)
- 🌐 Mesh сеть (ESP32 Mesh)
- 📊 Real-time мониторинг (WebSocket + MQTT)
- 🎛️ PID контроль (адаптивный)
- 📱 Web Dashboard (Vue 3 + Vuetify)
- 🔄 Real-time события (Laravel Echo + Reverb)

## 🏗️ Архитектура

```
ESP32 Node ──> Mesh ──> Root Node ──> MQTT ──> Laravel ──> PostgreSQL
                                                           └─> WebSocket ──> Vue.js
```

## 🛠️ Технологии

- **Backend:** Laravel 11, PostgreSQL, MQTT, WebSocket
- **Frontend:** Vue 3, Vuetify 3, Pinia, TypeScript, Vite
- **Hardware:** ESP32, ESP-IDF v5.5, ESP-MESH

## 📁 Структура проекта

```
mesh_hydro/
├── server/          # Backend + Frontend
├── root_node/       # Root Node firmware
├── node_ph_ec/      # pH/EC Node firmware
├── node_climate/    # Climate Node firmware
├── node_relay/      # Relay Node firmware
├── node_display/    # Display Node firmware
├── common/          # Shared code
├── doc/             # Documentation
└── tools/           # Build tools
```

## 🔗 Полезные ссылки

- **Frontend:** http://localhost:3000
- **Backend API:** http://localhost:8000/api
- **WebSocket:** ws://localhost:8080
- **MQTT:** localhost:1883

## 📝 Лицензия

MIT License

## 📞 Контакты

**Project:** Mesh Hydro System  
**Version:** 2.0  
**Framework:** Laravel 11 + Vue 3 + ESP-IDF v5.5
