# 🤖 AI INSTRUCTIONS - NODE Climate

## 🎯 Назначение узла

**NODE Climate (ESP32)** - Датчики климата (БЕЗ исполнителей!)

### Основные функции:
- 🌡️ SHT3x - температура и влажность (I2C)
- 🌫️ CCS811 - CO2 (I2C)
- ☀️ Trema Lux - освещенность (I2C)
- 📤 Отправка данных на ROOT каждые 10 сек
- ❌ **БЕЗ исполнителей** - только датчики!

### Критичные правила:
- ✅ **ТОЛЬКО датчики** - никаких реле/насосов
- ✅ Логика управления на **ROOT** (не здесь!)
- ✅ Если Climate offline → ROOT использует fallback логику

---

## 🔌 Распиновка

| GPIO | Назначение |
|------|------------|
| 17 | I2C SCL |
| 18 | I2C SDA |
| - | SHT3x (0x44) |
| - | CCS811 (0x5A) |
| - | Trema Lux (0x12) |

---

## 💻 Пример кода

```c
void climate_main_task(void *arg) {
    float temp, humidity;
    uint16_t co2, lux;
    
    while (1) {
        // Чтение датчиков
        sht3x_read(&temp, &humidity);
        ccs811_read(&co2);
        trema_lux_read(&lux);
        
        // Отправка телеметрии
        send_climate_telemetry(temp, humidity, co2, lux);
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Каждые 10 сек
    }
}
```

**Важно:** ROOT принимает решения о вентиляции, а не Climate node!

---

## 📚 Документация

См. `node_climate/README.md` и `doc/MESH_HYDRO_V2_FINAL_PLAN.md`

**Фаза 7 реализации (3-4 дня)**

