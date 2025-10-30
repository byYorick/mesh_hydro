<?php

namespace App\Services;

use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Event;
use App\Models\Command;
use App\Models\NodeError;
use App\Services\NotificationThrottleService;
use Illuminate\Support\Facades\Log;
use Exception;

class MqttService
{
    private MqttClient $mqtt;
    private string $clientId;
    private NotificationThrottleService $throttleService;

    public function __construct(NotificationThrottleService $throttleService)
    {
        $host = config('mqtt.host', 'localhost');
        $port = config('mqtt.port', 1883);
        $this->clientId = config('mqtt.client_id', 'hydro-server-') . uniqid();
        $this->throttleService = $throttleService;
        
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
            // Автоматическое подключение если не подключен
            if (!$this->mqtt->isConnected()) {
                $this->connect();
            }
            
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
            $validTypes = ['ph', 'ec', 'ph_ec', 'climate', 'relay', 'water', 'display', 'root'];
            $nodeType = 'unknown';
            
            // Проверяем поле 'node_type' (приоритетно) или 'type'
            if (isset($data['node_type']) && in_array($data['node_type'], $validTypes)) {
                $nodeType = $data['node_type'];
            } elseif (isset($data['type']) && in_array($data['type'], $validTypes)) {
                $nodeType = $data['type'];
            }

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
                'last_seen_at' => now(),
            ];
            
            // Только для новых узлов или если node_type = 'unknown'
            if ($isNewNode) {
                $updateData['node_type'] = $nodeType;
                $updateData['metadata'] = array_merge([
                    'created_via' => 'mqtt',
                    'created_at' => now()->toIso8601String(),
                ], $metadata);
            } else {
                // Обновляем node_type если был 'unknown' (для исправления уже существующих узлов)
                if ($node && $node->node_type === 'unknown' && $nodeType !== 'unknown') {
                    $updateData['node_type'] = $nodeType;
                    Log::info("Updating node_type from 'unknown' to '{$nodeType}'", [
                        'node_id' => $node->node_id
                    ]);
                }
                
                // Обновляем только metadata для существующих
                if (!empty($metadata)) {
                    $updateData['metadata'] = array_merge($node->metadata ?? [], $metadata);
                }
            }
            
            $node = Node::updateOrCreate(
                ['node_id' => $data['node_id']],
                $updateData
            );

            // Обновляем online статус на основе isOnline()
            $wasOnline = $node->online;
            $isOnline = $node->isOnline();
            
            if ($wasOnline !== $isOnline) {
                $node->update(['online' => $isOnline]);
            }

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

            // Специальная обработка событий насосов
            $message = $this->translateEventMessage($data['message'] ?? 'Unknown event');
            $level = $data['level'] ?? Event::LEVEL_INFO;
            
            // Если это событие насоса, создаем специальное сообщение
            if (isset($data['data']['event_type']) && strpos($data['data']['event_type'], 'pump_') === 0) {
                $message = $this->translatePumpEventMessage($data['data']);
                $level = $this->getPumpEventLevel($data['data']);
            }
            
