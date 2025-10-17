# NODE Display

**ESP32-S3 #2** - TFT дисплей + LVGL

## Функции

- TFT дисплей 240x320 (ILI9341)
- LVGL UI
- Ротационный энкодер (навигация)
- Показ данных всех узлов mesh-сети
- Запрос данных от ROOT

## Распиновка

| GPIO | Назначение |
|------|------------|
| 11 | LCD MOSI |
| 12 | LCD SCLK |
| 10 | LCD CS |
| 9 | LCD DC |
| 14 | LCD RST |
| 15 | LCD Backlight (PWM) |
| 4 | Encoder A |
| 5 | Encoder B |
| 6 | Encoder SW |

## Сборка

```bash
cd node_display
idf.py set-target esp32s3
idf.py build
idf.py -p COM4 flash monitor
```

