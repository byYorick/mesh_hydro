<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Zone;
use Carbon\Carbon;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Log;

/**
 * ⭐ ЗОНИРОВАНИЕ: REST API для работы с мультизонными данными.
 */
class ZoneController extends Controller
{
    /**
     * GET /api/zones
     * Возвращает сводку по активным зонам (mesh_network_id).
     */
    public function index(Request $request): JsonResponse
    {
        try {
            $query = Zone::query()->with(['rootNode.childNodes']);

            if ($request->has('available') && $request->boolean('available')) {
                $query->available();
            }

            if ($request->filled('type')) {
                $query->ofType($request->input('type'));
            }

            $zoneModels = $query->orderBy('name')->get();

            if ($zoneModels->isEmpty()) {
                return $this->legacyIndex();
            }

            $zones = $zoneModels->map(function (Zone $zone) {
                $rootNode = $zone->rootNode;
                $childNodes = $rootNode ? $rootNode->childNodes : collect();

                $nodesTotal = $childNodes->count() + ($rootNode ? 1 : 0);
                $nodesOnline = ($rootNode && $rootNode->online ? 1 : 0) +
                    $childNodes->where('online', true)->count();

                return [
                    'id' => $zone->id,
                    'zone' => $zone->mesh_network_id ?? $zone->name,
                    'zone_name' => $zone->name,
                    'root_node_id' => $zone->root_node_id,
                    'nodes_total' => $nodesTotal,
                    'nodes_online' => $nodesOnline,
                    'nodes_offline' => max(0, $nodesTotal - $nodesOnline),
                    'last_activity' => $rootNode?->last_seen_at?->toIso8601String(),
                    'greenhouse_id' => $zone->greenhouse_id,
                ];
            })->values();

            return response()->json([
                'success' => true,
                'data' => $zones,
                'meta' => [
                    'total' => $zones->count(),
                    'available' => $zoneModels->where('is_active', true)->where('is_available', true)->whereNull('current_cycle_id')->count(),
                    'busy' => $zoneModels->whereNotNull('current_cycle_id')->count(),
                    'updated_at' => now()->toIso8601String(),
                ],
            ]);
        } catch (\Throwable $e) {
            Log::error('Failed to fetch zones list', [
                'error' => $e->getMessage(),
            ]);

            return response()->json([
                'success' => false,
                'message' => 'Не удалось получить список зон',
            ], 500);
        }
    }

    protected function legacyIndex(): JsonResponse
    {
        $rawZones = Node::select('zone')
            ->selectRaw("MAX(CASE WHEN node_type = 'root' THEN root_node_id END) as root_node_id")
            ->selectRaw('COUNT(*) as nodes_total')
            ->selectRaw('SUM(CASE WHEN online THEN 1 ELSE 0 END) as nodes_online')
            ->selectRaw('MAX(last_seen_at) as last_seen_at')
            ->whereNotNull('zone')
            ->groupBy('zone')
            ->orderBy('zone')
            ->get();

        $zones = $rawZones
            ->filter(fn ($zone) => $this->validateZone($zone->zone))
            ->map(function ($zone) {
                return [
                    'zone' => $zone->zone,
                    'root_node_id' => $zone->root_node_id,
                    'nodes_total' => (int) $zone->nodes_total,
                    'nodes_online' => (int) $zone->nodes_online,
                    'nodes_offline' => (int) max(0, $zone->nodes_total - $zone->nodes_online),
                    'last_activity' => $zone->last_seen_at
                        ? Carbon::parse($zone->last_seen_at)->toIso8601String()
                        : null,
                ];
            })
            ->values();

        return response()->json([
            'success' => true,
            'data' => $zones,
            'meta' => [
                'total' => $zones->count(),
                'available' => $zones->count(),
                'updated_at' => now()->toIso8601String(),
            ],
        ]);
    }

