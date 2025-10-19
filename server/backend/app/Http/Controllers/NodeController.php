<?php

namespace App\Http\Controllers;

use App\Models\Node;
use App\Models\Command;
use App\Services\MqttService;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use App\Http\Requests\StoreNodeRequest;
use App\Http\Requests\UpdateNodeRequest;
use App\Http\Requests\SendCommandRequest;
use Illuminate\Support\Facades\Log;

class NodeController extends Controller
{
    /**
     * Список всех узлов с последней телеметрией
     */
    public function index(Request $request): JsonResponse
    {
        $query = Node::query();

        // Фильтр по типу
        if ($request->has('type')) {
            $query->where('node_type', $request->type);
        }

        // Фильтр по статусу
        if ($request->has('status')) {
            if ($request->status === 'online') {
                $query->online();
            } elseif ($request->status === 'offline') {
                $query->offline();
            }
        }

        // Загрузка последней телеметрии с appends
        $nodes = $query->with(['lastTelemetry'])->get();

        // Добавление вычисляемых полей через appends
        $nodes->makeVisible(['is_online', 'status_color', 'icon']);

        return response()->json($nodes);
    }

    /**
     * Детальная информация об узле
     */
    public function show(string $nodeId): JsonResponse
    {
        $node = Node::with([
            'telemetry' => function ($query) {
                $query->latest('received_at')->limit(100);
            },
            'events' => function ($query) {
                $query->latest()->limit(50);
            },
            'commands' => function ($query) {
                $query->latest()->limit(20);
            }
        ])
        ->where('node_id', $nodeId)
        ->firstOrFail();

        $node->is_online = $node->isOnline();
        $node->status_color = $node->status_color;
        $node->icon = $node->icon;

        return response()->json($node);
    }

    /**
     * Создание нового узла
     */
    public function store(StoreNodeRequest $request): JsonResponse
    {
        $validated = $request->validated();

        // Создаём новый узел (unique constraint защитит от дублей)
        try {
            $node = Node::create($validated);
            
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
            
        } catch (\Illuminate\Database\QueryException $e) {
            // Handle duplicate node_id (SQLite: 19, PostgreSQL/MySQL: 23000)
            if ($e->errorInfo[0] == 23000 || $e->errorInfo[1] == 19) {
                return response()->json([
                    'success' => false,
                    'error' => 'duplicate_node_id',
                    'message' => 'Узел с таким ID уже существует',
                ], 409);
            }
            
            throw $e;
        }
    }

    /**
     * Обновление конфигурации узла
     */
    public function updateConfig(Request $request, string $nodeId, MqttService $mqtt): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = $request->validate([
            'config' => 'required|array',
        ]);

        // Сохранение в БД
        $node->update(['config' => $validated['config']]);

        // Отправка конфигурации на узел через MQTT
        if ($node->isOnline()) {
            try {
                $mqtt->sendConfig($nodeId, $validated['config']);
                $message = 'Config updated and sent to node';
            } catch (\Exception $e) {
                $message = 'Config updated but failed to send to node: ' . $e->getMessage();
            }
        } else {
            $message = 'Config updated but node is offline';
        }

        return response()->json([
            'success' => true,
            'message' => $message,
            'node' => $node,
        ]);
    }

    /**
     * Отправка команды узлу
     */
    public function sendCommand(Request $request, string $nodeId, MqttService $mqtt): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = $request->validate([
            'command' => 'required|string|max:100',
            'params' => 'nullable|array',
        ]);

        // Проверка что узел онлайн
        if (!$node->isOnline()) {
            return response()->json([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        // Создание записи команды в БД
        $command = Command::create([
            'node_id' => $nodeId,
            'command' => $validated['command'],
            'params' => $validated['params'] ?? [],
            'status' => Command::STATUS_PENDING,
        ]);

        // Отправка через MQTT
        try {
            $mqtt->sendCommand(
                $nodeId,
                $validated['command'],
                $validated['params'] ?? [],
                $command->id
            );

            $command->markAsSent();

            return response()->json([
                'success' => true,
                'message' => 'Command sent to node',
                'command' => $command,
            ]);
        } catch (\Exception $e) {
            $command->markAsFailed($e->getMessage());

            return response()->json([
                'success' => false,
                'error' => 'Failed to send command: ' . $e->getMessage(),
                'command' => $command,
            ], 500);
        }
    }

    /**
     * Статистика по узлу
     */
    public function statistics(string $nodeId, Request $request): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $hours = $request->get('hours', 24);

        $telemetry = $node->telemetry()
            ->where('received_at', '>', now()->subHours($hours))
            ->orderBy('received_at', 'asc')
            ->get();

        return response()->json([
            'node_id' => $nodeId,
            'period_hours' => $hours,
            'data_points' => $telemetry->count(),
            'telemetry' => $telemetry,
        ]);
    }

    /**
     * Обновление основных полей узла
     */
    public function update(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = $request->validate([
            'zone' => 'nullable|string|max:100',
            'mac_address' => 'nullable|string|size:17',
            'config' => 'nullable|array',
            'metadata' => 'nullable|array',
        ]);

        $node->update($validated);

        return response()->json([
            'success' => true,
            'message' => 'Node updated',
            'node' => $node,
        ]);
    }

    /**
     * Удаление узла
     */
    public function destroy(string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();
        
        // Delete related data
        $node->telemetry()->delete();
        $node->events()->delete();
        $node->commands()->delete();
        
        // Delete node
        $node->delete();

        return response()->json([
            'success' => true,
            'message' => 'Node and all related data deleted',
        ]);
    }
}

