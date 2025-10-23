<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class PidPreset extends Model
{
    protected $table = 'pid_presets';

    protected $fillable = [
        'name',
        'description',
        'ph_config',
        'ec_config',
        'is_default',
        'user_id',
    ];

    protected $casts = [
        'ph_config' => 'array',
        'ec_config' => 'array',
        'is_default' => 'boolean',
        'created_at' => 'datetime',
        'updated_at' => 'datetime',
    ];

    public function scopeDefaults($query)
    {
        return $query->where('is_default', true);
    }

    public function scopeCustom($query)
    {
        return $query->where('is_default', false);
    }
}

