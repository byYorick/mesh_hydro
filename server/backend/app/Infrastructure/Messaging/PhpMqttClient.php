<?php

namespace App\Infrastructure\Messaging;

use App\Contracts\Messaging\MqttClientInterface;
use PhpMqtt\Client\ConnectionSettings;
use PhpMqtt\Client\MqttClient;
use Psr\Log\LoggerInterface;

class PhpMqttClient implements MqttClientInterface
{
    public function __construct(
        private readonly MqttClient $client,
        private readonly LoggerInterface $logger,
        private readonly ConnectionSettings $settings
    ) {
    }

    public function connect(): void
    {
        if ($this->client->isConnected()) {
            return;
        }

        $this->client->connect($this->settings, true);
    }

    public function disconnect(): void
    {
        if (!$this->client->isConnected()) {
            return;
        }

        $this->client->disconnect();
    }

    public function publish(string $topic, string $payload, int $qos = 0, bool $retain = false): void
    {
        $this->client->publish($topic, $payload, $qos, $retain);
    }

    public function subscribe(string $topic, callable $callback, int $qos = 0): void
    {
        $this->client->subscribe($topic, $callback, $qos);
    }

    public function unsubscribe(string $topic): void
    {
        $this->client->unsubscribe($topic);
    }

    public function loop(bool $blocking = true): void
    {
        $this->client->loop($blocking);
    }
}

