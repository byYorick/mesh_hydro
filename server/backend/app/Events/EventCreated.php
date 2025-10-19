<?php

namespace App\Events;

use App\Models\Event;
use Illuminate\Broadcasting\Channel;
use Illuminate\Broadcasting\InteractsWithSockets;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;
use Illuminate\Foundation\Events\Dispatchable;
use Illuminate\Queue\SerializesModels;

class EventCreated implements ShouldBroadcast
{
    use Dispatchable, InteractsWithSockets, SerializesModels;

    public $event;

    public function __construct(Event $event)
    {
        $this->event = $event;
    }

    public function broadcastOn()
    {
        return new Channel('hydro-system');
    }

    public function broadcastAs()
    {
        return 'event.created';
    }

    public function broadcastWith()
    {
        return [
            'id' => $this->event->id,
            'node_id' => $this->event->node_id,
            'level' => $this->event->level,
            'message' => $this->event->message,
            'data' => $this->event->data,
            'created_at' => $this->event->created_at,
        ];
    }
}

