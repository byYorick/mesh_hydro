<?php

return [

    /*
    |--------------------------------------------------------------------------
    | MQTT Broker Configuration
    |--------------------------------------------------------------------------
    |
    | Настройки подключения к Mosquitto MQTT брокеру
    |
    */

    'host' => env('MQTT_HOST', '127.0.0.1'),
    
    'port' => env('MQTT_PORT', 1883),
    
    'client_id' => env('MQTT_CLIENT_ID', 'hydro-server'),
    
    'username' => env('MQTT_USERNAME', ''),
    
    'password' => env('MQTT_PASSWORD', ''),
    
    'keep_alive' => env('MQTT_KEEP_ALIVE', 60),
    
    /*
    |--------------------------------------------------------------------------
    | MQTT Topics
    |--------------------------------------------------------------------------
    |
    | Топики для публикации и подписки
    |
    */
    
    'topics' => [
        'telemetry' => 'hydro/telemetry/#',
        'event' => 'hydro/event/#',
        'heartbeat' => 'hydro/heartbeat/#',
        'command' => 'hydro/command/',
        'config' => 'hydro/config/',
        'response' => 'hydro/response/#',
        'server_status' => 'hydro/server/status',
    ],
    
    /*
    |--------------------------------------------------------------------------
    | QoS Levels
    |--------------------------------------------------------------------------
    |
    | Уровни Quality of Service для разных типов сообщений
    | 0 = At most once (может потеряться)
    | 1 = At least once (гарантия доставки, могут быть дубли)
    | 2 = Exactly once (гарантия доставки без дублей, самый медленный)
    |
    */
    
    'qos' => [
        'telemetry' => 0,  // Телеметрия - можно потерять отдельные точки
        'event' => 1,      // События - важно доставить
        'command' => 1,    // Команды - важно доставить
        'config' => 1,     // Конфигурация - важно доставить
    ],

];

