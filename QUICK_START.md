# 🚀 Быстрый старт - Mesh Hydro System

## За 5 минут до первого запуска

### 1. Запустите систему

```bash
# В PowerShell
cd C:\esp\hydro\mesh\mesh_hydro\server
docker-compose -f docker-compose.dev.yml up -d

# Подождите 30 секунд
Start-Sleep -Seconds 30

# Выполните миграции
docker exec hydro_backend_dev php artisan migrate:fresh --seed

# Запустите MQTT Listener (в отдельном окне)
docker exec -it hydro_backend_dev php artisan mqtt:listen
```

### 2. Откройте Dashboard

Откройте браузер: **http://localhost:5173**

### 3. Создайте зону

1. Dashboard → **Zones** → **Добавить зону**
2. Заполните:
   - Название: "NFT Zone 1"
   - Root Node ID: "root_nft_001"
   - Mesh Network ID: "HYDRO_MESH_NFT_001"
   - Тип: NFT
3. Сохраните

### 4. Прошейте Root Node

```bash
cd root_node
idf.py menuconfig

# Настройте:
# - Root Node ID: root_nft_001
# - Mesh Network ID: HYDRO_MESH_NFT_001
# - WiFi SSID/Password
# - MQTT Broker: mqtt://192.168.1.100:1883

idf.py flash monitor
```

### 5. Прошейте Child Node (например, pH sensor)

```bash
cd node_ph
idf.py menuconfig

# Настройте:
# - Node ID: ph_ec_nft_001
# - Root Node ID: root_nft_001  ⭐ Важно!

idf.py flash monitor
```

### 6. Создайте цикл роста

1. Dashboard → **Growth Planner** → **Новый цикл**
2. Шаг 1: Выберите зону (NFT Zone 1)
3. Шаг 2: Выберите пресет (например, "Салат - Базовый")
4. Шаг 3: Укажите количество растений
5. Запустите!

### 7. Наблюдайте в реальном времени

- **Dashboard:** Общая информация
- **Zones:** Статус зон и узлов
- **Growth Planner:** Активные циклы
- **Analytics:** Графики параметров

## 📊 Что вы увидите

### Dashboard
```
┌─────────────────────────────────────┐
│ ⚡ System Status: ONLINE            │
│ 🏭 Zones: 1 active                  │
│ 📡 Nodes: 5 online / 5 total        │
│ 🌱 Active Cycles: 1                 │
└─────────────────────────────────────┘
```

### Zone Status
```
NFT Zone 1 [ACTIVE]
├── Root Node: root_nft_001 🟢
├── pH/EC Sensor: ph_ec_nft_001 🟢
├── Climate: climate_nft_001 🟢
├── Relay: relay_nft_001 🟢
└── Water: water_nft_001 🟢

Current Cycle: Салат - Базовый
├── Progress: 15%
├── Current Stage: Вегетация (Day 12/21)
├── Next Stage: Цветение (in 9 days)
└── Expected Harvest: 2024-12-15
```

### Growth Planner
```
📊 Салат - Базовый NFT
════════════════════════════════════
Прогресс: ████░░░░░░ 15% (Day 12/80)

Текущая стадия: Вегетация
├── pH: 6.0 ✅ (target: 6.0)
├── EC: 1.5 ✅ (target: 1.5)
├── Temp: 22.5°C ✅ (target: 22°C)
└── Humidity: 65% ✅ (target: 60-70%)

Следующая стадия: Цветение (in 9 days)
Ожидаемый урожай: 2024-12-15
```

## 💡 Полезные команды

### Backend

```bash
# Проверка статуса
docker exec hydro_backend_dev php artisan

# Проверка зон
docker exec hydro_backend_dev php artisan tinker
>>> Zone::with('rootNode')->get()

# Логи
docker exec hydro_backend_dev tail -f storage/logs/laravel.log

# Тесты
docker exec hydro_backend_dev php artisan test
```

### MQTT

```bash
# Подписаться на все топики
mosquitto_sub -h localhost -t "hydro/#" -v

# Подписаться на зону
mosquitto_sub -h localhost -t "hydro/nft1/#" -v

# Отправить команду
mosquitto_pub -h localhost -t "hydro/nft1/commands/ph_ec_001" \
  -m '{"command":"calibrate","params":{}}'
```

### ESP32

```bash
# Мониторинг логов
idf.py monitor

# Flash + Monitor
idf.py flash monitor

# Очистка NVS
idf.py erase-flash
```

## 🆘 Troubleshooting

### Узел не подключается

1. Проверьте Root Node ID в конфигурации
2. Проверьте Mesh Network ID (должен совпадать)
3. Проверьте WiFi подключение Root Node

### Телеметрия не доходит

1. Проверьте MQTT Listener запущен
2. Проверьте топики: `mosquitto_sub -t "#"`
3. Проверьте логи Backend

### Зона недоступна

1. Проверьте статус: `is_active = true`
2. Проверьте `current_cycle_id = null`
3. Проверьте Root Node онлайн

## 📚 Дополнительные ресурсы

- **Архитектура:** [ZONING_ARCHITECTURE_PLAN.md](./ZONING_ARCHITECTURE_PLAN.md)
- **Growth Planner:** [GROWTH_PLANNER_IMPLEMENTATION_PLAN.md](./GROWTH_PLANNER_IMPLEMENTATION_PLAN.md)
- **Настройка зон:** [ZONE_CONFIGURATION_GUIDE.md](./ZONE_CONFIGURATION_GUIDE.md)
- **Тестирование:** [README_TESTING.md](./README_TESTING.md)

---

**Версия:** 1.0  
**Статус:** ✅ PRODUCTION READY  
**Поддержка:** Полная документация доступна

