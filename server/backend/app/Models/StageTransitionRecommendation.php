<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Рекомендации перехода на стадию (semi-auto)
 */
class StageTransitionRecommendation extends Model
{
    use HasFactory;

    protected $fillable = [
        'cycle_id',
        'current_stage_id',
        'recommended_stage_id',
        'reason',
        'recommended_params',
        'status',
        'recommended_at',
        'responded_at',
        'responded_by_user_id',
    ];

    protected $casts = [
        'recommended_params' => 'array',
        'recommended_at' => 'datetime',
        'responded_at' => 'datetime',
    ];

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    public function currentStage(): BelongsTo
    {
        return $this->belongsTo(GrowthStage::class, 'current_stage_id');
    }

    public function recommendedStage(): BelongsTo
    {
        return $this->belongsTo(GrowthStage::class, 'recommended_stage_id');
    }

    /**
     * Scope: ожидающие ответа
     */
    public function scopePending($query)
    {
        return $query->where('status', 'pending');
    }

    /**
     * Одобрить рекомендацию
     */
    public function approve(?int $userId = null): bool
    {
        $this->update([
            'status' => 'approved',
            'responded_at' => now(),
            'responded_by_user_id' => $userId,
        ]);

        // Выполнить переход на следующую стадию
        return $this->cycle->transitionToNextStage();
    }

    /**
     * Отклонить рекомендацию
     */
    public function reject(?int $userId = null): void
    {
        $this->update([
            'status' => 'rejected',
            'responded_at' => now(),
            'responded_by_user_id' => $userId,
        ]);
    }
}

