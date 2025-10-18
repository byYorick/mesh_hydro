<?php

namespace App\Http\Controllers;

use App\Models\Event;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

class EventController extends Controller
{
    /**
     * Список событий с фильтрами
     */
    public function index(Request $request): JsonResponse
    {
        $query = Event::with('node');

        // Фильтр по узлу
        if ($request->has('node_id')) {
            $query->where('node_id', $request->node_id);
        }

        // Фильтр по уровню
        if ($request->has('level')) {
            $query->where('level', $request->level);
        }

        // Фильтр по статусу (активные/решенные)
        if ($request->has('status')) {
            if ($request->status === 'active') {
                $query->active();
            } elseif ($request->status === 'resolved') {
                $query->resolved();
            }
        }

        // Фильтр только критичные
        if ($request->boolean('critical_only')) {
            $query->critical();
        }

        // Фильтр по периоду
        if ($request->has('hours')) {
            $hours = (int) $request->hours;
            $query->where('created_at', '>', now()->subHours($hours));
        }

        // Сортировка
        $query->orderBy('created_at', 'desc');

        // Пагинация или лимит
        if ($request->has('paginate')) {
            $perPage = $request->get('per_page', 50);
            $events = $query->paginate($perPage);
        } else {
            $limit = $request->get('limit', 100);
            $events = $query->limit($limit)->get();
        }

        // Добавление вычисляемых полей
        if (!$request->has('paginate')) {
            $events->each(function ($event) {
                $event->level_color = $event->level_color;
                $event->level_icon = $event->level_icon;
            });
        }

        return response()->json($events);
    }

    /**
     * Детали события
     */
    public function show(int $id): JsonResponse
    {
        $event = Event::with('node')->findOrFail($id);

        $event->level_color = $event->level_color;
        $event->level_icon = $event->level_icon;

        return response()->json($event);
    }

    /**
     * Пометить событие как решенное
     */
    public function resolve(Request $request, int $id): JsonResponse
    {
        $event = Event::findOrFail($id);

        if ($event->isResolved()) {
            return response()->json([
                'success' => false,
                'error' => 'Event is already resolved',
            ], 400);
        }

        $resolvedBy = $request->get('resolved_by', 'manual');
        $event->resolve($resolvedBy);

        return response()->json([
            'success' => true,
            'message' => 'Event resolved',
            'event' => $event,
        ]);
    }

    /**
     * Массовое резолвение событий
     */
    public function resolveBulk(Request $request): JsonResponse
    {
        $validated = $request->validate([
            'event_ids' => 'required|array',
            'event_ids.*' => 'integer|exists:events,id',
            'resolved_by' => 'nullable|string|max:100',
        ]);

        $resolvedBy = $validated['resolved_by'] ?? 'bulk';

        $count = Event::whereIn('id', $validated['event_ids'])
            ->whereNull('resolved_at')
            ->update([
                'resolved_at' => now(),
                'resolved_by' => $resolvedBy,
            ]);

        return response()->json([
            'success' => true,
            'message' => "Resolved {$count} events",
            'count' => $count,
        ]);
    }

    /**
     * Статистика по событиям
     */
    public function statistics(Request $request): JsonResponse
    {
        $hours = $request->get('hours', 24);

        $query = Event::where('created_at', '>', now()->subHours($hours));

        $total = $query->count();
        $active = (clone $query)->active()->count();
        $resolved = (clone $query)->resolved()->count();

        $byLevel = (clone $query)
            ->selectRaw('level, COUNT(*) as count')
            ->groupBy('level')
            ->pluck('count', 'level');

        $byNode = (clone $query)
            ->selectRaw('node_id, COUNT(*) as count')
            ->groupBy('node_id')
            ->orderByDesc('count')
            ->limit(10)
            ->get();

        return response()->json([
            'period_hours' => $hours,
            'total' => $total,
            'active' => $active,
            'resolved' => $resolved,
            'by_level' => $byLevel,
            'top_nodes' => $byNode,
        ]);
    }

    /**
     * Удаление события (только для resolved событий)
     */
    public function destroy(int $id): JsonResponse
    {
        $event = Event::findOrFail($id);

        if (!$event->isResolved()) {
            return response()->json([
                'success' => false,
                'error' => 'Can only delete resolved events',
            ], 400);
        }

        $event->delete();

        return response()->json([
            'success' => true,
            'message' => 'Event deleted',
        ]);
    }
}

