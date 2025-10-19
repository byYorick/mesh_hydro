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
                Log::debug("📨 MQTT message received", [
                    'topic' => $topic,
                    'length' => strlen($message),
                    'preview' => substr($message, 0, 100)
                ]);
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

            // Валидация node_type
            $validTypes = ['ph_ec', 'climate', 'relay', 'water', 'display', 'root'];
            $nodeType = isset($data['type']) && in_array($data['type'], $validTypes) 
                ? $data['type'] 
                : 'unknown';

            // Сохранение телеметрии в БД
            $telemetry = Telemetry::create([
                'node_id' => $data['node_id'],
                'node_type' => $nodeType,
                'data' => $data['data'] ?? [],
                'received_at' => now(),
            ]);

            // Broadcast real-time event
            event(new \App\Events\TelemetryReceived($telemetry));

            // Обновление узла (или создание если новый)
            $node = Node::where('node_id', $data['node_id'])->first();
            $wasOnline = $node ? $node->online : false;
            $isNewNode = !$node;
            
            // Подготовка metadata
            $metadata = $node->metadata ?? [];
            if (isset($data['firmware'])) {
                $metadata['firmware'] = $data['firmware'];
            }
            if (isset($data['hardware'])) {
                $metadata['hardware'] = $data['hardware'];
            }
            if (isset($data['mac_address'])) {
                $metadata['mac_from_mqtt'] = $data['mac_address'];
            }
            
            // Создание или обновление узла
            $updateData = [
                'online' => true,
                'last_seen_at' => now(),
            ];
            
            // Только для новых узлов
            if ($isNewNode) {
                $updateData['node_type'] = $nodeType;
                $updateData['metadata'] = array_merge([
                    'created_via' => 'mqtt',
                    'created_at' => now()->toIso8601String(),
                ], $metadata);
            } else {
                // Обновляем только metadata для существующих
                if (!empty($metadata)) {
                    $updateData['metadata'] = array_merge($node->metadata ?? [], $metadata);
                }
            }
            
            $node = Node::updateOrCreate(
                ['node_id' => $data['node_id']],
                $updateData
            );

            // Broadcast status change если изменился или новый узел
            if ($wasOnline !== true) {
                event(new \App\Events\NodeStatusChanged($node, $wasOnline, true));
                
                if ($isNewNode) {
                    Log::info("New node auto-registered via MQTT", [
                        'node_id' => $node->node_id,
                        'node_type' => $node->node_type,
                    ]);
                }
            }

            Log::debug("Telemetry saved", [
                'node_id' => $data['node_id'],
                'type' => $nodeType
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

            // Broadcast event to frontend
            event(new \App\Events\EventCreated($event));

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
     * Автоматически создаёт узел если он не существует
     */
    public function handleHeartbeat(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'])) {
                return;
            }

            $nodeId = $data['node_id'];
            
            // Проверяем существует ли узел
            $node = Node::where('node_id', $nodeId)->first();
            
            if (!$node) {
                // АВТОПОИСК: Создаём новый узел автоматически
                $nodeType = $this->detectNodeType($nodeId, $data);
                
                $node = Node::create([
                    'node_id' => $nodeId,
                    'node_type' => $nodeType,
                    'zone' => 'Auto-discovered',
                    'online' => true,
                    'last_seen_at' => now(),
                    'mac_address' => $data['mac'] ?? null,
                    'metadata' => [
                        'discovered_at' => now()->toIso8601String(),
                        'discovered_via' => 'heartbeat',
                        'firmware' => $data['firmware'] ?? null,
                        'hardware' => $data['hardware'] ?? null,
                        'ip_address' => $data['ip'] ?? null,
                        'heap_free' => $data['heap_free'] ?? null,
                        'rssi_to_parent' => $data['rssi_to_parent'] ?? null,
                        'uptime' => $data['uptime'] ?? null,
                    ],
                ]);

                Log::info("🔍 AUTO-DISCOVERY: New node found via heartbeat", [
                    'node_id' => $nodeId,
                    'node_type' => $nodeType,
                    'mac' => $data['mac'] ?? 'unknown',
                ]);

                // Создаём событие об обнаружении нового узла
                Event::create([
                    'node_id' => $nodeId,
                    'level' => Event::LEVEL_INFO,
                    'message' => "New node auto-discovered: {$nodeId}",
                    'data' => ['node_type' => $nodeType],
                ]);

                // Broadcast новый узел на фронтенд
                event(new \App\Events\NodeDiscovered($node));
            } else {
                // Обновление last_seen_at и метаданных для существующего узла
                $metadata = $node->metadata ?? [];
                
                // Обновляем heap_free из heartbeat (если есть)
                if (isset($data['heap_free'])) {
                    $metadata['heap_free'] = $data['heap_free'];
                }
                
                // Обновляем RSSI
                if (isset($data['rssi_to_parent'])) {
                    $metadata['rssi_to_parent'] = $data['rssi_to_parent'];
                }
                
                // Обновляем uptime
                if (isset($data['uptime'])) {
                    $metadata['uptime'] = $data['uptime'];
                }
                
                // Обновляем MAC адрес (если пришёл в heartbeat)
                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $metadata['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }
                
                $updateData = [
                    'online' => true,
                    'last_seen_at' => now(),
                    'metadata' => $metadata,
                ];
                
                // Обновляем также поле mac_address в таблице nodes (если пришло)
                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }
                
                $node->update($updateData);
            }

            Log::debug("Heartbeat received", ['node_id' => $nodeId]);
        } catch (Exception $e) {
            Log::error("Heartbeat handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Обработка discovery топика (специальный топик для регистрации новых узлов)
     */
    public function handleDiscovery(string $topic, string $payload): void
    {
        try {
            Log::info("🔍 handleDiscovery called", [
                'topic' => $topic,
                'payload_length' => strlen($payload),
                'payload' => $payload
            ]);
            
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'])) {
                Log::warning("Invalid discovery data", [
                    'topic' => $topic,
                    'payload' => $payload,
                    'json_error' => json_last_error_msg()
                ]);
                return;
            }

            $nodeId = $data['node_id'];
            
            // Проверяем существует ли узел
            $existingNode = Node::where('node_id', $nodeId)->first();
            
            if ($existingNode) {
                Log::info("🔍 Discovery: Node already registered", [
                    'node_id' => $nodeId
                ]);
                
                // Обновляем метаданные если пришли новые данные
                if (isset($data['firmware']) || isset($data['hardware']) || isset($data['mac_address']) || isset($data['mac'])) {
                    $metadata = $existingNode->metadata ?? [];
                    $metadata['firmware'] = $data['firmware'] ?? $metadata['firmware'] ?? null;
                    $metadata['hardware'] = $data['hardware'] ?? $metadata['hardware'] ?? null;
                    $metadata['mac_address'] = $data['mac_address'] ?? $data['mac'] ?? $metadata['mac_address'] ?? null;
                    $metadata['ip_address'] = $data['ip'] ?? $metadata['ip_address'] ?? null;
                    $metadata['last_discovery'] = now()->toIso8601String();
                    
                    $updateData = [
                        'metadata' => $metadata,
                        'online' => true,
                        'last_seen_at' => now(),
                    ];
                    
                    // Обновляем также поле mac_address в таблице nodes
                    if (isset($data['mac_address']) || isset($data['mac'])) {
                        $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
                    }
                    
                    $existingNode->update($updateData);
                }
                
                return;
            }

            // АВТОПОИСК: Создаём новый узел
            // Используем node_type (тип узла), а не type (тип сообщения)
            $nodeType = $data['node_type'] ?? $this->detectNodeType($nodeId, $data);
            
            $node = Node::create([
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'zone' => $data['zone'] ?? 'Auto-discovered',
                'online' => true,
                'last_seen_at' => now(),
                'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,
                'metadata' => [
                    'discovered_at' => now()->toIso8601String(),
                    'discovered_via' => 'discovery_topic',
                    'firmware' => $data['firmware'] ?? null,
                    'hardware' => $data['hardware'] ?? null,
                    'ip_address' => $data['ip'] ?? null,
                    'sensors' => $data['sensors'] ?? [],
                    'capabilities' => $data['capabilities'] ?? [],
                    // Информация о памяти
                    'heap_free' => $data['heap_free'] ?? null,
                    'heap_min' => $data['heap_min'] ?? null,
                    'heap_total' => $data['heap_total'] ?? null,
                    // Flash память
                    'flash_total' => $data['flash_total'] ?? null,
                    'flash_used' => $data['flash_used'] ?? null,
                    // WiFi сигнал
                    'wifi_rssi' => $data['wifi_rssi'] ?? null,
                    // Mesh нод
                    'mesh_nodes' => $data['mesh_nodes'] ?? 0,
                ],
            ]);

            Log::info("🔍 AUTO-DISCOVERY: New node registered", [
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'mac' => $data['mac'] ?? 'unknown',
                'firmware' => $data['firmware'] ?? 'unknown',
            ]);

            // Создаём событие об обнаружении
            Event::create([
                'node_id' => $nodeId,
                'level' => Event::LEVEL_INFO,
                'message' => "New node auto-discovered and registered: {$nodeId}",
                'data' => [
                    'node_type' => $nodeType,
                    'firmware' => $data['firmware'] ?? null,
                    'hardware' => $data['hardware'] ?? null,
                ],
            ]);

            // Broadcast новый узел на фронтенд
            event(new \App\Events\NodeDiscovered($node));

            Log::info("✅ Node discovery complete", ['node_id' => $nodeId]);
            
        } catch (Exception $e) {
            Log::error("Discovery handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Определение типа узла по его ID или данным
     */
    private function detectNodeType(string $nodeId, array $data): string
    {
        // ВАЖНО: Определяем по префиксу node_id ПЕРВЫМ (до проверки $data['type'])
        // Потому что $data['type'] может быть "heartbeat"/"telemetry" (тип сообщения, а не узла!)
        if (str_starts_with($nodeId, 'root_')) {
            return 'root';
        }
        if (str_starts_with($nodeId, 'climate_')) {
            return 'climate';
        }
        if (str_starts_with($nodeId, 'ph_ec_')) {
            return 'ph_ec';
        }
        if (str_starts_with($nodeId, 'relay_')) {
            return 'relay';
        }
        if (str_starts_with($nodeId, 'water_')) {
            return 'water';
        }
        if (str_starts_with($nodeId, 'display_')) {
            return 'display';
        }

        // Проверяем явный тип узла в данных (только если это не тип сообщения!)
        if (isset($data['node_type'])) {
            return $data['node_type'];
        }

        // Определяем по наличию сенсоров в данных
        if (isset($data['sensors'])) {
            $sensors = $data['sensors'];
            if (in_array('ph', $sensors) || in_array('ec', $sensors)) {
                return 'ph_ec';
            }
            if (in_array('temperature', $sensors) || in_array('humidity', $sensors)) {
                return 'climate';
            }
        }

        // По умолчанию - неизвестный тип
        return 'unknown';
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

