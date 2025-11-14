<?php

namespace App\Services\Nodes\Domain;

use App\Events\EventCreated;
use App\Events\NodeDiscovered;
use App\Events\NodeStatusChanged;
use App\Events\NodeStatusUpdate;
use App\Models\Event;
use App\Models\Node;
use App\Services\Nodes\Domain\Contracts\NodeStatusServiceInterface;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\Log;

class NodeStatusService implements NodeStatusServiceInterface
{
    public function handleTelemetry(Node $node, Collection $payload, string $meshId): void
    {
        $wasNew = !$node->exists;
        $wasOnline = (bool) $node->online;

        $nodeType = (string) $payload->get('node_type', $node->node_type ?? 'unknown');
        if ($wasNew) {
            $node->zone = $meshId;
        }

        $metadata = $this->mergeMetadata($node->metadata ?? [], $payload, $meshId, $nodeType, $wasNew);

        $update = [
            'last_seen_at' => now(),
            'metadata' => $metadata,
            'zone' => $meshId,
        ];

        if ($payload->has('root_node_id')) {
            $update['root_node_id'] = $payload->get('root_node_id');
        } elseif ($nodeType === 'root') {
            $update['root_node_id'] = $node->node_id;
        }

        if ($payload->has('mac_address') || $payload->has('mac')) {
            $update['mac_address'] = $payload->get('mac_address') ?? $payload->get('mac');
        }

        $node->fill($update);
        $node->node_type = $nodeType;

        $node->save();
        $node->refresh();

        $this->updateOnlineStatus($node, $wasOnline, $meshId, $wasNew);

        if ($wasNew) {
            $this->logNewNode($node, 'New node auto-registered via MQTT');
        }
    }

    public function handleOffline(Node $node, string $meshId): void
    {
        $event = Event::create([
            'node_id' => $node->node_id,
            'level' => Event::LEVEL_WARNING,
            'message' => "Узел {$node->node_id} перешёл в офлайн",
            'data' => [
                'last_seen' => $node->last_seen_at?->toDateTimeString(),
                'node_type' => $node->node_type,
                'zone' => $node->zoneCode() ?? $meshId,
            ],
        ]);

        event(new EventCreated($event));
    }

    public function handleHeartbeat(Node $node, Collection $payload, string $meshId): void
    {
        $wasNew = !$node->exists;
        $wasOnline = (bool) $node->online;

        if ($wasNew) {
            $node->zone = $meshId;
        }

        $metadata = $this->mergeMetadata($node->metadata ?? [], $payload, $meshId, $node->node_type ?? 'unknown', $wasNew);

        $update = [
            'last_seen_at' => now(),
            'metadata' => $metadata,
            'zone' => $meshId,
        ];

        if ($payload->has('root_node_id')) {
            $update['root_node_id'] = $payload->get('root_node_id');
        } elseif ($node->node_type === 'root') {
            $update['root_node_id'] = $node->node_id;
        }

        if ($payload->has('mac_address') || $payload->has('mac')) {
            $update['mac_address'] = $payload->get('mac_address') ?? $payload->get('mac');
        }

        $node->update($update);
        $node->refresh();

        $this->updateOnlineStatus($node, $wasOnline, $meshId, $wasNew);

        if ($wasNew) {
            $this->createDiscoveryEvent($node, $meshId);
        }
    }

    private function mergeMetadata(array $existing, Collection $payload, string $meshId, string $nodeType, bool $wasNew): array
    {
        $metadata = $existing;

        foreach (['firmware', 'hardware', 'ip', 'heap_free', 'rssi_to_parent', 'uptime', 'sensors', 'capabilities'] as $key) {
            if ($payload->has($key)) {
                $metadata[$key] = $payload->get($key);
            }
        }

        if ($payload->has('mac_address') || $payload->has('mac')) {
            $metadata['mac_address'] = $payload->get('mac_address') ?? $payload->get('mac');
        }

        $metadata['mesh_network_id'] = $meshId;

        if ($wasNew) {
            $metadata = array_merge($metadata, [
                'created_via' => 'mqtt',
                'created_at' => now()->toIso8601String(),
            ]);
        }

        if ($nodeType === 'deprecated_ph_ec') {
            $metadata['deprecated'] = true;
            $metadata['deprecated_reason'] = 'combined_ph_ec_node';
        }

        return $metadata;
    }

    private function updateOnlineStatus(Node $node, bool $wasOnline, string $meshId, bool $wasNew = false): void
    {
        $isOnline = $node->isOnline();

        if ($wasOnline !== $isOnline) {
            $node->update(['online' => $isOnline]);
            event(new NodeStatusChanged($node, $wasOnline, $isOnline));

            if ($isOnline) {
                if ($wasNew) {
                    $this->logNewNode($node, 'New node auto-registered via MQTT');
                }
            } else {
                $this->handleOffline($node, $meshId);
            }

            return;
        }

        if ($isOnline) {
            if ($wasNew) {
                $this->logNewNode($node, 'New node auto-registered via MQTT');
            } else {
                event(new NodeStatusUpdate($node));
            }
        }
    }

    private function createDiscoveryEvent(Node $node, string $meshId): void
    {
        Log::info('AUTO-DISCOVERY: New node found via heartbeat', [
            'node_id' => $node->node_id,
            'node_type' => $node->node_type,
        ]);

        $event = Event::create([
            'node_id' => $node->node_id,
            'level' => Event::LEVEL_INFO,
            'message' => "New node auto-discovered and registered: {$node->node_id}",
            'data' => [
                'node_type' => $node->node_type,
                'mesh_network_id' => $meshId,
            ],
        ]);

        event(new EventCreated($event));
    }

    private function logNewNode(Node $node, string $message): void
    {
        Log::info($message, [
            'node_id' => $node->node_id,
            'node_type' => $node->node_type,
        ]);

        event(new NodeDiscovered($node));
    }
}

