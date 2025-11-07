<?php

namespace App\Services;

use App\Models\StageTransitionRecommendation;
use App\Models\CycleNotification;
use App\Models\GrowthCycle;
use App\Services\TelegramService;
use App\Services\SmsService;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Event;

/**
 * ⭐ GROWTH PLANNER: Сервис для отправки уведомлений о циклах роста
 */
class GrowthNotificationService
{
    public function __construct(
        private TelegramService $telegram,
        private SmsService $sms
    ) {}

    /**
     * Отправка уведомления о рекомендации перехода стадии
     */
    public function sendStageTransitionRecommendation(
        StageTransitionRecommendation $recommendation
    ): void {
        $cycle = $recommendation->cycle;
        if (!$cycle) {
            return;
        }

        $fromStage = $recommendation->currentStage;
        $toStage = $recommendation->recommendedStage;
        $zone = $cycle->zone;

        $message = sprintf(
            "🌱 Рекомендация перехода стадии\n\n" .
            "Цикл: %s\n" .
            "Зона: %s\n" .
            "Текущая стадия: %s\n" .
            "Рекомендуемая стадия: %s\n" .
            "Причина: %s",
            $cycle->name ?? "Цикл #{$cycle->id}",
            $zone->name ?? "Не указана",
            $fromStage->name ?? "Неизвестна",
            $toStage->name ?? "Неизвестна",
            $recommendation->reason ?? "Автоматическая рекомендация"
        );

        // Сохранение уведомления в БД
        $notification = CycleNotification::create([
            'cycle_id' => $cycle->id,
            'type' => 'stage_transition',
            'title' => 'Рекомендация перехода стадии',
            'message' => $message,
            'data' => [
                'recommendation_id' => $recommendation->id,
                'from_stage_id' => $recommendation->current_stage_id,
                'to_stage_id' => $recommendation->recommended_stage_id,
            ],
            'is_read' => false,
        ]);

        // Отправка через разные каналы
        $this->sendTelegramNotification($notification, $message);
        $this->sendInAppNotification($notification);
        
        // SMS только для критичных случаев (можно настроить по необходимости)
        // $this->sendSmsNotification($notification, $message);

        Log::info("Stage transition recommendation notification sent", [
            'cycle_id' => $cycle->id,
            'recommendation_id' => $recommendation->id,
            'notification_id' => $notification->id,
        ]);
    }

    /**
     * Отправка уведомления о параметрах вне целевых значений
     */
    public function sendParameterDeviationNotification(
        GrowthCycle $cycle,
        array $deviations
    ): void {
        $zone = $cycle->zone;
        $currentStage = $cycle->currentStage;

        $message = sprintf(
            "⚠️ Параметры вне целевых значений\n\n" .
            "Цикл: %s\n" .
            "Зона: %s\n" .
            "Стадия: %s\n\n" .
            "Отклонения:\n%s",
            $cycle->name ?? "Цикл #{$cycle->id}",
            $zone->name ?? "Не указана",
            $currentStage->name ?? "Неизвестна",
            $this->formatDeviations($deviations)
        );

        $notification = CycleNotification::create([
            'cycle_id' => $cycle->id,
            'type' => 'parameter_deviation',
            'title' => 'Параметры вне целевых значений',
            'message' => $message,
            'data' => [
                'deviations' => $deviations,
            ],
            'is_read' => false,
        ]);

        $this->sendTelegramNotification($notification, $message);
        $this->sendInAppNotification($notification);

        Log::info("Parameter deviation notification sent", [
            'cycle_id' => $cycle->id,
            'notification_id' => $notification->id,
        ]);
    }

    /**
     * Отправка уведомления о задержке роста
     */
    public function sendGrowthDelayNotification(
        GrowthCycle $cycle,
        int $daysBehind
    ): void {
        $zone = $cycle->zone;

        $message = sprintf(
            "⏰ Задержка роста\n\n" .
            "Цикл: %s\n" .
            "Зона: %s\n" .
            "Задержка: %d дней\n" .
            "Текущий день: %d\n" .
            "Ожидаемый день: %d",
            $cycle->name ?? "Цикл #{$cycle->id}",
            $zone->name ?? "Не указана",
            $daysBehind,
            $cycle->current_day ?? 0,
            $cycle->current_day + $daysBehind
        );

        $notification = CycleNotification::create([
            'cycle_id' => $cycle->id,
            'type' => 'growth_delay',
            'title' => 'Задержка роста',
            'message' => $message,
            'data' => [
                'days_behind' => $daysBehind,
            ],
            'is_read' => false,
        ]);

        $this->sendTelegramNotification($notification, $message);
        $this->sendInAppNotification($notification);

        Log::info("Growth delay notification sent", [
            'cycle_id' => $cycle->id,
            'days_behind' => $daysBehind,
            'notification_id' => $notification->id,
        ]);
    }

