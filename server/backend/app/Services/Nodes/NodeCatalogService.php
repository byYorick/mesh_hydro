<?php

namespace App\Services\Nodes;

use App\Models\ConfigHistory;
use App\Models\Node;
use App\Models\PumpCalibration;
use Illuminate\Database\Eloquent\Builder;
use Illuminate\Support\Collection;

class NodeCatalogService
{
    public function listNodes(Collection $filters): Collection
    {
        $query = Node::query();

        $this->applyFilters($query, $filters);

        $nodes = $query
            ->with(['lastTelemetry', 'greenhouse'])
            ->get();

        $enriched = $nodes->map(function (Node $node) {
            return $this->enrichNode($node);
        });

        $status = $filters->get('status');
        if ($status === 'online') {
            $enriched = $enriched->filter(fn (Node $node) => (bool) $node->online);
        } elseif ($status === 'offline') {
            $enriched = $enriched->filter(fn (Node $node) => !(bool) $node->online);
        }

        return $enriched->values();
    }

    public function findNodeWithDetails(string $nodeId): Node
    {
        $node = Node::with([
            'telemetry' => fn ($query) => $query->latest('received_at')->limit(100),
            'lastTelemetry',
            'events' => fn ($query) => $query->latest()->limit(50),
            'commands' => fn ($query) => $query->latest()->limit(20),
            'greenhouse',
        ])
        ->where('node_id', $nodeId)
        ->firstOrFail();

        return $this->enrichNode($node);
    }

    public function collectStatistics(string $nodeId, int $hours): Collection
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $telemetry = $node->telemetry()
            ->where('received_at', '>', now()->subHours($hours))
            ->orderBy('received_at', 'asc')
            ->get();

        return collect([
            'node_id' => $nodeId,
            'period_hours' => $hours,
            'data_points' => $telemetry->count(),
            'telemetry' => $telemetry,
        ]);
    }

    public function loadPumpCalibrations(string $nodeId): Collection
    {
        $calibrations = PumpCalibration::where('node_id', $nodeId)
            ->orderBy('pump_id')
            ->get();

        return collect([
            'success' => true,
            'calibrations' => $calibrations,
        ]);
    }

    public function loadConfigHistory(string $nodeId): Collection
    {
        $history = ConfigHistory::where('node_id', $nodeId)
            ->orderBy('changed_at', 'desc')
            ->limit(50)
            ->get();

        return collect([
            'success' => true,
            'history' => $history,
        ]);
    }

    private function applyFilters(Builder $query, Collection $filters): void
    {
        $type = $filters->get('type');
        if (filled($type)) {
            $query->where('node_type', $type);
        }

        $status = $filters->get('status');
        if ($status === 'online') {
            $query->online();
        } elseif ($status === 'offline') {
            $query->offline();
        }

        if ($status && !filled($type)) {
            $query->where('node_type', '!=', 'root');
        }

        if ($filters->has('greenhouse_id') && filled($filters->get('greenhouse_id'))) {
            $query->where('greenhouse_id', $filters->get('greenhouse_id'));
        }
    }

    private function enrichNode(Node $node): Node
    {
        $node->online = $node->isOnline();
        $node->status_color = $node->status_color;
        $node->icon = $node->icon;
        $node->last_telemetry = $node->lastTelemetry;
        $node->last_data = $node->lastTelemetry?->data;
        $node->greenhouse_name = $node->greenhouse?->name;

        return $node;
    }
}