    /**
     * GET /api/zones/{zone}/nodes
     * Возвращает подробный список узлов выбранной зоны.
     */
    public function nodes(string $zone): JsonResponse
    {
        try {
            [$zoneKey, $zoneModel] = $this->resolveZoneIdentifier($zone);

            if (!$zoneKey) {
                return response()->json([
                    'success' => false,
                    'message' => 'Зона не найдена',
                ], 404);
            }

            if ($zoneModel) {
                $zoneModel->loadMissing('rootNode.childNodes.lastTelemetry');
            }

            $rootNode = $zoneModel?->rootNode
                ?? Node::where('root_node_id', $zoneModel?->root_node_id)->where('node_type', 'root')->first()
                ?? Node::where('node_id', $zoneModel?->root_node_id)->first()
                ?? Node::where('zone', $zoneKey)->where('node_type', 'root')->first();

            if (!$rootNode && $zoneModel && $zoneModel->root_node_id) {
                $rootNode = Node::firstOrCreate(
                    ['node_id' => $zoneModel->root_node_id],
                    [
                        'node_type' => 'root',
                        'root_node_id' => $zoneModel->root_node_id,
                        'zone' => $zoneModel->mesh_network_id,
                        'online' => false,
                        'metadata' => [
                            'created_via' => 'zone_api_auto',
                            'mesh_network_id' => $zoneModel->mesh_network_id,
                        ],
                    ]
                );
            }

            $childNodes = $rootNode
                ? $rootNode->childNodes()->with('lastTelemetry')->get()
                : collect();

            $rootNodeData = $rootNode ? [
                'node_id' => $rootNode->node_id,
                'node_type' => $rootNode->node_type,
                'online' => $rootNode->isOnline(),
                'last_seen_at' => $rootNode->last_seen_at?->toIso8601String(),
                'metadata' => $rootNode->metadata ?? new \stdClass(),
            ] : null;

            $childTransformed = $childNodes->map(function (Node $node) {
                return [
                    'node_id' => $node->node_id,
                    'node_type' => $node->node_type,
                    'online' => $node->isOnline(),
                    'last_seen_at' => $node->last_seen_at?->toIso8601String(),
                    'metadata' => $node->metadata ?? new \stdClass(),
                    'last_telemetry' => $node->lastTelemetry
                        ? [
                            'data' => $node->lastTelemetry->data ?? new \stdClass(),
                            'received_at' => $node->lastTelemetry->received_at?->toIso8601String(),
                        ]
                        : null,
                ];
            })->values();

            $totalNodes = $childTransformed->count() + ($rootNode ? 1 : 0);
            $onlineNodes = ($rootNode && $rootNode->isOnline() ? 1 : 0) +
                $childNodes->where(fn ($node) => $node->isOnline())->count();

            return response()->json([
                'success' => true,
                'data' => [
                    'root_node' => $rootNodeData,
                    'child_nodes' => $childTransformed,
                    'total_nodes' => $totalNodes,
                    'online_nodes' => $onlineNodes,
                ],
                'meta' => [
                    'zone' => $zoneModel?->mesh_network_id ?? $zoneKey,
                    'zone_id' => $zoneModel?->id,
                    'zone_name' => $zoneModel?->name,
                    'resolved_via' => is_numeric($zone) ? 'id' : 'mesh',
                    'updated_at' => now()->toIso8601String(),
                ],
            ]);
        } catch (\Throwable $e) {
            Log::error('Failed to fetch zone nodes', [
                'zone' => $zone,
                'error' => $e->getMessage(),
            ]);

            return response()->json([
                'success' => false,
                'message' => 'Не удалось получить список узлов зоны',
            ], 500);
        }
    }

    /**
     * GET /api/zones/{zone}/stats
     * Возвращает агрегированную статистику по зоне.
     */
    public function stats(string $zone): JsonResponse
    {
        try {
            [$zoneKey, $zoneModel] = $this->resolveZoneIdentifier($zone);

            if (!$zoneKey) {
                return response()->json([
                    'success' => false,
                    'message' => 'Зона не найдена',
                ], 404);
            }

            $baseQuery = Node::where('zone', $zoneKey);
            if (!$baseQuery->exists()) {
                return response()->json([
                    'success' => true,
                    'data' => [
                        'zone' => $zoneModel?->mesh_network_id ?? $zoneKey,
                        'nodes_total' => 0,
                        'nodes_online' => 0,
                        'nodes_offline' => 0,
                        'last_activity' => null,
                        'root_node' => null,
                        'nodes_by_type' => [],
                        'telemetry' => [
                            'averages' => new \stdClass(),
                            'latest' => [],
                        ],
                    ],
                    'meta' => [
                        'zone' => $zoneModel?->mesh_network_id ?? $zoneKey,
                        'zone_id' => $zoneModel?->id,
                        'zone_name' => $zoneModel?->name,
                        'resolved_via' => is_numeric($zone) ? 'id' : 'mesh',
                        'updated_at' => now()->toIso8601String(),
                    ],
                ]);
            }

            $nodesTotal = (clone $baseQuery)->count();
            $nodesOnline = (clone $baseQuery)->online()->count();
            $lastActivity = (clone $baseQuery)->max('last_seen_at');

            $nodesByType = (clone $baseQuery)
                ->select('node_type')
                ->selectRaw('COUNT(*) as total')
                ->selectRaw('SUM(CASE WHEN online THEN 1 ELSE 0 END) as online')
                ->groupBy('node_type')
                ->orderBy('node_type')
                ->get()
                ->map(function ($row) {
                    return [
                        'type' => $row->node_type,
                        'total' => (int) $row->total,
                        'online' => (int) $row->online,
                    ];
                });

            $rootNode = (clone $baseQuery)
                ->where('node_type', 'root')
                ->first(['node_id', 'online', 'last_seen_at', 'metadata']);

            $nodeIds = (clone $baseQuery)->pluck('node_id');

            $telemetryAggregates = $this->aggregateTelemetryMetrics($nodeIds);

            return response()->json([
                'success' => true,
                'data' => [
                    'zone' => $zoneModel?->mesh_network_id ?? $zoneKey,
                    'nodes_total' => $nodesTotal,
                    'nodes_online' => $nodesOnline,
                    'nodes_offline' => max(0, $nodesTotal - $nodesOnline),
                    'last_activity' => $lastActivity
                        ? Carbon::parse($lastActivity)->toIso8601String()
                        : null,
                    'root_node' => $rootNode ? [
                        'node_id' => $rootNode->node_id,
                        'online' => $rootNode->online,
                        'last_seen_at' => $rootNode->last_seen_at?->toIso8601String(),
                        'metadata' => $rootNode->metadata ?? new \stdClass(),
                    ] : null,
                    'nodes_by_type' => $nodesByType,
                    'telemetry' => $telemetryAggregates,
                ],
                'meta' => [
                    'zone' => $zoneModel?->mesh_network_id ?? $zoneKey,
                    'zone_id' => $zoneModel?->id,
                    'zone_name' => $zoneModel?->name,
                    'resolved_via' => is_numeric($zone) ? 'id' : 'mesh',
                    'updated_at' => now()->toIso8601String(),
                ],
            ]);
        } catch (\Throwable $e) {
            Log::error('Failed to fetch zone stats', [
                'zone' => $zone,
                'error' => $e->getMessage(),
            ]);

            return response()->json([
                'success' => false,
                'message' => 'Не удалось получить статистику зоны',
            ], 500);
        }
    }

