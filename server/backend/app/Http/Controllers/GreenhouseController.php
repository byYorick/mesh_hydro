<?php

namespace App\Http\Controllers;

use App\Http\Requests\StoreGreenhouseRequest;
use App\Http\Requests\UpdateGreenhouseRequest;
use App\Models\Greenhouse;
use App\Models\GreenhouseAutomationRule;
use App\Models\Node;
use App\Models\Zone;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Str;
use Illuminate\Validation\ValidationException;

class GreenhouseController extends Controller
{
    public function index(Request $request): JsonResponse
    {
        $query = Greenhouse::query()
            ->withCount([
                'zones',
                'nodes',
                'cycles as active_cycle_count' => function ($q) {
                    $q->where('status', 'active');
                },
            ]);

        if ($request->filled('status')) {
            $query->where('status', $request->string('status'));
        }

        $greenhouses = $query
            ->orderBy('name')
            ->get();

        return response()->json([
            'data' => $greenhouses->map(fn (Greenhouse $greenhouse) => $this->formatSummary($greenhouse)),
            'meta' => [
                'total' => $greenhouses->count(),
                'active' => $greenhouses->where('status', 'active')->count(),
            ],
        ]);
    }

    public function show(Greenhouse $greenhouse): JsonResponse
    {
        $greenhouse->load([
            'zones.rootNode',
            'nodes.lastTelemetry',
            'cycles' => function ($query) {
                $query->with(['zone', 'preset'])->orderByDesc('started_at');
            },
            'automationRules' => function ($query) {
                $query->orderBy('name');
            },
        ]);

        $greenhouse->loadCount([
            'zones',
            'nodes',
            'cycles as active_cycle_count' => function ($q) {
                $q->where('status', 'active');
            },
        ]);

        return response()->json([
            'data' => $this->formatDetail($greenhouse),
        ]);
    }

    public function store(StoreGreenhouseRequest $request): JsonResponse
    {
        $payload = $this->preparePayload($request->validated());

        $rootNode = $this->resolveRootNode($payload);

        $greenhouse = DB::transaction(function () use ($payload, $rootNode) {
            $greenhouse = Greenhouse::create($payload);

            if ($rootNode) {
                $rootNode->update(['greenhouse_id' => $greenhouse->id]);
                $this->linkRootZoneToGreenhouse($rootNode, $greenhouse);
            }

            return $greenhouse;
        });

        $greenhouse->loadCount(['zones', 'nodes', 'cycles as active_cycle_count' => function ($q) {
            $q->where('status', 'active');
        }]);

        Log::info('🏡 Теплица создана', [
            'greenhouse_id' => $greenhouse->id,
            'name' => $greenhouse->name,
        ]);

        return response()->json([
            'success' => true,
            'message' => 'Теплица успешно создана',
            'data' => $this->formatDetail($greenhouse),
        ], 201);
    }

    public function update(UpdateGreenhouseRequest $request, Greenhouse $greenhouse): JsonResponse
    {
        $validated = $request->validated();

        $rootNode = null;
        if (array_key_exists('root_node_id', $validated)) {
            if ($validated['root_node_id']) {
                $rootNode = Node::where('node_id', $validated['root_node_id'])->firstOrFail();
                if (!$rootNode->isRootNode()) {
                    return response()->json([
                        'success' => false,
                        'message' => 'Указанный узел не является Root Node',
                    ], 422);
                }
                $validated['root_node_mac'] = $validated['root_node_mac'] ?? $rootNode->mac_address;
            } else {
                $validated['root_node_id'] = null;
                $validated['root_node_mac'] = null;
            }
        }

        DB::transaction(function () use ($greenhouse, $validated, $rootNode) {
            $previousRootId = $greenhouse->root_node_id;

            $greenhouse->update($validated);

            if (array_key_exists('root_node_id', $validated)) {
                if ($previousRootId && (!$validated['root_node_id'] || $validated['root_node_id'] !== $previousRootId)) {
                    Node::where('node_id', $previousRootId)
                        ->where('greenhouse_id', $greenhouse->id)
                        ->update(['greenhouse_id' => null]);
                }

                if ($rootNode) {
                    $rootNode->update(['greenhouse_id' => $greenhouse->id]);
                }
            }
        });

        $greenhouse->loadCount(['zones', 'nodes', 'cycles as active_cycle_count' => function ($q) {
            $q->where('status', 'active');
        }]);

        Log::info('🏡 Теплица обновлена', [
            'greenhouse_id' => $greenhouse->id,
            'name' => $greenhouse->name,
        ]);

        return response()->json([
            'success' => true,
            'message' => 'Теплица обновлена',
            'data' => $this->formatDetail($greenhouse),
        ]);
    }

