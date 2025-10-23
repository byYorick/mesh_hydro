<?php

return [
    /*
    |--------------------------------------------------------------------------
    | MQTT Broker Configuration
    |--------------------------------------------------------------------------
    */

    'host' => env('MQTT_HOST', 'localhost'),
    'port' => env('MQTT_PORT', 1883),
    'client_id' => env('MQTT_CLIENT_ID', 'hydro_backend_' . uniqid()),
    'username' => env('MQTT_USERNAME', null),
    'password' => env('MQTT_PASSWORD', null),
    
    /*
    |--------------------------------------------------------------------------
    | MQTT Topics
    |--------------------------------------------------------------------------
    */

    'topics' => [
        'telemetry' => 'hydro/telemetry/+',
        'status' => 'hydro/status/+',
        'events' => 'hydro/events/+',
        'command' => 'hydro/command/#',
        'config' => 'hydro/config/#',
    ],

    /*
    |--------------------------------------------------------------------------
    | Connection Settings
    |--------------------------------------------------------------------------
    */

    'clean_session' => true,
    'keep_alive' => 60,
    'timeout' => 5,
    'reconnect_delay' => 5,
    'max_reconnect_attempts' => 10,
];
