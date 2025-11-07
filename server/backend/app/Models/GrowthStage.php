<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Модель стадии роста
 */
class GrowthStage extends Model
{
    use HasFactory;

    protected $fillable = [
        'preset_id',
        'name',
        'order',
        'duration_days',
        'target_params',
        'description',
        'care_instructions',
        'auto_actions',
    ];

    protected $casts = [
        'order' => 'integer',
        'duration_days' => 'integer',
        'target_params' => 'array',
        'auto_actions' => 'array',
    ];

    /**
     * Пресет
     */
    public function preset(): BelongsTo
    {
        return $this->belongsTo(GrowthPreset::class, 'preset_id');
    }

    /**
     * История использования этой стадии
     */
    public function stageHistory(): HasMany
    {
        return $this->hasMany(CycleStageHistory::class, 'stage_id');
    }

    /**
     * Получить целевой параметр
     */
    public function getTargetParam(string $param, $default = null)
    {
        return $this->target_params[$param] ?? $default;
    }

    /**
     * Установить целевой параметр
     */
    public function setTargetParam(string $param, $value): void
    {
        $params = $this->target_params ?? [];
        $params[$param] = $value;
        $this->target_params = $params;
        $this->save();
    }

    /**
     * Получить список автодействий
     */
    public function getAutoActions(): array
    {
        return $this->auto_actions ?? [];
    }

    /**
     * Иконка стадии
     */
    public function getIconAttribute(): string
    {
        $stageName = strtolower($this->name);
        
        if (str_contains($stageName, 'проращ') || str_contains($stageName, 'seed')) {
            return 'mdi-seed';
        }
        if (str_contains($stageName, 'вегет') || str_contains($stageName, 'vegetative')) {
            return 'mdi-leaf';
        }
        if (str_contains($stageName, 'цвет') || str_contains($stageName, 'flower')) {
            return 'mdi-flower';
        }
        if (str_contains($stageName, 'созре') || str_contains($stageName, 'fruit')) {
            return 'mdi-fruit-grapes';
        }
        if (str_contains($stageName, 'сбор') || str_contains($stageName, 'harvest')) {
            return 'mdi-basket';
        }
        
        return 'mdi-sprout-outline';
    }

    /**
     * Цвет стадии
     */
    public function getColorAttribute(): string
    {
        $colors = ['primary', 'success', 'warning', 'info', 'secondary'];
        return $colors[($this->order - 1) % count($colors)];
    }
}

