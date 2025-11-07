<?php

namespace App\Http\Controllers;

use App\Models\CycleNotification;
use App\Models\GrowthCycle;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

/**
 * ⭐ GROWTH PLANNER: Контроллер уведомлений
 */
class NotificationController extends Controller
{
    /**
     * Получить все уведомления
     */
    public function index(Request $request): JsonResponse
    {
        $query = CycleNotification::query();

        if ($request->has('cycle_id')) {
            $query->where('cycle_id', $request->cycle_id);
        }

        if ($request->has('type')) {
            $query->where('type', $request->type);
        }

        if ($request->has('unread_only') && $request->unread_only) {
            $query->where('is_read', false);
        }

        $notifications = $query->with('cycle.zone')
            ->orderBy('created_at', 'desc')
            ->paginate($request->get('per_page', 50));

        return response()->json($notifications);
    }

    /**
     * Пометить как прочитанное
     */
    public function markAsRead(CycleNotification $notification): JsonResponse
    {
        $notification->update(['is_read' => true]);

        return response()->json([
            'message' => 'Notification marked as read',
            'notification' => $notification,
        ]);
    }

    /**
     * Пометить все как прочитанные
     */
    public function markAllAsRead(Request $request): JsonResponse
    {
        $cycleId = $request->get('cycle_id');

        $query = CycleNotification::where('is_read', false);
        
        if ($cycleId) {
            $query->where('cycle_id', $cycleId);
        }

        $count = $query->update(['is_read' => true]);

        return response()->json([
            'message' => "{$count} notifications marked as read",
            'count' => $count,
        ]);
    }

    /**
     * Получить непрочитанные уведомления
     */
    public function getUnread(Request $request): JsonResponse
    {
        $notifications = CycleNotification::where('is_read', false)
            ->with('cycle.zone')
            ->orderBy('created_at', 'desc')
            ->limit($request->get('limit', 10))
            ->get();

        return response()->json([
            'count' => $notifications->count(),
            'notifications' => $notifications,
        ]);
    }
}

