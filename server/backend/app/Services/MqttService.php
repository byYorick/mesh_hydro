<?php

namespace App\Services;

use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Event;
use App\Models\Command;
use App\Models\NodeError;
use App\Models\NewNode;
use App\Services\NotificationThrottleService;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Cache;
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
            $username = config('mqtt.username');
            $password = config('mqtt.password');

            $settings = (new ConnectionSettings())
                ->setKeepAliveInterval(config('mqtt.keep_alive', 60))
                ->setLastWillTopic('hydro/server/status')
                ->setLastWillMessage('offline')
                ->setLastWillQualityOfService(1)
                ->setRetainLastWill(true);

            if (is_string($username) && trim($username) !== '') {
                $settings->setUsername($username);
            }

            if (is_string($password) && trim($password) !== '') {
                $settings->setPassword($password);
            }

            $this->mqtt->connect($settings, true);
            
            // Публикуем статус сервера
            $this->publish('hydro/server/status', 'online', 1, true);
            
            try {
                Cache::store('file')->put('mqtt.last_successful_connection', now(), 600);
            } catch (\Throwable $cacheError) {
                Log::warning('Failed to persist MQTT connection timestamp', ['error' => $cacheError->getMessage()]);
            }

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

            $meshId = $this->resolveZone($topic, $data, 'telemetry', $data['node_id']);
            if (!$meshId) {
                return;
            }

            $validTypes = ['ph', 'ec', 'ph_ec', 'climate', 'relay', 'water', 'display', 'root'];
            $nodeType = 'unknown';
            if (isset($data['node_type']) && in_array($data['node_type'], $validTypes)) {
                $nodeType = $data['node_type'];
            } elseif (isset($data['type']) && in_array($data['type'], $validTypes)) {
                $nodeType = $data['type'];
            }

            $telemetry = Telemetry::create([
                'node_id' => $data['node_id'],
                'node_type' => $nodeType,
                'data' => $data['data'] ?? [],
                'received_at' => now(),
            ]);

            event(new \App\Events\TelemetryReceived($telemetry));

            $node = Node::where('node_id', $data['node_id'])->first();
            $wasOnline = $node ? $node->online : false;
            $isNewNode = !$node;

            $metadata = $node->metadata ?? [];
            if (isset($data['firmware'])) {
                $metadata['firmware'] = $data['firmware'];
            }
            if (isset($data['hardware'])) {
                $metadata['hardware'] = $data['hardware'];
            }
            if (isset($data['mac_address']) || isset($data['mac'])) {
                $metadata['mac_address'] = $data['mac_address'] ?? $data['mac'];
            }
            if ($meshId) {
                $metadata['mesh_network_id'] = $meshId;
            }

            $updateData = [
                'last_seen_at' => now(),
            ];

            if (isset($data['root_node_id']) && $data['root_node_id']) {
                $updateData['root_node_id'] = $data['root_node_id'];
            } elseif ($nodeType === 'root') {
                $updateData['root_node_id'] = $data['node_id'];
            }

            if ($meshId) {
                $updateData['zone'] = $meshId;
            }

            if (isset($data['mac_address']) || isset($data['mac'])) {
                $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
            }

            if ($isNewNode) {
                $updateData['node_type'] = $nodeType;
                $updateData['metadata'] = array_merge([
                    'created_via' => 'mqtt',
                    'created_at' => now()->toIso8601String(),
                ], $metadata);
            } else {
                if ($node && $node->node_type === 'unknown' && $nodeType !== 'unknown') {
                    $updateData['node_type'] = $nodeType;
                    Log::info("Updating node_type from 'unknown' to '{$nodeType}'", [
                        'node_id' => $node->node_id
                    ]);
                }
                if (!empty($metadata)) {
                    $updateData['metadata'] = array_merge($node->metadata ?? [], $metadata);
                }
            }

            $node = Node::updateOrCreate(
                ['node_id' => $data['node_id']],
                $updateData
            );

            $node->refresh();

            $wasOnline = $node->online;
            $isOnline = $node->isOnline();

            if ($wasOnline !== $isOnline) {
                $node->update(['online' => $isOnline]);
                event(new \App\Events\NodeStatusChanged($node, $wasOnline, $isOnline));

                if ($wasOnline && !$isOnline) {
                    $statusEvent = Event::create([
                        'node_id' => $node->node_id,
                        'level' => Event::LEVEL_WARNING,
                        'message' => "Узел {$node->node_id} перешёл в офлайн",
                        'data' => [
                            'last_seen' => $node->last_seen_at?->toDateTimeString(),
                            'node_type' => $node->node_type,
                            'zone' => $node->zone,
                        ],
                    ]);

                    event(new \App\Events\EventCreated($statusEvent));
                }
            } else {
                event(new \App\Events\NodeStatusUpdate($node));
            }

            if ($isNewNode) {
                event(new \App\Events\NodeDiscovered($node));

                Log::info("New node auto-registered via MQTT", [
                    'node_id' => $node->node_id,
                    'node_type' => $node->node_type,
                ]);
            }

            Log::debug("Telemetry saved", [
                'node_id' => $data['node_id'],
                'type' => $nodeType,
                'mesh_network_id' => $meshId,
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

            $meshId = $this->resolveZone($topic, $data, 'event', $data['node_id']);
            if (!$meshId) {
                return;
            }

            // Специальная обработка событий насосов
            $message = $this->translateEventMessage($data['message'] ?? 'Unknown event');
            $level = $data['level'] ?? Event::LEVEL_INFO;
            
            // Получаем узел для определения типа и получения метаданных
            $node = Node::where('node_id', $data['node_id'])->first();
            if ($node) {
                $meta = $node->metadata ?? [];
                if (($meta['mesh_network_id'] ?? null) !== $meshId || $node->zone !== $meshId) {
                    $meta['mesh_network_id'] = $meshId;
                    $node->update(['metadata' => $meta, 'zone' => $meshId]);
                    $node->refresh();
                }
            }
            $nodeType = $node ? $node->node_type : ($data['node_type'] ?? 'unknown');
            
            // Подготовка данных события с добавлением node_type
            $eventData = $data['data'] ?? [];
            $eventData['node_type'] = $nodeType;
            $eventData['mesh_network_id'] = $meshId;
            
            // Если это событие насоса, создаем специальное сообщение
            // Проверяем по event_type или по наличию pump_id в данных
            $isPumpEvent = (isset($eventData['event_type']) && strpos($eventData['event_type'], 'pump_') === 0)
                || (isset($eventData['pump_id']) && (strpos($message, 'Насос') !== false || strpos($message, 'насос') !== false));
            
            if ($isPumpEvent) {
                // Если event_type не задан, определяем по сообщению
                if (!isset($eventData['event_type'])) {
                    if (strpos($message, 'запущен') !== false || strpos($message, 'start') !== false) {
                        $eventData['event_type'] = 'pump_start';
                    } elseif (strpos($message, 'остановлен') !== false || strpos($message, 'stop') !== false) {
                        $eventData['event_type'] = 'pump_stop';
                    } else {
                        $eventData['event_type'] = 'pump_start'; // По умолчанию
                    }
                }
                
                // ВАЖНО: Обязательно перезаписываем сообщение для событий насосов
                $message = $this->translatePumpEventMessage($eventData);
                $level = $this->getPumpEventLevel($eventData);
                
                Log::debug("Pump event message reformatted", [
                    'node_id' => $data['node_id'],
                    'node_type' => $nodeType,
                    'pump_id' => $eventData['pump_id'] ?? 'unknown',
                    'event_type' => $eventData['event_type'] ?? 'unknown',
                    'old_message' => $data['message'] ?? 'none',
                    'new_message' => $message,
                ]);
                
                // Сохраняем все метаданные из события насоса
                // Метаданные уже в $eventData из $data['data'], но нормализуем структуру
                if (isset($eventData['pid_data'])) {
                    // Преобразуем pid_data в удобную структуру
                    $pidData = $eventData['pid_data'];
                    if (is_array($pidData)) {
                        // Определяем тип насоса и сохраняем в соответствующее поле
                        $pumpId = $eventData['pump_id'] ?? 0;
                        if ($pumpId === 0) {
                            $eventData['pid_up'] = $pidData;
                        } elseif ($pumpId === 1) {
                            $eventData['pid_down'] = $pidData;
                        }
                    }
                }
                
                // Нормализуем ph поля
                if (isset($eventData['current_ph']) && !isset($eventData['ph'])) {
                    $eventData['ph'] = $eventData['current_ph'];
                }
                if (isset($eventData['ph_target']) && !isset($eventData['target'])) {
                    $eventData['target'] = $eventData['ph_target'];
                }
                
                // Добавляем метаданные из последней телеметрии если их нет в событии
                if ($node && (!isset($eventData['ph']) || !isset($eventData['pid_up']) || !isset($eventData['pid_down']))) {
                    $lastTelemetry = $node->telemetry()->orderBy('received_at', 'desc')->first();
                    if ($lastTelemetry && $lastTelemetry->data) {
                        $telemetryData = $lastTelemetry->data ?? [];
                        if (!isset($eventData['ph']) && isset($telemetryData['ph'])) {
                            $eventData['ph'] = $telemetryData['ph'];
                        }
                        if (!isset($eventData['pid_up']) && isset($telemetryData['pid_up'])) {
                            $eventData['pid_up'] = $telemetryData['pid_up'];
                        }
                        if (!isset($eventData['pid_down']) && isset($telemetryData['pid_down'])) {
                            $eventData['pid_down'] = $telemetryData['pid_down'];
                        }
                    }
                }
            }
            
            // Сохранение события в БД
            $event = Event::create([
                'node_id' => $data['node_id'],
                'level' => $level,
                'message' => $message,
                'data' => $eventData,
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
            $topicZone = $this->extractZoneFromTopic($topic);

            if (!$data) {
                return;
            }

            if (($data['type'] ?? null) === 'heartbeat' && isset($data['mac_address'])) {
                $handledBySetup = $this->processSetupHeartbeat($data, $topicZone);
                if ($handledBySetup) {
                    return;
                }
            }
            
            if (!isset($data['node_id'])) {
                return;
            }

            $nodeId = $data['node_id'];
            $meshId = $this->resolveZone($topic, $data, 'heartbeat', $nodeId);
            if (!$meshId) {
                return;
            }
            
            // Проверяем существует ли узел
            $node = Node::where('node_id', $nodeId)->first();
            
            if (!$node) {
                // АВТОПОИСК: Создаём новый узел автоматически
                $nodeType = $this->detectNodeType($nodeId, $data);
                
                $metadata = [
                    'discovered_at' => now()->toIso8601String(),
                    'discovered_via' => 'heartbeat',
                    'firmware' => $data['firmware'] ?? null,
                    'hardware' => $data['hardware'] ?? null,
                    'ip_address' => $data['ip'] ?? null,
                    'heap_free' => $data['heap_free'] ?? null,
                    'rssi_to_parent' => $data['rssi_to_parent'] ?? null,
                    'uptime' => $data['uptime'] ?? null,
                    'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,
                ];
                $metadata['mesh_network_id'] = $meshId;

                $node = Node::create([
                    'node_id' => $nodeId,
                    'node_type' => $nodeType,
                    'zone' => $meshId,
                    'last_seen_at' => now(),
                    'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,
                    'root_node_id' => $data['root_node_id'] ?? ($nodeType === 'root' ? $nodeId : null),
                    'metadata' => $metadata,
                ]);
                
                // Перезагружаем узел для корректного isOnline()
                $node->refresh();
                
                // Устанавливаем online на основе isOnline()
                $node->update(['online' => $node->isOnline()]);

                Log::info("🔍 AUTO-DISCOVERY: New node found via heartbeat", [
                    'node_id' => $nodeId,
                    'node_type' => $nodeType,
                    'mac' => $data['mac'] ?? 'unknown',
                ]);

                // Создаём событие об обнаружении нового узла
                $discoveryEvent = Event::create([
                    'node_id' => $nodeId,
                    'level' => Event::LEVEL_INFO,
                    'message' => "New node auto-discovered: {$nodeId}",
                    'data' => ['node_type' => $nodeType],
                ]);

                // Broadcast событие об обнаружении через WebSocket
                event(new \App\Events\EventCreated($discoveryEvent));

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
                $metadata['mesh_network_id'] = $meshId;
                
                $updateData = [
                    'last_seen_at' => now(),
                    'metadata' => $metadata,
                ];

                if (!empty($data['root_node_id'])) {
                    $updateData['root_node_id'] = $data['root_node_id'];
                } elseif ($node->node_type === 'root') {
                    $updateData['root_node_id'] = $node->node_id;
                }

                $updateData['zone'] = $meshId;
                
                // Обновляем также поле mac_address в таблице nodes (если пришло)
                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }
                
                $node->update($updateData);
                
                // Перезагружаем узел чтобы обновить last_seen_at в памяти
                $node->refresh();
                
                // Обновляем online статус на основе isOnline() после обновления last_seen_at
                $wasOnline = $node->online;
                $isOnline = $node->isOnline();
                
                if ($wasOnline !== $isOnline) {
                    $node->update(['online' => $isOnline]);
                    event(new \App\Events\NodeStatusChanged($node, $wasOnline, $isOnline));
                    
                    // Создаём событие в БД при переходе в офлайн
                    if ($wasOnline && !$isOnline) {
                        $statusEvent = Event::create([
                            'node_id' => $node->node_id,
                            'level' => Event::LEVEL_WARNING,
                            'message' => "Узел {$node->node_id} перешёл в офлайн",
                            'data' => [
                                'last_seen' => $node->last_seen_at?->toDateTimeString(),
                                'node_type' => $node->node_type,
                                'zone' => $node->zone,
                            ],
                        ]);
                        
                        // Broadcast событие через WebSocket
                        event(new \App\Events\EventCreated($statusEvent));
                    }
                } else {
                    // Даже если статус не изменился, отправляем обновление для UI
                    event(new \App\Events\NodeStatusUpdate($node));
                }
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
            $isSetupTopic = str_starts_with($topic, 'hydro/setup/');
            $meshId = null;

            if (!$data) {
                Log::warning("Invalid discovery data", [
                    'topic' => $topic,
                    'payload' => $payload,
                    'json_error' => json_last_error_msg()
                ]);
                return;
            }

            if (!$isSetupTopic) {
                $meshId = $this->resolveZone($topic, $data, 'discovery', $data['node_id'] ?? null);
                if (!$meshId) {
                    return;
                }
            }

            // Обработка сообщений новой системы подключения узлов
            if (isset($data['type'])) {
                if ($data['type'] === 'discovery' && isset($data['mac_address'])) {
                    $this->processSetupDiscovery($data, $meshId);
                    return;
                }

                if ($data['type'] === 'config_confirmation') {
                    $this->processSetupConfigConfirmation($data, $meshId);
                    return;
                }
            }
            
            if (!isset($data['node_id'])) {
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
                if ($meshId) {
                    $metadata['mesh_network_id'] = $meshId;
                }
                
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
                if (isset($data['mqtt_topic_prefix'])) {
                    $metadata['mqtt_topic_prefix'] = $data['mqtt_topic_prefix'];
                }
                
                $updateData = [
                    'metadata' => $metadata,
                    'last_seen_at' => now(),  // ✅ ВСЕГДА обновляем!
                ];
                
                // Обновляем также поле mac_address в таблице nodes
                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }
                
                if (!empty($data['root_node_id'])) {
                    $updateData['root_node_id'] = $data['root_node_id'];
                } elseif ($existingNode->node_type === 'root') {
                    $updateData['root_node_id'] = $existingNode->node_id;
                }

                $updateData['zone'] = $meshId;

                $existingNode->update($updateData);
                
                // Обновляем online статус на основе isOnline()
                $wasOnline = $existingNode->online;
                $isOnline = $existingNode->isOnline();
                
                if ($wasOnline !== $isOnline) {
                    $existingNode->update(['online' => $isOnline]);
                    event(new \App\Events\NodeStatusChanged($existingNode, $wasOnline, $isOnline));
                    
                    // Создаём событие в БД при переходе в офлайн
                    if ($wasOnline && !$isOnline) {
                        $statusEvent = Event::create([
                            'node_id' => $existingNode->node_id,
                            'level' => Event::LEVEL_WARNING,
                            'message' => "Узел {$existingNode->node_id} перешёл в офлайн",
                            'data' => [
                                'last_seen' => $existingNode->last_seen_at?->toDateTimeString(),
                                'node_type' => $existingNode->node_type,
                                'zone' => $existingNode->zone,
                            ],
                        ]);
                        
                        // Broadcast событие через WebSocket
                        event(new \App\Events\EventCreated($statusEvent));
                    }
                }
                
                return;
            }

            // АВТОПОИСК: Создаём новый узел
            // Используем node_type (тип узла), а не type (тип сообщения)
            $nodeType = $data['node_type'] ?? $this->detectNodeType($nodeId, $data);
            
            $metadata = [
                'discovered_at' => now()->toIso8601String(),
                'discovered_via' => 'discovery_topic',
                'firmware' => $data['firmware'] ?? null,
                'hardware' => $data['hardware'] ?? null,
                'ip_address' => $data['ip'] ?? null,
                'sensors' => $data['sensors'] ?? [],
                'capabilities' => $data['capabilities'] ?? [],
                'heap_free' => $data['heap_free'] ?? null,
                'heap_min' => $data['heap_min'] ?? null,
                'heap_total' => $data['heap_total'] ?? null,
                'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,
                'wifi_rssi' => $data['wifi_rssi'] ?? null,
                'mesh_nodes' => $data['mesh_nodes'] ?? 0,
                'flash_total' => $data['flash_total'] ?? null,
                'flash_used' => $data['flash_used'] ?? null,
            ];
            $metadata['mesh_network_id'] = $meshId;
            if (isset($data['mqtt_topic_prefix'])) {
                $metadata['mqtt_topic_prefix'] = $data['mqtt_topic_prefix'];
            }

            $node = Node::create([
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'zone' => $meshId,
                'last_seen_at' => now(),
                'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,
                'root_node_id' => $data['root_node_id'] ?? ($nodeType === 'root' ? $nodeId : null),
                'metadata' => $metadata,
            ]);
            
            // Перезагружаем узел для корректного isOnline()
            $node->refresh();
            
            // Устанавливаем online на основе isOnline()
            $node->update(['online' => $node->isOnline()]);

            Log::info("🔍 AUTO-DISCOVERY: New node registered", [
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'mac' => $data['mac'] ?? 'unknown',
                'firmware' => $data['firmware'] ?? 'unknown',
            ]);

            // Создаём событие об обнаружении
            $discoveryEvent = Event::create([
                'node_id' => $nodeId,
                'level' => Event::LEVEL_INFO,
                'message' => "New node auto-discovered and registered: {$nodeId}",
                'data' => [
                    'node_type' => $nodeType,
                    'firmware' => $data['firmware'] ?? null,
                    'hardware' => $data['hardware'] ?? null,
                ],
            ]);

            // Broadcast событие об обнаружении через WebSocket
            event(new \App\Events\EventCreated($discoveryEvent));

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
     * Обработка setup discovery
     */
    private function processSetupDiscovery(array $data, ?string $meshId = null): void
    {
        $mac = strtoupper($data['mac_address']);
        $pin = $data['pin'] ?? $this->generateSetupPin($mac);
        $tempMeshId = $data['temp_mesh_id'] ?? $this->generateTempMeshId($pin);
        $isRoot = ($data['node_type'] ?? 'node') === 'root';

        // Если узел уже зарегистрирован в основной таблице - игнорируем setup discovery
        $nodeId = $data['node_id'] ?? null;
        if ($nodeId && Node::where('node_id', $nodeId)->exists()) {
            NewNode::where('mac_address', $mac)->delete();
            return;
        }

        if (Node::where('mac_address', $mac)->exists()) {
            NewNode::where('mac_address', $mac)->delete();
            return;
        }

        $newNode = NewNode::firstOrNew(['mac_address' => $mac]);
        $wasNew = !$newNode->exists;

        $newNode->mac_address = $mac;
        $newNode->node_type = $data['node_type'] ?? ($isRoot ? 'root' : 'node');
        $newNode->is_root = $isRoot;
        $newNode->pin = $pin;
        $newNode->temp_mesh_id = $tempMeshId;
        $newNode->status = $wasNew ? 'discovered' : $newNode->status;
        $newNode->discovered_at = $newNode->discovered_at ?? now();
        $newNode->last_heartbeat_at = now();

        $metadata = array_filter([
            'chip_model' => $data['chip_model'] ?? $data['chip'] ?? null,
            'firmware_version' => $data['firmware_version'] ?? $data['firmware'] ?? null,
            'ip_address' => $data['ip_address'] ?? $data['ip'] ?? null,
            'rssi' => $data['rssi'] ?? $data['wifi_rssi'] ?? null,
            'temp_mesh_id' => $tempMeshId,
        ]);
        if ($meshId) {
            $metadata['mesh_network_id'] = $meshId;
        }

        $existingMetadata = $newNode->metadata ?? [];
        $newNode->metadata = array_merge($existingMetadata, $metadata);

        $newNode->save();

        if ($wasNew) {
            event(new \App\Events\NewNodeDiscovered($newNode));
        } else {
            event(new \App\Events\NewNodeUpdated($newNode));
        }
    }

    /**
     * Возвращает true, если heartbeat полностью обработан (setup режим)
     * Возвращает false, если нужно продолжить обычную обработку (узел уже сконфигурирован)
     */
    private function processSetupHeartbeat(array $data, ?string $meshId = null): bool
    {
        $mac = strtoupper($data['mac_address']);
        $newNode = NewNode::firstOrNew(['mac_address' => $mac]);

        // Игнорируем heartbeat, если узел уже зарегистрирован в основной таблице
        $nodeId = $data['node_id'] ?? null;
        if ($nodeId && Node::where('node_id', $nodeId)->exists()) {
            NewNode::where('mac_address', $mac)->delete();
            return false;
        }

        if (Node::where('mac_address', $mac)->exists()) {
            NewNode::where('mac_address', $mac)->delete();
            return false;
        }

        if (!$newNode->exists) {
            // Если heartbeat пришёл впервые без discovery, инициируем базовую запись
            $newNode->node_type = $data['node_type'] ?? 'node';
            $newNode->is_root = ($data['node_type'] ?? null) === 'root';
            $newNode->pin = $this->generateSetupPin($mac);
            $newNode->temp_mesh_id = $this->generateTempMeshId($newNode->pin);
            $newNode->discovered_at = now();
            $newNode->status = 'discovered';
        }

        $metadata = array_filter([
            'firmware_version' => $data['firmware_version'] ?? $data['firmware'] ?? null,
            'chip_model' => $data['chip_model'] ?? null,
            'rssi' => $data['rssi'] ?? $data['wifi_rssi'] ?? null,
            'uptime' => $data['uptime'] ?? null,
        ]);
        if ($meshId) {
            $metadata['mesh_network_id'] = $meshId;
        }

        $existingMetadata = $newNode->metadata ?? [];
        $newNode->metadata = array_merge($existingMetadata, $metadata);
        $newNode->last_heartbeat_at = now();
        $newNode->save();

        event(new \App\Events\NewNodeUpdated($newNode));

        return true;
    }

    private function processSetupConfigConfirmation(array $data, ?string $meshId = null): void
    {
        $mac = strtoupper($data['mac_address'] ?? ($data['mac'] ?? ''));
        if (!$mac) {
            return;
        }

        $newNode = NewNode::where('mac_address', $mac)->first();
        if (!$newNode) {
            return;
        }

        $status = $data['status'] ?? 'success';
        if ($status !== 'success') {
            $newNode->update(['status' => 'failed']);
            event(new \App\Events\NewNodeUpdated($newNode));
            return;
        }

        $nodeId = $data['node_id'] ?? null;
        if (!$nodeId) {
            return;
        }

        $attributes = [
            'zone' => $data['zone'] ?? $meshId,
            'root_node_id' => $data['root_node_id'] ?? null,
            'metadata' => array_filter([
                'mesh_id' => $data['mesh_id'] ?? $meshId,
                'mqtt_topic_prefix' => $data['mqtt_topic_prefix'] ?? null,
                'configured_at' => now()->toIso8601String(),
            ]),
            'config' => isset($data['config']) && is_array($data['config']) ? $data['config'] : null,
        ];

        $node = $newNode->confirmConfiguration($nodeId, $attributes);
        event(new \App\Events\NewNodeConfigured($nodeId, $mac));
        event(new \App\Events\NodeDiscovered($node));
    }

    private function generateSetupPin(string $mac): string
    {
        $hex = strtoupper(str_replace(':', '', $mac));
        return substr(str_pad($hex, 6, '0', STR_PAD_LEFT), -6);
    }

    private function generateTempMeshId(string $pin): string
    {
        return 'HYDRO_' . $pin;
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
        $node = Node::where('node_id', $nodeId)->first();
        if (!$node) {
            throw new Exception("Node not found: {$nodeId}");
        }

        $meshId = $node->zone ?? $node->metadata['mesh_network_id'] ?? null;
        if (!$this->validateZone($meshId)) {
            throw new Exception("Node {$nodeId} has invalid zone: {$meshId}");
        }

        $payload = json_encode([
            'type' => 'command',
            'command_id' => $commandId,
            'node_id' => $nodeId,
            'mesh_network_id' => $meshId,
            'root_node_id' => $node->root_node_id,
            'command' => $command,
            'params' => $params,
            'timestamp' => time(),
        ]);

        $topic = "hydro/{$meshId}/command/{$nodeId}";
        $this->publish($topic, $payload, 1);
        
        Log::info("Command sent", [
            'node_id' => $nodeId,
            'command' => $command,
            'command_id' => $commandId,
            'zone' => $meshId,
        ]);
    }

    /**
     * Отправка конфигурации узлу
     */
    public function sendConfig(string $nodeId, array $config): void
    {
        $node = Node::where('node_id', $nodeId)->first();
        if (!$node) {
            throw new Exception("Node not found: {$nodeId}");
        }

        $meshId = $node->zone ?? $node->metadata['mesh_network_id'] ?? null;
        if (!$this->validateZone($meshId)) {
            throw new Exception("Node {$nodeId} has invalid zone: {$meshId}");
        }

        $payload = json_encode([
            'type' => 'config',
            'node_id' => $nodeId,
            'mesh_network_id' => $meshId,
            'root_node_id' => $node->root_node_id,
            'config' => $config,
            'timestamp' => time(),
        ]);

        $topic = "hydro/{$meshId}/config/{$nodeId}";
        $this->publish($topic, $payload, 1);
        
        Log::info("Config sent", [
            'node_id' => $nodeId,
            'zone' => $meshId,
        ]);
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
            if (config('telegram.enabled', false)) {
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

            $meshId = $this->resolveZone($topic, $data, 'error', $data['node_id']);
            if (!$meshId) {
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
                $errorEvent = Event::create([
                    'node_id' => $error->node_id,
                    'level' => Event::LEVEL_CRITICAL,
                    'message' => "Critical error: {$error->message}",
                    'data' => [
                        'error_code' => $error->error_code,
                        'error_type' => $error->error_type,
                        'diagnostics' => $error->diagnostics,
                    ],
                ]);

                // Broadcast событие через WebSocket
                event(new \App\Events\EventCreated($errorEvent));

                // Отправка уведомлений для критичных ошибок
                $this->sendNotifications($errorEvent);
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
            $meshId = $this->resolveZone($topic, $data, 'config_response', $nodeId);
            if (!$meshId) {
                return;
            }
            
            // Проверка что config - это массив
            if (!is_array($config)) {
                Log::warning("Config is not an array", [
                    'node_id' => $nodeId,
                    'config_type' => gettype($config)
                ]);
                return;
            }

            // ⭐ GROWTH PLANNER: Обработка подтверждения конфигурации
            $confirmationId = $data['_confirmation_id'] ?? $config['_confirmation_id'] ?? null;
            
            if ($confirmationId) {
                // Удаляем служебное поле из конфигурации
                unset($config['_confirmation_id']);
                
                $configService = app(\App\Services\NodeConfigurationService::class);
                $confirmation = $configService->handleConfigurationConfirmation(
                    $nodeId,
                    $config,
                    $confirmationId
                );

                if ($confirmation) {
                    Log::info("📋 Configuration confirmation processed", [
                        'node_id' => $nodeId,
                        'confirmation_id' => $confirmationId,
                        'cycle_id' => $confirmation->cycle_id,
                    ]);
                }
            }
            
            Log::info("📋 Config response received", [
                'node_id' => $nodeId,
                'config_keys' => array_keys($config),
                'confirmation_id' => $confirmationId
            ]);
            
            // Сохранение конфигурации в кэш (1 час)
            Cache::put("node_config:{$nodeId}", $config, 3600);
            
            // Обновление узла в БД
            $node = Node::where('node_id', $nodeId)->first();
            if ($node) {
                $metadata = $node->metadata ?? [];
                $metadata['mesh_network_id'] = $meshId;
                $node->update([
                    'config' => $config,
                    'last_seen_at' => now(),
                    'zone' => $meshId,
                    'metadata' => $metadata,
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
        
        // Округляем дозу до 1 знака после точки
        $doseFormatted = number_format($dose, 1, '.', '');
        
        // Определяем название насоса (pH down/pH up вместо #1)
        $pumpName = $this->getPumpName($pumpId, $data);
        
        // Для pH нод: "насос pH down/pH up", для других: "Насос {название}"
        $pumpLabel = ($data['node_type'] === 'ph' && ($pumpId === 0 || $pumpId === 1)) 
            ? "насос {$pumpName}" 
            : "Насос {$pumpName}";
        
        switch ($eventType) {
            case 'pump_start':
                return "🚰 {$pumpLabel} запущен: {$doseFormatted} мл ({$duration} мс)";
            case 'pump_stop':
                return "🛑 {$pumpLabel} остановлен: {$doseFormatted} мл ({$duration} мс)";
            case 'pump_emergency_stop':
                return "🚨 Аварийная остановка {$pumpLabel}";
            case 'pump_timeout':
                return "⏰ Таймаут {$pumpLabel}";
            case 'pump_calibration_start':
                return "🔧 Начало калибровки {$pumpLabel}";
            case 'pump_calibration_end':
                return "✅ Калибровка {$pumpLabel} завершена";
            default:
                return "🔧 Событие {$pumpLabel}: {$eventType}";
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
                    return 'pH up';
                case 1:
                    return 'pH down';
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
                    return 'pH up';
                case 1:
                    return 'pH down';
                case 2:
                    return 'EC A';
                case 3:
                    return 'EC B';
                case 4:
                    return 'EC C';
                default:
                    return "#{$pumpId}";
            }
        }
        
        // Для неизвестных типов узлов - возвращаем только номер
        return "#{$pumpId}";
    }

    /**
     * Пытается определить mesh/zone из топика/пейлоада и валидирует её.
     * Возвращает null если зону определить нельзя (сообщение нужно отклонить).
     */
    private function resolveZone(string $topic, array $data, string $context, ?string $nodeId = null): ?string
    {
        $zoneFromTopic = $this->extractZoneFromTopic($topic);

        if ($zoneFromTopic && $this->validateZone($zoneFromTopic)) {
            return $zoneFromTopic;
        }

        $payloadZone = $data['mesh_network_id']
            ?? $data['mesh_id']
            ?? $data['zone']
            ?? null;

        if ($payloadZone && $this->validateZone($payloadZone)) {
            return $payloadZone;
        }

        Log::error("MQTT {$context} message rejected: missing valid zone", [
            'topic' => $topic,
            'node_id' => $nodeId,
            'payload_zone' => $payloadZone ?? 'missing',
        ]);

        if ($context === 'heartbeat' && $nodeId) {
            Event::create([
                'node_id' => $nodeId,
                'level' => Event::LEVEL_CRITICAL,
                'message' => "Node sent {$context} without valid zone - REJECTED",
                'data' => [
                    'topic' => $topic,
                    'payload_zone' => $payloadZone ?? 'missing',
                ],
            ]);
        }

        return null;
    }

    /**
     * Проверяет, что строка зоны соответствует требованиям.
     */
    private function validateZone(?string $zone): bool
    {
        if (!$zone) {
            return false;
        }

        if (strtolower($zone) === 'setup' || strtoupper($zone) === 'UNCONFIGURED') {
            return false;
        }

        $length = strlen($zone);
        if ($length < 3 || $length > 31) {
            return false;
        }

        return (bool) preg_match('/^[a-zA-Z0-9_-]+$/', $zone);
    }

    /**
     * Извлечение mesh/zone из MQTT-топика формата hydro/{mesh_id}/...
     */
    private function extractZoneFromTopic(string $topic): ?string
    {
        $parts = explode('/', $topic);
        if (count($parts) >= 3 && $parts[0] === 'hydro') {
            $zone = $parts[1];
            if ($zone === 'setup') {
                return null;
            }

            return $this->validateZone($zone) ? $zone : null;
        }

        return null;
    }
}

