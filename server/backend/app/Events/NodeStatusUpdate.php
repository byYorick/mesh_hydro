<?php

namespace App\Events;

use App\Models\Node;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

/**
 * Событие периодического обновления статуса узла
 * Отправляется при каждом heartbeat для обновления UI
 */
class NodeStatusUpdate implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public $node;

    public function __construct(Node $node)
    {
        $this->node = $node;
    }

    public function broadcastOn()
    {
        return new Channel('hydro-system');
    }

    public function broadcastAs()
    {
        return 'node.status.update';
    }

    public function broadcastWith()
    {
        return [
            'node_id' => $this->node->node_id,
            'node_type' => $this->node->node_type,
            'zone' => $this->node->zoneCode(),
            'online' => $this->node->isOnline(),
            'last_seen_at' => $this->node->last_seen_at?->toIso8601String(),
            'status_color' => $this->node->status_color,
            'icon' => $this->node->icon,
            'metadata' => $this->node->metadata ?? [],
        ];
    }
}