    public function destroy(Greenhouse $greenhouse): JsonResponse
    {
        if ($greenhouse->zones()->exists()) {
            return response()->json([
                'success' => false,
                'message' => 'Нельзя удалить теплицу с прикрепленными зонами',
            ], 422);
        }

        if ($greenhouse->nodes()->exists()) {
            return response()->json([
                'success' => false,
                'message' => 'Нельзя удалить теплицу с прикрепленными узлами',
            ], 422);
        }

        if ($greenhouse->cycles()->exists()) {
            return response()->json([
                'success' => false,
                'message' => 'Нельзя удалить теплицу с историей циклов',
            ], 422);
        }

        $greenhouse->delete();

        Log::warning('🏡 Теплица удалена', [
            'greenhouse_id' => $greenhouse->id,
        ]);

        return response()->json([
            'success' => true,
            'message' => 'Теплица удалена',
        ]);
    }

    public function zones(Greenhouse $greenhouse): JsonResponse
    {
        $zones = $greenhouse->zones()
            ->with(['rootNode', 'currentCycle'])
            ->orderBy('name')
            ->get()
            ->map(function (Zone $zone) {
                return [
                    'id' => $zone->id,
                    'zone_id' => $zone->id,
                    'zone_name' => $zone->name,
                    'zone_mesh_id' => $zone->mesh_network_id,
                    'zone_type' => $zone->zone_type,
                    'is_active' => $zone->is_active,
                    'is_available' => $zone->is_available,
                    'current_cycle_id' => $zone->current_cycle_id,
                    'assigned_nodes' => $zone->assigned_nodes,
                    'metadata' => [
                        'location' => $zone->location,
                        'reservoir_volume_liters' => $zone->reservoir_volume_liters,
                        'plant_capacity' => $zone->plant_capacity,
                    ],
                ];
            });

        return response()->json($zones);
    }

    public function attachZone(Request $request, Greenhouse $greenhouse): JsonResponse
    {
        $validated = $request->validate([
            'zone_id' => ['required', 'integer', 'exists:zones,id'],
        ]);

        $zone = Zone::findOrFail($validated['zone_id']);

        if ($zone->greenhouse_id && $zone->greenhouse_id !== $greenhouse->id) {
            return response()->json([
                'success' => false,
                'message' => 'Зона уже привязана к другой теплице',
            ], 409);
        }

        $zone->update(['greenhouse_id' => $greenhouse->id]);

        return response()->json([
            'success' => true,
            'data' => [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'zone_mesh_id' => $zone->mesh_network_id,
            ],
        ]);
    }

