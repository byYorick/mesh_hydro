<?php

namespace App\Http\Controllers;

use App\Models\Zone;
use App\Models\Node;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\Validator;
use Illuminate\Support\Facades\Log;

/**
 * ⭐ ЗОНИРОВАНИЕ: Controller для управления зонами
 * 
 * API Endpoints:
 * - GET    /api/zones              Список всех зон
 * - POST   /api/zones              Создать зону
 * - GET    /api/zones/{id}         Получить зону
 * - PUT    /api/zones/{id}         Обновить зону
 * - DELETE /api/zones/{id}         Удалить зону
 * - GET    /api/zones/{id}/nodes   Получить узлы зоны
 * - POST   /api/zones/{id}/command Отправить команду узлу зоны
 */
class ZoneController extends Controller
{
    /**
     * Получить список всех зон
     * 
     * Query params:
     * - available: true/false - только доступные для новых циклов
     * - active: true/false - только активные зоны
     * - type: nft/dwc/drip/... - фильтр по типу
     * 
     * @param Request $request
     * @return JsonResponse
     */
    public function index(Request $request): JsonResponse
    {
        $query = Zone::with(['rootNode', 'currentCycle', 'greenhouse']);

        // Фильтр по доступности
        if ($request->has('available')) {
            if ($request->boolean('available')) {
                $query->available();
            }
        }

        // Фильтр по активности
        if ($request->has('active')) {
            if ($request->boolean('active')) {
                $query->where('is_active', true);
            } else {
                $query->where('is_active', false);
            }
        }

        // Фильтр по типу зоны
        if ($request->has('type')) {
            $query->ofType($request->input('type'));
        }

        $zones = $query->get();
        $zones->each(function (Zone $zone) {
            $zone->greenhouse_name = $zone->greenhouse?->name;
        });

        return response()->json([
            'success' => true,
            'data' => $zones,
            'meta' => [
                'total' => $zones->count(),
                'available' => $zones->where('is_available', true)->count(),
                'with_cycles' => $zones->whereNotNull('current_cycle_id')->count(),
            ],
        ]);
    }

    /**
     * Создать новую зону
     * 
     * @param Request $request
     * @return JsonResponse
     */
    public function store(Request $request): JsonResponse
    {
        $validator = Validator::make($request->all(), [
            'name' => 'required|string|max:255|unique:zones,name',
            'description' => 'nullable|string',
            'location' => 'nullable|string|max:255',
            'root_node_id' => 'required|string|max:32|unique:zones,root_node_id|exists:nodes,node_id',
            'mesh_network_id' => 'required|string|max:32|unique:zones,mesh_network_id',
            'mqtt_topic_prefix' => 'required|string|max:64',
            'zone_type' => 'required|in:nft,dwc,ebb_flow,drip,aeroponics,other',
            'reservoir_volume_liters' => 'nullable|numeric|min:0',
            'growing_area_m2' => 'nullable|numeric|min:0',
            'plant_capacity' => 'nullable|integer|min:0',
            'is_active' => 'boolean',
            'is_available' => 'boolean',
            'greenhouse_id' => 'nullable|integer|exists:greenhouses,id',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'errors' => $validator->errors(),
            ], 422);
        }

        // Проверка что root_node действительно Root Node
        $rootNode = Node::where('node_id', $request->input('root_node_id'))->first();
        if (!$rootNode || !$rootNode->isRootNode()) {
            return response()->json([
                'success' => false,
                'message' => 'Указанный узел не является Root Node',
            ], 422);
        }

        $zone = Zone::create($request->all());
        $zone->load(['rootNode', 'greenhouse']);
        $zone->greenhouse_name = $zone->greenhouse?->name;

        Log::info("⭐ Зона создана: {$zone->name} (ID: {$zone->id}, Root: {$zone->root_node_id})");

