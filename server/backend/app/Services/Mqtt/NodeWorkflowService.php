<?php

namespace App\Services\Mqtt;

use App\Events\EventCreated;
use App\Events\NewNodeConfigured;
use App\Events\NewNodeDiscovered;
use App\Events\NewNodeUpdated;
use App\Events\NodeConfigUpdated;
use App\Events\NodeDiscovered;
use App\Events\NodeStatusChanged;
use App\Events\NodeStatusUpdate;
use App\Events\TelemetryReceived;
use App\Models\Event;
use App\Models\Command;
use App\Models\NewNode;
use App\Models\Node;
use App\Models\NodeError;
use App\Models\PumpCalibration;
use App\Models\Telemetry;
use App\Services\Mqtt\DTO\MqttMessageContext;
use App\Services\Mqtt\Enum\MqttMessageType;
use App\Services\NodeConfigurationService;
use App\Services\Nodes\Contracts\NodeZoneResolverInterface;
use App\Services\Nodes\Domain\Contracts\NodeStatusServiceInterface;
use App\Services\NotificationThrottleService;
use App\Services\SmsService;
use App\Services\TelegramService;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Support\Arr;
use Illuminate\Support\Facades\Cache;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Collection;
use Throwable;

class NodeWorkflowService
{
    private array $deprecatedWarnings = [];

    public function __construct(
        private readonly NotificationThrottleService $throttleService,
        private readonly NodeConfigurationService $configurationService,
        private readonly NodeZoneResolverInterface $zoneResolver,
        private readonly NodeStatusServiceInterface $nodeStatusService,
        private readonly ?TelegramService $telegramService = null,
        private readonly ?SmsService $smsService = null,
    ) {
    }

    public function handle(MqttMessageContext $context): void
    {
        match ($context->type) {
            MqttMessageType::Telemetry => $this->handleTelemetry($context),
            MqttMessageType::Heartbeat => $this->handleHeartbeat($context),
            MqttMessageType::Event => $this->handleEvent($context),
            MqttMessageType::Discovery => $this->handleDiscovery($context),
            MqttMessageType::ConfigResponse => $this->handleConfigResponse($context),
            MqttMessageType::Error => $this->handleError($context),
            MqttMessageType::SetupDiscovery => $this->handleSetupDiscovery($context),
            MqttMessageType::SetupHeartbeat => $this->handleSetupHeartbeat($context),
            default => Log::warning('Unhandled MQTT message type', [
                'type' => $context->type->value,
                'topic' => $context->topic,
            ]),
        };
    }

