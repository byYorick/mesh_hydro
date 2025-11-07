<?php

namespace App\Http\Controllers;

use App\Models\GrowthCycle;
use App\Models\Zone;
use App\Models\GrowthPreset;
use App\Models\CycleStageHistory;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Log;

/**
 * ⭐ GROWTH PLANNER: Контроллер циклов роста (привязан к зонам!)
 */
class GrowthCycleController extends Controller
{
    public function index(Request $request)
    {
        $query = GrowthCycle::with(['zone', 'preset', 'culture', 'currentStage']);

        if ($request->has('zone_id')) {
            $query->where('zone_id', $request->zone_id);
        }

        if ($request->has('status')) {
            $query->where('status', $request->status);
        }

        $cycles = $query->orderBy('started_at', 'desc')->get();

        return response()->json($cycles);
    }

    public function show(GrowthCycle $cycle)
    {
        $cycle->load([
            'zone',
            'preset.stages',
            'culture',
            'currentStage',
            'stageHistory.stage',
            'notifications' => function ($q) {
                $q->orderBy('created_at', 'desc')->limit(10);
            },
        ]);

        return response()->json($cycle);
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Создание цикла с привязкой к зоне
     */
    public function store(Request $request)
    {
        $validated = $request->validate([
            'zone_id' => 'required|exists:zones,id',
            'preset_id' => 'required|exists:growth_presets,id',
            'plant_count' => 'nullable|integer|min:1',
            'notes' => 'nullable|string',
        ]);

        // Проверка доступности зоны
        $zone = Zone::find($validated['zone_id']);
        
        if (!$zone->isAvailableForCycle()) {
            return response()->json([
                'message' => 'Zone is not available for new cycle',
                'current_cycle' => $zone->currentCycle,
            ], 409);
        }

        $preset = GrowthPreset::with('stages', 'culture')->find($validated['preset_id']);

        DB::beginTransaction();
        try {
            // Создать цикл
            $cycle = GrowthCycle::create([
                'zone_id' => $validated['zone_id'],
                'preset_id' => $preset->id,
                'culture_id' => $preset->culture_id,
                'current_stage_id' => $preset->stages->first()->id,
                'started_at' => now(),
                'expected_harvest_at' => now()->addDays($preset->total_days),
                'status' => 'active',
                'plant_count' => $validated['plant_count'] ?? null,
                'notes' => $validated['notes'] ?? null,
            ]);

            // Создать запись истории для первой стадии
            CycleStageHistory::create([
                'cycle_id' => $cycle->id,
                'stage_id' => $preset->stages->first()->id,
                'started_at' => now(),
                'applied_params' => $preset->stages->first()->target_params,
            ]);

            // Обновить зону
            $zone->update(['current_cycle_id' => $cycle->id, 'is_available' => false]);

            // Инкремент счетчика использования пресета
            $preset->incrementUsage();

            DB::commit();

            $cycle->load(['zone', 'preset', 'culture', 'currentStage']);
            
            Log::info("Growth cycle created", [
                'cycle_id' => $cycle->id,
                'zone_id' => $zone->id,
                'preset_id' => $preset->id,
            ]);

            return response()->json($cycle, 201);
        } catch (\Exception $e) {
            DB::rollBack();
            Log::error("Error creating growth cycle: " . $e->getMessage());
            return response()->json(['message' => 'Error creating cycle', 'error' => $e->getMessage()], 500);
        }
    }

    public function update(Request $request, GrowthCycle $cycle)
    {
        $validated = $request->validate([
            'plant_count' => 'nullable|integer|min:1',
            'notes' => 'nullable|string',
            'status' => 'sometimes|in:planning,active,paused,harvested,failed,cancelled',
        ]);

        $cycle->update($validated);

        return response()->json($cycle);
    }

    /**
     * Завершить цикл (сбор урожая)
     */
    public function harvest(Request $request, GrowthCycle $cycle)
    {
        $validated = $request->validate([
            'harvest_weight_kg' => 'required|numeric|min:0',
            'notes' => 'nullable|string',
            'rating' => 'nullable|integer|min:1|max:5',
        ]);

        DB::beginTransaction();
        try {
            // Завершить текущую стадию
            $currentHistory = $cycle->stageHistory()
                ->whereNull('ended_at')
                ->first();

            if ($currentHistory) {
                $durationDays = max(1, (int) $currentHistory->started_at->diffInDays(now()));
                $currentHistory->update([
                    'ended_at' => now(),
                    'actual_duration_days' => $durationDays,
                ]);
            }

            // Обновить цикл
            $cycle->update([
                'status' => 'harvested',
                'ended_at' => now(),
                'actual_harvest_at' => now(),
                'harvest_weight_kg' => $validated['harvest_weight_kg'],
                'notes' => $cycle->notes . "\n\nHarvest: " . ($validated['notes'] ?? ''),
                'rating' => $validated['rating'] ?? null,
            ]);

            // Освободить зону
            $cycle->zone->update([
                'current_cycle_id' => null,
                'is_available' => true,
            ]);

            DB::commit();

            Log::info("Growth cycle harvested", [
                'cycle_id' => $cycle->id,
                'harvest_weight_kg' => $validated['harvest_weight_kg'],
            ]);

            return response()->json($cycle);
        } catch (\Exception $e) {
            DB::rollBack();
            return response()->json(['message' => 'Error harvesting cycle', 'error' => $e->getMessage()], 500);
        }
    }

    /**
     * Отменить цикл
     */
    public function cancel(Request $request, GrowthCycle $cycle)
    {
        $validated = $request->validate([
            'reason' => 'required|string',
        ]);

        DB::beginTransaction();
        try {
            $cycle->update([
                'status' => 'cancelled',
                'ended_at' => now(),
                'notes' => $cycle->notes . "\n\nCancelled: " . $validated['reason'],
            ]);

            // Освободить зону
            $cycle->zone->update([
                'current_cycle_id' => null,
                'is_available' => true,
            ]);

            DB::commit();

            return response()->json($cycle);
        } catch (\Exception $e) {
            DB::rollBack();
            return response()->json(['message' => 'Error cancelling cycle', 'error' => $e->getMessage()], 500);
        }
    }

    /**
     * Получить текущую статистику цикла
     */
    public function stats(GrowthCycle $cycle)
    {
        $stats = [
            'current_day' => $cycle->current_day,
            'remaining_days' => $cycle->remaining_days,
            'progress_percent' => $cycle->progress,
            'stage_count' => $cycle->preset->stages()->count(),
            'current_stage_order' => $cycle->currentStage?->order,
            'total_notifications' => $cycle->notifications()->count(),
            'unread_notifications' => $cycle->notifications()->unread()->count(),
            'average_params' => [
                'ph' => $cycle->parameterLogs()->avg('avg_ph'),
                'ec' => $cycle->parameterLogs()->avg('avg_ec'),
                'temp' => $cycle->parameterLogs()->avg('avg_temp'),
                'humidity' => $cycle->parameterLogs()->avg('avg_humidity'),
            ],
            'resource_consumption' => [
                'water_liters' => $cycle->parameterLogs()->sum('water_consumed_liters'),
                'nutrient_a_ml' => $cycle->parameterLogs()->sum('nutrient_a_ml'),
                'nutrient_b_ml' => $cycle->parameterLogs()->sum('nutrient_b_ml'),
            ],
        ];

        return response()->json($stats);
    }

    /**
     * Ручной переход на следующую стадию
     */
    public function transition(Request $request, GrowthCycle $cycle)
    {
        $validated = $request->validate([
            'to_stage_id' => 'required|exists:growth_stages,id',
            'notes' => 'nullable|string',
        ]);

        $nextStage = \App\Models\GrowthStage::find($validated['to_stage_id']);

        // Проверка что стадия принадлежит текущему пресету
        if ($nextStage->preset_id !== $cycle->preset_id) {
            return response()->json([
                'message' => 'Stage does not belong to current preset',
            ], 422);
        }

        DB::beginTransaction();
        try {
            // Завершить текущую стадию
            $currentHistory = $cycle->stageHistory()
                ->whereNull('ended_at')
                ->first();

            if ($currentHistory) {
                $durationDays = max(1, (int) $currentHistory->started_at->diffInDays(now()));
                $currentHistory->update([
                    'ended_at' => now(),
                    'actual_duration_days' => $durationDays,
                ]);
            }

            // Создать новую запись истории
            CycleStageHistory::create([
                'cycle_id' => $cycle->id,
                'stage_id' => $nextStage->id,
                'started_at' => now(),
                'applied_params' => $nextStage->target_params,
            ]);

            // Обновить цикл
            $cycle->update([
                'current_stage_id' => $nextStage->id,
            ]);

            DB::commit();

            Log::info("Cycle stage transition", [
                'cycle_id' => $cycle->id,
                'from_stage' => $currentHistory?->stage_id,
                'to_stage' => $nextStage->id,
            ]);

            return response()->json($cycle->fresh(['currentStage', 'stageHistory']));
        } catch (\Exception $e) {
            DB::rollBack();
            return response()->json(['message' => 'Error transitioning stage', 'error' => $e->getMessage()], 500);
        }
    }

    /**
     * Принять рекомендацию перехода стадии
     */
    public function acceptTransition(GrowthCycle $cycle, \App\Models\StageTransitionRecommendation $recommendation)
    {
        // Проверка что рекомендация принадлежит циклу
        if ($recommendation->cycle_id !== $cycle->id) {
            return response()->json([
                'message' => 'Recommendation does not belong to this cycle',
            ], 422);
        }

        // Проверка статуса
        if ($recommendation->status !== 'pending') {
            return response()->json([
                'message' => 'Recommendation is not pending',
                'status' => $recommendation->status,
            ], 422);
        }

        DB::beginTransaction();
        try {
            // Выполнить переход
            $nextStage = $recommendation->recommendedStage;

            // Завершить текущую стадию
            $currentHistory = $cycle->stageHistory()
                ->whereNull('ended_at')
                ->first();

            if ($currentHistory) {
                $durationDays = max(1, (int) $currentHistory->started_at->diffInDays(now()));
                $currentHistory->update([
                    'ended_at' => now(),
                    'actual_duration_days' => $durationDays,
                ]);
            }

            // Создать новую запись истории
            CycleStageHistory::create([
                'cycle_id' => $cycle->id,
                'stage_id' => $nextStage->id,
                'started_at' => now(),
                'applied_params' => $recommendation->recommended_params ?? $nextStage->target_params,
            ]);

            // Обновить цикл
            $cycle->update([
                'current_stage_id' => $nextStage->id,
            ]);

            // Обновить рекомендацию
            $recommendation->update([
                'status' => 'accepted',
                'responded_at' => now(),
            ]);

            DB::commit();

            Log::info("Cycle transition recommendation accepted", [
                'cycle_id' => $cycle->id,
                'recommendation_id' => $recommendation->id,
                'to_stage' => $nextStage->id,
            ]);

            return response()->json($cycle->fresh(['currentStage', 'stageHistory']));
        } catch (\Exception $e) {
            DB::rollBack();
            return response()->json(['message' => 'Error accepting transition', 'error' => $e->getMessage()], 500);
        }
    }
}