        return response()->json([
            'success' => true,
            'message' => 'Зона успешно создана',
            'data' => $zone,
        ], 201);
    }

    /**
     * Получить информацию о зоне
     * 
     * @param int $id
     * @return JsonResponse
     */
    public function show(int $id): JsonResponse
    {
        $zone = Zone::with(['rootNode', 'currentCycle', 'nodeAssignments.node', 'greenhouse'])->find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        $zone->greenhouse_name = $zone->greenhouse?->name;

        return response()->json([
            'success' => true,
            'data' => $zone,
        ]);
    }

    /**
     * Обновить зону
     * 
     * @param Request $request
     * @param int $id
     * @return JsonResponse
     */
    public function update(Request $request, int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        $validator = Validator::make($request->all(), [
            'name' => 'sometimes|string|max:255|unique:zones,name,' . $id,
            'description' => 'nullable|string',
            'location' => 'nullable|string|max:255',
            'zone_type' => 'sometimes|in:nft,dwc,ebb_flow,drip,aeroponics,other',
            'reservoir_volume_liters' => 'nullable|numeric|min:0',
            'growing_area_m2' => 'nullable|numeric|min:0',
            'plant_capacity' => 'nullable|integer|min:0',
            'is_active' => 'boolean',
            'is_available' => 'boolean',
            'greenhouse_id' => 'nullable|integer|exists:greenhouses,id',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'errors' => $validator->errors(),
            ], 422);
        }

        $zone->update($request->all());
        $zone->load(['rootNode', 'greenhouse']);
        $zone->greenhouse_name = $zone->greenhouse?->name;

        Log::info("⭐ Зона обновлена: {$zone->name} (ID: {$zone->id})");

        return response()->json([
            'success' => true,
            'message' => 'Зона успешно обновлена',
            'data' => $zone,
        ]);
    }

    /**
     * Удалить зону
     * 
     * @param int $id
     * @return JsonResponse
     */
    public function destroy(int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        // Проверка что нет активного цикла
        if ($zone->currentCycle) {
            return response()->json([
                'success' => false,
                'message' => 'Невозможно удалить зону с активным циклом роста',
            ], 422);
        }

        $zoneName = $zone->name;
        $zone->delete();

        Log::warning("⭐ Зона удалена: {$zoneName} (ID: {$id})");

        return response()->json([
            'success' => true,
            'message' => 'Зона успешно удалена',
        ]);
    }

    /**
     * Получить узлы зоны
     * 
     * @param int $id
     * @return JsonResponse
     */
    public function getNodes(int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        // Получаем Root Node
        $rootNode = $zone->rootNode;

        if (!$rootNode) {
            return response()->json([
                'success' => false,
                'message' => 'Root Node не найден',
            ], 404);
        }

        // Получаем все дочерние узлы
        $childNodes = $rootNode->childNodes()->with('lastTelemetry')->get();

        return response()->json([
            'success' => true,
            'data' => [
                'root_node' => $rootNode,
                'child_nodes' => $childNodes,
                'total_nodes' => $childNodes->count() + 1,
                'online_nodes' => $childNodes->where('online', true)->count() + ($rootNode->online ? 1 : 0),
            ],
        ]);
    }

    /**
     * Отправить команду узлу зоны
     * 
     * @param Request $request
     * @param int $id
     * @return JsonResponse
     */
    public function sendCommand(Request $request, int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        $validator = Validator::make($request->all(), [
            'node_id' => 'required|string|exists:nodes,node_id',
            'command' => 'required|string',
            'params' => 'nullable|array',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'errors' => $validator->errors(),
            ], 422);
        }

        // Проверка что узел принадлежит этой зоне
        $node = Node::where('node_id', $request->input('node_id'))->first();
        
        if ($node->root_node_id !== $zone->root_node_id) {
            return response()->json([
                'success' => false,
                'message' => 'Узел не принадлежит этой зоне',
            ], 422);
        }

        // TODO: Отправка команды через MQTT
        // Формат топика: hydro/zone{N}/command/{node_id}
        $mqttTopic = $zone->mqtt_topic_prefix . 'commands/' . $request->input('node_id');
        
        Log::info("⭐ Команда для зоны {$zone->name}: {$request->input('command')} → {$request->input('node_id')}");

        return response()->json([
            'success' => true,
            'message' => 'Команда отправлена',
            'data' => [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'node_id' => $request->input('node_id'),
                'command' => $request->input('command'),
                'params' => $request->input('params', []),
                'mqtt_topic' => $mqttTopic,
            ],
        ]);
    }

    /**
     * Проверить доступность зоны для нового цикла
     * 
     * @param int $id
     * @return JsonResponse
     */
    public function checkAvailability(int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        $isAvailable = $zone->isAvailableForCycle();
        $busyNodes = $zone->checkNodesAvailability();

        return response()->json([
            'success' => true,
            'data' => [
                'is_available' => $isAvailable,
                'zone_status' => [
                    'is_active' => $zone->is_active,
                    'is_available' => $zone->is_available,
                    'has_current_cycle' => $zone->current_cycle_id !== null,
                ],
                'busy_nodes' => $busyNodes,
                'conflicts_found' => count($busyNodes) > 0,
            ],
        ]);
    }

    /**
     * Получить Root Node зоны
     * 
     * @param int $id
     * @return JsonResponse
     */
    public function getRootNode(int $id): JsonResponse
    {
        $zone = Zone::with('rootNode')->find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        return response()->json([
            'success' => true,
            'data' => $zone->rootNode,
        ]);
    }

    /**
     * Получить телеметрию зоны
     * 
     * @param Request $request
     * @param int $id
     * @return JsonResponse
     */
    public function getTelemetry(Request $request, int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        // Получаем узлы зоны
        $nodes = $zone->getAllNodes();
        $nodeIds = $nodes->pluck('node_id')->toArray();

        if (empty($nodeIds)) {
            return response()->json([
                'success' => true,
                'data' => [],
            ]);
        }

        // Получаем телеметрию для узлов зоны
        $query = \App\Models\Telemetry::whereIn('node_id', $nodeIds);

        // Фильтр по времени
        if ($request->has('from')) {
            $query->where('received_at', '>=', $request->input('from'));
        }

        if ($request->has('to')) {
            $query->where('received_at', '<=', $request->input('to'));
        }

        $telemetry = $query->orderBy('received_at', 'desc')
            ->limit($request->input('limit', 100))
            ->get();

        return response()->json([
            'success' => true,
            'data' => $telemetry,
            'meta' => [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'nodes_count' => count($nodeIds),
            ],
        ]);
    }

    /**
     * Получить статистику зоны
     * 
     * @param Request $request
     * @param int $id
     * @return JsonResponse
     */
    public function getStatistics(Request $request, int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        $nodes = $zone->getAllNodes();
        $nodeIds = $nodes->pluck('node_id')->toArray();

        if (empty($nodeIds)) {
            return response()->json([
                'success' => true,
                'data' => [],
            ]);
        }

        $field = $request->input('field', 'ph');
        
        // Получаем статистику из телеметрии
        $stats = \DB::table('telemetry')
            ->whereIn('node_id', $nodeIds)
            ->where('received_at', '>=', now()->subDays(7))
            ->selectRaw("
                AVG((data->>'$field')::numeric) as avg_$field,
                MIN((data->>'$field')::numeric) as min_$field,
                MAX((data->>'$field')::numeric) as max_$field,
                COUNT(*) as readings_count
            ")
            ->first();

        return response()->json([
            'success' => true,
            'data' => $stats,
            'meta' => [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'field' => $field,
            ],
        ]);
    }

    /**
     * Назначить узел зоне
     * 
     * @param Request $request
     * @param int $id
     * @return JsonResponse
     */
    public function assignNode(Request $request, int $id): JsonResponse
    {
        $zone = Zone::find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

        $validator = Validator::make($request->all(), [
            'node_id' => 'required|string|exists:nodes,node_id',
            'role' => 'required|string|in:ph_node,climate_node,relay_node,water_node,display_node',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'errors' => $validator->errors(),
            ], 422);
        }

        $nodeId = $request->input('node_id');
        $role = $request->input('role');

        // Проверяем, что узел не используется в другой зоне
        $node = Node::where('node_id', $nodeId)->first();
        
        // Если у узла другой root_node_id, возвращаем ошибку
        if ($node->root_node_id && $node->root_node_id !== $zone->root_node_id) {
            return response()->json([
                'success' => false,
                'message' => 'Узел уже назначен другой зоне',
                'data' => [
                    'node_id' => $nodeId,
                    'current_zone' => $node->zoneRelation?->name,
                ],
            ], 422);
        }

        // Обновляем root_node_id узла
        $node->update(['root_node_id' => $zone->root_node_id]);

        // Обновляем assigned_nodes зоны
        $assignedNodes = $zone->assigned_nodes ?? [];
        $assignedNodes[$role] = $nodeId;
        $zone->update(['assigned_nodes' => $assignedNodes]);

        // Создаем запись в истории назначений
        \App\Models\ZoneNodeAssignment::create([
            'zone_id' => $zone->id,
            'node_id' => $nodeId,
            'node_role' => $role,
            'assigned_at' => now(),
        ]);

        Log::info("⭐ Узел назначен зоне", [
            'zone_id' => $zone->id,
            'zone_name' => $zone->name,
            'node_id' => $nodeId,
            'role' => $role,
        ]);

        return response()->json([
            'success' => true,
            'message' => 'Узел успешно назначен зоне',
            'data' => [
                'zone_id' => $zone->id,
                'node_id' => $nodeId,
                'role' => $role,
            ],
        ]);
    }

    /**
     * Проверить доступность узлов
     * 
     * @param Request $request
     * @return JsonResponse
     */
    public function checkNodeAvailability(Request $request): JsonResponse
    {
        $validator = Validator::make($request->all(), [
            'node_ids' => 'required|array',
            'node_ids.*' => 'string|exists:nodes,node_id',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'errors' => $validator->errors(),
            ], 422);
        }

        $nodeIds = $request->input('node_ids');
        $availability = [];

        foreach ($nodeIds as $nodeId) {
            $node = Node::where('node_id', $nodeId)->first();
            
            // Проверяем, используется ли узел в активной зоне
            $usedInZone = null;
            if ($node->root_node_id) {
                $zone = Zone::where('root_node_id', $node->root_node_id)
                    ->where('is_active', true)
                    ->first();
                $usedInZone = $zone ? $zone->name : null;
            }

            $availability[$nodeId] = [
                'available' => is_null($usedInZone),
                'used_in_zone' => $usedInZone,
                'online' => $node->online,
            ];
        }

        return response()->json([
            'success' => true,
            'data' => $availability,
        ]);
    }
}

