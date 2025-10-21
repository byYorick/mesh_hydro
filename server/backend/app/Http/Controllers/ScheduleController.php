<?php

namespace App\Http\Controllers;

use App\Models\Schedule;
use App\Models\Node;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\Log;

class ScheduleController extends Controller
{
    /**
     * Получить все расписания для узла
     */
    public function index(string $nodeId): JsonResponse
    {
        $schedules = Schedule::where('node_id', $nodeId)
            ->orderBy('priority', 'desc')
            ->orderBy('time_start')
            ->get();

        return response()->json([
            'success' => true,
            'schedules' => $schedules,
        ]);
    }

    /**
     * Создать новое расписание
     */
    public function store(Request $request, string $nodeId): JsonResponse
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $validated = $request->validate([
            'name' => 'required|string|max:100',
            'time_start' => 'required|date_format:H:i',
            'time_end' => 'required|date_format:H:i',
            'config' => 'required|array',
            'enabled' => 'boolean',
            'priority' => 'integer|min:0|max:100',
            'days_of_week' => 'nullable|array',
            'days_of_week.*' => 'integer|min:1|max:7',
        ]);

        $validated['node_id'] = $nodeId;

        $schedule = Schedule::create($validated);

        Log::info("Schedule created", [
            'node_id' => $nodeId,
            'schedule_id' => $schedule->id,
            'name' => $validated['name']
        ]);

        return response()->json([
            'success' => true,
            'message' => 'Schedule created successfully',
            'schedule' => $schedule,
        ], 201);
    }

    /**
     * Обновить расписание
     */
    public function update(Request $request, string $nodeId, int $scheduleId): JsonResponse
    {
        $schedule = Schedule::where('node_id', $nodeId)
            ->where('id', $scheduleId)
            ->firstOrFail();

        $validated = $request->validate([
            'name' => 'sometimes|string|max:100',
            'time_start' => 'sometimes|date_format:H:i',
            'time_end' => 'sometimes|date_format:H:i',
            'config' => 'sometimes|array',
            'enabled' => 'boolean',
            'priority' => 'integer|min:0|max:100',
            'days_of_week' => 'nullable|array',
            'days_of_week.*' => 'integer|min:1|max:7',
        ]);

        $schedule->update($validated);

        return response()->json([
            'success' => true,
            'message' => 'Schedule updated successfully',
            'schedule' => $schedule,
        ]);
    }

    /**
     * Удалить расписание
     */
    public function destroy(string $nodeId, int $scheduleId): JsonResponse
    {
        $schedule = Schedule::where('node_id', $nodeId)
            ->where('id', $scheduleId)
            ->firstOrFail();

        $schedule->delete();

        return response()->json([
            'success' => true,
            'message' => 'Schedule deleted successfully',
        ]);
    }

    /**
     * Получить активное расписание для узла
     */
    public function getActive(string $nodeId): JsonResponse
    {
        $activeSchedule = Schedule::getActiveForNode($nodeId);

        return response()->json([
            'success' => true,
            'active_schedule' => $activeSchedule,
        ]);
    }
}

