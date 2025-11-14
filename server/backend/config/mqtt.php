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
    'log_channel' => env('MQTT_LOG_CHANNEL'),
    'status_topic' => env('MQTT_STATUS_TOPIC', 'hydro/server/status'),
    
    /*
    |--------------------------------------------------------------------------
    | MQTT Topics
    |--------------------------------------------------------------------------
    */

    'topics' => [
        'telemetry' => 'hydro/+/telemetry/#',
        'heartbeat' => 'hydro/+/heartbeat/#',
        'events' => 'hydro/+/event/#',
        'discovery' => 'hydro/+/discovery',
        'config_response' => 'hydro/+/config_response/#',
        'errors' => 'hydro/+/error/#',
        'response' => 'hydro/+/response/#',
        'setup_discovery' => 'hydro/setup/discovery',
        'setup_heartbeat' => 'hydro/setup/heartbeat/#',
    ],

    'subscriptions' => [
        'telemetry' => App\Services\Mqtt\Enum\MqttMessageType::Telemetry,
        'heartbeat' => App\Services\Mqtt\Enum\MqttMessageType::Heartbeat,
        'events' => App\Services\Mqtt\Enum\MqttMessageType::Event,
        'discovery' => App\Services\Mqtt\Enum\MqttMessageType::Discovery,
        'config_response' => App\Services\Mqtt\Enum\MqttMessageType::ConfigResponse,
        'errors' => App\Services\Mqtt\Enum\MqttMessageType::Error,
        'response' => App\Services\Mqtt\Enum\MqttMessageType::CommandResponse,
        'setup_discovery' => App\Services\Mqtt\Enum\MqttMessageType::SetupDiscovery,
        'setup_heartbeat' => App\Services\Mqtt\Enum\MqttMessageType::SetupHeartbeat,
    ],

    'handlers' => [
        App\Services\Mqtt\Enum\MqttMessageType::Telemetry->value => App\Services\Mqtt\Handlers\TelemetryHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::Heartbeat->value => App\Services\Mqtt\Handlers\HeartbeatHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::Event->value => App\Services\Mqtt\Handlers\EventHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::Discovery->value => App\Services\Mqtt\Handlers\DiscoveryHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::ConfigResponse->value => App\Services\Mqtt\Handlers\ConfigResponseHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::Error->value => App\Services\Mqtt\Handlers\ErrorHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::CommandResponse->value => App\Services\Mqtt\Handlers\CommandResponseHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::SetupDiscovery->value => App\Services\Mqtt\Handlers\SetupDiscoveryHandler::class,
        App\Services\Mqtt\Enum\MqttMessageType::SetupHeartbeat->value => App\Services\Mqtt\Handlers\SetupHeartbeatHandler::class,
    ],

    'retry' => [
        'max_attempts' => env('MQTT_RECONNECT_MAX_ATTEMPTS', 10),
        'delay_seconds' => env('MQTT_RECONNECT_DELAY', 5),
    ],

    /*
    |--------------------------------------------------------------------------
    | Connection Settings
    |--------------------------------------------------------------------------
    */

    'clean_session' => true,
    'keep_alive' => 60,
    'timeout' => 5,
];
