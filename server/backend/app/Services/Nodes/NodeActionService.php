<?php

namespace App\Services\Nodes;

use App\Exceptions\Nodes\DuplicateNodeException;
use App\Models\Command;
use App\Models\ConfigHistory;
use App\Models\Node;
use App\Models\PumpCalibration;
use App\Services\MqttService;
use App\Services\NodeConfigurationService;
use App\Services\TelegramService;
use Illuminate\Database\QueryException;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Log;
use Throwable;

class NodeActionService
{
    public function __construct(
        private readonly NodeConfigurationService $configurationService,
        private readonly MqttService $mqttService,
        private readonly ?TelegramService $telegramService = null,
    ) {
    }

    public function createNode(Collection $payload): Node
    {
        try {
            return Node::create($payload->toArray());
        } catch (QueryException $exception) {
            if ($this->isDuplicateKey($exception)) {
                throw new DuplicateNodeException('Node with this identifier already exists.', previous: $exception);
            }

            throw $exception;
        }
    }

    public function updateNode(Node $node, Collection $changes): Node
    {
        $node->update($changes->toArray());

        if ($changes->has('greenhouse_id')) {
            $node->load('greenhouse');
        }

        return $node;
    }

    public function deleteNode(Node $node): void
    {
        DB::transaction(function () use ($node) {
            $node->telemetry()->delete();
            $node->events()->delete();
            $node->commands()->delete();
            $node->delete();
        });
    }

    public function updateConfig(Node $node, Collection $data, ?string $performedBy): Collection
    {
        $newConfig = $data->get('config', []);
        $comment = $data->get('comment');
        $cycleId = $data->get('cycle_id');
        $requireConfirmation = (bool) $data->get('require_confirmation', false);

        $oldConfig = $node->config ?? [];

        ConfigHistory::logChange(
            $node->node_id,
            $oldConfig,
            $newConfig,
            'update_config',
            $performedBy ?? 'api',
            $comment
        );

        $node->update(['config' => $newConfig]);

        if ($requireConfirmation && $node->isOnline()) {
            $confirmation = $this->configurationService->sendConfigurationWithConfirmation(
                $node->node_id,
                $newConfig,
                $cycleId
            );

            return $this->response([
                'success' => true,
                'status' => 'awaiting_confirmation',
                'message' => 'Config updated and sent to node (awaiting confirmation)',
                'node' => $node,
                'confirmation_id' => $confirmation->id,
                'confirmation' => $confirmation,
            ]);
        }

        if ($node->isOnline()) {
            try {
                $this->mqttService->sendConfig($node->node_id, $newConfig);

                return $this->response([
                    'success' => true,
                    'status' => 'sent',
                    'message' => 'Config updated and sent to node',
                    'node' => $node,
                ]);
            } catch (Throwable $exception) {
                Log::warning('Failed to send node config over MQTT', [
                    'node_id' => $node->node_id,
                    'error' => $exception->getMessage(),
                ]);

                return $this->response([
                    'success' => true,
                    'status' => 'queued',
                    'message' => 'Config updated but failed to send to node: ' . $exception->getMessage(),
                    'node' => $node,
                ]);
            }
        }

        return $this->response([
            'success' => true,
            'status' => 'queued',
            'message' => 'Config updated but node is offline',
            'node' => $node,
        ]);
    }

    public function dispatchCommand(Node $node, Collection $data): Collection
    {
        $commandName = $data->get('command');
        $params = $data->get('params', []);

        $command = Command::create([
            'node_id' => $node->node_id,
            'command' => $commandName,
            'params' => $params,
            'status' => Command::STATUS_PENDING,
        ]);

        if (!$node->isOnline()) {
            return $this->response([
                'success' => true,
                'status' => 'queued',
                'message' => 'Node is offline, command queued for delivery',
                'command' => $command,
            ]);
        }

        try {
            $this->mqttService->sendCommand(
                $node->node_id,
                $commandName,
                $params,
                $command->id
            );

            $command->markAsSent();

            return $this->response([
                'success' => true,
                'status' => 'sent',
                'message' => 'Command sent to node',
                'command' => $command->fresh(),
            ]);
        } catch (Throwable $exception) {
            $command->markAsFailed($exception->getMessage());

            return $this->response([
                'success' => false,
                'status' => 'failed',
                'error' => 'Failed to send command: ' . $exception->getMessage(),
                'command' => $command->fresh(),
            ], 500);
        }
    }