    /**
     * Агрегирует телеметрические показатели для набора узлов.
     */
    private function aggregateTelemetryMetrics($nodeIds): array
    {
        if ($nodeIds->isEmpty()) {
            return [
                'averages' => new \stdClass(),
                'latest' => [],
            ];
        }

        $latestTelemetry = Telemetry::whereIn('node_id', $nodeIds)
            ->orderByDesc('received_at')
            ->limit(200)
            ->get(['node_id', 'data', 'received_at']);

        if ($latestTelemetry->isEmpty()) {
            return [
                'averages' => new \stdClass(),
                'latest' => [],
            ];
        }

        $numericSums = [];
        $numericCounts = [];

        foreach ($latestTelemetry as $entry) {
            foreach ($entry->data ?? [] as $key => $value) {
                if (is_numeric($value)) {
                    $numericSums[$key] = ($numericSums[$key] ?? 0) + $value;
                    $numericCounts[$key] = ($numericCounts[$key] ?? 0) + 1;
                }
            }
        }

        $averages = [];
        foreach ($numericSums as $key => $sum) {
            $averages[$key] = round($sum / max(1, $numericCounts[$key]), 3);
        }

        return [
            'averages' => (object) $averages,
            'latest' => $latestTelemetry->map(function ($entry) {
                return [
                    'node_id' => $entry->node_id,
                    'data' => $entry->data,
                    'received_at' => $entry->received_at?->toIso8601String(),
                ];
            })->take(10)->values(),
        ];
    }

    /**
     * Проверяет валидность зоны.
     */
    private function validateZone(?string $zone): bool
    {
        if (!$zone) {
            return false;
        }

        $lower = strtolower($zone);
        if ($lower === 'setup' || strtoupper($zone) === 'UNCONFIGURED') {
            return false;
        }

        $length = strlen($zone);
        if ($length < 3 || $length > 31) {
            return false;
        }

        return (bool) preg_match('/^[a-zA-Z0-9_-]+$/', $zone);
    }

    /**
     * Определяет mesh_network_id по переданному идентификатору (mesh_id, id, name).
     *
     * @return array{0: string|null, 1: \App\Models\Zone|null}
     */
    private function resolveZoneIdentifier(string $zone): array
    {
        // 1. Если передан числовой id
        if (is_numeric($zone)) {
            $zoneModel = Zone::find((int) $zone);
            if ($zoneModel && $zoneModel->mesh_network_id) {
                return [$zoneModel->mesh_network_id, $zoneModel];
            }
        }

        // 2. Прямое совпадение по mesh_network_id
        if ($this->validateZone($zone)) {
            $zoneModel = Zone::where('mesh_network_id', $zone)->first();
            return [$zone, $zoneModel];
        }

        // 3. Попытка найти по имени зоны
        $zoneModel = Zone::where('name', $zone)->first();
        if ($zoneModel && $zoneModel->mesh_network_id) {
            return [$zoneModel->mesh_network_id, $zoneModel];
        }

        return [null, null];
    }
}