            // Сохранение события в БД
            $event = Event::create([
                'node_id' => $data['node_id'],
                'level' => $level,
                'message' => $message,
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
                
                // ВСЕГДА обновляем last_seen_at при discovery (независимо от данных)
                $metadata = $existingNode->metadata ?? [];
                $metadata['last_discovery'] = now()->toIso8601String();
                
                // Обновляем метаданные если пришли новые данные
                if (isset($data['firmware'])) {
                    $metadata['firmware'] = $data['firmware'];
                }
                if (isset($data['hardware'])) {
                    $metadata['hardware'] = $data['hardware'];
                }
                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $metadata['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }
                if (isset($data['ip'])) {
                    $metadata['ip_address'] = $data['ip'];
                }
                
                $updateData = [
                    'metadata' => $metadata,
                    'last_seen_at' => now(),  // ✅ ВСЕГДА обновляем!
                ];
                
                // Обновляем также поле mac_address в таблице nodes
                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }
                
                $existingNode->update($updateData);
                
                // Обновляем online статус на основе isOnline()
                $wasOnline = $existingNode->online;
                $isOnline = $existingNode->isOnline();
                
                if ($wasOnline !== $isOnline) {
                    $existingNode->update(['online' => $isOnline]);
                    event(new \App\Events\NodeStatusChanged($existingNode, $wasOnline, $isOnline));
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
        if (str_starts_with($nodeId, 'ph_')) {
            return 'ph';  // НОВЫЙ: отдельная нода pH
        }
        if (str_starts_with($nodeId, 'ec_')) {
            return 'ec';  // НОВЫЙ: отдельная нода EC
        }
        if (str_starts_with($nodeId, 'ph_ec_')) {
            return 'ph_ec';  // СТАРЫЙ: объединенная нода
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
            if (in_array('ph', $sensors) && !in_array('ec', $sensors)) {
                return 'ph';  // Только pH датчик
            }
            if (in_array('ec', $sensors) && !in_array('ph', $sensors)) {
                return 'ec';  // Только EC датчик
            }
            if (in_array('ph', $sensors) && in_array('ec', $sensors)) {
                return 'ph_ec';  // Оба датчика
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
            $eventType = $this->mapEventLevelToType($event->level);
            $message = "Event: {$event->message} (Node: {$event->node_id})";
            
            // Проверяем throttling
            if (!$this->throttleService->canSendNotification($eventType, $event->node_id, $message)) {
                Log::debug("Notification throttled", [
                    'event_id' => $event->id,
                    'node_id' => $event->node_id,
                    'level' => $event->level,
                    'type' => $eventType
                ]);
                return;
            }

            // Telegram уведомление
            if (config('telegram.enabled', true)) {
                app(TelegramService::class)->sendAlert($event);
            }

            // SMS уведомление (только для критичных)
            if (config('sms.enabled', false) && $event->isCritical()) {
                app(SmsService::class)->sendAlert($event);
            }
            
            // Регистрируем отправку для throttling
            $this->throttleService->markNotificationSent($eventType, $event->node_id, $message);
            
        } catch (Exception $e) {
            Log::error("Notification sending error", [
                'event_id' => $event->id,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Маппинг уровня события в тип для throttling
     */
    private function mapEventLevelToType(string $level): string
    {
        return match($level) {
            'critical', 'emergency' => 'critical',
            'warning', 'error' => 'warning',
            default => 'info'
        };
    }

    /**
     * Обработка ошибок узлов
     * Топик: hydro/error/{node_id}
     */
    public function handleError(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'])) {
                Log::warning("Invalid error data", [
                    'topic' => $topic,
                    'payload' => $payload
                ]);
                return;
            }

            // Сохранение ошибки в БД
            $error = NodeError::create([
                'node_id' => $data['node_id'],
                'error_code' => $data['error_code'] ?? 'UNKNOWN_ERROR',
                'error_type' => $data['error_type'] ?? NodeError::TYPE_SOFTWARE,
                'severity' => $data['severity'] ?? NodeError::SEVERITY_MEDIUM,
                'message' => $data['message'] ?? 'Unknown error occurred',
                'stack_trace' => $data['stack_trace'] ?? null,
                'diagnostics' => $data['diagnostics'] ?? [],
                'occurred_at' => isset($data['timestamp']) 
                    ? \Carbon\Carbon::createFromTimestamp($data['timestamp'])
                    : now(),
            ]);

            Log::error("Node error occurred", [
                'node_id' => $error->node_id,
                'error_code' => $error->error_code,
                'severity' => $error->severity,
                'message' => $error->message,
            ]);

            // Создание события для критичных ошибок
            if ($error->isCritical()) {
                Event::create([
                    'node_id' => $error->node_id,
                    'level' => Event::LEVEL_CRITICAL,
                    'message' => "Critical error: {$error->message}",
                    'data' => [
                        'error_code' => $error->error_code,
                        'error_type' => $error->error_type,
                        'diagnostics' => $error->diagnostics,
                    ],
                ]);

                // Отправка уведомлений для критичных ошибок
                $this->sendNotifications($error);
            }

            // Broadcast error to frontend
            event(new \App\Events\ErrorOccurred($error));

        } catch (Exception $e) {
            Log::error("Error handling error (meta!)", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Отправка уведомлений о критичной ошибке
     */
    private function sendErrorNotifications(NodeError $error): void
    {
        try {
            $errorType = $this->mapErrorSeverityToType($error->severity);
            $message = "Error: {$error->message} (Node: {$error->node_id}, Code: {$error->error_code})";
            
            // Проверяем throttling
            if (!$this->throttleService->canSendNotification($errorType, $error->node_id, $message)) {
                Log::debug("Error notification throttled", [
                    'error_id' => $error->id,
                    'node_id' => $error->node_id,
                    'severity' => $error->severity,
                    'type' => $errorType
                ]);
                return;
            }

            // Telegram уведомление
            if (config('telegram.enabled', false)) {
                try {
                    app(TelegramService::class)->sendErrorAlert($error);
                } catch (\Exception $e) {
                    Log::error("Failed to send Telegram alert", [
                        'error' => $e->getMessage()
                    ]);
                }
            }

            // SMS уведомление (только для critical)
            if (config('sms.enabled', false) && $error->isCritical()) {
                app(SmsService::class)->sendErrorAlert($error);
            }
            
            // Регистрируем отправку для throttling
            $this->throttleService->markNotificationSent($errorType, $error->node_id, $message);
            
        } catch (Exception $e) {
            Log::error("Notification sending error", [
                'error_id' => $error->id,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Маппинг серьезности ошибки в тип для throttling
     */
    private function mapErrorSeverityToType(string $severity): string
    {
        return match($severity) {
            'critical' => 'critical',
            'high', 'medium' => 'warning',
            default => 'info'
        };
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

    /**
     * Обработка ответа с конфигурацией от узла
     */
    public function handleConfigResponse(string $topic, string $payload): void
    {
        try {
            Log::info("📋 handleConfigResponse called", [
                'topic' => $topic,
                'payload_length' => strlen($payload)
            ]);
            
            $data = json_decode($payload, true);
            
            if (!$data || !isset($data['node_id'], $data['config'])) {
                Log::warning("Invalid config_response data", [
                    'topic' => $topic,
                    'payload' => $payload,
                    'json_error' => json_last_error_msg()
                ]);
                return;
            }

            $nodeId = $data['node_id'];
            $config = $data['config'];
            
            // Проверка что config - это массив
            if (!is_array($config)) {
                Log::warning("Config is not an array", [
                    'node_id' => $nodeId,
                    'config_type' => gettype($config)
                ]);
                return;
            }
            
            Log::info("📋 Config response received", [
                'node_id' => $nodeId,
                'config_keys' => array_keys($config)
            ]);
            
            // Сохранение конфигурации в кэш (1 час)
            Cache::put("node_config:{$nodeId}", $config, 3600);
            
            // Обновление узла в БД
            $node = Node::where('node_id', $nodeId)->first();
            if ($node) {
                $node->update([
                    'config' => $config,
                    'last_seen_at' => now()
                ]);
                
                Log::info("📋 Node config updated in DB", ['node_id' => $nodeId]);
            }
            
            // Сохранение калибровки насосов в БД
            if (isset($config['pumps_calibration']) && is_array($config['pumps_calibration'])) {
                foreach ($config['pumps_calibration'] as $pumpCal) {
                    if (isset($pumpCal['pump_id'])) {
                        \App\Models\PumpCalibration::updateOrCreate(
                            [
                                'node_id' => $nodeId,
                                'pump_id' => $pumpCal['pump_id']
                            ],
                            [
                                'ml_per_second' => $pumpCal['ml_per_second'] ?? 1.0,
                                'calibration_volume_ml' => $pumpCal['calibration_volume_ml'] ?? null,
                                'calibration_time_ms' => $pumpCal['calibration_time_ms'] ?? null,
                                'is_calibrated' => $pumpCal['is_calibrated'] ?? false,
                                'calibrated_at' => isset($pumpCal['last_calibrated']) && $pumpCal['last_calibrated'] > 0
                                    ? \Carbon\Carbon::createFromTimestamp($pumpCal['last_calibrated'])
                                    : null,
                            ]
                        );
                    }
                }
                
                Log::info("📋 Pump calibrations saved", [
                    'node_id' => $nodeId,
                    'pumps_count' => count($config['pumps_calibration'])
                ]);
            }
            
            // Отправка события через WebSocket
            broadcast(new \App\Events\NodeConfigUpdated($nodeId, $config));
            
            Log::info("📋 Config response processed successfully", ['node_id' => $nodeId]);
            
        } catch (Exception $e) {
            Log::error("Error handling config_response", [
                'topic' => $topic,
                'error' => $e->getMessage(),
                'trace' => $e->getTraceAsString()
            ]);
        }
    }

    /**
     * Перевод сообщений событий на русский язык
     */
    private function translateEventMessage(string $message): string
    {
        $translations = [
            'pH far from target, aggressive correction' => 'pH далеко от цели, агрессивная коррекция',
            'pH correction in progress' => 'Коррекция pH в процессе',
            'pH target reached' => 'Цель pH достигнута',
            'EC far from target, aggressive correction' => 'EC далеко от цели, агрессивная коррекция',
            'EC correction in progress' => 'Коррекция EC в процессе',
            'EC target reached' => 'Цель EC достигнута',
            'Pump started' => 'Насос запущен',
            'Pump stopped' => 'Насос остановлен',
            'Calibration completed' => 'Калибровка завершена',
            'Node offline' => 'Узел офлайн',
            'Node online' => 'Узел онлайн',
            'Critical error' => 'Критическая ошибка',
            'Warning' => 'Предупреждение',
            'Info' => 'Информация',
            'Debug' => 'Отладка',
        ];

        return $translations[$message] ?? $message;
    }

    /**
     * Перевод сообщений событий насосов на русский язык
     */
    private function translatePumpEventMessage(array $data): string
    {
        $eventType = $data['event_type'] ?? 'unknown';
        $pumpId = $data['pump_id'] ?? 0;
        $dose = $data['dose_ml'] ?? 0;
        $duration = $data['duration_ms'] ?? 0;
        
        // Определяем название насоса
        $pumpName = $this->getPumpName($pumpId, $data);
        
        switch ($eventType) {
            case 'pump_start':
                return "🚰 Насос {$pumpName} запущен: {$dose} мл ({$duration} мс)";
            case 'pump_stop':
                return "🛑 Насос {$pumpName} остановлен: {$dose} мл ({$duration} мс)";
            case 'pump_emergency_stop':
                return "🚨 Аварийная остановка насоса {$pumpName}";
            case 'pump_timeout':
                return "⏰ Таймаут насоса {$pumpName}";
            case 'pump_calibration_start':
                return "🔧 Начало калибровки насоса {$pumpName}";
            case 'pump_calibration_end':
                return "✅ Калибровка насоса {$pumpName} завершена";
            default:
                return "🔧 Событие насоса {$pumpName}: {$eventType}";
        }
    }

    /**
     * Получение уровня события насоса
     */
    private function getPumpEventLevel(array $data): string
    {
        $eventType = $data['event_type'] ?? 'unknown';
        
        switch ($eventType) {
            case 'pump_emergency_stop':
                return Event::LEVEL_CRITICAL;
            case 'pump_timeout':
                return Event::LEVEL_WARNING;
            case 'pump_start':
            case 'pump_stop':
            case 'pump_calibration_start':
            case 'pump_calibration_end':
            default:
                return Event::LEVEL_INFO;
        }
    }

    /**
     * Получение названия насоса
     */
    private function getPumpName(int $pumpId, array $data): string
    {
        $nodeType = $data['node_type'] ?? 'unknown';
        
        // Для pH нод
        if ($nodeType === 'ph') {
            switch ($pumpId) {
                case 0:
                    return 'pH UP';
                case 1:
                    return 'pH DOWN';
                default:
                    return "pH #{$pumpId}";
            }
        }
        
        // Для EC нод
        if ($nodeType === 'ec') {
            switch ($pumpId) {
                case 0:
                    return 'EC A';
                case 1:
                    return 'EC B';
                case 2:
                    return 'EC C';
                default:
                    return "EC #{$pumpId}";
            }
        }
        
        // Для ph_ec нод
        if ($nodeType === 'ph_ec') {
            switch ($pumpId) {
                case 0:
                    return 'pH UP';
                case 1:
                    return 'pH DOWN';
                case 2:
                    return 'EC A';
                case 3:
                    return 'EC B';
                case 4:
                    return 'EC C';
                default:
                    return "Pump #{$pumpId}";
            }
        }
        
        return "Насос #{$pumpId}";
    }
}