    public function handleTelemetry(MqttMessageContext $context): void
    {
        try {
            $payload = collect($context->json() ?? []);
            if ($payload->isEmpty() || !$payload->has('node_id')) {
                Log::warning('Telemetry payload without node_id', ['topic' => $context->topic]);
                return;
            }

            $nodeId = (string) $payload->get('node_id');

            $meshId = $this->zoneResolver->resolve($context->topic, $payload, 'telemetry', $nodeId);
            if (!$meshId) {
                return;
            }

            $nodeType = $this->sanitizeNodeType(
                $payload->get('node_type') ?? $payload->get('type') ?? 'unknown',
                $nodeId
            );

            $telemetry = Telemetry::create([
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'data' => $payload->get('data', []),
                'received_at' => now(),
            ]);

            event(new TelemetryReceived($telemetry));

            $payload->put('node_type', $nodeType);
            $node = Node::firstOrNew(['node_id' => $nodeId]);

            $this->nodeStatusService->handleTelemetry($node, $payload, $meshId);

            Log::debug('Telemetry saved', [
                'node_id' => $nodeId,
                'type' => $nodeType,
                'mesh_network_id' => $meshId,
            ]);
        } catch (Throwable $e) {
            Log::error('Telemetry handling error', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
            ]);
        }
    }

    public function handleEvent(MqttMessageContext $context): void
    {
        try {
            $payload = collect($context->json() ?? []);
            if ($payload->isEmpty() || !$payload->has('node_id')) {
                Log::warning('Invalid event data', [
                    'topic' => $context->topic,
                    'payload' => $context->payload,
                ]);
                return;
            }

            $nodeId = (string) $payload->get('node_id');

            $meshId = $this->zoneResolver->resolve($context->topic, $payload, 'event', $nodeId);
            if (!$meshId) {
                return;
            }

            $message = $this->translateEventMessage($payload->get('message', 'Unknown event'));
            $level = $payload->get('level', Event::LEVEL_INFO);

            $node = Node::where('node_id', $nodeId)->first();
            if ($node) {
                $meta = $node->metadata ?? [];
                if (($meta['mesh_network_id'] ?? null) !== $meshId || $node->zoneCode() !== $meshId) {
                    $meta['mesh_network_id'] = $meshId;
                    $node->update(['metadata' => $meta, 'zone' => $meshId]);
                    $node->refresh();
                }
            }
            $nodeType = $node?->node_type ?? $payload->get('node_type', 'unknown');

            $eventData = (array) $payload->get('data', []);
            $eventData['node_type'] = $nodeType;
            $eventData['mesh_network_id'] = $meshId;

            $event = Event::create([
                'node_id' => $nodeId,
                'level' => $level,
                'message' => $message,
                'data' => $eventData,
            ]);

            Log::info('Event saved', [
                'node_id' => $event->node_id,
                'level' => $event->level,
                'message' => $event->message,
            ]);

            event(new EventCreated($event));

            if ($event->isCritical()) {
                $this->sendNotifications($event);
            }
        } catch (Throwable $e) {
            Log::error('Event handling error', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
            ]);
        }
    }

    public function handleHeartbeat(MqttMessageContext $context): void
    {
        try {
            $payload = collect($context->json() ?? []);
            if ($payload->isEmpty()) {
                return;
            }

            if ($payload->get('type') === 'heartbeat' && $payload->has('mac_address')) {
                $handledBySetup = $this->processSetupHeartbeatPayload($context->topic, $payload->toArray());
                if ($handledBySetup) {
                    return;
                }
            }

            if (!$payload->has('node_id')) {
                return;
            }

            $nodeId = (string) $payload->get('node_id');
            $meshId = $this->zoneResolver->resolve($context->topic, $payload, 'heartbeat', $nodeId);
            if (!$meshId) {
                return;
            }

            $node = Node::firstOrNew(['node_id' => $nodeId]);
            $detectedType = $this->detectNodeType($nodeId, $data);
            $nodeType = $this->sanitizeNodeType(
                $payload->get('node_type', $detectedType),
                $nodeId
            );

            $payload->put('node_type', $nodeType);

            if (!$node->exists) {
                $node->node_type = $nodeType;
            }

            $this->nodeStatusService->handleHeartbeat($node, $payload, $meshId);

            Log::debug('Heartbeat received', ['node_id' => $nodeId]);
        } catch (Throwable $e) {
            Log::error('Heartbeat handling error', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
            ]);
        }
    }

    public function handleDiscovery(MqttMessageContext $context): void
    {
        try {
            Log::info('handleDiscovery called', [
                'topic' => $context->topic,
                'payload_length' => strlen($context->payload),
                'payload' => $context->payload,
            ]);

            $payload = collect($context->json() ?? []);
            $data = $payload->toArray();
            $isSetupTopic = str_starts_with($context->topic, 'hydro/setup/');
            $meshId = null;

            if ($payload->isEmpty()) {
                Log::warning('Invalid discovery data', [
                    'topic' => $context->topic,
                    'payload' => $context->payload,
                    'json_error' => json_last_error_msg(),
                ]);
                return;
            }

            if (!$isSetupTopic) {
                $meshId = $this->zoneResolver->resolve(
                    $context->topic,
                    $payload,
                    'discovery',
                    $payload->get('node_id')
                );
                if (!$meshId) {
                    return;
                }
            }

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
                Log::warning('Invalid discovery data', [
                    'topic' => $context->topic,
                    'payload' => $context->payload,
                    'json_error' => json_last_error_msg(),
                ]);
                return;
            }

            $nodeId = $data['node_id'];
            $existingNode = Node::where('node_id', $nodeId)->first();

            if ($existingNode) {
                Log::info('Discovery: Node already registered', ['node_id' => $nodeId]);

                $metadata = $existingNode->metadata ?? [];
                $metadata['last_discovery'] = now()->toIso8601String();
                if ($meshId) {
                    $metadata['mesh_network_id'] = $meshId;
                }
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
                    'last_seen_at' => now(),
                    'zone' => $meshId,
                ];

                if (isset($data['mac_address']) || isset($data['mac'])) {
                    $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
                }

                if (!empty($data['root_node_id'])) {
                    $updateData['root_node_id'] = $data['root_node_id'];
                } elseif ($existingNode->node_type === 'root') {
                    $updateData['root_node_id'] = $existingNode->node_id;
                }

                $existingNode->update($updateData);

                $wasOnline = $existingNode->online;
                $isOnline = $existingNode->isOnline();

                if ($wasOnline !== $isOnline) {
                    $existingNode->update(['online' => $isOnline]);
                    event(new NodeStatusChanged($existingNode, $wasOnline, $isOnline));

                    if ($wasOnline && !$isOnline) {
                        $statusEvent = Event::create([
                            'node_id' => $existingNode->node_id,
                            'level' => Event::LEVEL_WARNING,
                            'message' => "Узел {$existingNode->node_id} перешёл в офлайн",
                            'data' => [
                                'last_seen' => $existingNode->last_seen_at?->toDateTimeString(),
                                'node_type' => $existingNode->node_type,
                                'zone' => $existingNode->zoneCode(),
                            ],
                        ]);

                        event(new EventCreated($statusEvent));
                    }
                }

                return;
            }

            $nodeType = $this->sanitizeNodeType($data['node_type'] ?? $this->detectNodeType($nodeId, $data), $nodeId);

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
                'mesh_network_id' => $meshId,
            ];
            if (isset($data['mqtt_topic_prefix'])) {
                $metadata['mqtt_topic_prefix'] = $data['mqtt_topic_prefix'];
            }
            if ($nodeType === 'deprecated_ph_ec') {
                $metadata['deprecated'] = true;
                $metadata['deprecated_reason'] = 'combined_ph_ec_node';
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

            $node->refresh();
            $node->update(['online' => $node->isOnline()]);

            Log::info('AUTO-DISCOVERY: New node registered', [
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'mac' => $data['mac'] ?? 'unknown',
                'firmware' => $data['firmware'] ?? 'unknown',
            ]);

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

            event(new EventCreated($discoveryEvent));
            event(new NodeDiscovered($node));

            Log::info('Node discovery complete', ['node_id' => $nodeId]);
        } catch (Throwable $e) {
            Log::error('Discovery handling error', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
            ]);
        }
    }

    public function handleConfigResponse(MqttMessageContext $context): void
    {
        try {
            Log::info('handleConfigResponse called', [
                'topic' => $context->topic,
                'payload_length' => strlen($context->payload),
            ]);

            $payload = collect($context->json() ?? []);
            $data = $payload->toArray();

            if ($payload->isEmpty() || !$payload->has('node_id') || !$payload->has('config')) {
                Log::warning('Invalid config_response data', [
                    'topic' => $context->topic,
                    'payload' => $context->payload,
                    'json_error' => json_last_error_msg(),
                ]);
                return;
            }

            $nodeId = (string) $payload->get('node_id');
            $meshId = $this->zoneResolver->resolve($context->topic, $payload, 'config_response', $nodeId);
            if (!$meshId) {
                return;
            }

            if (!is_array($data['config'])) {
                Log::warning('Config is not an array', [
                    'node_id' => $nodeId,
                    'config_type' => gettype($data['config']),
                ]);
                return;
            }

            $confirmationId = $data['_confirmation_id'] ?? $data['config']['_confirmation_id'] ?? null;

            $config = $data['config'];
            if (isset($config['_confirmation_id'])) {
                unset($config['_confirmation_id']);
            }

            if ($confirmationId) {
                $confirmation = $this->configurationService->handleConfigurationConfirmation(
                    $nodeId,
                    $config,
                    $confirmationId
                );

                if ($confirmation) {
                    Log::info('Configuration confirmation processed', [
                        'node_id' => $nodeId,
                        'confirmation_id' => $confirmationId,
                        'cycle_id' => $confirmation->cycle_id,
                    ]);
                }
            }

            Log::info('Config response received', [
                'node_id' => $nodeId,
                'config_keys' => array_keys($config),
                'confirmation_id' => $confirmationId,
            ]);

            Cache::put("node_config:{$nodeId}", $config, 3600);

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

                Log::info('Node config updated in DB', ['node_id' => $nodeId]);
            }

            if (isset($config['pumps_calibration']) && is_array($config['pumps_calibration'])) {
                foreach ($config['pumps_calibration'] as $pumpCal) {
                    if (isset($pumpCal['pump_id'])) {
                        PumpCalibration::updateOrCreate(
                            [
                                'node_id' => $nodeId,
                                'pump_id' => $pumpCal['pump_id'],
                            ],
                            [
                                'ml_per_second' => $pumpCal['ml_per_second'] ?? 1.0,
                                'calibration_volume_ml' => $pumpCal['calibration_volume_ml'] ?? null,
                                'calibration_time_ms' => $pumpCal['calibration_time_ms'] ?? null,
                                'is_calibrated' => $pumpCal['is_calibrated'] ?? false,
                                'calibrated_at' => isset($pumpCal['last_calibrated']) && $pumpCal['last_calibrated'] > 0
                                    ? now()->setTimestamp($pumpCal['last_calibrated'])
                                    : null,
                            ]
                        );
                    }
                }

                Log::info('Pump calibrations saved', [
                    'node_id' => $nodeId,
                    'pumps_count' => count($config['pumps_calibration']),
                ]);
            }

            broadcast(new NodeConfigUpdated($nodeId, $config));

            Log::info('Config response processed successfully', ['node_id' => $nodeId]);
        } catch (Throwable $e) {
            Log::error('Error handling config_response', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
                'trace' => $e->getTraceAsString(),
            ]);
        }
    }

    public function handleError(MqttMessageContext $context): void
    {
        try {
            $payload = collect($context->json() ?? []);

            if ($payload->isEmpty() || !$payload->has('node_id')) {
                Log::warning('Invalid error data', [
                    'topic' => $context->topic,
                    'payload' => $context->payload,
                ]);
                return;
            }

            $nodeId = (string) $payload->get('node_id');
            $meshId = $this->zoneResolver->resolve($context->topic, $payload, 'error', $nodeId);
            if (!$meshId) {
                return;
            }

            $error = NodeError::create([
                'node_id' => $nodeId,
                'error_code' => $payload->get('error_code', 'UNKNOWN_ERROR'),
                'error_type' => $payload->get('error_type', NodeError::TYPE_SOFTWARE),
                'severity' => $payload->get('severity', NodeError::SEVERITY_MEDIUM),
                'message' => $payload->get('message', 'Unknown error occurred'),
                'stack_trace' => $payload->get('stack_trace'),
                'diagnostics' => $payload->get('diagnostics', []),
                'occurred_at' => $payload->has('timestamp')
                    ? now()->setTimestamp((int) $payload->get('timestamp'))
                    : now(),
            ]);

            Log::error('Node error occurred', [
                'node_id' => $error->node_id,
                'error_code' => $error->error_code,
                'severity' => $error->severity,
                'message' => $error->message,
            ]);

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

                event(new EventCreated($errorEvent));
                $this->sendNotifications($errorEvent);
            }

            event(new \App\Events\ErrorOccurred($error));
        } catch (Throwable $e) {
            Log::error('Error handling error message', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
            ]);
        }
    }

    public function handleSetupDiscovery(MqttMessageContext $context): void
    {
        $data = $context->json();
        if (!$data) {
            Log::warning('Setup discovery: invalid JSON', ['topic' => $context->topic]);
            return;
        }

        $this->processSetupDiscovery($data, null);
    }

    public function handleSetupHeartbeat(MqttMessageContext $context): void
    {
        $data = $context->json();
        if (!$data) {
            return;
        }

        $this->processSetupHeartbeatPayload($context->topic, $data);
    }

    public function handleCommandResponse(MqttMessageContext $context): void
    {
        try {
            $data = $context->json();
            if (!$data || !isset($data['command_id'])) {
                Log::warning('Command response without command_id', [
                    'topic' => $context->topic,
                    'payload' => $context->payload,
                ]);
                return;
            }

            $command = Command::find($data['command_id']);
            if (!$command) {
                Log::warning('Command response referencing unknown command', [
                    'command_id' => $data['command_id'],
                ]);
                return;
            }

            $status = strtolower($data['status'] ?? '');

            match ($status) {
                'completed' => $command->markAsCompleted($data['response'] ?? []),
                'acknowledged' => $command->markAsAcknowledged(),
                'failed' => $command->markAsFailed($data['error'] ?? 'Unknown error'),
                default => Log::warning('Command response with unexpected status', [
                    'command_id' => $command->id,
                    'status' => $data['status'] ?? 'missing',
                ]),
            };

            Log::info('Command response processed', [
                'command_id' => $command->id,
                'status' => $status,
            ]);
        } catch (Throwable $e) {
            Log::error('Command response handling error', [
                'topic' => $context->topic,
                'error' => $e->getMessage(),
            ]);
        }
    }

    private function processSetupHeartbeatPayload(string $topic, array $data): bool
    {
        $mac = strtoupper($data['mac_address']);
        $newNode = NewNode::firstOrNew(['mac_address' => $mac]);

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
            $rawType = $data['node_type'] ?? 'node';
            $sanitizedType = $this->sanitizeNodeType($rawType, $nodeId);
            $newNode->node_type = $sanitizedType;
            $newNode->is_root = $sanitizedType === 'root';
            $newNode->pin = $this->generateSetupPin($mac);
            $newNode->temp_mesh_id = $this->generateTempMeshId($newNode->pin);
            $newNode->discovered_at = now();
            $newNode->status = 'discovered';
        } else {
            if (!empty($data['node_type'])) {
                $newNode->node_type = $this->sanitizeNodeType($data['node_type'], $nodeId);
            }
            $newNode->is_root = $newNode->node_type === 'root';
        }

        $metadata = array_filter([
            'firmware_version' => $data['firmware_version'] ?? $data['firmware'] ?? null,
            'chip_model' => $data['chip_model'] ?? null,
            'rssi' => $data['rssi'] ?? $data['wifi_rssi'] ?? null,
            'uptime' => $data['uptime'] ?? null,
            'pairing_mesh_tag' => $data['pairing_mesh_tag'] ?? null,
        ]);

        if ($newNode->node_type === 'deprecated_ph_ec') {
            $metadata['deprecated'] = true;
            $metadata['deprecated_reason'] = 'combined_ph_ec_node';
        }

        $existingMetadata = $newNode->metadata ?? [];
        $newNode->metadata = array_merge($existingMetadata, $metadata);
        $newNode->last_heartbeat_at = now();
        $newNode->save();

        event(new NewNodeUpdated($newNode));

        return true;
    }

    private function processSetupDiscovery(array $data, ?string $meshId = null): void
    {
        $mac = strtoupper($data['mac_address']);
        $pin = $data['pin'] ?? $this->generateSetupPin($mac);
        $tempMeshId = $data['temp_mesh_id'] ?? $this->generateTempMeshId($pin);
        $isRoot = ($data['node_type'] ?? 'node') === 'root';

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

        $newNodeType = $data['node_type'] ?? ($isRoot ? 'root' : 'node');
        $newNode->mac_address = $mac;
        $newNode->node_type = $this->sanitizeNodeType($newNodeType, $data['node_id'] ?? null);
        $newNode->is_root = $newNode->node_type === 'root';
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
            'pairing_mesh_tag' => $data['pairing_mesh_tag'] ?? null,
        ]);
        if ($meshId) {
            $metadata['mesh_network_id'] = $meshId;
        }
        if ($newNode->node_type === 'deprecated_ph_ec') {
            $metadata['deprecated'] = true;
            $metadata['deprecated_reason'] = 'combined_ph_ec_node';
        }

        $existingMetadata = $newNode->metadata ?? [];
        $newNode->metadata = array_merge($existingMetadata, $metadata);

        $newNode->save();

        if ($wasNew) {
            event(new NewNodeDiscovered($newNode));
        } else {
            event(new NewNodeUpdated($newNode));
        }
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
            event(new NewNodeUpdated($newNode));
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
        event(new NewNodeConfigured($nodeId, $mac));
        event(new NodeDiscovered($node));
    }

    private function detectNodeType(string $nodeId, array $data): string
    {
        if (str_starts_with($nodeId, 'root_')) {
            return 'root';
        }
        if (str_starts_with($nodeId, 'climate_')) {
            return 'climate';
        }
        if (str_starts_with($nodeId, 'ph_ec_')) {
            return $this->sanitizeNodeType('ph_ec', $nodeId);
        }
        if (str_starts_with($nodeId, 'ph_')) {
            return 'ph';
        }
        if (str_starts_with($nodeId, 'ec_')) {
            return 'ec';
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

        if (isset($data['node_type'])) {
            return $this->sanitizeNodeType($data['node_type'], $nodeId);
        }

        if (isset($data['sensors'])) {
            $sensors = $data['sensors'];
            if (in_array('ph', $sensors) && !in_array('ec', $sensors)) {
                return 'ph';
            }
            if (in_array('ec', $sensors) && !in_array('ph', $sensors)) {
                return 'ec';
            }
            if (in_array('ph', $sensors) && in_array('ec', $sensors)) {
                return $this->sanitizeNodeType('ph_ec', $nodeId);
            }
            if (in_array('temperature', $sensors) || in_array('humidity', $sensors)) {
                return 'climate';
            }
        }

        return 'unknown';
    }

    private function sanitizeNodeType(?string $nodeType, ?string $nodeId = null): string
    {
        if ($nodeType === null) {
            return 'unknown';
        }

        if ($nodeType === 'ph_ec') {
            $this->notifyDeprecatedPhEc($nodeId);
            return 'deprecated_ph_ec';
        }

        return $nodeType;
    }

    private function notifyDeprecatedPhEc(?string $nodeId): void
    {
        $normalizedId = $nodeId ?? 'unknown_ph_ec_node';

        if (isset($this->deprecatedWarnings[$normalizedId])) {
            return;
        }

        $this->deprecatedWarnings[$normalizedId] = true;

        Log::warning('Обнаружен устаревший узел комбинированного типа ph_ec', [
            'node_id' => $normalizedId,
        ]);

        if ($nodeId) {
            try {
                Event::create([
                    'node_id' => $nodeId,
                    'level' => Event::LEVEL_WARNING,
                    'message' => 'Узел типа ph_ec больше не поддерживается. Разделите на отдельные pH и EC узлы.',
                    'data' => [
                        'node_type' => 'ph_ec',
                        'deprecated' => true,
                    ],
                ]);
            } catch (Throwable $eventError) {
                Log::error('Не удалось создать событие о депрекейте ph_ec', [
                    'node_id' => $nodeId,
                    'error' => $eventError->getMessage(),
                ]);
            }
        }
    }

    private function translateEventMessage(string $message): string
    {
        return match ($message) {
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
            default => $message,
        };
    }

    private function sendNotifications(Event $event): void
    {
        try {
            $eventType = $this->mapEventLevelToType($event->level);
            $message = "Event: {$event->message} (Node: {$event->node_id})";

            if (!$this->throttleService->canSendNotification($eventType, $event->node_id, $message)) {
                Log::debug('Notification throttled', [
                    'event_id' => $event->id,
                    'node_id' => $event->node_id,
                    'level' => $event->level,
                    'type' => $eventType,
                ]);
                return;
            }

            if (config('telegram.enabled', false) && $this->telegramService) {
                $this->telegramService->sendAlert($event);
            }

            if (config('sms.enabled', false) && $event->isCritical() && $this->smsService) {
                $this->smsService->sendAlert($event);
            }

            $this->throttleService->markNotificationSent($eventType, $event->node_id, $message);
        } catch (Throwable $e) {
            Log::error('Notification sending error', [
                'event_id' => $event->id,
                'error' => $e->getMessage(),
            ]);
        }
    }

    private function mapEventLevelToType(string $level): string
    {
        return match ($level) {
            'critical', 'emergency' => 'critical',
            'warning', 'error' => 'warning',
            default => 'info',
        };
    }

    private function mapErrorSeverityToType(string $severity): string
    {
        return match ($severity) {
            'critical' => 'critical',
            'high', 'medium' => 'warning',
            default => 'info',
        };
    }

    private function generateSetupPin(string $mac): string
    {
        $hex = strtoupper(str_replace(':', '', $mac));
        return substr(str_pad($hex, 6, '0', STR_PAD_LEFT), -6);
    }

    private function generateTempMeshId(string $pin): string
    {
        return 'ROOT_PAIR_' . $pin;
    }
}

