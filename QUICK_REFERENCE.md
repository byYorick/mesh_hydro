# 🚀 Mesh Hydro System — Быстрая справка

## 📍 Доступ к системе

| Сервис | URL | Описание |
|--------|-----|----------|
| **Веб-интерфейс** | http://localhost:3000 | Главный дашборд |
| **Документация** | http://localhost:3000/documentation | Полная документация в UI |
| **Настройки PID** | http://localhost:3000/settings | PID параметры + пресеты |
| **Backend API** | http://localhost:8000 | REST API |
| **API Health** | http://localhost:8000/api/health | Проверка работы |
| **MQTT Broker** | localhost:1883 | Для ESP32 узлов |
| **PostgreSQL** | localhost:5432 | База данных |

---

## ⚡ Быстрые команды

### Запуск/остановка
```bash
# Запустить всё
cd D:\mesh\mesh_hydro\server
docker-compose up -d

# Остановить всё
docker-compose down

# Перезапустить
docker-compose restart

# Статус
docker-compose ps
```

### Логи
```bash
# Все логи
docker-compose logs -f

# Конкретный сервис
docker logs hydro_backend -f
docker logs hydro_mqtt_listener -f
docker logs hydro_postgres
```

### База данных
```bash
# Миграции + сидеры
docker exec hydro_backend php artisan migrate:fresh --seed --force

# Бэкап
docker exec hydro_postgres pg_dump -U hydro hydro_system > backup.sql

# Статус
curl http://localhost:3000/api/status
```

---

## 🎛️ PID Быстрые пресеты

В веб-интерфейсе: **Настройки → PID Контроллеры**

| Пресет | Применение | pH | EC |
|--------|------------|----|----|
| **Салат NFT (100L)** | Листовые в NFT | 5.8 | 1.6 |
| **Томаты капельный (300L)** | Плодовые крупные | 6.0 | 2.7 |
| **DWC малая (50L)** | Малые системы | 5.8 | 1.4 |
| **Консервативный** | Безопасный старт | 5.8 | 1.6 |

**Один клик → все параметры настроены!**

---

## 📊 Оптимальные параметры культур

| Культура | pH | EC вегетация | EC плодоношение | Температура |
|----------|----|--------------|-----------------| ------------|
| Салат | 5.5–6.0 | 1.2–1.8 | - | 18–22°C |
| Томаты | 5.8–6.2 | 2.0–2.5 | 2.5–3.5 | 22–26°C |
| Огурцы | 5.8–6.0 | 1.7–2.5 | 1.7–2.5 | 24–28°C |
| Базилик | 5.5–6.5 | 1.0–1.6 | - | 20–25°C |
| Клубника | 5.5–6.0 | 1.2–2.0 | 1.2–2.0 | 20–24°C |

---

## 🔧 Прошивка ESP32 узлов

```bash
# ROOT узел
cd D:\mesh\mesh_hydro\root_node
idf.py build flash monitor

# Climate узел
cd D:\mesh\mesh_hydro\node_climate
idf.py build flash monitor

# pH/EC узел
cd D:\mesh\mesh_hydro\node_ph_ec
idf.py build flash monitor
```

**Автообнаружение:** узлы появятся в интерфейсе через 5–10 секунд!

---

## 🆘 Решение проблем

### Белая страница фронтенда
```bash
# 1. Проверить логи
docker logs hydro_frontend

# 2. Проверить API
curl http://localhost:3000/api/health

# 3. Очистить кэш браузера (Ctrl+Shift+R)
```

### MQTT не подключается
```bash
# 1. Проверить брокер
docker logs hydro_mosquitto

# 2. Проверить listener
docker logs hydro_mqtt_listener | tail -20

# 3. Перезапустить
docker-compose restart mqtt_listener
```

### База данных не работает
```bash
# 1. Очистить кэш конфига
docker exec hydro_backend php artisan config:clear

# 2. Проверить миграции
docker exec hydro_backend php artisan migrate:status

# 3. Пересоздать (УДАЛИТ ДАННЫЕ!)
docker exec hydro_backend php artisan migrate:fresh --seed --force
```

---

## 📚 Где что искать

### Хочу узнать про...

| Тема | Где смотреть |
|------|--------------|
| **PID настройка** | http://localhost:3000/documentation → PID Контроллеры |
| **Архитектура проекта** | http://localhost:3000/documentation → Архитектура |
| **Системы выращивания** | http://localhost:3000/documentation → Гидропоника |
| **Калибровка датчиков** | http://localhost:3000/documentation → Калибровка |
| **Мониторинг** | server/MONITORING_OPERATIONS_GUIDE.md |
| **Рецепты удобрений** | server/HYDROPONIC_GUIDE.md |

---

## 💡 Полезные ссылки

- **HydroBuddy:** scienceinhydroponics.com — калькулятор растворов
- **PID симулятор:** pidtuner.com
- **DLI калькулятор:** waveformlighting.com/horticulture/daily-light-integral-calculator
- **Форум:** forum.growclub.one (русский)
- **Reddit:** r/Hydroponics (350k+ участников)

---

## 🎯 Чек-лист первого запуска

- [ ] Docker запущен: `docker-compose ps`
- [ ] База данных создана: `docker exec hydro_backend php artisan migrate:status`
- [ ] Сидеры выполнены: есть тестовые узлы на дашборде
- [ ] Фронтенд открывается: http://localhost:3000
- [ ] API отвечает: http://localhost:3000/api/health
- [ ] MQTT подключён: статус "mqtt: connected"
- [ ] Документация доступна: http://localhost:3000/documentation
- [ ] Настройки PID: применён пресет для вашей системы

---

**Версия:** 2.0.1  
**Дата:** 20.10.2025  
**Всё работает!** ✅

