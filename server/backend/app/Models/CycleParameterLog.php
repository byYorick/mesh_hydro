<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Логи параметров по дням
 */
class CycleParameterLog extends Model
{
    use HasFactory;

    protected $fillable = [
        'cycle_id',
        'log_date',
        'day_number',
        'avg_ph',
        'avg_ec',
        'avg_temp',
        'avg_humidity',
        'avg_co2',
        'avg_lux',
        'min_ph',
        'max_ph',
        'min_temp',
        'max_temp',
        'water_consumed_liters',
        'nutrient_a_ml',
        'nutrient_b_ml',
        'ph_down_ml',
        'ph_up_ml',
    ];

    protected $casts = [
        'log_date' => 'date',
        'day_number' => 'integer',
        'avg_ph' => 'decimal:2',
        'avg_ec' => 'decimal:2',
        'avg_temp' => 'decimal:2',
        'avg_humidity' => 'decimal:2',
        'avg_co2' => 'integer',
        'avg_lux' => 'integer',
        'min_ph' => 'decimal:2',
        'max_ph' => 'decimal:2',
        'min_temp' => 'decimal:2',
        'max_temp' => 'decimal:2',
        'water_consumed_liters' => 'decimal:3',
        'nutrient_a_ml' => 'decimal:2',
        'nutrient_b_ml' => 'decimal:2',
        'ph_down_ml' => 'decimal:2',
        'ph_up_ml' => 'decimal:2',
    ];

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }
}

