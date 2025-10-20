<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class Command extends Model
{
    /**
     * Таблица в БД
     */
    protected $table = 'commands';

    /**
     * Поля доступные для массового заполнения
     */
    protected $fillable = [
        'node_id',
        'command',        // "calibrate", "open_windows", "pump_on", "reboot"
        'params',         // jsonb - параметры команды
        'status',         // "pending", "sent", "acknowledged", "failed"
        'sent_at',        // timestamp когда отправлена
        'acknowledged_at', // timestamp когда узел подтвердил получение
        'completed_at',   // timestamp когда выполнена
        'response',       // jsonb - ответ от узла
        'error',          // текст ошибки если не выполнена
        'user_id',        // кто отправил команду
        'timeout_at',     // timestamp когда команда должна быть отменена
        'timeout_seconds', // таймаут в секундах
    ];

    /**
     * Приведение типов
     */
    protected $casts = [
        'params' => 'array',
        'response' => 'array',
        'sent_at' => 'datetime',
        'acknowledged_at' => 'datetime',
        'completed_at' => 'datetime',
        'timeout_at' => 'datetime',
        'timeout_seconds' => 'integer',
    ];

    /**
     * Константы статусов команд
     */
    const STATUS_PENDING = 'pending';
    const STATUS_SENT = 'sent';
    const STATUS_ACKNOWLEDGED = 'acknowledged';
    const STATUS_COMPLETED = 'completed';
    const STATUS_FAILED = 'failed';

    /**
     * Отношение: команда принадлежит узлу
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Проверка: команда завершена?
     */
    public function isCompleted(): bool
    {
        return $this->status === self::STATUS_COMPLETED;
    }

    /**
     * Проверка: команда провалилась?
     */
    public function isFailed(): bool
    {
        return $this->status === self::STATUS_FAILED;
    }

    /**
     * Проверка: команда в процессе?
     */
    public function isPending(): bool
    {
        return in_array($this->status, [self::STATUS_PENDING, self::STATUS_SENT, self::STATUS_ACKNOWLEDGED]);
    }

    /**
     * Пометить команду как отправленную
     */
    public function markAsSent(): void
    {
        $this->update([
            'status' => self::STATUS_SENT,
            'sent_at' => now(),
        ]);
    }

    /**
     * Пометить команду как подтвержденную
     */
    public function markAsAcknowledged(): void
    {
        $this->update([
            'status' => self::STATUS_ACKNOWLEDGED,
            'acknowledged_at' => now(),
        ]);
    }

    /**
     * Пометить команду как завершенную
     */
    public function markAsCompleted(array $response = []): void
    {
        $this->update([
            'status' => self::STATUS_COMPLETED,
            'completed_at' => now(),
            'response' => $response,
        ]);
    }

    /**
     * Пометить команду как провалившуюся
     */
    public function markAsFailed(string $error): void
    {
        $this->update([
            'status' => self::STATUS_FAILED,
            'error' => $error,
        ]);
    }

    /**
     * Scope: только ожидающие команды
     */
    public function scopePending($query)
    {
        return $query->whereIn('status', [self::STATUS_PENDING, self::STATUS_SENT, self::STATUS_ACKNOWLEDGED]);
    }

    /**
     * Scope: только завершенные команды
     */
    public function scopeCompleted($query)
    {
        return $query->where('status', self::STATUS_COMPLETED);
    }

    /**
     * Scope: только проваленные команды
     */
    public function scopeFailed($query)
    {
        return $query->where('status', self::STATUS_FAILED);
    }

    /**
     * Scope: для конкретного узла
     */
    public function scopeForNode($query, string $nodeId)
    {
        return $query->where('node_id', $nodeId);
    }

    /**
     * Scope: просроченные команды
     */
    public function scopeTimedOut($query)
    {
        return $query->where('timeout_at', '<', now())
                    ->whereIn('status', [self::STATUS_PENDING, self::STATUS_SENT, self::STATUS_ACKNOWLEDGED]);
    }

    /**
     * Проверка: команда просрочена?
     */
    public function isTimedOut(): bool
    {
        return $this->timeout_at && $this->timeout_at->isPast() && $this->isPending();
    }

    /**
     * Установить таймаут команды
     */
    public function setTimeout(int $seconds = 300): void
    {
        $this->update([
            'timeout_seconds' => $seconds,
            'timeout_at' => now()->addSeconds($seconds),
        ]);
    }

    /**
     * Пометить команду как просроченную
     */
    public function markAsTimedOut(): void
    {
        $this->update([
            'status' => self::STATUS_FAILED,
            'error' => 'Command timed out after ' . $this->timeout_seconds . ' seconds',
        ]);
    }

    /**
     * Создать команду с таймаутом
     */
    public static function createWithTimeout(array $data, int $timeoutSeconds = 300): self
    {
        $command = self::create(array_merge($data, [
            'timeout_seconds' => $timeoutSeconds,
            'timeout_at' => now()->addSeconds($timeoutSeconds),
        ]));

        return $command;
    }
}

