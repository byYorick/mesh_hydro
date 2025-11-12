<?php

namespace App\Events;

use App\Models\NewNode;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class NewNodeDiscovered implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public NewNode $newNode;

    public function __construct(NewNode $newNode)
    {
        $this->newNode = $newNode;
    }

    public function broadcastOn(): array
    {
        return [new Channel('hydro-setup')];
    }

    public function broadcastAs(): string
    {
        return 'new-node.discovered';
    }

    public function broadcastWith(): array
    {
        return [
            'mac_address' => $this->newNode->mac_address,
            'node_type' => $this->newNode->node_type,
            'is_root' => $this->newNode->is_root,
            'pin' => $this->newNode->pin,
            'temp_mesh_id' => $this->newNode->temp_mesh_id,
            'status' => $this->newNode->status,
            'metadata' => $this->newNode->metadata,
            'discovered_at' => optional($this->newNode->discovered_at)->toIso8601String(),
            'last_heartbeat_at' => optional($this->newNode->last_heartbeat_at)->toIso8601String(),
        ];
    }
}
