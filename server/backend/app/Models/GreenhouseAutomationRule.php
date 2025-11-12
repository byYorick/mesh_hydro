<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class GreenhouseAutomationRule extends Model
{
    use HasFactory;

    protected $table = 'greenhouse_automation_rules';

    protected $fillable = [
        'greenhouse_id',
        'name',
        'description',
        'enabled',
        'trigger_type',
        'trigger_config',
        'actions',
    ];

    protected $casts = [
        'enabled' => 'boolean',
        'trigger_config' => 'array',
        'actions' => 'array',
        'created_at' => 'datetime',
        'updated_at' => 'datetime',
    ];

    public function greenhouse(): BelongsTo
    {
        return $this->belongsTo(Greenhouse::class);
    }
}


