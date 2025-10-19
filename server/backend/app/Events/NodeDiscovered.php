<?php

namespace App\Events;

use App\Models\Node;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class NodeDiscovered implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public Node $node;

    /**
     * Create a new event instance.
     */
    public function __construct(Node $node)
    {
        $this->node = $node;
    }

    /**
     * Get the channels the event should broadcast on.
     */
    public function broadcastOn(): array
    {
        return [
            new Channel('hydro-system'),
        ];
    }

    /**
     * The event's broadcast name.
     */
    public function broadcastAs(): string
    {
        return 'node.discovered';
    }

    /**
     * Get the data to broadcast.
     */
    public function broadcastWith(): array
    {
        return [
            'node' => [
                'id' => $this->node->id,
                'node_id' => $this->node->node_id,
                'node_type' => $this->node->node_type,
                'zone' => $this->node->zone,
                'mac_address' => $this->node->mac_address,
                'online' => $this->node->online,
                'metadata' => $this->node->metadata,
                'discovered_at' => $this->node->metadata['discovered_at'] ?? null,
                'discovered_via' => $this->node->metadata['discovered_via'] ?? null,
            ],
            'message' => "New node discovered: {$this->node->node_id}",
            'timestamp' => now()->toIso8601String(),
        ];
    }
}


