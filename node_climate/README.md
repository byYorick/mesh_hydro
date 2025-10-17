# NODE Climate

**ESP32** - Датчики климата

## Функции

- SHT3x (температура, влажность)
- CCS811 (CO2)
- Trema Lux (освещенность)
- Только датчики (НЕТ исполнителей)

## Распиновка

| GPIO | Назначение |
|------|------------|
| 17 | I2C SCL |
| 18 | I2C SDA |

## Сборка

```bash
cd node_climate
idf.py set-target esp32
idf.py build
idf.py -p COM6 flash monitor
```

