<?php

namespace App\Http\Controllers;

use App\Models\StageTransitionRecommendation;
use App\Models\GrowthCycle;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\Log;

/**
 * ⭐ GROWTH PLANNER: Контроллер рекомендаций переходов стадий
 */
class StageTransitionController extends Controller
{
    /**
     * Получить активные рекомендации
     */
    public function index(Request $request): JsonResponse
    {
        $query = StageTransitionRecommendation::query();

        if ($request->has('cycle_id')) {
            $query->where('cycle_id', $request->cycle_id);
        }

        if ($request->has('status')) {
            $query->where('status', $request->status);
        } else {
            // По умолчанию только pending
            $query->where('status', 'pending');
        }

        $recommendations = $query->with([
            'cycle.zone',
            'currentStage',
            'recommendedStage',
        ])
        ->orderBy('recommended_at', 'desc')
        ->get();

        return response()->json($recommendations);
    }

    /**
     * Получить рекомендацию
     */
    public function show(StageTransitionRecommendation $recommendation): JsonResponse
    {
        $recommendation->load([
            'cycle.zone',
            'currentStage',
            'recommendedStage',
        ]);

        return response()->json($recommendation);
    }

    /**
     * Принять рекомендацию
     */
    public function accept(StageTransitionRecommendation $recommendation): JsonResponse
    {
        if ($recommendation->status !== 'pending') {
            return response()->json([
                'message' => 'Recommendation is not pending',
                'status' => $recommendation->status,
            ], 422);
        }

        // Используем метод из GrowthCycleController
        $cycle = $recommendation->cycle;
        
        // TODO: Переход через существующий метод
        // $cycle->acceptTransition($recommendation);
        
        $recommendation->update([
            'status' => 'accepted',
            'responded_at' => now(),
        ]);

        Log::info("Stage transition accepted", [
            'recommendation_id' => $recommendation->id,
            'cycle_id' => $cycle->id,
        ]);

        return response()->json([
            'message' => 'Transition accepted',
            'recommendation' => $recommendation->fresh(),
        ]);
    }

    /**
     * Отклонить рекомендацию
     */
    public function reject(Request $request, StageTransitionRecommendation $recommendation): JsonResponse
    {
        if ($recommendation->status !== 'pending') {
            return response()->json([
                'message' => 'Recommendation is not pending',
                'status' => $recommendation->status,
            ], 422);
        }

        $validated = $request->validate([
            'reason' => 'nullable|string|max:500',
        ]);

        $recommendation->update([
            'status' => 'rejected',
            'responded_at' => now(),
            'rejection_reason' => $validated['reason'] ?? null,
        ]);

        Log::info("Stage transition rejected", [
            'recommendation_id' => $recommendation->id,
            'cycle_id' => $recommendation->cycle_id,
            'reason' => $validated['reason'] ?? 'No reason provided',
        ]);

        return response()->json([
            'message' => 'Transition rejected',
            'recommendation' => $recommendation->fresh(),
        ]);
    }
}

