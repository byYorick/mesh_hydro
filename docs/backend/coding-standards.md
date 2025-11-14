# Backend Coding Standards

## Code Style
- Соблюдаем PSR-12.
- Laravel Pint (`composer lint`) — единственный источник форматирования.
- Не отключаем правила Pint без обсуждения.

## Коллекции
- Используем `Illuminate\Support\Collection` вместо «сырых» массивов для бизнес-логики.
- Массовые операции (`map`, `filter`, `reduce`) выполняем над коллекциями.

## Документация
- Каждому публичному методу — PHPDoc с типами.
- ADR фиксируем в `docs/architecture`.

## Статический анализ и тесты
- `composer analyse` — PHPStan (level 0 на старте, повышаем после рефакторинга).
- `composer test` — обязательный шаг перед merge.

## Инструменты
- Устанавливаем зависимости: `composer install`.
- Локально: `composer lint`, `composer analyse`, `composer test`.

