<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class Telemetry extends Model
{
    /**
     * Таблица в БД
     */
    protected $table = 'telemetry';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'node_id',
        'node_type',
        'data',           // jsonb - данные телеметрии (зависят от типа узла)
        'received_at',    // timestamp получения на сервере
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'data' => 'array',
        'received_at' => 'datetime',
    ];

    /**
     * Отношение: телеметрия принадлежит узлу
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Scope: телеметрия за последние N часов
     */
    public function scopeRecent($query, int $hours = 24)
    {
        return $query->where('received_at', '>', now()->subHours($hours));
    }

    /**
     * Scope: телеметрия для конкретного узла
     */
    public function scopeForNode($query, string $nodeId)
    {
        return $query->where('node_id', $nodeId);
    }

    /**
     * Scope: телеметрия по типу узла
     */
    public function scopeForNodeType($query, string $nodeType)
    {
        return $query->where('node_type', $nodeType);
    }

    /**
     * Примеры структуры данных для разных типов узлов:
     * 
     * pH/EC узел:
     * {
     *   "ph": 6.5,
     *   "ec": 1.8,
     *   "temp": 24.3,
     *   "timestamp": 1697548805
     * }
     * 
     * Climate узел:
     * {
     *   "temp": 24.5,
     *   "humidity": 65.2,
     *   "co2": 850,
     *   "light": 45000,
     *   "timestamp": 1697548805
     * }
     * 
     * Water узел:
     * {
     *   "level": 75.5,
     *   "flow": 2.3,
     *   "temp": 22.1,
     *   "timestamp": 1697548805
     * }
     */
}

