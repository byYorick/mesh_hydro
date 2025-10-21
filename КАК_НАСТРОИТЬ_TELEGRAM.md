# 📱 КАК НАСТРОИТЬ TELEGRAM УВЕДОМЛЕНИЯ

## Шаг 1: Создать Telegram бота

1. Открыть Telegram
2. Найти бота **@BotFather**
3. Отправить команду `/newbot`
4. Ввести название бота: `Hydro Mesh Bot`
5. Ввести username бота: `hydro_mesh_bot` (должен быть уникальным)
6. **Скопировать токен:** `123456789:ABCdef...`

---

## Шаг 2: Получить Chat ID

### Вариант А (Простой):
1. Найти бота **@userinfobot**
2. Отправить ему `/start`
3. Он ответит твоим **chat_id:** `987654321`

### Вариант Б (Через API):
1. Отправить сообщение своему боту: `/start`
2. Открыть в браузере:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
3. Найти в JSON: `"chat":{"id":987654321...}`

---

## Шаг 3: Настроить Backend

```bash
cd server/backend

# Открыть .env файл
nano .env  # или notepad .env
```

Добавить:
```bash
TELEGRAM_ENABLED=true
TELEGRAM_BOT_TOKEN=123456789:ABCdef...
TELEGRAM_CHAT_ID=987654321
TELEGRAM_NOTIFY_WARNINGS=true
TELEGRAM_NOTIFY_INFO=false
```

---

## Шаг 4: Протестировать

```bash
php artisan tinker
```

В tinker:
```php
app(\App\Services\TelegramService::class)->sendTestMessage()
```

**Должно прийти сообщение:**
```
🟢 [INFO] 19:30

🌿 Hydro Mesh System

Telegram уведомления настроены и работают!
```

---

## ✅ Готово!

Теперь система будет отправлять уведомления:
- 🔴 При критичных событиях (pH/EC вне диапазона)
- 🟡 При предупреждениях
- 🟢 При калибровке насосов (если включено)

---

## ⚙️ Дополнительные настройки

### Уровни уведомлений:

```bash
# В .env:

TELEGRAM_NOTIFY_WARNINGS=true   # Предупреждения
TELEGRAM_NOTIFY_INFO=false      # Информационные (калибровка и т.д.)
```

**По умолчанию:**
- ✅ Emergency - ВСЕГДА
- ✅ Critical - ВСЕГДА
- ⚠️ Warning - НАСТРАИВАЕТСЯ
- ℹ️ Info - НАСТРАИВАЕТСЯ

---

## 🔧 Troubleshooting

### Сообщения не приходят:

1. Проверить токен и chat_id
2. Проверить что TELEGRAM_ENABLED=true
3. Проверить логи:
   ```bash
   tail -f storage/logs/laravel.log | grep Telegram
   ```

### Ошибка "Chat not found":
- Убедись что отправил боту `/start`

### Ошибка "Unauthorized":
- Проверь правильность токена

---

**Готово!** 📱✅

