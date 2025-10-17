# 🤖 AI INSTRUCTIONS - NODE Water

## 🎯 Назначение узла

**NODE Water (ESP32-C3)** - Управление водой (насосы + клапаны)

### Основные функции:
- 💧 Насос подачи воды (реле)
- 💧 Насос слива воды (реле)
- 🚰 3 соленоидных клапана (зоны полива)
- 📏 Датчик уровня воды (опционально)
- 📥 Команды от ROOT или по расписанию

### Критичные правила:
- ✅ Safety: макс время насоса (60 сек)
- ✅ Проверка уровня воды перед подачей
- ✅ Логирование всех операций

---

## 🔌 Распиновка

| GPIO | Назначение |
|------|------------|
| 1 | Насос подачи |
| 2 | Насос слива |
| 3 | Клапан зона 1 |
| 4 | Клапан зона 2 |
| 5 | Клапан зона 3 |
| 6 | Датчик уровня (опц.) |

---

## 💻 Пример кода

```c
void water_zone_activate(int zone_num, uint32_t duration_ms) {
    // 1. Проверка уровня воды
    if (!check_water_level()) {
        ESP_LOGE(TAG, "Low water level!");
        return;
    }
    
    // 2. Открыть клапан зоны
    gpio_set_level(valve_gpio[zone_num], 1);
    
    // 3. Включить насос подачи
    gpio_set_level(GPIO_PUMP_IN, 1);
    
    // 4. Таймер
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    // 5. Выключить
    gpio_set_level(GPIO_PUMP_IN, 0);
    gpio_set_level(valve_gpio[zone_num], 0);
    
    // 6. Логирование
    log_water_usage(zone_num, duration_ms);
}
```

---

## 📚 Документация

См. `node_water/README.md` и `doc/MESH_HYDRO_V2_FINAL_PLAN.md`

**Фаза 8 реализации (2-3 дня)**

