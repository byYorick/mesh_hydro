# 🎯 NODE Climate - ГОТОВ К ПРОШИВКЕ!

## ✅ Статус: ПОЛНОСТЬЮ ГОТОВ

**Дата сборки**: 18.10.2025, обновлено после всех исправлений  
**Версия ESP-IDF**: v5.5  
**Target**: ESP32  
**Размер**: 1.04 МБ / 1.5 МБ (32% свободно)  

---

## 🚀 БЫСТРЫЙ СТАРТ

### 1. Прошивка устройства:
```bash
.\flash.bat COM4
```
(замените COM4 на ваш порт)

### 2. Ожидаемый вывод:
```
I CLIMATE: === NODE Climate Starting ===
I CLIMATE: [Step 1/7] Initializing NVS...
I CLIMATE: [Step 2/7] Loading configuration...
I CLIMATE: [Step 3/7] Initializing I2C...
I CLIMATE: [Step 4/7] Initializing Sensors...
W CLIMATE:    WARNING: SHT3x init failed (датчик не подключен)
W CLIMATE:    WARNING: CCS811 init failed (датчик не подключен)  
W CLIMATE:    WARNING: Lux sensor init failed (датчик не подключен)
W CLIMATE: WARNING: All sensors failed. Running in degraded mode.
I CLIMATE: [Step 5/7] Initializing Mesh (NODE mode)...
I mesh_manager: NODE mode (router: 'Yorick', max_conn=6)
I CLIMATE: Mesh started
I CLIMATE: [Step 6/7] Initializing Climate Controller...
I CLIMATE: [Step 7/7] Starting Climate Controller...
I CLIMATE: === NODE Climate READY ===
```

### 3. Подключение датчиков (опционально):
- Подключите SHT3x, CCS811, Lux к I2C
- Перезагрузите ESP32
- WARNING исчезнут, появится "OK"

---

## 📡 Конфигурация Mesh

```
Mesh ID:       HYDRO1
Mesh Password: hydro_mesh_2025
WiFi Channel:  1
Router:        Yorick / pro100parol
Max Nodes:     6 дочерних узлов
```

⚠️ **ROOT узел должен иметь ИДЕНТИЧНЫЕ настройки!**

---

## 🔍 Проверка работы

### После прошивки проверьте:

1. ✅ **Инициализация завершена** - "NODE Climate READY"
2. ✅ **Mesh запущен** - "Mesh started"
3. ⚠️ **Датчики** - могут быть WARNING (это нормально без подключения)
4. 🔄 **Ожидание ROOT** - узел будет искать ROOT для подключения

### Когда ROOT появится:
```
I mesh: <MESH_EVENT_PARENT_CONNECTED>
I mesh: layer:2, rtableSize:1, <1001>
I mesh_manager: Mesh connected (layer: 2)
```

### Отправка телеметрии (каждые 10 сек):
```
W climate_ctrl: SHT3x read failed - using default values
W climate_ctrl: CCS811 read failed - using default value
W climate_ctrl: Lux sensor read failed - using default value
W climate_ctrl: All sensors failed - skipping telemetry
```
(Или полная телеметрия если датчики подключены)

---

## 🐛 Возможные проблемы

### "All sensors failed"
- ✅ **Это нормально** если датчики не подключены
- ⚠️ Подключите датчики к I2C (GPIO17=SCL, GPIO18=SDA)
- 🔄 Перезагрузите ESP32

### "Mesh offline"
- ⚠️ ROOT узел не запущен
- ⚠️ Неправильные mesh credentials
- 🔄 Запустите ROOT узел с теми же настройками

### "Router not found"
- ⚠️ WiFi "Yorick" недоступен
- ⚠️ Неправильный пароль
- 🔧 Исправьте в `main/app_main.c` и пересоберите

---

## 📦 Файлы для прошивки

Находятся в `build/`:
- `bootloader/bootloader.bin` (0x1000)
- `partition_table/partition-table.bin` (0x8000)
- `node_climate.bin` (0x10000)

---

## 🎯 Следующие действия

1. ✅ **Прошейте ESP32**: `.\flash.bat COM4`
2. ✅ **Проверьте логи** в мониторе
3. ✅ **Запустите ROOT узел** (должен быть уже запущен!)
4. ⏳ **Дождитесь подключения** к mesh
5. 📊 **Проверьте телеметрию** на ROOT/MQTT
6. 🔌 **Подключите датчики** (при необходимости)

---

**ПРОЕКТ ПОЛНОСТЬЮ ГОТОВ К РАБОТЕ!** 🚀

Вся mesh-инфраструктура настроена, датчики опциональны.  
Узел работает в degraded mode без датчиков и полном режиме с ними.

