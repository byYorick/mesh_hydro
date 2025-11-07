<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Relations\HasOne;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Factories\HasFactory;

class Zone extends Model
{
    use HasFactory;

    /**
     * Таблица в БД
     */
    protected $table = 'zones';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'name',
        'description',
        'root_node_id',
        'mesh_network_id',
        'mqtt_topic_prefix',
        'location',
        'zone_type',
        'reservoir_volume_liters',
        'growing_area_m2',
        'plant_capacity',
        'assigned_nodes',
        'is_active',
        'is_available',
        'current_cycle_id',
        'image_url',
        'notes',
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'assigned_nodes' => 'array',
        'reservoir_volume_liters' => 'decimal:2',
        'growing_area_m2' => 'decimal:2',
        'plant_capacity' => 'integer',
        'is_active' => 'boolean',
        'is_available' => 'boolean',
        'created_at' => 'datetime',
        'updated_at' => 'datetime',
    ];

    /**
     * Root Node зоны
     */
    public function rootNode(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'root_node_id', 'node_id');
    }

    /**
     * Текущий активный цикл
     */
    public function currentCycle(): HasOne
    {
        return $this->hasOne(GrowthCycle::class, 'id', 'current_cycle_id');
    }

    /**
     * Все циклы зоны
     */
    public function cycles(): HasMany
    {
        return $this->hasMany(GrowthCycle::class, 'zone_id');
    }

    /**
     * Все узлы зоны (через root_node_id)
     */
    public function nodes(): HasMany
    {
        return $this->hasMany(Node::class, 'root_node_id', 'root_node_id');
    }

    /**
     * История назначения узлов
     */
    public function nodeAssignments(): HasMany
    {
        return $this->hasMany(ZoneNodeAssignment::class, 'zone_id');
    }

    /**
     * Получить узел по роли
     * 
     * @param string $role ph_node, climate_node, relay_node, etc.
     * @return string|null
     */
    public function getNodeByRole(string $role): ?string
    {
        return data_get($this->assigned_nodes, $role);
    }

    /**
     * Получить все узлы зоны (ID)
     * 
     * @return array
     */
    public function getAllNodes(): array
    {
        return array_filter([
            $this->getNodeByRole('ph_node'),
            $this->getNodeByRole('climate_node'),
            $this->getNodeByRole('relay_node'),
            $this->getNodeByRole('water_node'),
            $this->getNodeByRole('display_node'),
        ]);
    }

    /**
     * Проверка доступности для нового цикла
     * 
     * @return bool
     */
    public function isAvailableForCycle(): bool
    {
        return $this->is_available && 
               $this->is_active && 
               is_null($this->current_cycle_id);
    }

    /**
     * Проверка занятости узлов другими зонами
     * 
     * @return array ['node_id' => ['zone_name' => ..., 'zone_id' => ...]]
     */
    public function checkNodesAvailability(): array
    {
        $nodes = $this->getAllNodes();
        $busy = [];

        foreach ($nodes as $nodeId) {
            // Проверяем, не используется ли узел в другой активной зоне
            $otherZone = self::where('id', '!=', $this->id)
                ->where('is_active', true)
                ->where('assigned_nodes', 'like', "%$nodeId%")
                ->whereNotNull('current_cycle_id')
                ->first();

            if ($otherZone) {
                $busy[$nodeId] = [
                    'zone_name' => $otherZone->name,
                    'zone_id' => $otherZone->id,
                ];
            }
        }

        return $busy;
    }

    /**
     * Получить MQTT topic prefix
     * 
     * @return string
     */
    public function getMqttTopicPrefix(): string
    {
        return $this->mqtt_topic_prefix ?? "hydro/zone{$this->id}/";
    }

    /**
     * Scope: Активные зоны
     */
    public function scopeActive($query)
    {
        return $query->where('is_active', true);
    }

    /**
     * Scope: Доступные зоны (для новых циклов)
     */
    public function scopeAvailable($query)
    {
        return $query->where('is_available', true)
                     ->where('is_active', true)
                     ->whereNull('current_cycle_id');
    }

    /**
     * Scope: По типу зоны
     */
    public function scopeOfType($query, string $type)
    {
        return $query->where('zone_type', $type);
    }

    /**
     * Получить иконку зоны
     * 
     * @return string
     */
    public function getIconAttribute(): string
    {
        return match($this->zone_type) {
            'nft' => 'mdi-water-pump',
            'dwc' => 'mdi-water',
            'ebb_flow' => 'mdi-waves',
            'drip' => 'mdi-water-opacity',
            default => 'mdi-sprout',
        };
    }

    /**
     * Получить статус зоны
     * 
     * @return array
     */
    public function getStatusAttribute(): array
    {
        return [
            'is_active' => $this->is_active,
            'is_available' => $this->is_available,
            'has_active_cycle' => !is_null($this->current_cycle_id),
            'nodes_count' => count($this->getAllNodes()),
            'root_node_online' => $this->rootNode?->online ?? false,
        ];
    }
}

