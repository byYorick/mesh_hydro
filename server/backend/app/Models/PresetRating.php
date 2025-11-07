<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Оценка пресета пользователем
 */
class PresetRating extends Model
{
    use HasFactory;

    protected $fillable = [
        'preset_id',
        'cycle_id',
        'user_id',
        'rating',
        'comment',
    ];

    protected $casts = [
        'rating' => 'integer',
    ];

    public function preset(): BelongsTo
    {
        return $this->belongsTo(GrowthPreset::class, 'preset_id');
    }

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    /**
     * После сохранения - обновить средний рейтинг пресета
     */
    protected static function booted()
    {
        static::saved(function ($rating) {
            $rating->preset->updateAverageRating();
        });

        static::deleted(function ($rating) {
            $rating->preset->updateAverageRating();
        });
    }
}

