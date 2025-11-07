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
        $query = Zone::with(['rootNode', 'currentCycle']);

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

        Log::info("⭐ Зона создана: {$zone->name} (ID: {$zone->id}, Root: {$zone->root_node_id})");

        return response()->json([
            'success' => true,
            'message' => 'Зона успешно создана',
            'data' => $zone->load('rootNode'),
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
        $zone = Zone::with(['rootNode', 'currentCycle', 'nodeAssignments.node'])->find($id);

        if (!$zone) {
            return response()->json([
                'success' => false,
                'message' => 'Зона не найдена',
            ], 404);
        }

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
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'errors' => $validator->errors(),
            ], 422);
        }

        $zone->update($request->all());

        Log::info("⭐ Зона обновлена: {$zone->name} (ID: {$zone->id})");

        return response()->json([
            'success' => true,
            'message' => 'Зона успешно обновлена',
            'data' => $zone->load('rootNode'),
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
        $mqttTopic = $zone->mqtt_topic_prefix . 'command/' . $request->input('node_id');
        
        Log::info("⭐ Команда для зоны {$zone->name}: {$request->input('command')} → {$request->input('node_id')}");

        return response()->json([
            'success' => true,
            'message' => 'Команда отправлена',
            'data' => [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'node_id' => $request->input('node_id'),
                'command' => $request->input('command'),
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
}

