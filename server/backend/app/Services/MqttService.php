<?php

namespace App\Services;

use App\Contracts\Messaging\MqttClientInterface;
use App\Services\Mqtt\CommandPublisher;
use App\Services\Mqtt\Enum\MqttMessageType;
use App\Services\Mqtt\MqttMessageRouter;
use Illuminate\Support\Facades\Cache;
use Illuminate\Support\Facades\Log;
use Throwable;

/**
 * @deprecated Используйте напрямую MqttClientInterface, MqttMessageRouter и CommandPublisher.
 *             Класс сохранён для совместимости со старым кодом.
 */
class MqttService
{
    public function __construct(
        private readonly MqttClientInterface $client,
        private readonly MqttMessageRouter $router,
        private readonly CommandPublisher $commandPublisher,
    ) {
    }

    public function connect(): void
    {
        $this->client->connect();

        $statusTopic = config('mqtt.status_topic', 'hydro/server/status');
        $this->client->publish($statusTopic, 'online', 1, true);

        try {
            Cache::forever('mqtt.last_successful_connection', now());
        } catch (Throwable $e) {
            Log::warning('Failed to persist MQTT connection timestamp', [
                'error' => $e->getMessage(),
            ]);
            }
            
            Log::info('MQTT connected', [
            'broker' => config('mqtt.host') . ':' . config('mqtt.port'),
        ]);
    }

    public function disconnect(): void
    {
        try {
            $statusTopic = config('mqtt.status_topic', 'hydro/server/status');
            $this->client->publish($statusTopic, 'offline', 1, true);
        } catch (Throwable $e) {
            Log::warning('Failed to publish offline status before disconnect', [
                'error' => $e->getMessage(),
            ]);
        }

        $this->client->disconnect();
            Log::info('MQTT disconnected');
    }

    public function subscribe(string $topic, MqttMessageType $type, int $qos = 0): void
    {
        $this->client->subscribe(
            $topic,
            function (string $resolvedTopic, string $payload) use ($type): void {
                Log::debug('MQTT message received', [
                    'topic' => $resolvedTopic,
                    'length' => strlen($payload),
                    'preview' => substr($payload, 0, 150),
                ]);

                $this->router->dispatch($type, $resolvedTopic, $payload);
            },
            $qos,
        );

        Log::info('MQTT subscribed', [
                'topic' => $topic,
            'qos' => $qos,
            'type' => $type->value,
        ]);
    }

    public function unsubscribe(string $topic): void
    {
        $this->client->unsubscribe($topic);
        Log::info('MQTT unsubscribed', ['topic' => $topic]);
    }

    public function loop(bool $blocking = true): void
    {
        $this->client->loop($blocking);
    }

    public function sendCommand(string $nodeId, string $command, array $params = [], ?int $commandId = null): void
    {
        $this->commandPublisher->sendCommand($nodeId, $command, $params, $commandId);
    }

    public function sendConfig(string $nodeId, array $config): void
    {
        $this->commandPublisher->sendConfig($nodeId, $config);
    }
}

