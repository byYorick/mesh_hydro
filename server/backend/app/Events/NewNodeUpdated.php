<?php

namespace App\Events;

use App\Models\NewNode;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class NewNodeUpdated implements ShouldBroadcast
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
        return 'new-node.updated';
    }

    public function broadcastWith(): array
    {
        return [
            'mac_address' => $this->newNode->mac_address,
            'status' => $this->newNode->status,
            'last_heartbeat_at' => optional($this->newNode->last_heartbeat_at)->toIso8601String(),
            'is_online' => $this->newNode->isOnline(),
            'metadata' => $this->newNode->metadata,
        ];
    }
}
