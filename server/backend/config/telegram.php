<?php

return [

    /*
    |--------------------------------------------------------------------------
    | Telegram Bot Configuration
    |--------------------------------------------------------------------------
    |
    | Настройки для Telegram бота
    | Получить токен: https://t.me/BotFather
    |
    */

    'enabled' => env('TELEGRAM_ENABLED', true),
    
    'bot_token' => env('TELEGRAM_BOT_TOKEN', ''),
    
    'chat_id' => env('TELEGRAM_CHAT_ID', ''),
    
    /*
    |--------------------------------------------------------------------------
    | Notification Settings
    |--------------------------------------------------------------------------
    |
    | Настройки уведомлений
    |
    */
    
    'notifications' => [
        // Отправлять уведомления для этих уровней событий
        'levels' => [
            'info' => false,
            'warning' => true,
            'critical' => true,
            'emergency' => true,
        ],
        
        // Тихие уведомления (без звука)
        'silent' => [
            'info' => true,
            'warning' => false,
            'critical' => false,
            'emergency' => false,
        ],
    ],

];

