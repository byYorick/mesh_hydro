<?php

namespace App\Events;

use App\Models\Node;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class NodeStatusChanged implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public $node;
    public $previousStatus;
    public $currentStatus;

    public function __construct(Node $node, bool $previousStatus, bool $currentStatus)
    {
        $this->node = $node;
        $this->previousStatus = $previousStatus;
        $this->currentStatus = $currentStatus;
    }

    public function broadcastOn()
    {
        return new Channel('hydro-system');
    }

    public function broadcastAs()
    {
        return 'node.status.changed';
    }

    public function broadcastWith()
    {
        return [
            'node_id' => $this->node->node_id,
            'node_type' => $this->node->node_type,
            'zone' => $this->node->zone,
            'online' => $this->currentStatus,
            'previous_online' => $this->previousStatus,
            'last_seen_at' => $this->node->last_seen_at,
        ];
    }
}

