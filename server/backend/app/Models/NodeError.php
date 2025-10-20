<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class NodeError extends Model
{
    /**
     * Таблица в БД
     */
    protected $table = 'node_errors';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'node_id',
        'error_code',        // "SENSOR_READ_FAIL", "HEAP_LOW", "NETWORK_TIMEOUT"
        'error_type',        // "hardware", "software", "network", "sensor"
        'severity',          // "low", "medium", "high", "critical"
        'message',           // Описание ошибки
        'stack_trace',       // Stack trace если доступен
        'diagnostics',       // JSONB - диагностическая информация
        'occurred_at',       // Timestamp когда произошла ошибка
        'resolved_at',       // Timestamp когда решена
        'resolution_notes',  // Заметки о решении
        'resolved_by',       // Кто решил
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'diagnostics' => 'array',
        'occurred_at' => 'datetime',
        'resolved_at' => 'datetime',
    ];

    /**
     * Константы severity levels
     */
    const SEVERITY_LOW = 'low';
    const SEVERITY_MEDIUM = 'medium';
    const SEVERITY_HIGH = 'high';
    const SEVERITY_CRITICAL = 'critical';

    /**
     * Константы error types
     */
    const TYPE_HARDWARE = 'hardware';
    const TYPE_SOFTWARE = 'software';
    const TYPE_NETWORK = 'network';
    const TYPE_SENSOR = 'sensor';

    /**
     * Отношение: ошибка принадлежит узлу
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Проверка: критичная ошибка?
     */
    public function isCritical(): bool
    {
        return $this->severity === self::SEVERITY_CRITICAL;
    }

    /**
     * Проверка: ошибка решена?
     */
    public function isResolved(): bool
    {
        return $this->resolved_at !== null;
    }

    /**
     * Проверка: активная ошибка?
     */
    public function isActive(): bool
    {
        return $this->resolved_at === null;
    }

    /**
     * Пометить ошибку как решенную
     */
    public function resolve(string $resolvedBy = 'auto', ?string $notes = null): void
    {
        $this->update([
            'resolved_at' => now(),
            'resolved_by' => $resolvedBy,
            'resolution_notes' => $notes,
        ]);
    }

    /**
     * Scope: только нерешенные ошибки
     */
    public function scopeUnresolved($query)
    {
        return $query->whereNull('resolved_at');
    }

    /**
     * Scope: только решенные ошибки
     */
    public function scopeResolved($query)
    {
        return $query->whereNotNull('resolved_at');
    }

    /**
     * Scope: только критичные ошибки
     */
    public function scopeCritical($query)
    {
        return $query->where('severity', self::SEVERITY_CRITICAL);
    }

    /**
     * Scope: по уровню severity
     */
    public function scopeBySeverity($query, string $severity)
    {
        return $query->where('severity', $severity);
    }

    /**
     * Scope: по типу ошибки
     */
    public function scopeByType($query, string $type)
    {
        return $query->where('error_type', $type);
    }

    /**
     * Scope: для конкретного узла
     */
    public function scopeForNode($query, string $nodeId)
    {
        return $query->where('node_id', $nodeId);
    }

    /**
     * Получить цвет для severity
     */
    public function getSeverityColorAttribute(): string
    {
        return match($this->severity) {
            self::SEVERITY_LOW => 'info',
            self::SEVERITY_MEDIUM => 'warning',
            self::SEVERITY_HIGH => 'orange',
            self::SEVERITY_CRITICAL => 'error',
            default => 'grey',
        };
    }

    /**
     * Получить иконку для error type
     */
    public function getTypeIconAttribute(): string
    {
        return match($this->error_type) {
            self::TYPE_HARDWARE => 'mdi-chip',
            self::TYPE_SOFTWARE => 'mdi-bug',
            self::TYPE_NETWORK => 'mdi-wifi-alert',
            self::TYPE_SENSOR => 'mdi-thermometer-alert',
            default => 'mdi-alert-circle',
        };
    }
}

