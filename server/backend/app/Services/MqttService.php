<?php

namespace App\Services;

use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Event;
use App\Models\Command;
use Illuminate\Support\Facades\Log;
use Exception;

class MqttService
{
    private MqttClient $mqtt;
    private string $clientId;

    public function __construct()
    {
        $host = config('mqtt.host', 'localhost');
        $port = config('mqtt.port', 1883);
        $this->clientId = config('mqtt.client_id', 'hydro-server-') . uniqid();
        
        $this->mqtt = new MqttClient($host, $port, $this->clientId);
    }

    /**
     * Подключение к MQTT брокеру
     */
    public function connect(): void
    {
        try {
            $settings = (new ConnectionSettings())
                ->setUsername(config('mqtt.username'))
                ->setPassword(config('mqtt.password'))
                ->setKeepAliveInterval(config('mqtt.keep_alive', 60))
                ->setLastWillTopic('hydro/server/status')
                ->setLastWillMessage('offline')
                ->setLastWillQualityOfService(1)
                ->setRetainLastWill(true);

            $this->mqtt->connect($settings, true);
            
            // Публикуем статус сервера
            $this->publish('hydro/server/status', 'online', 1, true);
            
            Log::info('MQTT connected', [
                'client_id' => $this->clientId,
                'broker' => config('mqtt.host') . ':' . config('mqtt.port')
            ]);
        } catch (Exception $e) {
            Log::error('MQTT connection failed', [
                'error' => $e->getMessage()
            ]);
            throw $e;
        }
    }

