<?php

namespace App\Services\Mqtt;

use App\Contracts\Messaging\MqttClientInterface;
use App\Models\Node;
use Illuminate\Support\Facades\Log;
use Throwable;

class CommandPublisher
{
    public function __construct(
        private readonly MqttClientInterface $client
    ) {
    }

    public function sendCommand(string $nodeId, string $command, array $params = [], ?int $commandId = null): void
    {
        $node = Node::where('node_id', $nodeId)->first();
        if (!$node) {
            throw new \RuntimeException("Node not found: {$nodeId}");
        }

        $meshId = $node->zoneCode() ?? $node->metadata['mesh_network_id'] ?? null;
        if (!$meshId || !$this->validateZone($meshId)) {
            throw new \RuntimeException("Node {$nodeId} has invalid zone: {$meshId}");
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
        $this->client->publish($topic, $payload, 1);

        Log::info('Command sent', [
            'node_id' => $nodeId,
            'command' => $command,
            'command_id' => $commandId,
            'zone' => $meshId,
        ]);
    }

    public function sendConfig(string $nodeId, array $config): void
    {
        $node = Node::where('node_id', $nodeId)->first();
        if (!$node) {
            throw new \RuntimeException("Node not found: {$nodeId}");
        }

        $meshId = $node->zoneCode() ?? $node->metadata['mesh_network_id'] ?? null;
        if (!$meshId || !$this->validateZone($meshId)) {
            throw new \RuntimeException("Node {$nodeId} has invalid zone: {$meshId}");
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
        $this->client->publish($topic, $payload, 1);

        Log::info('Config sent', [
            'node_id' => $nodeId,
            'zone' => $meshId,
        ]);
    }

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
}

