<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class PumpCalibration extends Model
{
    protected $fillable = [
        'node_id',
        'pump_id',
        'ml_per_second',
        'calibration_volume_ml',
        'calibration_time_ms',
        'is_calibrated',
        'calibrated_at',
    ];

    protected $casts = [
        'ml_per_second' => 'float',
        'calibration_volume_ml' => 'float',
        'calibration_time_ms' => 'integer',
        'is_calibrated' => 'boolean',
        'calibrated_at' => 'datetime',
    ];

    /**
     * Связь с узлом
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }
}

