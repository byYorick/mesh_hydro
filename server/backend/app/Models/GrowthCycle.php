<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Relations\HasOne;
use Illuminate\Database\Eloquent\Factories\HasFactory;
use Carbon\Carbon;

/**
 * ⭐ GROWTH PLANNER: Модель цикла роста (привязан к зоне!)
 */
class GrowthCycle extends Model
{
    use HasFactory;

    protected $fillable = [
        'zone_id',
        'preset_id',
        'culture_id',
        'current_stage_id',
        'started_at',
        'expected_harvest_at',
        'actual_harvest_at',
        'ended_at',
        'status',
        'plant_count',
        'harvest_weight_kg',
        'notes',
        'rating',
        'created_by_user_id',
    ];

    protected $casts = [
        'started_at' => 'datetime',
        'expected_harvest_at' => 'datetime',
        'actual_harvest_at' => 'datetime',
        'ended_at' => 'datetime',
        'plant_count' => 'integer',
        'harvest_weight_kg' => 'decimal:3',
        'rating' => 'integer',
    ];

    /**
     * ⭐ ЗОНИРОВАНИЕ: Зона, в которой растёт цикл
     */
    public function zone(): BelongsTo
    {
        return $this->belongsTo(Zone::class, 'zone_id');
    }

    /**
     * Пресет
     */
    public function preset(): BelongsTo
    {
        return $this->belongsTo(GrowthPreset::class, 'preset_id');
    }

    /**
     * Культура
     */
    public function culture(): BelongsTo
    {
        return $this->belongsTo(GrowthCulture::class, 'culture_id');
    }

    /**
     * Текущая стадия
     */
    public function currentStage(): BelongsTo
    {
        return $this->belongsTo(GrowthStage::class, 'current_stage_id');
    }

    /**
     * История стадий
     */
    public function stageHistory(): HasMany
    {
        return $this->hasMany(CycleStageHistory::class, 'cycle_id')->orderBy('started_at');
    }

    /**
     * Логи параметров
     */
    public function parameterLogs(): HasMany
    {
        return $this->hasMany(CycleParameterLog::class, 'cycle_id')->orderBy('log_date');
    }

    /**
     * Уведомления
     */
    public function notifications(): HasMany
    {
        return $this->hasMany(CycleNotification::class, 'cycle_id');
    }

    /**
     * Рекомендации перехода
     */
    public function transitionRecommendations(): HasMany
    {
        return $this->hasMany(StageTransitionRecommendation::class, 'cycle_id');
    }

    /**
     * Подтверждения конфигураций
     */
    public function configConfirmations(): HasMany
    {
        return $this->hasMany(NodeConfigurationConfirmation::class, 'cycle_id');
    }

    /**
     * Снимок для сравнения
     */
    public function comparisonSnapshot(): HasOne
    {
        return $this->hasOne(CycleComparisonSnapshot::class, 'cycle_id');
    }

    /**
     * Оценка пресета
     */
    public function presetRating(): HasOne
    {
        return $this->hasOne(PresetRating::class, 'cycle_id');
    }

    /**
     * Scope: активные циклы
     */
    public function scopeActive($query)
    {
        return $query->where('status', 'active');
    }

    /**
     * Scope: завершённые циклы
     */
    public function scopeCompleted($query)
    {
        return $query->whereIn('status', ['harvested', 'ended']);
    }

    /**
     * Scope: по зоне
     */
    public function scopeInZone($query, int $zoneId)
    {
        return $query->where('zone_id', $zoneId);
    }

    /**
     * Получить прогресс цикла (%)
     */
    public function getProgressAttribute(): float
    {
        if (!$this->started_at || !$this->expected_harvest_at) {
            return 0;
        }

        $total = $this->started_at->diffInDays($this->expected_harvest_at);
        $elapsed = $this->started_at->diffInDays(now());

        if ($total <= 0) return 100;

        return min(100, ($elapsed / $total) * 100);
    }

    /**
     * Получить текущий день цикла
     */
    public function getCurrentDayAttribute(): int
    {
        if (!$this->started_at) {
            return 0;
        }

        return $this->started_at->diffInDays(now()) + 1;
    }

    /**
     * Получить оставшиеся дни
     */
    public function getRemainingDaysAttribute(): int
    {
        if (!$this->expected_harvest_at || $this->status !== 'active') {
            return 0;
        }

        $remaining = now()->diffInDays($this->expected_harvest_at, false);
        return max(0, $remaining);
    }

    /**
     * Получить количество дней в текущей стадии
     */
    public function getDaysInCurrentStage(): int
    {
        if (!$this->current_stage_id || !$this->started_at) {
            return 0;
        }

        // Находим запись в истории для текущей стадии
        $stageHistory = $this->stageHistory()
            ->where('stage_id', $this->current_stage_id)
            ->orderBy('started_at', 'desc')
            ->first();

        if (!$stageHistory || !$stageHistory->started_at) {
            return 0;
        }

        $startDate = Carbon::parse($stageHistory->started_at);
        return $startDate->diffInDays(now());
    }

    /**
     * Проверить, пора ли переходить на следующую стадию
     */
    public function shouldTransitionToNextStage(): bool
    {
        if (!$this->currentStage) {
            return false;
        }

        $currentHistory = $this->stageHistory()
            ->where('stage_id', $this->current_stage_id)
            ->whereNull('ended_at')
            ->first();

        if (!$currentHistory) {
            return false;
        }

        $elapsedDays = Carbon::parse($currentHistory->started_at)->diffInDays(now());
        return $elapsedDays >= $this->currentStage->duration_days;
    }

    /**
     * Получить следующую стадию
     */
    public function getNextStage(): ?GrowthStage
    {
        if (!$this->currentStage) {
            return null;
        }

        return $this->preset->stages()
            ->where('order', '>', $this->currentStage->order)
            ->orderBy('order')
            ->first();
    }

    /**
     * Переход на следующую стадию
     */
    public function transitionToNextStage(): bool
    {
        $nextStage = $this->getNextStage();
        
        if (!$nextStage) {
            return false;
        }

        // Завершить текущую стадию
        $currentHistory = $this->stageHistory()
            ->where('stage_id', $this->current_stage_id)
            ->whereNull('ended_at')
            ->first();

        if ($currentHistory) {
            $currentHistory->update([
                'ended_at' => now(),
                'actual_duration_days' => (int) Carbon::parse($currentHistory->started_at)->diffInDays(now()),
            ]);
        }

        // Начать новую стадию
        CycleStageHistory::create([
            'cycle_id' => $this->id,
            'stage_id' => $nextStage->id,
            'started_at' => now(),
        ]);

        $this->update(['current_stage_id' => $nextStage->id]);

        return true;
    }

    /**
     * Статус цвет
     */
    public function getStatusColorAttribute(): string
    {
        return match($this->status) {
            'planning' => 'info',
            'active' => 'success',
            'paused' => 'warning',
            'harvested' => 'primary',
            'failed' => 'error',
            'cancelled' => 'grey',
            default => 'grey',
        };
    }

    /**
     * Статус иконка
     */
    public function getStatusIconAttribute(): string
    {
        return match($this->status) {
            'planning' => 'mdi-calendar-clock',
            'active' => 'mdi-play-circle',
            'paused' => 'mdi-pause-circle',
            'harvested' => 'mdi-check-circle',
            'failed' => 'mdi-close-circle',
            'cancelled' => 'mdi-cancel',
            default => 'mdi-help-circle',
        };
    }
}

