<?php

namespace App\Events;

use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class NewNodeConfigured implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public string $nodeId;
    public string $macAddress;

    public function __construct(string $nodeId, string $macAddress)
    {
        $this->nodeId = $nodeId;
        $this->macAddress = $macAddress;
    }

    public function broadcastOn(): array
    {
        return [new Channel('hydro-setup')];
    }

    public function broadcastAs(): string
    {
        return 'new-node.configured';
    }

    public function broadcastWith(): array
    {
        return [
            'node_id' => $this->nodeId,
            'mac_address' => $this->macAddress,
        ];
    }
}
