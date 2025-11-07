<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Модель культуры растения
 */
class GrowthCulture extends Model
{
    use HasFactory;

    protected $fillable = [
        'name',
        'slug',
        'category',
        'description',
        'typical_cycle_days',
        'optimal_temp_min',
        'optimal_temp_max',
        'optimal_humidity_min',
        'optimal_humidity_max',
        'image_url',
        'is_system',
        'is_active',
    ];

    protected $casts = [
        'typical_cycle_days' => 'integer',
        'optimal_temp_min' => 'decimal:2',
        'optimal_temp_max' => 'decimal:2',
        'optimal_humidity_min' => 'decimal:2',
        'optimal_humidity_max' => 'decimal:2',
        'is_system' => 'boolean',
        'is_active' => 'boolean',
    ];

    /**
     * Пресеты для этой культуры
     */
    public function presets(): HasMany
    {
        return $this->hasMany(GrowthPreset::class, 'culture_id');
    }

    /**
     * Циклы этой культуры
     */
    public function cycles(): HasMany
    {
        return $this->hasMany(GrowthCycle::class, 'culture_id');
    }

    /**
     * Scope: только активные культуры
     */
    public function scopeActive($query)
    {
        return $query->where('is_active', true);
    }

    /**
     * Scope: по категории
     */
    public function scopeByCategory($query, string $category)
    {
        return $query->where('category', $category);
    }

    /**
     * Получить иконку по категории
     */
    public function getIconAttribute(): string
    {
        return match($this->category) {
            'leafy_greens' => 'mdi-leaf',
            'berries' => 'mdi-fruit-cherries',
            'vegetables' => 'mdi-carrot',
            'herbs' => 'mdi-flower',
            default => 'mdi-sprout',
        };
    }

    /**
     * Получить цвет категории
     */
    public function getCategoryColorAttribute(): string
    {
        return match($this->category) {
            'leafy_greens' => 'success',
            'berries' => 'error',
            'vegetables' => 'warning',
            'herbs' => 'info',
            default => 'grey',
        };
    }
}

