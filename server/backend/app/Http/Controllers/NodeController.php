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

        // Добавление вычисляемых полей для каждого узла
        $nodes->each(function ($node) {
            // Обновляем online статус на основе isOnline() метода
            $node->online = $node->isOnline();
            $node->status_color = $node->status_color;
            $node->icon = $node->icon;
        });

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

        // Обновляем online статус на основе isOnline() метода
        $node->online = $node->isOnline();
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
            // Handle duplicate node_id (PostgreSQL: 23000)
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
            'comment' => 'nullable|string|max:500',
        ]);

        // Сохранение старой конфигурации для истории
        $oldConfig = $node->config ?? [];

        // Логирование изменений в истории
        \App\Models\ConfigHistory::logChange(
            $nodeId,
            $oldConfig,
            $validated['config'],
            'update_config',
            $request->user()?->email ?? 'api',
            $validated['comment'] ?? null
        );

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

    /**
     * Ручной запуск насоса
     */
    public function runPump(Request $request, string $nodeId, MqttService $mqtt): JsonResponse
    {
        // Простое отладочное сообщение
        error_log("=== RUNPUMP CALLED ===");
        error_log("Node ID: " . $nodeId);
        error_log("Request data: " . json_encode($request->all()));
        error_log("Request method: " . $request->method());
        error_log("Request headers: " . json_encode($request->headers->all()));
        
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        // Отладочная информация
        Log::info("runPump request data", [
            'node_id' => $nodeId,
            'request_data' => $request->all(),
            'content_type' => $request->header('Content-Type'),
        ]);

        $validated = $request->validate([
            'pump_id' => 'required|integer|min:0|max:5',
            'duration_sec' => 'required|numeric|min:0.1|max:30',
        ]);

        // Проверка что узел онлайн
        if (!$node->isOnline()) {
            return response()->json([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        // Отправка команды через MQTT
        try {
            $mqtt->sendCommand(
                $nodeId,
                'run_pump_manual',
                [
                    'pump_id' => $validated['pump_id'],
                    'duration_sec' => $validated['duration_sec'],
                ]
            );

            Log::info("Pump run command sent", [
                'node_id' => $nodeId,
                'pump_id' => $validated['pump_id'],
                'duration_sec' => $validated['duration_sec'],
            ]);

            return response()->json([
                'success' => true,
                'message' => "Pump {$validated['pump_id']} started for {$validated['duration_sec']} seconds",
            ]);
        } catch (\Exception $e) {
            Log::error("Failed to send pump run command", [
                'node_id' => $nodeId,
                'error' => $e->getMessage(),
            ]);

            return response()->json([
                'success' => false,
                'error' => 'Failed to send command: ' . $e->getMessage(),
            ], 500);
        }
    }

    /**
     * Калибровка насоса
     */
    public function calibratePump(Request $request, string $nodeId, MqttService $mqtt): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = $request->validate([
            'pump_id' => 'required|integer|min:0|max:5',
            'duration_sec' => 'required|numeric|min:0.1|max:60',
            'volume_ml' => 'required|numeric|min:0.1|max:1000',
        ]);

        // Проверка что узел онлайн
        if (!$node->isOnline()) {
            return response()->json([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        // Расчет производительности
        $mlPerSecond = $validated['volume_ml'] / $validated['duration_sec'];

        // Получить старую калибровку для истории
        $oldCalibration = \App\Models\PumpCalibration::where('node_id', $nodeId)
            ->where('pump_id', $validated['pump_id'])
            ->first();

        // Сохранение калибровки в БД
        $calibration = \App\Models\PumpCalibration::updateOrCreate(
            [
                'node_id' => $nodeId,
                'pump_id' => $validated['pump_id'],
            ],
            [
                'ml_per_second' => $mlPerSecond,
                'calibration_volume_ml' => $validated['volume_ml'],
                'calibration_time_ms' => (int)($validated['duration_sec'] * 1000),
                'is_calibrated' => true,
                'calibrated_at' => now(),
            ]
        );

        // Логирование в истории
        \App\Models\ConfigHistory::logChange(
            $nodeId,
            $oldCalibration ? ['pump_' . $validated['pump_id'] . '_ml_per_sec' => $oldCalibration->ml_per_second] : [],
            ['pump_' . $validated['pump_id'] . '_ml_per_sec' => $mlPerSecond],
            'calibrate_pump',
            $request->user()?->email ?? 'api',
            "Pump #{$validated['pump_id']}: {$validated['volume_ml']} ml in {$validated['duration_sec']} sec"
        );

        // Отправка конфига калибровки в NVS на узел
        try {
            // Отправляем конфиг калибровки в NVS
            $mqtt->sendCommand(
                $nodeId,
                'set_config',
                [
                    'pump_' . $validated['pump_id'] . '_ml_per_sec' => $mlPerSecond,
                    'pump_' . $validated['pump_id'] . '_calibration_volume' => $validated['volume_ml'],
                    'pump_' . $validated['pump_id'] . '_calibration_time' => $validated['duration_sec'],
                ]
            );

            Log::info("Pump calibration saved and sent", [
                'node_id' => $nodeId,
                'pump_id' => $validated['pump_id'],
                'ml_per_second' => $mlPerSecond,
            ]);

            // Отправка Telegram уведомления
            if (config('telegram.enabled', false)) {
                try {
                    app(\App\Services\TelegramService::class)->sendCalibrationAlert(
                        $nodeId,
                        $validated['pump_id'],
                        $mlPerSecond
                    );
                } catch (\Exception $e) {
                    Log::warning("Failed to send Telegram notification", ['error' => $e->getMessage()]);
                }
            }

            return response()->json([
                'success' => true,
                'message' => "Pump {$validated['pump_id']} calibrated: {$mlPerSecond} ml/s",
                'calibration' => $calibration,
            ]);
        } catch (\Exception $e) {
            Log::error("Failed to send calibration command", [
                'node_id' => $nodeId,
                'error' => $e->getMessage(),
            ]);

            return response()->json([
                'success' => false,
                'error' => 'Calibration saved to DB but failed to send to node: ' . $e->getMessage(),
                'calibration' => $calibration,
            ], 500);
        }
    }

    /**
     * Запрос конфигурации от узла
     */
    public function requestConfig(string $nodeId, MqttService $mqtt): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        // Проверка что узел онлайн
        if (!$node->isOnline()) {
            return response()->json([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        // Отправка команды get_config
        try {
            $mqtt->sendCommand($nodeId, 'get_config', []);

            Log::info("Config request sent to node", ['node_id' => $nodeId]);

            return response()->json([
                'success' => true,
                'message' => 'Config request sent to node. Check WebSocket for response.',
            ]);
        } catch (\Exception $e) {
            Log::error("Failed to request config", [
                'node_id' => $nodeId,
                'error' => $e->getMessage(),
            ]);

            return response()->json([
                'success' => false,
                'error' => 'Failed to send request: ' . $e->getMessage(),
            ], 500);
        }
    }

    /**
     * Получение калибровки насосов
     */
    public function getPumpCalibrations(string $nodeId): JsonResponse
    {
        $calibrations = \App\Models\PumpCalibration::where('node_id', $nodeId)
            ->orderBy('pump_id')
            ->get();

        return response()->json([
            'success' => true,
            'calibrations' => $calibrations,
        ]);
    }

    /**
     * Получение истории изменений конфигурации
     */
    public function getConfigHistory(string $nodeId): JsonResponse
    {
        $history = \App\Models\ConfigHistory::where('node_id', $nodeId)
            ->orderBy('changed_at', 'desc')
            ->limit(50)
            ->get();

        return response()->json([
            'success' => true,
            'history' => $history,
        ]);
    }
}

