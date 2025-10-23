<?php

namespace App\Events;

use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class NodeConfigUpdated implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public string $nodeId;
    public array $config;

    /**
     * Create a new event instance.
     */
    public function __construct(string $nodeId, array $config)
    {
        $this->nodeId = $nodeId;
        $this->config = $config;
    }

    /**
     * Get the channels the event should broadcast on.
     */
    public function broadcastOn(): Channel
    {
        return new Channel('hydro');
    }

    /**
     * The event's broadcast name.
     */
    public function broadcastAs(): string
    {
        return 'node.config.updated';
    }

    /**
     * Data to broadcast.
     */
    public function broadcastWith(): array
    {
        return [
            'node_id' => $this->nodeId,
            'config' => $this->config,
            'timestamp' => now()->toIso8601String(),
        ];
    }
}

