<?php

namespace App\Events;

use App\Models\Telemetry;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class TelemetryReceived implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public $telemetry;

    public function __construct(Telemetry $telemetry)
    {
        $this->telemetry = $telemetry;
    }

    public function broadcastOn()
    {
        return new Channel('hydro-system');
    }

    public function broadcastAs()
    {
        return 'telemetry.received';
    }

    public function broadcastWith()
    {
        return [
            'node_id' => $this->telemetry->node_id,
            'node_type' => $this->telemetry->node_type,
            'data' => $this->telemetry->data,
            'received_at' => $this->telemetry->received_at,
        ];
    }
}

