<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class ZoneNodeAssignment extends Model
{
    /**
     * Таблица в БД
     */
    protected $table = 'zone_node_assignments';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'zone_id',
        'node_id',
        'node_role',
        'assigned_at',
        'unassigned_at',
        'cycle_id',
        'is_active',
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'assigned_at' => 'datetime',
        'unassigned_at' => 'datetime',
        'is_active' => 'boolean',
        'created_at' => 'datetime',
        'updated_at' => 'datetime',
    ];

    /**
     * Зона
     */
    public function zone(): BelongsTo
    {
        return $this->belongsTo(Zone::class, 'zone_id');
    }

    /**
     * Узел
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Цикл (если связано с циклом)
     */
    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    /**
     * Scope: Активные назначения
     */
    public function scopeActive($query)
    {
        return $query->where('is_active', true)
                     ->whereNull('unassigned_at');
    }

    /**
     * Scope: По зоне
     */
    public function scopeForZone($query, int $zoneId)
    {
        return $query->where('zone_id', $zoneId);
    }

    /**
     * Scope: По узлу
     */
    public function scopeForNode($query, string $nodeId)
    {
        return $query->where('node_id', $nodeId);
    }
}

