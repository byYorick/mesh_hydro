<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Support\Facades\Http;
use Illuminate\Support\Facades\Log;
use App\Services\MqttService;
use App\Models\Node;

class NewNode extends Model
{
    protected $fillable = [
        'mac_address',
        'node_type',
        'is_root',
        'pin',
        'temp_mesh_id',
        'status',
        'metadata',
        'discovered_at',
        'last_heartbeat_at',
        'configured_at',
    ];

    protected $casts = [
        'metadata' => 'array',
        'is_root' => 'boolean',
        'discovered_at' => 'datetime',
        'last_heartbeat_at' => 'datetime',
        'configured_at' => 'datetime',
    ];

    public function isOnline(): bool
    {
        if (!$this->last_heartbeat_at) {
            return false;
        }

        return $this->last_heartbeat_at->diffInSeconds(now()) < 30;
    }

    public function scopeOnline($query)
    {
        return $query->where('last_heartbeat_at', '>=', now()->subSeconds(30));
    }

    public function configure(array $config): bool
    {
        if ($this->status === 'confirmed') {
            return false;
        }

        if (!$this->isOnline()) {
            Log::warning('Attempt to configure offline new node', [
                'mac_address' => $this->mac_address,
            ]);
            return false;
        }

        // Добавляем PIN в конфигурацию для проверки на устройстве
        $config['pin'] = $this->pin;

        if ($this->is_root) {
            $result = $this->sendConfigViaHttp($config);
        } else {
            $result = $this->sendConfigViaMqtt($config);
        }

        if ($result) {
            $this->status = 'configuring';
            $this->save();
        }

        return $result;
    }

    protected function sendConfigViaHttp(array $config): bool
    {
        $ip = $this->metadata['ip_address'] ?? null;
        if (!$ip) {
            Log::error('Cannot send config via HTTP without IP address', [
                'mac_address' => $this->mac_address,
            ]);
            return false;
        }

        try {
            $response = Http::timeout(5)->post("http://{$ip}/api/config", $config);
            if ($response->successful()) {
                return true;
            }

            Log::error('HTTP config request failed', [
                'mac_address' => $this->mac_address,
                'status' => $response->status(),
                'body' => $response->body(),
            ]);
            return false;
        } catch (\Throwable $e) {
            Log::error('HTTP config request error', [
                'mac_address' => $this->mac_address,
                'error' => $e->getMessage(),
            ]);
            return false;
        }
    }

    protected function sendConfigViaMqtt(array $config): bool
    {
        try {
            $nodeId = $config['node_id'] ?? $this->mac_address;
            app(MqttService::class)->sendCommand(
                $nodeId,
                'write_config',
                $config
            );
            return true;
        } catch (\Throwable $e) {
            Log::error('MQTT config send failed', [
                'mac_address' => $this->mac_address,
                'error' => $e->getMessage(),
            ]);
            return false;
        }
    }

    public function confirmConfiguration(string $nodeId, array $attributes = []): Node
    {
        $this->update([
            'status' => 'confirmed',
            'configured_at' => now(),
        ]);

        return $this->moveToNodes($nodeId, $attributes);
    }

    public function moveToNodes(string $nodeId, array $attributes = []): Node
    {
        $metadata = $this->metadata ?? [];
        if (isset($attributes['metadata']) && is_array($attributes['metadata'])) {
            $metadata = array_merge($metadata, $attributes['metadata']);
        }

        $payload = [
            'node_type' => $this->node_type,
            'mac_address' => $this->mac_address,
            'online' => false,
            'metadata' => $metadata,
        ];

        if (isset($attributes['zone'])) {
            $payload['zone'] = $attributes['zone'];
        }

        if (isset($attributes['root_node_id'])) {
            $payload['root_node_id'] = $attributes['root_node_id'];
        }

        if (isset($attributes['config']) && is_array($attributes['config'])) {
            $payload['config'] = $attributes['config'];
        }

        $node = Node::updateOrCreate(
            ['node_id' => $nodeId],
            $payload
        );

        $this->delete();

        return $node;
    }

    public function updateHeartbeat(?array $metadata = null): void
    {
        $this->last_heartbeat_at = now();
        if ($metadata) {
            $currentMetadata = $this->metadata ?? [];
            $this->metadata = array_merge($currentMetadata, $metadata);
        }
        $this->save();
    }
}
