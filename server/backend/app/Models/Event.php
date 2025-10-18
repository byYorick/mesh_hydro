<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class Event extends Model
{
    /**
     * Таблица в БД
     */
    protected $table = 'events';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'node_id',
        'level',          // "info", "warning", "critical", "emergency"
        'message',        // "pH critically low", "Temperature out of range"
        'data',           // jsonb - дополнительные данные события
        'resolved_at',    // timestamp когда проблема решена
        'resolved_by',    // кто решил (user_id или 'auto')
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'data' => 'array',
        'resolved_at' => 'datetime',
    ];

    /**
     * Константы уровней событий
     */
    const LEVEL_INFO = 'info';
    const LEVEL_WARNING = 'warning';
    const LEVEL_CRITICAL = 'critical';
    const LEVEL_EMERGENCY = 'emergency';

    /**
     * Отношение: событие принадлежит узлу
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Проверка: критичное событие?
     */
    public function isCritical(): bool
    {
        return in_array($this->level, [self::LEVEL_CRITICAL, self::LEVEL_EMERGENCY]);
    }

    /**
     * Проверка: активное (нерешенное) событие?
     */
    public function isActive(): bool
    {
        return $this->resolved_at === null;
    }

    /**
     * Проверка: решенное событие?
     */
    public function isResolved(): bool
    {
        return $this->resolved_at !== null;
    }

    /**
     * Пометить событие как решенное
     */
    public function resolve(string $resolvedBy = 'auto'): void
    {
        $this->update([
            'resolved_at' => now(),
            'resolved_by' => $resolvedBy,
        ]);
    }

    /**
     * Scope: только активные события
     */
    public function scopeActive($query)
    {
        return $query->whereNull('resolved_at');
    }

    /**
     * Scope: только решенные события
     */
    public function scopeResolved($query)
    {
        return $query->whereNotNull('resolved_at');
    }

    /**
     * Scope: только критичные события
     */
    public function scopeCritical($query)
    {
        return $query->whereIn('level', [self::LEVEL_CRITICAL, self::LEVEL_EMERGENCY]);
    }

    /**
     * Scope: по уровню события
     */
    public function scopeOfLevel($query, string $level)
    {
        return $query->where('level', $level);
    }

    /**
     * Scope: для конкретного узла
     */
    public function scopeForNode($query, string $nodeId)
    {
        return $query->where('node_id', $nodeId);
    }

    /**
     * Получить цвет для уровня события
     */
    public function getLevelColorAttribute(): string
    {
        return match($this->level) {
            self::LEVEL_INFO => 'blue',
            self::LEVEL_WARNING => 'orange',
            self::LEVEL_CRITICAL => 'red',
            self::LEVEL_EMERGENCY => 'purple',
            default => 'grey',
        };
    }

    /**
     * Получить иконку для уровня события
     */
    public function getLevelIconAttribute(): string
    {
        return match($this->level) {
            self::LEVEL_INFO => 'mdi-information',
            self::LEVEL_WARNING => 'mdi-alert',
            self::LEVEL_CRITICAL => 'mdi-alert-circle',
            self::LEVEL_EMERGENCY => 'mdi-fire',
            default => 'mdi-help-circle',
        };
    }
}

