<?php

namespace App\Services\Nodes;

use App\Models\Event;
use App\Services\Nodes\Contracts\NodeZoneResolverInterface;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\Log;

class ZoneResolver implements NodeZoneResolverInterface
{
    public function resolve(string $topic, Collection $payload, string $context, ?string $nodeId = null): ?string
    {
        $zoneFromTopic = $this->extractFromTopic($topic);
        if ($zoneFromTopic !== null) {
            return $zoneFromTopic;
        }

        $zoneFromPayload = $this->extractFromPayload($payload);
        if ($zoneFromPayload !== null) {
            return $zoneFromPayload;
        }

        Log::error("MQTT {$context} message rejected: missing valid zone", [
            'topic' => $topic,
            'node_id' => $nodeId,
            'payload_zone' => $payload->only(['mesh_network_id', 'mesh_id', 'zone']),
        ]);

        if ($context === 'heartbeat' && $nodeId) {
            Event::create([
                'node_id' => $nodeId,
                'level' => Event::LEVEL_CRITICAL,
                'message' => "Node sent {$context} without valid zone - REJECTED",
                'data' => [
                    'topic' => $topic,
                    'payload_zone' => $payload->get('mesh_network_id')
                        ?? $payload->get('mesh_id')
                        ?? $payload->get('zone')
                        ?? 'missing',
                ],
            ]);
        }

        return null;
    }

    private function extractFromTopic(string $topic): ?string
    {
        $parts = explode('/', $topic);
        if (count($parts) < 3 || $parts[0] !== 'hydro') {
            return null;
        }

        $zone = $parts[1];
        if ($zone === 'setup') {
            return null;
        }

        return $this->isValid($zone) ? $zone : null;
    }

    private function extractFromPayload(Collection $payload): ?string
    {
        $zone = $payload->get('mesh_network_id')
            ?? $payload->get('mesh_id')
            ?? $payload->get('zone');

        return $this->isValid($zone) ? $zone : null;
    }

    private function isValid(?string $zone): bool
    {
        if (!$zone) {
            return false;
        }

        if (strtolower($zone) === 'setup' || strtoupper($zone) === 'UNCONFIGURED') {
            return false;
        }

        $length = strlen($zone);
        if ($length < 3 || $length > 31) {
            return false;
        }

        return (bool) preg_match('/^[a-zA-Z0-9_-]+$/', $zone);
    }
}