    /**
     * Подписка на топик
     */
    public function subscribe(string $topic, callable $callback, int $qos = 0): void
    {
        try {
            $this->mqtt->subscribe($topic, function ($topic, $message) use ($callback) {
                $callback($topic, $message);
            }, $qos);
            
            Log::info("MQTT subscribed", ['topic' => $topic, 'qos' => $qos]);
        } catch (Exception $e) {
            Log::error("MQTT subscribe failed", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
            throw $e;
        }
    }

    /**
     * Публикация сообщения в топик
     */
    public function publish(string $topic, string $message, int $qos = 0, bool $retain = false): void
    {
        try {
            $this->mqtt->publish($topic, $message, $qos, $retain);
            
            Log::debug("MQTT published", [
                'topic' => $topic,
                'qos' => $qos,
                'retain' => $retain,
                'message_length' => strlen($message)
            ]);
        } catch (Exception $e) {
            Log::error("MQTT publish failed", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
            throw $e;
        }
    }

    /**
     * Бесконечный цикл ожидания сообщений
     */
    public function loop(bool $blocking = true): void
    {
        try {
            $this->mqtt->loop($blocking);
        } catch (Exception $e) {
            Log::error("MQTT loop error", ['error' => $e->getMessage()]);
            throw $e;
        }
    }

    /**
     * Отключение от MQTT брокера
     */
    public function disconnect(): void
    {
        try {
            // Публикуем статус офлайн перед отключением
            $this->publish('hydro/server/status', 'offline', 1, true);
            
            $this->mqtt->disconnect();
            Log::info('MQTT disconnected');
        } catch (Exception $e) {
            Log::error("MQTT disconnect error", ['error' => $e->getMessage()]);
        }
    }

    /**
     * Обработка телеметрии
     */
    public function handleTelemetry(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'])) {
                Log::warning("Invalid telemetry data", [
                    'topic' => $topic,
                    'payload' => $payload
                ]);
                return;
            }

            // Сохранение телеметрии в БД
            Telemetry::create([
                'node_id' => $data['node_id'],
                'node_type' => $data['type'] ?? 'unknown',
                'data' => $data['data'] ?? [],
                'received_at' => now(),
            ]);

            // Обновление статуса узла
            Node::updateOrCreate(
                ['node_id' => $data['node_id']],
                [
                    'node_type' => $data['type'] ?? 'unknown',
                    'online' => true,
                    'last_seen_at' => now(),
                ]
            );

            Log::debug("Telemetry saved", [
                'node_id' => $data['node_id'],
                'type' => $data['type'] ?? 'unknown'
            ]);
        } catch (Exception $e) {
            Log::error("Telemetry handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Обработка событий
     */
    public function handleEvent(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'])) {
                Log::warning("Invalid event data", [
                    'topic' => $topic,
                    'payload' => $payload
                ]);
                return;
            }

            // Сохранение события в БД
            $event = Event::create([
                'node_id' => $data['node_id'],
                'level' => $data['level'] ?? Event::LEVEL_INFO,
                'message' => $data['message'] ?? 'Unknown event',
                'data' => $data['data'] ?? [],
            ]);

            Log::info("Event saved", [
                'node_id' => $event->node_id,
                'level' => $event->level,
                'message' => $event->message
            ]);

            // Если критичное событие - отправить уведомления
            if ($event->isCritical()) {
                $this->sendNotifications($event);
            }
        } catch (Exception $e) {
            Log::error("Event handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Обработка heartbeat (живой сигнал от узла)
     */
    public function handleHeartbeat(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'])) {
                return;
            }

            // Обновление last_seen_at
            Node::where('node_id', $data['node_id'])->update([
                'online' => true,
                'last_seen_at' => now(),
            ]);

            Log::debug("Heartbeat received", ['node_id' => $data['node_id']]);
        } catch (Exception $e) {
            Log::error("Heartbeat handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Обработка ответов на команды
     */
    public function handleCommandResponse(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['command_id'])) {
                return;
            }

            $command = Command::find($data['command_id']);
            if (!$command) {
                Log::warning("Command not found", ['command_id' => $data['command_id']]);
                return;
            }

            // Обновление статуса команды
            if ($data['status'] === 'completed') {
                $command->markAsCompleted($data['response'] ?? []);
            } elseif ($data['status'] === 'acknowledged') {
                $command->markAsAcknowledged();
            } elseif ($data['status'] === 'failed') {
                $command->markAsFailed($data['error'] ?? 'Unknown error');
            }

            Log::info("Command response received", [
                'command_id' => $command->id,
                'status' => $data['status']
            ]);
        } catch (Exception $e) {
            Log::error("Command response handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Отправка команды узлу
     */
    public function sendCommand(string $nodeId, string $command, array $params = [], ?int $commandId = null): void
    {
        $payload = json_encode([
            'type' => 'command',
            'command_id' => $commandId,
            'node_id' => $nodeId,
            'command' => $command,
            'params' => $params,
            'timestamp' => time(),
        ]);

        $topic = "hydro/command/{$nodeId}";
        $this->publish($topic, $payload, 1);
        
        Log::info("Command sent", [
            'node_id' => $nodeId,
            'command' => $command,
            'command_id' => $commandId
        ]);
    }

    /**
     * Отправка конфигурации узлу
     */
    public function sendConfig(string $nodeId, array $config): void
    {
        $payload = json_encode([
            'type' => 'config',
            'node_id' => $nodeId,
            'config' => $config,
            'timestamp' => time(),
        ]);

        $topic = "hydro/config/{$nodeId}";
        $this->publish($topic, $payload, 1);
        
        Log::info("Config sent", ['node_id' => $nodeId]);
    }

    /**
     * Отправка уведомлений о критичных событиях
     */
    private function sendNotifications(Event $event): void
    {
        try {
            // Telegram уведомление
            if (config('telegram.enabled', true)) {
                app(TelegramService::class)->sendAlert($event);
            }

            // SMS уведомление
            if (config('sms.enabled', false)) {
                app(SmsService::class)->sendAlert($event);
            }
        } catch (Exception $e) {
            Log::error("Notification sending error", [
                'event_id' => $event->id,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Проверка подключения
     */
    public function isConnected(): bool
    {
        try {
            return $this->mqtt->isConnected();
        } catch (Exception $e) {
            return false;
        }
    }
}