    public function detachZone(Greenhouse $greenhouse, Zone $zone): JsonResponse
    {
        if ($zone->greenhouse_id !== $greenhouse->id) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не принадлежит указанной теплице',
            ], 404);
        }

        if ($zone->current_cycle_id) {
            return response()->json([
                'success' => false,
                'message' => 'Нельзя отвязать зону с активным циклом',
            ], 422);
        }

        $zone->update(['greenhouse_id' => null]);

        return response()->json([
            'success' => true,
            'message' => 'Зона отвязана от теплицы',
        ]);
    }

    public function nodes(Greenhouse $greenhouse): JsonResponse
    {
        $nodes = $greenhouse->nodes()
            ->with(['lastTelemetry', 'zoneRelation'])
            ->orderBy('node_type')
            ->orderBy('node_id')
            ->get()
            ->map(function (Node $node) {
                return [
                    'node_id' => $node->node_id,
                    'node_type' => $node->node_type,
                    'is_root' => $node->isRootNode(),
                    'root_node_id' => $node->root_node_id,
                    'zone_mesh_id' => $node->zoneCode(),
                    'zone_id' => $node->zoneRelation?->id,
                    'zone_name' => $node->zoneRelation?->name,
                    'last_seen_at' => $node->last_seen_at?->toIso8601String(),
                    'online' => $node->isOnline(),
                    'metadata' => $node->metadata ?? new \stdClass(),
                ];
            });

        return response()->json($nodes);
    }

    public function attachNode(Request $request, Greenhouse $greenhouse): JsonResponse
    {
        $validated = $request->validate([
            'node_id' => ['required', 'string', 'exists:nodes,node_id'],
            'as_root' => ['nullable', 'boolean'],
        ]);

        $node = Node::where('node_id', $validated['node_id'])->firstOrFail();

        if ($node->greenhouse_id && $node->greenhouse_id !== $greenhouse->id) {
            return response()->json([
                'success' => false,
                'message' => 'Узел уже привязан к другой теплице',
            ], 409);
        }

        if (!empty($validated['as_root']) && !$node->isRootNode()) {
            return response()->json([
                'success' => false,
                'message' => 'Узел должен быть ROOT для назначения в качестве базового',
            ], 422);
        }

        DB::transaction(function () use ($greenhouse, $node, $validated) {
            $node->update(['greenhouse_id' => $greenhouse->id]);

            if (!empty($validated['as_root'])) {
                $greenhouse->update([
                    'root_node_id' => $node->node_id,
                    'root_node_mac' => $node->mac_address,
                ]);
            }
        });

        return response()->json([
            'success' => true,
            'data' => [
                'node_id' => $node->node_id,
                'node_type' => $node->node_type,
                'is_root' => $node->isRootNode(),
            ],
        ]);
    }

    public function detachNode(Greenhouse $greenhouse, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->first();

        if (!$node) {
            return response()->json([
                'success' => false,
                'message' => 'Узел не найден',
            ], 404);
        }

        if ($node->greenhouse_id !== $greenhouse->id) {
            return response()->json([
                'success' => false,
                'message' => 'Узел не принадлежит указанной теплице',
            ], 404);
        }

        if ($greenhouse->root_node_id === $node->node_id) {
            $greenhouse->update([
                'root_node_id' => null,
                'root_node_mac' => null,
            ]);
        }

        $node->update(['greenhouse_id' => null]);

        return response()->json([
            'success' => true,
            'message' => 'Узел отвязан от теплицы',
        ]);
    }

    public function listAutomationRules(Greenhouse $greenhouse): JsonResponse
    {
        $rules = $greenhouse->automationRules()
            ->orderBy('name')
            ->get();

        return response()->json($rules);
    }

    public function createAutomationRule(Request $request, Greenhouse $greenhouse): JsonResponse
    {
        $validated = $request->validate([
            'name' => ['required', 'string', 'max:150'],
            'description' => ['nullable', 'string', 'max:512'],
            'enabled' => ['nullable', 'boolean'],
            'trigger_type' => ['required', 'string', 'max:64'],
            'trigger_config' => ['required', 'array'],
            'actions' => ['required', 'array', 'min:1'],
        ]);

        $rule = $greenhouse->automationRules()->create([
            'name' => $validated['name'],
            'description' => $validated['description'] ?? null,
            'enabled' => $validated['enabled'] ?? true,
            'trigger_type' => $validated['trigger_type'],
            'trigger_config' => $validated['trigger_config'],
            'actions' => $validated['actions'],
        ]);

        return response()->json($rule, 201);
    }

    public function updateAutomationRule(Request $request, Greenhouse $greenhouse, GreenhouseAutomationRule $rule): JsonResponse
    {
        if ($rule->greenhouse_id !== $greenhouse->id) {
            return response()->json([
                'success' => false,
                'message' => 'Правило не принадлежит указанной теплице',
            ], 404);
        }

        $validated = $request->validate([
            'name' => ['sometimes', 'string', 'max:150'],
            'description' => ['nullable', 'string', 'max:512'],
            'enabled' => ['sometimes', 'boolean'],
            'trigger_type' => ['sometimes', 'string', 'max:64'],
            'trigger_config' => ['sometimes', 'array'],
            'actions' => ['sometimes', 'array', 'min:1'],
        ]);

        $rule->update($validated);

        return response()->json($rule->fresh());
    }

    public function deleteAutomationRule(Greenhouse $greenhouse, GreenhouseAutomationRule $rule): JsonResponse
    {
        if ($rule->greenhouse_id !== $greenhouse->id) {
            return response()->json([
                'success' => false,
                'message' => 'Правило не принадлежит указанной теплице',
            ], 404);
        }

        $rule->delete();

        return response()->json([
            'success' => true,
            'message' => 'Правило удалено',
        ]);
    }

    private function formatSummary(Greenhouse $greenhouse): array
    {
        return [
            'id' => $greenhouse->id,
            'name' => $greenhouse->name,
            'code' => $greenhouse->code,
            'description' => $greenhouse->description,
            'status' => $greenhouse->status,
            'root_node_id' => $greenhouse->root_node_id,
            'root_node_mac' => $greenhouse->root_node_mac,
            'zone_count' => $greenhouse->zones_count ?? $greenhouse->zones()->count(),
            'node_count' => $greenhouse->nodes_count ?? $greenhouse->nodes()->count(),
            'active_cycle_count' => $greenhouse->active_cycle_count ?? $greenhouse->cycles()->where('status', 'active')->count(),
            'alert_count' => 0,
            'image_url' => $greenhouse->image_url,
            'created_at' => optional($greenhouse->created_at)->toIso8601String(),
            'updated_at' => optional($greenhouse->updated_at)->toIso8601String(),
            'climate_profiles' => $greenhouse->settings['climate_profiles'] ?? null,
        ];
    }

    private function formatDetail(Greenhouse $greenhouse): array
    {
        $summary = $this->formatSummary($greenhouse);

        $settings = $greenhouse->settings ?? [];

        $summary['tags'] = $greenhouse->tags ?? [];
        $summary['settings'] = $settings ?: new \stdClass();
        $summary['notes'] = $greenhouse->description;
        $summary['climate_profiles'] = $settings['climate_profiles'] ?? [];
        $summary['zones'] = $greenhouse->relationLoaded('zones')
            ? $greenhouse->zones->map(function (Zone $zone) {
                return [
                    'id' => $zone->id,
                    'name' => $zone->name,
                    'mesh_network_id' => $zone->mesh_network_id,
                    'zone_type' => $zone->zone_type,
                    'is_active' => $zone->is_active,
                    'is_available' => $zone->is_available,
                    'current_cycle_id' => $zone->current_cycle_id,
                ];
            })->values()
            : [];
        $summary['nodes'] = $greenhouse->relationLoaded('nodes')
            ? $greenhouse->nodes->map(function (Node $node) {
                return [
                    'node_id' => $node->node_id,
                    'node_type' => $node->node_type,
                    'is_root' => $node->isRootNode(),
                    'last_seen_at' => $node->last_seen_at?->toIso8601String(),
                    'online' => $node->isOnline(),
                ];
            })->values()
            : [];
        $summary['automation_rules'] = $greenhouse->relationLoaded('automationRules')
            ? $greenhouse->automationRules->values()
            : [];

        return $summary;
    }

    /**
     * Подготавливает данные для создания теплицы.
     */
    private function preparePayload(array $validated): array
    {
        $payload = $validated;

        $payload['status'] = $payload['status'] ?? 'active';

        $payload['settings'] = $this->prepareSettings($payload);

        if (array_key_exists('tags', $payload) && is_array($payload['tags'])) {
            $payload['tags'] = array_values($payload['tags']);
        }

        if (empty($payload['root_node_id'])) {
            $payload['root_node_id'] = null;
            $payload['root_node_mac'] = null;
        } elseif (empty($payload['root_node_mac'])) {
            $payload['root_node_mac'] = null;
        }

        unset($payload['climate_profiles'], $payload['location'], $payload['timezone'], $payload['mesh_group']);

        return $payload;
    }

    /**
     * Проверяет и возвращает root-узел для новой теплицы.
     *
     * @param array $payload Передается по ссылке для обновления MAC-адреса.
     */
    private function resolveRootNode(array &$payload): ?Node
    {
        if (empty($payload['root_node_id'])) {
            return null;
        }

        $rootNode = Node::where('node_id', $payload['root_node_id'])->first();

        if (!$rootNode) {
            throw ValidationException::withMessages([
                'root_node_id' => 'Указанный узел не найден в системе',
            ]);
        }

        if (!$rootNode->isRootNode()) {
            throw ValidationException::withMessages([
                'root_node_id' => 'Указанный узел не является Root Node',
            ]);
        }

        if ($rootNode->root_node_id && $rootNode->root_node_id !== $rootNode->node_id) {
            throw ValidationException::withMessages([
                'root_node_id' => 'Узел зарегистрирован как дочерний и не может быть назначен базовым',
            ]);
        }

        if ($rootNode->greenhouse_id) {
            throw ValidationException::withMessages([
                'root_node_id' => 'Узел уже привязан к другой теплице. Сначала отвяжите его.',
            ]);
        }

        $zone = $rootNode->zoneRelation;
        if ($zone && $zone->greenhouse_id) {
            throw ValidationException::withMessages([
                'root_node_id' => 'Root Node уже используется в зоне, принадлежащей другой теплице.',
            ]);
        }

        $payload['root_node_mac'] = $rootNode->mac_address ?: ($payload['root_node_mac'] ?? null);

        return $rootNode;
    }

    /**
     * Привязывает существующую зону root-узла к теплице, если она свободна.
     */
    private function linkRootZoneToGreenhouse(Node $rootNode, Greenhouse $greenhouse): void
    {
        $zone = $rootNode->zoneRelation;

        if ($zone && !$zone->greenhouse_id) {
            $zone->update(['greenhouse_id' => $greenhouse->id]);
        }
    }

    private function prepareSettings(array $payload): array
    {
        $settings = $payload['settings'] ?? [];
        $settings = is_array($settings) ? $settings : [];

        if (!empty($payload['climate_profiles']) && is_array($payload['climate_profiles'])) {
            $settings['climate_profiles'] = $this->decorateClimateProfiles($payload['climate_profiles']);
        }

        return $settings;
    }

    private function decorateClimateProfiles(array $profiles): array
    {
        return array_values(array_map(function (array $profile) {
            $profile['id'] = $profile['id'] ?? (string) Str::uuid();

            if (isset($profile['day']) && empty($profile['day'])) {
                unset($profile['day']);
            }

            if (isset($profile['night']) && empty($profile['night'])) {
                unset($profile['night']);
            }

            if (isset($profile['settings']) && empty($profile['settings'])) {
                unset($profile['settings']);
            }

            return $profile;
        }, $profiles));
    }
}


