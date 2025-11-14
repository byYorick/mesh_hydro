<?php

namespace App\Models;

use App\Models\Node;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Support\Facades\Log;

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
