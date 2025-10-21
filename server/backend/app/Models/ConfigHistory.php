<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class ConfigHistory extends Model
{
    protected $table = 'config_history';

    const UPDATED_AT = null; // Отключаем updated_at
    const CREATED_AT = 'changed_at';

    protected $fillable = [
        'node_id',
        'user_id',
        'old_config',
        'new_config',
        'changes',
        'change_type',
        'comment',
    ];

    protected $casts = [
        'old_config' => 'array',
        'new_config' => 'array',
        'changes' => 'array',
        'changed_at' => 'datetime',
    ];

    /**
     * Связь с узлом
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Создать запись истории
     */
    public static function logChange(
        string $nodeId,
        array $oldConfig,
        array $newConfig,
        string $changeType,
        ?string $userId = null,
        ?string $comment = null
    ): self {
        $changes = self::calculateDiff($oldConfig, $newConfig);

        return self::create([
            'node_id' => $nodeId,
            'user_id' => $userId ?? 'system',
            'old_config' => $oldConfig,
            'new_config' => $newConfig,
            'changes' => $changes,
            'change_type' => $changeType,
            'comment' => $comment,
        ]);
    }

    /**
     * Вычислить разницу между конфигурациями
     */
    private static function calculateDiff(array $old, array $new): array
    {
        $changes = [];

        foreach ($new as $key => $value) {
            $oldValue = $old[$key] ?? null;
            
            // Для массивов используем json сравнение
            if (is_array($value) && is_array($oldValue)) {
                if (json_encode($oldValue) !== json_encode($value)) {
                    $changes[$key] = [
                        'old' => $oldValue,
                        'new' => $value,
                    ];
                }
            }
            // Для скалярных значений обычное сравнение
            elseif ($oldValue !== $value) {
                $changes[$key] = [
                    'old' => $oldValue,
                    'new' => $value,
                ];
            }
        }

        return $changes;
    }
}

