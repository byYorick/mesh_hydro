<?php

namespace App\Services\Mqtt;

use App\Services\Mqtt\Contracts\MqttMessageHandlerInterface;
use App\Services\Mqtt\DTO\MqttMessageContext;
use App\Services\Mqtt\Enum\MqttMessageType;
use Illuminate\Contracts\Container\Container;
use InvalidArgumentException;

class MqttMessageRouter
{
    /**
     * @param array<string, class-string<MqttMessageHandlerInterface>> $handlerMap
     */
    public function __construct(
        private readonly Container $container,
        private readonly array $handlerMap
    ) {
    }

    public function dispatch(MqttMessageType $type, string $topic, string $payload): void
    {
        $handler = $this->resolveHandler($type);

        $context = new MqttMessageContext(
            $type,
            $topic,
            $payload,
            now()
        );

        $handler->handle($context);
    }

    private function resolveHandler(MqttMessageType $type): MqttMessageHandlerInterface
    {
        $key = $type->value;

        if (!array_key_exists($key, $this->handlerMap)) {
            throw new InvalidArgumentException("Handler for MQTT message type [{$key}] is not configured.");
        }

        return $this->container->make($this->handlerMap[$key]);
    }
}

