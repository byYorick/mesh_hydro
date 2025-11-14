<?php

namespace App\Services\Mqtt\Enum;

enum MqttMessageType: string
{
    case Telemetry = 'telemetry';
    case Heartbeat = 'heartbeat';
    case Event = 'event';
    case Discovery = 'discovery';
    case ConfigResponse = 'config_response';
    case Error = 'error';
    case SetupDiscovery = 'setup_discovery';
    case SetupHeartbeat = 'setup_heartbeat';
    case CommandResponse = 'command_response';
}

