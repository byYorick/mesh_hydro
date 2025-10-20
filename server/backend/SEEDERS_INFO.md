# ⚠️ Информация о Seeders

**Дата:** 2025-10-20  
**Статус:** ⚠️ Тестовые данные - НЕ используются в Production

---

## 🎯 Что такое Seeders?

Seeders - это классы Laravel для заполнения БД **тестовыми данными**.

---

## 📂 Текущие Seeders

### **1. NodeSeeder.php**
Создаёт **6 тестовых узлов:**
- root_001 (ROOT)
- ph_ec_001 (pH/EC)
- climate_001 (Climate)
- relay_001 (Relay)
- water_001 (Water) - offline
- display_001 (Display)

**С полной конфигурацией и metadata**

### **2. TelemetrySeeder.php**
Создаёт **576 записей телеметрии** (24 часа):
- 288 записей для pH/EC (каждые 5 мин)
- 288 записей для Climate (каждые 5 мин)

**С синтетическими данными (sin/cos + random)**

### **3. EventSeeder.php**
Создаёт **5 тестовых событий:**
- 2 resolved (warning, info)
- 3 active (critical, warning, emergency)

---

## ⚠️ ВАЖНО для Production

### **Seeders НЕ запускаются автоматически!**

Проверка в `docker-compose.yml`:
```yaml
backend:
  command: >
    sh -c "php artisan migrate --force &&
           php artisan serve --host=0.0.0.0 --port=8000"
```

✅ **Нет `php artisan db:seed`** - сидеры не запускаются!

---

## 🧪 Когда использовать Seeders?

### **✅ Для разработки/тестирования:**
```bash
# Локально или в dev окружении
docker compose exec backend php artisan db:seed

# Или конкретный seeder
docker compose exec backend php artisan db:seed --class=NodeSeeder
```

### **❌ НЕ использовать в Production:**
- Production использует **реальные данные от ESP32**
- Auto-discovery создаёт узлы автоматически
- Телеметрия приходит от реальных датчиков

---

## 🔍 Как проверить что Seeders не активны?

### **1. Проверить docker-compose.yml:**
```bash
cat docker-compose.yml | grep "db:seed"
# Должно быть пусто!
```

### **2. Проверить количество узлов:**
```bash
docker compose exec postgres psql -U hydro -d hydro_system -c "SELECT COUNT(*) FROM nodes;"
```

**Если 0 - отлично!** Seeders не запускались.

### **3. Проверить источник узлов:**
```sql
SELECT node_id, metadata->>'created_via' AS source FROM nodes;
```

**Production узлы имеют:**
- `mqtt` - созданы через MQTT
- `heartbeat` - обнаружены через heartbeat
- `discovery_topic` - через discovery топик

**Тестовые узлы имеют:**
- `seeder` - созданы сидером ❌

---

## 🎯 Рекомендации

### **1. Для Development:**
```bash
# Первый запуск с тестовыми данными
docker compose up -d
docker compose exec backend php artisan migrate:fresh --seed

# Dashboard покажет 6 узлов с данными
```

### **2. Для Production:**
```bash
# Запуск БЕЗ сидеров
docker compose up -d

# База пустая, ждём реальные ESP32
# Dashboard: "Total Nodes: 0"
# ✅ Правильно!
```

### **3. Сброс к чистой БД:**
```bash
# Удалить все данные (включая тестовые)
docker compose exec backend php artisan migrate:fresh

# Или полностью пересоздать
docker compose down -v
docker compose up -d
```

---

## 📊 Структура Seeders

```php
// database/seeders/DatabaseSeeder.php
class DatabaseSeeder extends Seeder
{
    public function run(): void
    {
        $this->call([
            NodeSeeder::class,      // 6 узлов
            TelemetrySeeder::class, // 576 записей
            EventSeeder::class,     // 5 событий
        ]);
    }
}
```

---

## ⚠️ Проблемы с Seeders в Production

### **Если случайно запустили seeders:**

#### **1. Дублирование узлов:**
```
Тестовый: ph_ec_001 (MAC: AA:BB:CC:DD:EE:01)
Реальный: ph_ec_001 (MAC: реальный MAC)
```

**Конфликт!** `node_id` должен быть уникальным.

#### **2. Неправильная статистика:**
```
Dashboard показывает:
- 6 тестовых узлов (всегда online)
- 576 записей синтетической телеметрии
```

**Решение:** Очистить БД

#### **3. Как очистить:**
```bash
# Вариант 1: Удалить только тестовые узлы
docker compose exec postgres psql -U hydro -d hydro_system -c \
  "DELETE FROM nodes WHERE metadata->>'created_via' = 'seeder';"

# Вариант 2: Полная очистка (сброс)
docker compose exec backend php artisan migrate:fresh

# Вариант 3: Пересоздать все
docker compose down -v && docker compose up -d
```

---

## ✅ Текущее состояние

### **docker-compose.yml:**
```yaml
command: >
  sh -c "php artisan migrate --force &&
         php artisan serve --host=0.0.0.0 --port=8000"
```

✅ **Seeders НЕ запускаются**  
✅ **Только миграции**  
✅ **Production готов**  

---

## 🎉 Вывод

**Seeders настроены правильно!**

- ✅ Не запускаются автоматически
- ✅ Доступны для тестирования
- ✅ Production использует реальные данные
- ✅ Auto-discovery работает

**Никаких изменений не требуется!** 🚀

---

## 📚 Дополнительные команды

### **Запуск конкретного seeder:**
```bash
docker compose exec backend php artisan db:seed --class=NodeSeeder
docker compose exec backend php artisan db:seed --class=TelemetrySeeder
docker compose exec backend php artisan db:seed --class=EventSeeder
```

### **Пересоздать БД с сидерами:**
```bash
docker compose exec backend php artisan migrate:fresh --seed
```

### **Только миграции (без сидеров):**
```bash
docker compose exec backend php artisan migrate:fresh
```

---

**Создано:** 2025-10-20  
**Автор:** AI Assistant  
**Версия:** 1.0  
**Статус:** ✅ Seeders под контролем

