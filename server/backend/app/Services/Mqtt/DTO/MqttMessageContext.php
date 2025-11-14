<?php

namespace App\Services\Mqtt\DTO;

use App\Services\Mqtt\Enum\MqttMessageType;
use Carbon\CarbonInterface;

class MqttMessageContext
{
    public function __construct(
        public readonly MqttMessageType $type,
        public readonly string $topic,
        public readonly string $payload,
        public readonly CarbonInterface $receivedAt
    ) {
    }

    /**
     * Decode payload into array, fallback to empty array.
     */
    public function json(): array
    {
        $decoded = json_decode($this->payload, true);

        return is_array($decoded) ? $decoded : [];
    }
}

