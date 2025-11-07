<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Модель пресета цикла роста
 */
class GrowthPreset extends Model
{
    use HasFactory;

    protected $fillable = [
        'culture_id',
        'name',
        'slug',
        'description',
        'preset_type',
        'created_by_user_id',
        'total_days',
        'difficulty',
        'recommended_system',
        'usage_count',
        'avg_rating',
        'is_public',
        'is_active',
    ];

    protected $casts = [
        'total_days' => 'integer',
        'usage_count' => 'integer',
        'avg_rating' => 'decimal:2',
        'is_public' => 'boolean',
        'is_active' => 'boolean',
    ];

    /**
     * Культура
     */
    public function culture(): BelongsTo
    {
        return $this->belongsTo(GrowthCulture::class, 'culture_id');
    }

    /**
     * Стадии пресета
     */
    public function stages(): HasMany
    {
        return $this->hasMany(GrowthStage::class, 'preset_id')->orderBy('order');
    }

    /**
     * Циклы, использующие этот пресет
     */
    public function cycles(): HasMany
    {
        return $this->hasMany(GrowthCycle::class, 'preset_id');
    }

    /**
     * Оценки пресета
     */
    public function ratings(): HasMany
    {
        return $this->hasMany(PresetRating::class, 'preset_id');
    }

    /**
     * Scope: системные пресеты
     */
    public function scopeSystem($query)
    {
        return $query->where('preset_type', 'system');
    }

    /**
     * Scope: пользовательские пресеты
     */
    public function scopeCustom($query)
    {
        return $query->where('preset_type', 'custom');
    }

    /**
     * Scope: публичные пресеты
     */
    public function scopePublic($query)
    {
        return $query->where('is_public', true);
    }

    /**
     * Scope: активные пресеты
     */
    public function scopeActive($query)
    {
        return $query->where('is_active', true);
    }

    /**
     * Инкремент счётчика использования
     */
    public function incrementUsage(): void
    {
        $this->increment('usage_count');
    }

    /**
     * Обновить средний рейтинг
     */
    public function updateAverageRating(): void
    {
        $avgRating = $this->ratings()->avg('rating');
        $this->update(['avg_rating' => $avgRating]);
    }

    /**
     * Иконка сложности
     */
    public function getDifficultyIconAttribute(): string
    {
        return match($this->difficulty) {
            'easy' => 'mdi-emoticon-happy-outline',
            'medium' => 'mdi-emoticon-neutral-outline',
            'hard' => 'mdi-emoticon-confused-outline',
            default => 'mdi-help-circle-outline',
        };
    }

    /**
     * Цвет сложности
     */
    public function getDifficultyColorAttribute(): string
    {
        return match($this->difficulty) {
            'easy' => 'success',
            'medium' => 'warning',
            'hard' => 'error',
            default => 'grey',
        };
    }
}

