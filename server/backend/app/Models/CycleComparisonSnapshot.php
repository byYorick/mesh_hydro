<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Снимок для сравнения циклов
 */
class CycleComparisonSnapshot extends Model
{
    use HasFactory;

    protected $fillable = [
        'cycle_id',
        'total_days',
        'total_harvest_kg',
        'avg_ph',
        'avg_ec',
        'avg_temp',
        'avg_humidity',
        'total_water_liters',
        'total_nutrient_a_ml',
        'total_nutrient_b_ml',
        'total_ph_correction_ml',
        'yield_per_plant',
        'water_per_kg_harvest',
        'total_issues_count',
        'critical_issues_count',
    ];

    protected $casts = [
        'total_days' => 'integer',
        'total_harvest_kg' => 'decimal:3',
        'avg_ph' => 'decimal:2',
        'avg_ec' => 'decimal:2',
        'avg_temp' => 'decimal:2',
        'avg_humidity' => 'decimal:2',
        'total_water_liters' => 'decimal:2',
        'total_nutrient_a_ml' => 'decimal:2',
        'total_nutrient_b_ml' => 'decimal:2',
        'total_ph_correction_ml' => 'decimal:2',
        'yield_per_plant' => 'decimal:3',
        'water_per_kg_harvest' => 'decimal:2',
        'total_issues_count' => 'integer',
        'critical_issues_count' => 'integer',
    ];

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    /**
     * Создать снимок из цикла
     */
    public static function createFromCycle(GrowthCycle $cycle): self
    {
        $parameterLogs = $cycle->parameterLogs;

        return self::create([
            'cycle_id' => $cycle->id,
            'total_days' => $cycle->current_day,
            'total_harvest_kg' => $cycle->harvest_weight_kg,
            'avg_ph' => $parameterLogs->avg('avg_ph'),
            'avg_ec' => $parameterLogs->avg('avg_ec'),
            'avg_temp' => $parameterLogs->avg('avg_temp'),
            'avg_humidity' => $parameterLogs->avg('avg_humidity'),
            'total_water_liters' => $parameterLogs->sum('water_consumed_liters'),
            'total_nutrient_a_ml' => $parameterLogs->sum('nutrient_a_ml'),
            'total_nutrient_b_ml' => $parameterLogs->sum('nutrient_b_ml'),
            'total_ph_correction_ml' => $parameterLogs->sum('ph_down_ml') + $parameterLogs->sum('ph_up_ml'),
            'yield_per_plant' => $cycle->plant_count > 0 ? $cycle->harvest_weight_kg / $cycle->plant_count : null,
            'water_per_kg_harvest' => $cycle->harvest_weight_kg > 0 ? $parameterLogs->sum('water_consumed_liters') / $cycle->harvest_weight_kg : null,
            'total_issues_count' => $cycle->notifications()->count(),
            'critical_issues_count' => $cycle->notifications()->critical()->count(),
        ]);
    }
}

