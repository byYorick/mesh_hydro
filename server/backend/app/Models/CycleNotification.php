<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Factories\HasFactory;

/**
 * ⭐ GROWTH PLANNER: Уведомления для циклов
 */
class CycleNotification extends Model
{
    use HasFactory;

    protected $fillable = [
        'cycle_id',
        'type',
        'title',
        'message',
        'data',
        'is_read',
        'is_actioned',
        'sent_web',
        'sent_telegram',
        'sent_sms',
    ];

    protected $casts = [
        'data' => 'array',
        'is_read' => 'boolean',
        'is_actioned' => 'boolean',
        'sent_web' => 'boolean',
        'sent_telegram' => 'boolean',
        'sent_sms' => 'boolean',
    ];

    public function cycle(): BelongsTo
    {
        return $this->belongsTo(GrowthCycle::class, 'cycle_id');
    }

    /**
     * Scope: непрочитанные
     */
    public function scopeUnread($query)
    {
        return $query->where('is_read', false);
    }

    /**
     * Scope: критичные
     */
    public function scopeCritical($query)
    {
        return $query->whereIn('type', ['parameter_deviation', 'resource_low']);
    }

    /**
     * Отметить как прочитанное
     */
    public function markAsRead(): void
    {
        $this->update(['is_read' => true]);
    }

    /**
     * Отметить действие выполненным
     */
    public function markAsActioned(): void
    {
        $this->update(['is_actioned' => true]);
    }
}

