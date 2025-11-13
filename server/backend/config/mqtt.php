<?php

$username = env('MQTT_USERNAME');
$password = env('MQTT_PASSWORD');

return [
    /*
    |--------------------------------------------------------------------------
    | MQTT Broker Configuration
    |--------------------------------------------------------------------------
    */

    'host' => env('MQTT_HOST', 'localhost'),
    'port' => env('MQTT_PORT', 1883),
    'client_id' => env('MQTT_CLIENT_ID', 'hydro_backend_' . uniqid()),
    'username' => is_string($username) && trim($username) === '' ? null : $username,
    'password' => is_string($password) && trim($password) === '' ? null : $password,
    
    /*
    |--------------------------------------------------------------------------
    | MQTT Topics
    |--------------------------------------------------------------------------
    */

    'topics' => [
        'telemetry' => 'hydro/+/telemetry/+',
        'status' => 'hydro/+/status/+',
        'events' => 'hydro/+/event/+',
        'command' => 'hydro/+/command/#',
        'config' => 'hydro/+/config/#',
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
