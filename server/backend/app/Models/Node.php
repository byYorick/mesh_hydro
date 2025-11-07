<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Relations\HasOne;
use Illuminate\Database\Eloquent\Factories\HasFactory;
use Carbon\Carbon;

class Node extends Model
{
    use HasFactory;

    /**
     * Таблица в БД
     */
    protected $table = 'nodes';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'node_id',        // "ph_ec_001", "climate_001"
        'node_type',      // "ph_ec", "climate", "relay", "water", "display", "root"
        'root_node_id',   // ⭐ НОВОЕ: "root_001" - привязка к Root Node
        'zone',           // Deprecated: "Zone 1", "Zone 2" (для обратной совместимости)
        'mac_address',    // "AA:BB:CC:DD:EE:FF"
        'online',         // true/false
        'last_seen_at',   // timestamp последнего контакта
        'config',         // jsonb - конфигурация узла
        'metadata',       // jsonb - дополнительная информация (версия прошивки и т.д.)
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'online' => 'boolean',
        'last_seen_at' => 'datetime',
        'config' => 'array',
        'metadata' => 'array',
    ];

    /**
     * ⭐ ЗОНИРОВАНИЕ: Root Node (для обычных узлов)
     */
    public function rootNode(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'root_node_id', 'node_id');
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Дочерние узлы (для Root Node)
     * Исключает сам Root Node из списка (где node_id != root_node_id)
     */
    public function childNodes(): HasMany
    {
        return $this->hasMany(Node::class, 'root_node_id', 'node_id')
            ->whereColumn('node_id', '!=', 'root_node_id');
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Зона узла (через root_node_id)
     */
    public function zone(): HasOne
    {
        return $this->hasOne(Zone::class, 'root_node_id', 'root_node_id');
    }

    /**
     * Отношение: узел имеет много записей телеметрии
     */
    public function telemetry(): HasMany
    {
        return $this->hasMany(Telemetry::class, 'node_id', 'node_id');
    }

    /**
     * Отношение: узел имеет много событий
     */
    public function events(): HasMany
    {
        return $this->hasMany(Event::class, 'node_id', 'node_id');
    }

    /**
     * Отношение: узел имеет много команд
     */
    public function commands(): HasMany
    {
        return $this->hasMany(Command::class, 'node_id', 'node_id');
    }

    /**
     * Получить последнюю телеметрию
     * Оптимизировано для использования индексов
     */
    public function lastTelemetry()
    {
        return $this->hasOne(Telemetry::class, 'node_id', 'node_id')
            ->orderBy('received_at', 'desc')
            ->limit(1);
    }

    /**
     * Проверка: узел онлайн?
     * Использует единый таймаут из конфигурации
     */
    public function isOnline(): bool
    {
        if (!$this->last_seen_at) {
            return false;
        }
        
        $timeout = config('hydro.node_offline_timeout', 30); // секунд
        return $this->last_seen_at->diffInSeconds(now()) < $timeout;
    }

    /**
     * Обновить статус "видели узел"
     */
    public function updateLastSeen(): void
    {
        $this->update([
            'last_seen_at' => now(),
        ]);
        
        // Перезагружаем чтобы обновить last_seen_at в памяти
        $this->refresh();
        
        // Устанавливаем online на основе isOnline()
        $this->update(['online' => $this->isOnline()]);
    }

    /**
     * Получить иконку для типа узла
     */
    public function getIconAttribute(): string
    {
        return match($this->node_type) {
            'ph_ec' => 'mdi-flask',
            'ph' => 'mdi-flask-outline',
            'ec' => 'mdi-flash-outline',
            'climate' => 'mdi-thermometer',
            'relay' => 'mdi-electric-switch',
            'water' => 'mdi-water',
            'display' => 'mdi-monitor',
            'root' => 'mdi-server-network',
            default => 'mdi-chip',
        };
    }

    /**
     * Получить цвет статуса
     */
    public function getStatusColorAttribute(): string
    {
        if (!$this->last_seen_at) {
            return 'grey';
        }

        $timeout = config('hydro.node_offline_timeout', 30);
        $heartbeatInterval = config('hydro.heartbeat_interval', 10);
        $seconds = $this->last_seen_at->diffInSeconds(now());
        
        if ($seconds < $heartbeatInterval) return 'success';      // Онлайн: < интервала heartbeat
        if ($seconds < ($timeout * 0.7)) return 'success';         // Онлайн: < 70% таймаута
        if ($seconds < $timeout) return 'warning';                 // Предупреждение: 70-100% таймаута
        return 'error';                                             // Офлайн: > таймаута
    }

    /**
     * Scope: только онлайн узлы
     */
    public function scopeOnline($query)
    {
        $timeout = config('hydro.node_offline_timeout', 30);
        return $query->where('last_seen_at', '>', now()->subSeconds($timeout));
    }

    /**
     * Scope: только офлайн узлы
     */
    public function scopeOffline($query)
    {
        $timeout = config('hydro.node_offline_timeout', 30);
        return $query->where(function($q) use ($timeout) {
            $q->whereNull('last_seen_at')
              ->orWhere('last_seen_at', '<=', now()->subSeconds($timeout));
        });
    }

    /**
     * Scope: по типу узла
     */
    public function scopeOfType($query, string $type)
    {
        return $query->where('node_type', $type);
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Проверка - это Root Node?
     */
    public function isRootNode(): bool
    {
        return $this->node_type === 'root';
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Получить все узлы своей mesh сети
     */
    public function getMeshNodes()
    {
        if ($this->isRootNode()) {
            return $this->childNodes;
        } else {
            return $this->rootNode?->childNodes ?? collect();
        }
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Scope - узлы определенного Root Node
     */
    public function scopeInZone($query, string $rootNodeId)
    {
        return $query->where('root_node_id', $rootNodeId);
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Scope - только Root Nodes
     */
    public function scopeRootNodes($query)
    {
        return $query->where('node_type', 'root');
    }
}

