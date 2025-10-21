<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Carbon\Carbon;

class Schedule extends Model
{
    protected $fillable = [
        'node_id',
        'name',
        'time_start',
        'time_end',
        'config',
        'enabled',
        'priority',
        'days_of_week',
    ];

    protected $casts = [
        'config' => 'array',
        'enabled' => 'boolean',
        'priority' => 'integer',
        'days_of_week' => 'array',
    ];

    /**
     * Связь с узлом
     */
    public function node(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    /**
     * Проверка активно ли расписание сейчас
     */
    public function isActiveNow(): bool
    {
        if (!$this->enabled) {
            return false;
        }

        $now = Carbon::now();
        $currentTime = $now->format('H:i:s');
        $currentDay = $now->dayOfWeekIso; // 1 = Monday, 7 = Sunday

        // Проверка дня недели
        if ($this->days_of_week !== null && !in_array($currentDay, $this->days_of_week)) {
            return false;
        }

        // Проверка времени
        $start = Carbon::parse($this->time_start)->format('H:i:s');
        $end = Carbon::parse($this->time_end)->format('H:i:s');

        // Обработка переход через полночь
        if ($start < $end) {
            return $currentTime >= $start && $currentTime < $end;
        } else {
            return $currentTime >= $start || $currentTime < $end;
        }
    }

    /**
     * Получить активные расписания для узла
     */
    public static function getActiveForNode(string $nodeId): ?self
    {
        return self::where('node_id', $nodeId)
            ->where('enabled', true)
            ->get()
            ->filter(fn($schedule) => $schedule->isActiveNow())
            ->sortByDesc('priority')
            ->first();
    }
}

