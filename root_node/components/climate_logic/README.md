# Climate Logic

Резервная логика управления климатом (fallback).

## Назначение

Активируется автоматически когда Climate node offline > 30 сек.

Использует **простую таймерную логику**:
- Форточки: открытие каждый час на 5 минут
- Вентиляция: включение каждые 10 минут на 2 минуты

## Почему fallback?

Climate node может отключиться по разным причинам:
- Проблемы с питанием
- Сбой датчиков
- Проблемы mesh соединения

Без fallback логики климат останется неуправляемым → перегрев теплицы!

## API

```c
// Инициализация и запуск (автоматически)
climate_logic_init();

// Проверка статуса
if (climate_logic_is_fallback_active()) {
    // Fallback режим активен
    // Climate node offline
}
```

## Логика работы

### Когда Climate online:
```
Climate node → ROOT → Relay node
(умная логика на основе датчиков)
```

### Когда Climate offline:
```
ROOT climate_logic → Relay node
(простая таймерная логика)
```

## Параметры fallback

В `climate_logic.c`:
- `WINDOW_OPEN_INTERVAL_MS` - интервал проветривания
- `WINDOW_OPEN_DURATION_MS` - длительность
- `FAN_CHECK_INTERVAL_MS` - интервал вентиляции

Можно настроить под конкретную теплицу.

