<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasMany;

class Greenhouse extends Model
{
    use HasFactory;

    protected $fillable = [
        'name',
        'code',
        'location',
        'description',
        'timezone',
        'status',
        'mesh_group',
        'root_node_id',
        'root_node_mac',
        'image_url',
        'tags',
        'settings',
    ];

    protected $casts = [
        'tags' => 'array',
        'settings' => 'array',
        'created_at' => 'datetime',
        'updated_at' => 'datetime',
    ];

    public function zones(): HasMany
    {
        return $this->hasMany(Zone::class);
    }

    public function nodes(): HasMany
    {
        return $this->hasMany(Node::class);
    }

    public function cycles(): HasMany
    {
        return $this->hasMany(GrowthCycle::class);
    }

    public function automationRules(): HasMany
    {
        return $this->hasMany(GreenhouseAutomationRule::class);
    }
}


