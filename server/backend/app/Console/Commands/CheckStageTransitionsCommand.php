<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\GrowthCycle;
use App\Models\StageTransitionRecommendation;
use App\Services\MqttService;
use Illuminate\Support\Facades\Log;
use Carbon\Carbon;

class CheckStageTransitionsCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'growth:check-transitions';

    /**
     * The console command description.
     */
    protected $description = 'Проверка активных циклов роста и рекомендации по переходам стадий';

    /**
     * Execute the console command.
     */
    public function handle(MqttService $mqtt): int
    {
        $this->info('🔍 Проверка переходов стадий для активных циклов...');
        $this->newLine();

        // Получаем все активные циклы
        $activeCycles = GrowthCycle::where('status', 'active')
            ->with(['preset.stages', 'currentStage', 'zone'])
            ->get();

        if ($activeCycles->isEmpty()) {
            $this->info('✅ Нет активных циклов для проверки');
            return 0;
        }

        $this->info("Найдено {$activeCycles->count()} активных циклов");
        $this->newLine();

        $recommendationsCount = 0;

        foreach ($activeCycles as $cycle) {
            $this->line("Проверка цикла #{$cycle->id}: {$cycle->name} (Зона: {$cycle->zone->name})");

            try {
                $recommendation = $this->checkCycleStageTransition($cycle);

                if ($recommendation) {
                    $recommendationsCount++;
                    $this->info("  ✅ Рекомендация создана для перехода на стадию: {$recommendation->next_stage->name}");
                } else {
                    $this->line("  ⏳ Цикл на правильной стадии");
                }
            } catch (\Exception $e) {
                $this->error("  ❌ Ошибка при проверке цикла #{$cycle->id}: {$e->getMessage()}");
                Log::error("Error checking cycle stage transition", [
                    'cycle_id' => $cycle->id,
                    'error' => $e->getMessage(),
                    'trace' => $e->getTraceAsString()
                ]);
            }
        }

        $this->newLine();
        $this->info("✅ Проверка завершена. Создано рекомендаций: {$recommendationsCount}");

        return 0;
    }

    /**
     * Проверка необходимости перехода стадии для цикла
     */
    protected function checkCycleStageTransition(GrowthCycle $cycle): ?StageTransitionRecommendation
    {
        // Проверяем, что у цикла есть пресет и текущая стадия
        if (!$cycle->preset || !$cycle->current_stage_id) {
            return null;
        }

        $currentStage = $cycle->currentStage;
        if (!$currentStage) {
            return null;
        }

        // Получаем следующую стадию
        $nextStage = $cycle->getNextStage();
        if (!$nextStage) {
            // Цикл уже на последней стадии
            return null;
        }

        // Проверяем, не создана ли уже рекомендация для этого перехода
        $existingRecommendation = StageTransitionRecommendation::where('cycle_id', $cycle->id)
            ->where('current_stage_id', $currentStage->id)
            ->where('recommended_stage_id', $nextStage->id)
            ->where('status', 'pending')
            ->first();

        if ($existingRecommendation) {
            return null; // Рекомендация уже существует
        }

        // Проверяем условия перехода
        $shouldTransition = $this->shouldTransitionToNextStage($cycle, $currentStage, $nextStage);

        if (!$shouldTransition) {
            return null;
        }

        // Создаем рекомендацию
        $recommendation = StageTransitionRecommendation::create([
            'cycle_id' => $cycle->id,
            'current_stage_id' => $currentStage->id,
            'recommended_stage_id' => $nextStage->id,
            'status' => 'pending',
            'reason' => $this->getTransitionReason($cycle, $currentStage, $nextStage),
            'recommended_params' => $nextStage->target_parameters,
            'recommended_at' => now(),
        ]);

        // Логируем создание рекомендации
        Log::info("Stage transition recommendation created", [
            'cycle_id' => $cycle->id,
            'from_stage' => $currentStage->name,
            'to_stage' => $nextStage->name,
        ]);

        // ⭐ GROWTH PLANNER: Отправка уведомления о рекомендации
        try {
            $notificationService = app(\App\Services\GrowthNotificationService::class);
            $notificationService->sendStageTransitionRecommendation($recommendation);
        } catch (\Exception $e) {
            Log::error("Failed to send stage transition notification", [
                'recommendation_id' => $recommendation->id,
                'error' => $e->getMessage(),
            ]);
        }

        return $recommendation;
    }

    /**
     * Проверка условий для перехода на следующую стадию
     */
    protected function shouldTransitionToNextStage(
        GrowthCycle $cycle,
        $currentStage,
        $nextStage
    ): bool {
        // Проверка 1: Время в стадии
        $daysInCurrentStage = $cycle->getDaysInCurrentStage();
        if ($daysInCurrentStage < $currentStage->duration_days) {
            // Минимальное время не прошло
            return false;
        }

        // Проверка 2: Параметры соответствуют целевым значениям следующей стадии
        // TODO: Получить последние параметры из телеметрии зоны
        // Пока проверяем только время

        // Проверка 3: Дополнительные условия (если есть в auto_actions)
        // TODO: Проверить выполнение auto_actions текущей стадии

        return true;
    }

    /**
     * Получение причины перехода
     */
    protected function getTransitionReason(
        GrowthCycle $cycle,
        $currentStage,
        $nextStage
    ): string {
        $daysInStage = $cycle->getDaysInCurrentStage();
        $reasons = [];

        if ($daysInStage >= $currentStage->duration_days) {
            $reasons[] = "Минимальная длительность стадии достигнута ({$daysInStage} дней)";
        }

        // TODO: Добавить проверку параметров
        // if ($parametersMatch) {
        //     $reasons[] = "Параметры соответствуют целевым значениям следующей стадии";
        // }

        return implode('. ', $reasons) ?: "Автоматическая рекомендация на основе времени";
    }
}

