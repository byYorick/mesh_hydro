<?php

namespace App\Services\Mqtt\Contracts;

use App\Services\Mqtt\DTO\MqttMessageContext;

interface MqttMessageHandlerInterface
{
    public function handle(MqttMessageContext $context): void;
}

