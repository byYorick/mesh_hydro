<?php

namespace App\Services;

use App\Models\GrowthCycle;
use App\Models\Node;
use App\Models\NodeConfigurationConfirmation;
use App\Services\MqttService;
use Carbon\Carbon;
use Illuminate\Support\Facades\Log;

/**
 * ⭐ GROWTH PLANNER: Сервис для управления подтверждениями конфигурации узлов
 */
class NodeConfigurationService
{
    public function __construct(
        private readonly MqttService $mqttService,
    ) {
    }

    /**
     * Отправка конфигурации узлу с отслеживанием подтверждения
     */
    public function sendConfigurationWithConfirmation(
        string $nodeId,
        array $config,
        ?int $cycleId = null,
        ?string $commandId = null
    ): NodeConfigurationConfirmation {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        // Создаем запись о подтверждении
        $confirmation = NodeConfigurationConfirmation::create([
            'node_id' => $nodeId,
            'cycle_id' => $cycleId,
            'command_id' => $commandId,
            'sent_config' => $config,
            'status' => 'pending',
            'sent_at' => now(),
        ]);

        // Отправляем конфигурацию через MQTT
        try {
            // Добавляем ID подтверждения в конфигурацию для обратной связи
            $configWithId = array_merge($config, [
                '_confirmation_id' => $confirmation->id,
            ]);

            $this->mqttService->sendConfig($nodeId, $configWithId);

            Log::info("Configuration sent with confirmation tracking", [
                'node_id' => $nodeId,
                'confirmation_id' => $confirmation->id,
                'cycle_id' => $cycleId,
            ]);

        } catch (\Exception $e) {
            $confirmation->fail("Failed to send via MQTT: " . $e->getMessage());
            
            Log::error("Failed to send configuration", [
                'node_id' => $nodeId,
                'confirmation_id' => $confirmation->id,
                'error' => $e->getMessage(),
            ]);
        }

        return $confirmation;
    }

    /**
     * Обработка подтверждения конфигурации от узла
     */
    public function handleConfigurationConfirmation(
        string $nodeId,
        array $confirmedConfig,
        ?int $confirmationId = null
    ): ?NodeConfigurationConfirmation {
        // Если передан ID подтверждения, находим по нему
        if ($confirmationId) {
            $confirmation = NodeConfigurationConfirmation::find($confirmationId);
        } else {
            // Иначе ищем последнее ожидающее подтверждение для этого узла
            $confirmation = NodeConfigurationConfirmation::where('node_id', $nodeId)
                ->where('status', 'pending')
                ->orderBy('sent_at', 'desc')
                ->first();
        }

        if (!$confirmation) {
            Log::warning("Configuration confirmation received but no pending record found", [
                'node_id' => $nodeId,
                'confirmation_id' => $confirmationId,
            ]);
            return null;
        }

        // Подтверждаем конфигурацию
        $confirmation->confirm($confirmedConfig);

        Log::info("Configuration confirmed by node", [
            'node_id' => $nodeId,
            'confirmation_id' => $confirmation->id,
            'cycle_id' => $confirmation->cycle_id,
        ]);

        // Если это связано с циклом роста, можно обновить статус цикла
        if ($confirmation->cycle_id) {
            $this->handleCycleConfigurationUpdate($confirmation);
        }

        return $confirmation;
    }

    /**
     * Обработка обновления конфигурации для цикла роста
     */
    protected function handleCycleConfigurationUpdate(NodeConfigurationConfirmation $confirmation): void
    {
        $cycle = GrowthCycle::find($confirmation->cycle_id);
        if (!$cycle) {
            return;
        }

        // Проверяем, все ли узлы зоны подтвердили конфигурацию
        $zone = $cycle->zone;
        if (!$zone) {
            return;
        }

        // Получаем все узлы зоны
        $zoneNodes = $zone->getAllNodes();
        $nodeIds = $zoneNodes->pluck('node_id')->toArray();

        // Проверяем подтверждения для всех узлов зоны
        $pendingConfirmations = NodeConfigurationConfirmation::where('cycle_id', $cycle->id)
            ->whereIn('node_id', $nodeIds)
            ->where('status', 'pending')
            ->count();

        if ($pendingConfirmations === 0) {
            // Все узлы подтвердили конфигурацию
            Log::info("All zone nodes confirmed configuration for cycle", [
                'cycle_id' => $cycle->id,
                'zone_id' => $zone->id,
            ]);

            // TODO: Можно отправить уведомление или обновить статус цикла
        }
    }

    /**
     * Проверка таймаутов подтверждений
     */
    public function checkTimeouts(): int
    {
        $timeouts = NodeConfigurationConfirmation::where('status', 'pending')
            ->where('sent_at', '<', now()->subSeconds(60))
            ->get();

        $count = 0;
        foreach ($timeouts as $confirmation) {
            $confirmation->fail('Timeout: No confirmation received within 60 seconds');
            $count++;

            Log::warning("Configuration confirmation timeout", [
                'confirmation_id' => $confirmation->id,
                'node_id' => $confirmation->node_id,
                'cycle_id' => $confirmation->cycle_id,
            ]);
        }

        return $count;
    }

    /**
     * Получить статус подтверждений для цикла
     */
    public function getCycleConfirmationStatus(int $cycleId): array
    {
        $confirmations = NodeConfigurationConfirmation::where('cycle_id', $cycleId)
            ->with('node')
            ->get();

        $status = [
            'total' => $confirmations->count(),
            'confirmed' => $confirmations->where('status', 'confirmed')->count(),
            'pending' => $confirmations->where('status', 'pending')->count(),
            'failed' => $confirmations->where('status', 'failed')->count(),
            'timeout' => $confirmations->where('status', 'timeout')->count(),
            'confirmations' => $confirmations,
        ];

        return $status;
    }

    /**
     * Обработка ответа конфигурации от узла (алиас для handleConfigurationConfirmation)
     */
    public function handleConfigurationResponse(
        int $confirmationId,
        array $receivedConfig
    ): ?NodeConfigurationConfirmation {
        $confirmation = NodeConfigurationConfirmation::find($confirmationId);
        
        if (!$confirmation) {
            return null;
        }

        return $this->handleConfigurationConfirmation(
            $confirmation->node_id,
            $receivedConfig,
            $confirmationId
        );
    }

    /**
     * Пометить подтверждение как проваленное
     */
    public function markAsFailed(int $confirmationId, string $reason = 'Unknown error'): bool
    {
        $confirmation = NodeConfigurationConfirmation::find($confirmationId);
        
        if (!$confirmation) {
            return false;
        }

        $confirmation->fail($reason);
        
        Log::warning("Configuration marked as failed", [
            'confirmation_id' => $confirmationId,
            'node_id' => $confirmation->node_id,
            'reason' => $reason,
        ]);

        return true;
    }

    /**
     * Получить ожидающие подтверждения для узла
     */
    public function getPendingConfirmations(string $nodeId)
    {
        return NodeConfigurationConfirmation::where('node_id', $nodeId)
            ->where('status', 'pending')
            ->orderBy('sent_at', 'desc')
            ->get();
    }
}

