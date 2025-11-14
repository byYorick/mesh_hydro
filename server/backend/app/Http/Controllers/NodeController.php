<?php

namespace App\Http\Controllers;

use App\Exceptions\Nodes\DuplicateNodeException;
use App\Http\Requests\StoreNodeRequest;
use App\Http\Requests\UpdateNodeRequest;
use App\Models\Node;
use App\Services\Nodes\NodeActionService;
use App\Services\Nodes\NodeCatalogService;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\Log;

class NodeController extends Controller
{
    public function __construct(
        private readonly NodeCatalogService $nodeCatalogService,
        private readonly NodeActionService $nodeActionService,
    ) {
    }

    /**
     * Список всех узлов с последней телеметрией
     */
    public function index(Request $request): JsonResponse
    {
        $filters = collect([
            'type' => $request->input('type'),
            'status' => $request->input('status'),
            'greenhouse_id' => $request->has('greenhouse_id')
                ? $request->integer('greenhouse_id')
                : null,
        ]);

        $nodes = $this->nodeCatalogService->listNodes($filters);

        return response()->json($nodes);
    }

    /**
     * Детальная информация об узле
     */
    public function show(string $nodeId): JsonResponse
    {
        $node = $this->nodeCatalogService->findNodeWithDetails($nodeId);

        return response()->json($node);
    }

    /**
     * Создание нового узла
     */
    public function store(StoreNodeRequest $request): JsonResponse
    {
        $validated = $request->validated();

        try {
            $node = $this->nodeActionService->createNode(collect($validated));

            Log::info("Node created via API", [
                'node_id' => $node->node_id,
                'node_type' => $node->node_type,
                'created_by' => 'api',
            ]);
            
            return response()->json([
                'success' => true,
                'message' => 'Node created successfully',
                'node' => $node,
            ], 201);
        } catch (DuplicateNodeException $e) {
            return response()->json([
                'success' => false,
                'error' => 'duplicate_node_id',
                'message' => 'Узел с таким ID уже существует',
            ], 409);
        }
    }

    /**
     * Обновление конфигурации узла
     */
    public function updateConfig(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = collect($request->validate([
            'config' => 'required|array',
            'comment' => 'nullable|string|max:500',
            'cycle_id' => 'nullable|integer|exists:growth_cycles,id',
            'require_confirmation' => 'nullable|boolean',
        ]))->put('require_confirmation', (bool) $request->boolean('require_confirmation'));

        $result = $this->nodeActionService->updateConfig(
            $node,
            $validated,
            $request->user()?->email
        );

        return $this->jsonFromCollection($result);
    }

    /**
     * Отправка команды узлу
     */
    public function sendCommand(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = collect($request->validate([
            'command' => 'required|string|max:100',
            'params' => 'nullable|array',
        ]));

        $result = $this->nodeActionService->dispatchCommand($node, $validated);

        return $this->jsonFromCollection($result);
    }

    /**
     * Статистика по узлу
     */
    public function statistics(string $nodeId, Request $request): JsonResponse
    {
        $hours = (int) $request->get('hours', 24);
        $statistics = $this->nodeCatalogService->collectStatistics($nodeId, $hours);

        return response()->json($statistics);
    }

    /**
     * Обновление основных полей узла
     */
    public function update(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = collect($request->validate([
            'zone' => 'nullable|string|max:100',
            'mac_address' => 'nullable|string|size:17',
            'config' => 'nullable|array',
            'metadata' => 'nullable|array',
            'greenhouse_id' => 'nullable|integer|exists:greenhouses,id',
        ]));

        $updated = $this->nodeActionService->updateNode($node, $validated);

        return response()->json([
            'success' => true,
            'message' => 'Node updated',
            'node' => $updated,
        ]);
    }

    /**
     * Удаление узла
     */
    public function destroy(string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $this->nodeActionService->deleteNode($node);

        return response()->json([
            'success' => true,
            'message' => 'Node and all related data deleted',
        ]);
    }

    /**
     * Ручной запуск насоса
     */
    public function runPump(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = collect($request->validate([
            'pump_id' => 'required|integer|min:0|max:5',
            'duration_sec' => 'required|numeric|min:0.1|max:30',
        ]));

        $result = $this->nodeActionService->runPump($node, $validated);

        return $this->jsonFromCollection($result);
    }

    /**
     * Калибровка насоса
     */
    public function calibratePump(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = collect($request->validate([
            'pump_id' => 'required|integer|min:0|max:5',
            'duration_sec' => 'required|numeric|min:0.1|max:60',
            'volume_ml' => 'required|numeric|min:0.1|max:1000',
        ]));

        $result = $this->nodeActionService->calibratePump(
            $node,
            $validated,
            $request->user()?->email
        );

        return $this->jsonFromCollection($result);
    }

    /**
     * Запрос конфигурации от узла
     */
    public function requestConfig(string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $result = $this->nodeActionService->requestConfig($node);

        return $this->jsonFromCollection($result);
    }

    /**
     * Получение калибровки насосов
     */
    public function getPumpCalibrations(string $nodeId): JsonResponse
    {
        return response()->json(
            $this->nodeCatalogService->loadPumpCalibrations($nodeId)
        );
    }

    /**
     * Получение истории изменений конфигурации
     */
    public function getConfigHistory(string $nodeId): JsonResponse
    {
        return response()->json(
            $this->nodeCatalogService->loadConfigHistory($nodeId)
        );
    }

    private function jsonFromCollection(Collection $result): JsonResponse
    {
        $status = $result->get('http_status', 200);
        $payload = $result->except('http_status');

        return response()->json($payload, $status);
    }
}

