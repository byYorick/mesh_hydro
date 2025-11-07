<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: История смены стадий
 */
class CycleStageHistory extends Model
{
    use HasFactory;

    protected $table = 'cycle_stage_history';

    protected $fillable = [
        'cycle_id',
        'stage_id',
        'started_at',
        'ended_at',
        'actual_duration_days',
        'applied_params',
        'notes',
    ];

    protected $casts = [
        'started_at' => 'datetime',
        'ended_at' => 'datetime',
        'actual_duration_days' => 'integer',
        'applied_params' => 'array',
    ];

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    public function stage(): BelongsTo
    {
        return $this->belongsTo(GrowthStage::class, 'stage_id');
    }

    /**
     * Проверить, активна ли стадия
     */
    public function isActive(): bool
    {
        return $this->ended_at === null;
    }
}

