<?php

namespace App\Services\Nodes\Domain\Contracts;

use App\Models\Node;
use Illuminate\Support\Collection;

interface NodeStatusServiceInterface
{
    public function handleTelemetry(Node $node, Collection $payload, string $meshId): void;

    public function handleOffline(Node $node, string $meshId): void;

    public function handleHeartbeat(Node $node, Collection $payload, string $meshId): void;
}


