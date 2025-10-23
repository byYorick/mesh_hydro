<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\Schedule;
use App\Models\Node;
use App\Services\MqttService;
use Illuminate\Support\Facades\Log;

class SchedulerRunCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'scheduler:run 
                            {--check-interval=60 : Интервал проверки в секундах}';

    /**
     * The console command description.
     */
    protected $description = 'Запуск scheduler для автоматического применения расписаний';

    /**
     * Execute the console command.
     */
    public function handle(MqttService $mqtt): int
    {
        $this->info('╔════════════════════════════════════════╗');
        $this->info('║   Scheduler for Hydro Mesh System     ║');
        $this->info('╚════════════════════════════════════════╝');
        $this->newLine();

        $checkInterval = (int) $this->option('check-interval');
        $lastCheck = [];

        $this->info("🔄 Scheduler started (checking every {$checkInterval} seconds)");
        $this->info("Press Ctrl+C to stop");
        $this->newLine();

        while (true) {
            try {
                $this->checkAndApplySchedules($mqtt, $lastCheck);
            } catch (\Exception $e) {
                $this->error("❌ Scheduler error: {$e->getMessage()}");
                Log::error("Scheduler error", [
                    'error' => $e->getMessage(),
                    'trace' => $e->getTraceAsString()
                ]);
            }

            sleep($checkInterval);
        }

        return 0;
    }

    /**
     * Проверка и применение активных расписаний
     */
    private function checkAndApplySchedules(MqttService $mqtt, array &$lastCheck): void
    {
        // Получить все узлы с расписаниями
        $nodesWithSchedules = Schedule::where('enabled', true)
            ->select('node_id')
            ->distinct()
            ->pluck('node_id');

        foreach ($nodesWithSchedules as $nodeId) {
            // Получить активное расписание
            $activeSchedule = Schedule::getActiveForNode($nodeId);

            if ($activeSchedule) {
                // Проверка что это новое расписание (не то же что было)
                if (!isset($lastCheck[$nodeId]) || $lastCheck[$nodeId] !== $activeSchedule->id) {
                    $this->applySchedule($nodeId, $activeSchedule, $mqtt);
                    $lastCheck[$nodeId] = $activeSchedule->id;
                }
            } else {
                // Нет активного расписания - сбросить
                unset($lastCheck[$nodeId]);
            }
        }
    }

    /**
     * Применение расписания к узлу
     */
    private function applySchedule(string $nodeId, Schedule $schedule, MqttService $mqtt): void
    {
        $this->line("⏰ [{$nodeId}] Applying schedule: {$schedule->name}");

        // Проверка что узел онлайн
        $node = Node::where('node_id', $nodeId)->first();
        if (!$node || !$node->isOnline()) {
            $this->warn("   ⚠️ Node offline, schedule skipped");
            Log::warning("Schedule skipped: node offline", [
                'node_id' => $nodeId,
                'schedule' => $schedule->name
            ]);
            return;
        }

        // Отправка конфигурации через MQTT
        try {
            $mqtt->sendConfig($nodeId, $schedule->config);

            $this->info("   ✓ Config sent to node");
            Log::info("Schedule applied", [
                'node_id' => $nodeId,
                'schedule' => $schedule->name,
                'config' => $schedule->config
            ]);

            // Логирование в истории конфигураций
            \App\Models\ConfigHistory::logChange(
                $nodeId,
                $node->config ?? [],
                $schedule->config,
                'schedule_applied',
                'scheduler',
                "Auto-applied schedule: {$schedule->name}"
            );

        } catch (\Exception $e) {
            $this->error("   ✗ Failed to send config: {$e->getMessage()}");
            Log::error("Failed to apply schedule", [
                'node_id' => $nodeId,
                'schedule' => $schedule->name,
                'error' => $e->getMessage()
            ]);
        }
    }
}

