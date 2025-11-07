<?php

namespace App\Http\Controllers;

use App\Services\GrowthAnalyticsService;
use App\Models\GrowthCycle;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

/**
 * ⭐ GROWTH PLANNER: Контроллер для аналитики циклов роста
 */
class GrowthAnalyticsController extends Controller
{
    public function __construct(
        private GrowthAnalyticsService $analytics
    ) {}

    /**
     * Получить график параметров по стадиям
     */
    public function getParameterChart(Request $request, int $cycleId): JsonResponse
    {
        $parameters = $request->get('parameters', []);
        
        if (is_string($parameters)) {
            $parameters = explode(',', $parameters);
        }

        $chartData = $this->analytics->getParameterChartByStages($cycleId, $parameters);

        return response()->json($chartData);
    }

    /**
     * Получить статистику параметров
     */
    public function getParameterStatistics(Request $request, int $cycleId): JsonResponse
    {
        $stageId = $request->get('stage_id');
        
        $statistics = $this->analytics->getParameterStatistics($cycleId, $stageId);

        return response()->json($statistics);
    }

    /**
     * Сравнение циклов
     */
    public function compareCycles(Request $request): JsonResponse
    {
        $request->validate([
            'cycle_ids' => 'required|array',
            'cycle_ids.*' => 'integer|exists:growth_cycles,id',
        ]);

        $comparison = $this->analytics->compareCycles($request->cycle_ids);

        return response()->json($comparison);
    }

    /**
     * Создать снимок для сравнения
     */
    public function createSnapshot(int $cycleId): JsonResponse
    {
        $snapshot = $this->analytics->createComparisonSnapshot($cycleId);

        return response()->json($snapshot, 201);
    }

    /**
     * Получить отчет о цикле
     */
    public function getCycleReport(int $cycleId): JsonResponse
    {
        $report = $this->analytics->getCycleReport($cycleId);

        return response()->json($report);
    }
}


