<?php

namespace App\Services\Nodes\Contracts;

use Illuminate\Support\Collection;

interface NodeZoneResolverInterface
{
    public function resolve(string $topic, Collection $payload, string $context, ?string $nodeId = null): ?string;
}