    /**
     * Отправка уведомления о приближении сбора урожая
     */
    public function sendHarvestApproachingNotification(
        GrowthCycle $cycle,
        int $daysUntilHarvest
    ): void {
        $zone = $cycle->zone;

        $message = sprintf(
            "🌾 Приближается сбор урожая\n\n" .
            "Цикл: %s\n" .
            "Зона: %s\n" .
            "До сбора: %d дней\n" .
            "Ожидаемая дата: %s",
            $cycle->name ?? "Цикл #{$cycle->id}",
            $zone->name ?? "Не указана",
            $daysUntilHarvest,
            $cycle->expected_harvest_at?->format('d.m.Y') ?? "Не указана"
        );

        $notification = CycleNotification::create([
            'cycle_id' => $cycle->id,
            'type' => 'harvest_approaching',
            'title' => 'Приближается сбор урожая',
            'message' => $message,
            'data' => [
                'days_until_harvest' => $daysUntilHarvest,
                'expected_harvest_at' => $cycle->expected_harvest_at?->toIso8601String(),
            ],
            'is_read' => false,
        ]);

        $this->sendTelegramNotification($notification, $message);
        $this->sendInAppNotification($notification);

        Log::info("Harvest approaching notification sent", [
            'cycle_id' => $cycle->id,
            'days_until_harvest' => $daysUntilHarvest,
            'notification_id' => $notification->id,
        ]);
    }

    /**
     * Отправка еженедельного отчета о цикле
     */
    public function sendWeeklyCycleReport(GrowthCycle $cycle): void
    {
        $zone = $cycle->zone;
        $currentStage = $cycle->currentStage;
        $progress = round($cycle->progress ?? 0);

        $message = sprintf(
            "📊 Еженедельный отчет\n\n" .
            "Цикл: %s\n" .
            "Зона: %s\n" .
            "Текущая стадия: %s\n" .
            "Прогресс: %d%%\n" .
            "День цикла: %d\n" .
            "Осталось дней: %d",
            $cycle->name ?? "Цикл #{$cycle->id}",
            $zone->name ?? "Не указана",
            $currentStage->name ?? "Неизвестна",
            $progress,
            $cycle->current_day ?? 0,
            $cycle->remaining_days ?? 0
        );

        $notification = CycleNotification::create([
            'cycle_id' => $cycle->id,
            'type' => 'weekly_report',
            'title' => 'Еженедельный отчет',
            'message' => $message,
            'data' => [
                'progress' => $progress,
                'current_day' => $cycle->current_day,
                'remaining_days' => $cycle->remaining_days,
            ],
            'is_read' => false,
        ]);

        $this->sendTelegramNotification($notification, $message);
        $this->sendInAppNotification($notification);

        Log::info("Weekly cycle report sent", [
            'cycle_id' => $cycle->id,
            'notification_id' => $notification->id,
        ]);
    }

    /**
     * Отправка Telegram уведомления
     */
    protected function sendTelegramNotification(
        CycleNotification $notification,
        string $message
    ): void {
        if (!config('telegram.enabled', false)) {
            return;
        }

        $level = $this->mapNotificationTypeToLevel($notification->type);
        $this->telegram->sendAlert($message, $level);
    }

    /**
     * Отправка SMS уведомления
     */
    protected function sendSmsNotification(
        CycleNotification $notification,
        string $message
    ): void {
        if (!config('sms.enabled', false)) {
            return;
        }

        // SMS только для критичных уведомлений
        if (!in_array($notification->type, ['stage_transition', 'parameter_deviation'])) {
            return;
        }

        $this->sms->sendAlert($message);
    }

    /**
     * Отправка In-app уведомления (через WebSocket)
     */
    protected function sendInAppNotification(CycleNotification $notification): void
    {
        // Broadcast через Laravel Events/WebSocket
        event(new \App\Events\CycleNotificationCreated($notification));
    }

    /**
     * Форматирование отклонений параметров
     */
    protected function formatDeviations(array $deviations): string
    {
        $formatted = [];
        foreach ($deviations as $param => $deviation) {
            $value = $deviation['value'] ?? 'N/A';
            $target = $deviation['target'] ?? 'N/A';
            $status = $deviation['status'] ?? 'unknown';
            
            $emoji = match($status) {
                'high' => '⬆️',
                'low' => '⬇️',
                'normal' => '✅',
                default => '⚠️',
            };
            
            $formatted[] = sprintf(
                "%s %s: %s (цель: %s)",
                $emoji,
                ucfirst($param),
                $value,
                $target
            );
        }
        
        return implode("\n", $formatted);
    }

    /**
     * Маппинг типа уведомления в уровень для Telegram
     */
    protected function mapNotificationTypeToLevel(string $type): string
    {
        return match($type) {
            'stage_transition' => 'info',
            'parameter_deviation' => 'warning',
            'growth_delay' => 'warning',
            'harvest_approaching' => 'info',
            'weekly_report' => 'info',
            default => 'info',
        };
    }
}

