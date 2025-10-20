<?php

namespace App\Events;

use App\Models\NodeError;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class ErrorOccurred implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public NodeError $error;

    public function __construct(NodeError $error)
    {
        $this->error = $error;
    }

    public function broadcastOn(): array
    {
        return [
            new Channel('hydro-system'),
        ];
    }

    public function broadcastAs(): string
    {
        return 'error.occurred';
    }

    public function broadcastWith(): array
    {
        return [
            'id' => $this->error->id,
            'node_id' => $this->error->node_id,
            'error_code' => $this->error->error_code,
            'error_type' => $this->error->error_type,
            'severity' => $this->error->severity,
            'message' => $this->error->message,
            'diagnostics' => $this->error->diagnostics,
            'occurred_at' => $this->error->occurred_at,
        ];
    }
}