    public function runPump(Node $node, Collection $data): Collection
    {
        if (!$node->isOnline()) {
            return $this->response([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        $payload = [
            'pump_id' => (int) $data->get('pump_id'),
            'duration_sec' => (float) $data->get('duration_sec'),
        ];

        Log::info('Manual pump run request', [
            'node_id' => $node->node_id,
            'payload' => $payload,
        ]);

        try {
            $this->mqttService->sendCommand(
                $node->node_id,
                'run_pump_manual',
                $payload
            );

            return $this->response([
                'success' => true,
                'message' => sprintf(
                    'Pump %d started for %s seconds',
                    $payload['pump_id'],
                    $payload['duration_sec']
                ),
            ]);
        } catch (Throwable $exception) {
            Log::error('Failed to send pump run command', [
                'node_id' => $node->node_id,
                'error' => $exception->getMessage(),
            ]);

            return $this->response([
                'success' => false,
                'error' => 'Failed to send command: ' . $exception->getMessage(),
            ], 500);
        }
    }

    public function calibratePump(Node $node, Collection $data, ?string $performedBy): Collection
    {
        if (!$node->isOnline()) {
            return $this->response([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        $pumpId = (int) $data->get('pump_id');
        $duration = (float) $data->get('duration_sec');
        $volume = (float) $data->get('volume_ml');
        $mlPerSecond = $volume / $duration;

        $oldCalibration = PumpCalibration::where('node_id', $node->node_id)
            ->where('pump_id', $pumpId)
            ->first();

        $calibration = PumpCalibration::updateOrCreate(
            [
                'node_id' => $node->node_id,
                'pump_id' => $pumpId,
            ],
            [
                'ml_per_second' => $mlPerSecond,
                'calibration_volume_ml' => $volume,
                'calibration_time_ms' => (int) ($duration * 1000),
                'is_calibrated' => true,
                'calibrated_at' => now(),
            ]
        );

        ConfigHistory::logChange(
            $node->node_id,
            $oldCalibration ? ['pump_' . $pumpId . '_ml_per_sec' => $oldCalibration->ml_per_second] : [],
            ['pump_' . $pumpId . '_ml_per_sec' => $mlPerSecond],
            'calibrate_pump',
            $performedBy ?? 'api',
            sprintf('Pump #%d: %.2f ml in %.2f sec', $pumpId, $volume, $duration)
        );

        try {
            $this->mqttService->sendCommand(
                $node->node_id,
                'set_config',
                [
                    'pump_' . $pumpId . '_ml_per_sec' => $mlPerSecond,
                    'pump_' . $pumpId . '_calibration_volume' => $volume,
                    'pump_' . $pumpId . '_calibration_time' => $duration,
                ]
            );

            if (config('telegram.enabled', false) && $this->telegramService) {
                try {
                    $this->telegramService->sendCalibrationAlert(
                        $node->node_id,
                        $pumpId,
                        $mlPerSecond
                    );
                } catch (Throwable $exception) {
                    Log::warning('Failed to push telegram calibration alert', [
                        'node_id' => $node->node_id,
                        'error' => $exception->getMessage(),
                    ]);
                }
            }

            return $this->response([
                'success' => true,
                'message' => sprintf('Pump %d calibrated: %.2f ml/s', $pumpId, $mlPerSecond),
                'calibration' => $calibration,
            ]);
        } catch (Throwable $exception) {
            Log::error('Failed to send pump calibration command', [
                'node_id' => $node->node_id,
                'error' => $exception->getMessage(),
            ]);

            return $this->response([
                'success' => false,
                'error' => 'Calibration saved to DB but failed to send to node: ' . $exception->getMessage(),
                'calibration' => $calibration,
            ], 500);
        }
    }

    public function requestConfig(Node $node): Collection
    {
        if (!$node->isOnline()) {
            return $this->response([
                'success' => false,
                'error' => 'Node is offline',
            ], 400);
        }

        try {
            $this->mqttService->sendCommand($node->node_id, 'get_config', []);

            return $this->response([
                'success' => true,
                'message' => 'Config request sent to node. Check WebSocket for response.',
            ]);
        } catch (Throwable $exception) {
            Log::error('Failed to request config from node', [
                'node_id' => $node->node_id,
                'error' => $exception->getMessage(),
            ]);

            return $this->response([
                'success' => false,
                'error' => 'Failed to send request: ' . $exception->getMessage(),
            ], 500);
        }
    }

    private function response(array $payload, int $status = 200): Collection
    {
        return collect($payload + ['http_status' => $status]);
    }

    private function isDuplicateKey(QueryException $exception): bool
    {
        $sqlState = $exception->errorInfo[0] ?? null;
        $driverCode = $exception->errorInfo[1] ?? null;

        return $sqlState === '23000' || $sqlState === '23505' || $driverCode === 19;
    }
}


