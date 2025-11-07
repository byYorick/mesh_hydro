<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;
use App\Models\Command;

/**
 * ⭐ GROWTH PLANNER: Подтверждения конфигурации от узлов
 */
class NodeConfigurationConfirmation extends Model
{
    use HasFactory;

    protected $fillable = [
        'cycle_id',
        'node_id',
        'sent_config',
        'confirmed_config',
        'sent_at',
        'confirmed_at',
        'failed_at',
        'status',
        'error_message',
    ];

    protected $casts = [
        'sent_config' => 'array',
        'confirmed_config' => 'array',
        'sent_at' => 'datetime',
        'confirmed_at' => 'datetime',
        'failed_at' => 'datetime',
    ];

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    public function command(): BelongsTo
    {
        return $this->belongsTo(Command::class, 'command_id');
    }

    /**
     * Scope: ожидающие подтверждения
     */
    public function scopePending($query)
    {
        return $query->where('status', 'pending');
    }

    /**
     * Подтвердить конфигурацию
     */
    public function confirm(array $confirmedConfig): void
    {
        $this->update([
            'status' => 'confirmed',
            'confirmed_config' => $confirmedConfig,
            'confirmed_at' => now(),
        ]);
    }

    /**
     * Отметить как провальную
     */
    public function fail(string $errorMessage): void
    {
        $this->update([
            'status' => 'failed',
            'failed_at' => now(),
            'error_message' => $errorMessage,
        ]);
    }

    /**
     * Проверить таймаут (более 60 секунд без ответа)
     */
    public function checkTimeout(): bool
    {
        if ($this->status !== 'pending') {
            return false;
        }

        $seconds = $this->sent_at->diffInSeconds(now());
        if ($seconds > 60) {
            $this->update(['status' => 'timeout']);
            return true;
        }

        return false;
    }
}

