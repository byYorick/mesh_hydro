# 🔧 Стек технологий Mesh Hydro V2

---

## 📊 ТЕКУЩИЙ СТЕК (РЕКОМЕНДОВАН)

```
┌─────────────────────────────────────────────────────┐
│                    ТЕПЛИЦА                          │
│                                                     │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐      │
│  │ ESP32-S3  │  │  ESP32    │  │ ESP32-S3  │      │
│  │   ROOT    │  │  Climate  │  │   pH/EC   │      │
│  │           │  │           │  │           │      │
│  │ C lang    │  │ C lang    │  │ C lang    │      │
│  │ ESP-IDF   │  │ ESP-IDF   │  │ ESP-IDF   │      │
│  │ 5.3       │  │ 5.3       │  │ 5.3       │      │
│  └─────┬─────┘  └─────┬─────┘  └─────┬─────┘      │
│        │              │              │             │
│        └──────────────┴──────────────┘             │
│                 WiFi Mesh                          │
│                     │                              │
└─────────────────────┼──────────────────────────────┘
                      │
                   WiFi + MQTT
                      │
┌─────────────────────▼──────────────────────────────┐
│              ПК / СЕРВЕР (DOCKER)                  │
│                                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │        Mosquitto MQTT Broker 2.0            │  │
│  │              Port: 1883                     │  │
│  └───────────────────┬─────────────────────────┘  │
│                      │                             │
│  ┌───────────────────▼─────────────────────────┐  │
│  │         Laravel 11 Backend                  │  │
│  │         • PHP 8.2                           │  │
│  │         • Eloquent ORM                      │  │
│  │         • REST API                          │  │
│  │         • MQTT Listener                     │  │
│  │         • PostgreSQL Database ──┐           │  │
│  │           (port: 5432)          │           │  │
│  └───────────────────┬─────────────┴───────────┘  │
│                      │                             │
│                   REST API                         │
│                      │                             │
│  ┌───────────────────▼─────────────────────────┐  │
│  │         Vue.js 3 Frontend                   │  │
│  │         • Vuetify 3 UI                      │  │
│  │         • Material Design                   │  │
│  │         • Real-time charts                  │  │
│  │         Port: 3000                          │  │
│  └─────────────────────────────────────────────┘  │
│                                                     │
└─────────────────────────────────────────────────────┘
              │
              │ HTTP
              ▼
     🌐 http://localhost:3000
         (Браузер)
```

---

## 🎯 КОМПОНЕНТЫ

### 🔌 ESP32 (Firmware)
```yaml
Язык: C
Framework: ESP-IDF 5.3
RTOS: FreeRTOS
Mesh: ESP-WIFI-MESH
Protocol: MQTT
```

### 📡 MQTT Broker
```yaml
Broker: Mosquitto 2.0
Port: 1883
QoS: 0/1
Retained: Yes
```

### 🔧 Backend
```yaml
Framework: Laravel 11
Language: PHP 8.2
ORM: Eloquent
API: RESTful
```

### 🗄️ Database
```yaml
Type: PostgreSQL 15
Database: hydro_system
Host: postgres (Docker)
Port: 5432
Backup: pg_dump
```

### 🎨 Frontend
```yaml
Framework: Vue.js 3
UI Library: Vuetify 3
Design: Material Design
Bundler: Vite
```

### 🐳 Deployment
```yaml
Container: Docker
Orchestration: Docker Compose
Containers: 4
RAM: ~200 MB
```

---

## ⭐ ОЦЕНКА

```
┌───────────────────────────────────────────┐
│  Производительность    ⭐⭐⭐⭐ (4/5)      │
│  Простота              ⭐⭐⭐⭐⭐ (5/5)    │
│  Экосистема            ⭐⭐⭐⭐⭐ (5/5)    │
│  Стоимость             ⭐⭐⭐⭐⭐ (5/5)    │
│  Deployment            ⭐⭐⭐⭐⭐ (5/5)    │
│  Масштабируемость      ⭐⭐⭐⭐ (4/5)      │
│  Документация          ⭐⭐⭐⭐⭐ (5/5)    │
├───────────────────────────────────────────┤
│  ИТОГО:               ⭐⭐⭐⭐⭐ (5/5)    │
└───────────────────────────────────────────┘
```

---

## ✅ ПРЕИМУЩЕСТВА

### 1. Простота разработки
```php
// Laravel - код пишется быстро
Route::get('/nodes', function() {
    return Node::with('telemetry')->get();
});
```

### 2. Простой деплой
```bash
docker-start.bat  # Всё работает!
```

### 3. Низкая стоимость
```
Raspberry Pi 4: $0/мес (после покупки)
VPS: $5-10/мес
```

### 4. Достаточная производительность
```
Ваша нагрузка: 12 записей/мин
PostgreSQL capacity: 10000+ записей/сек
Запас: 50000x ✅
```

---

## 🚀 ЗАПУСК

```bash
# ESP32
cd root_node && idf.py flash
cd node_climate && idf.py flash

# Server
cd server && docker-start.bat

# Открыть
http://localhost:3000
```

---

## 📚 ДОКУМЕНТАЦИЯ

| Компонент | Документация |
|-----------|--------------|
| **ESP-IDF** | https://docs.espressif.com/ |
| **Mosquitto** | https://mosquitto.org/documentation/ |
| **Laravel** | https://laravel.com/docs |
| **Vue.js** | https://vuejs.org/guide/ |
| **Vuetify** | https://vuetifyjs.com/ |

---

## ✅ ВЫВОД

**Текущий стек - ИДЕАЛЕН для проекта!** ⭐⭐⭐⭐⭐

**Не нужно ничего менять!** ✅

---

**Дата:** 2025-10-18  
**Версия:** 1.0

