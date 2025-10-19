# 🔄 КАК ОТПРАВИТЬ DISCOVERY СНОВА

**Проблема:** MAC адрес не отображается, потому что Discovery было отправлено до обновления кода backend.

---

## 🎯 РЕШЕНИЯ (на выбор)

### ✅ Вариант 1: Перезагрузить узел (самое быстрое)

**Для climate_001 (COM10):**
1. Отключи USB кабель
2. Подожди 3 секунды
3. Подключи USB кабель обратно
4. Узел перезагрузится и отправит Discovery с MAC адресом
5. Подожди 15 секунд
6. Обнови страницу Dashboard

**Ожидаемый лог:**
```
I (11420) climate_ctrl: 🔍 Discovery sent: climate_001 (RSSI=-45)
```

Backend получит Discovery с MAC адресом и обновит БД.

---

### ✅ Вариант 2: Прошить узел заново

```batch
tools\flash_climate.bat
```

При первом подключении узел автоматически отправит Discovery.

---

### ✅ Вариант 3: Добавить периодическую отправку Discovery (долго)

**Изменить код ESP32:**

В `node_climate/components/climate_controller/climate_controller.c`:

```c
// В main_task, после отправки telemetry:
if ((iteration % 60) == 0) {  // Каждую минуту
    send_discovery();
}
```

**Пересобрать и прошить:**
```batch
cd node_climate
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py build
idf.py -p COM10 flash
```

---

## 🧪 ПРОВЕРКА ПОСЛЕ ПЕРЕЗАГРУЗКИ

### 1. Проверь БД:

```bash
docker exec hydro_backend php artisan tinker --execute="echo json_encode(App\Models\Node::where('node_id', 'climate_001')->first()->only(['node_id', 'mac_address']), JSON_PRETTY_PRINT);"
```

**Должно быть:**
```json
{
    "node_id": "climate_001",
    "mac_address": "00:4B:12:37:D5:A4"  ← ДОЛЖЕН ПОЯВИТЬСЯ!
}
```

---

### 2. Проверь Dashboard:

Открой детали узла climate_001 → Метаданные узла

**Должно появиться:**
```
🔌 MAC адрес
   00:4B:12:37:D5:A4
```

---

## 📋 СТАТУС СЕЙЧАС

### В БД есть MAC:
- ✅ root_98a316f5fde8: `98:A3:16:F5:FD:E8`

### Нет MAC:
- ❌ climate_001: `null`
- ❌ ph_ec_3cfd01: `null`

---

## 🚀 САМОЕ БЫСТРОЕ РЕШЕНИЕ

**ПЕРЕЗАГРУЗИ УЗЛЫ:**

1. **climate_001 (COM10):** Отключи/подключи USB
2. **ph_ec узел (COM9):** Отключи/подключи USB
3. Подожди 15 секунд
4. Обнови Dashboard

**MAC адреса должны появиться!** 🔌✅

