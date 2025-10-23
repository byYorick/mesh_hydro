<?php

return [

    /*
    |--------------------------------------------------------------------------
    | Hydro System Configuration
    |--------------------------------------------------------------------------
    |
    | Общие настройки для Mesh Hydro System
    |
    */

    'system_name' => env('APP_NAME', 'Mesh Hydro System'),
    
    /*
    |--------------------------------------------------------------------------
    | Node Settings
    |--------------------------------------------------------------------------
    */
    
    // Таймаут офлайн узла (секунды)
    'node_offline_timeout' => env('NODE_OFFLINE_TIMEOUT', 20),
    
    // Поддерживаемые типы узлов
    'node_types' => [
        'root' => 'Root Node',
        'ph_ec' => 'pH/EC Sensor',
        'climate' => 'Climate Sensor',
        'relay' => 'Relay Controller',
        'water' => 'Water Level Sensor',
        'display' => 'Display Node',
    ],
    
    /*
    |--------------------------------------------------------------------------
    | Telemetry Settings
    |--------------------------------------------------------------------------
    */
    
    // Сколько дней хранить телеметрию
    'telemetry_retention_days' => env('TELEMETRY_RETENTION_DAYS', 365),
    
    /*
    |--------------------------------------------------------------------------
    | Event Settings
    |--------------------------------------------------------------------------
    */
    
    // Автоматическое резолвение событий через N часов
    'event_auto_resolve_hours' => env('EVENT_AUTO_RESOLVE_HOURS', 24),
    
    // Emergency события не резолвятся автоматически
    'emergency_no_auto_resolve' => true,

];

