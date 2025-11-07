<?php

namespace App\Services;

use App\Models\GrowthCycle;
use App\Models\CycleParameterLog;
use App\Models\CycleComparisonSnapshot;
use App\Models\CycleStageHistory;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Log;
use Carbon\Carbon;

/**
 * ⭐ GROWTH PLANNER: Сервис для аналитики циклов роста
 */
class GrowthAnalyticsService
{
    /**
     * Получить график параметров по стадиям для цикла
     */
    public function getParameterChartByStages(int $cycleId, array $parameters = []): array
    {
        $cycle = GrowthCycle::with(['preset.stages', 'stageHistory.stage'])->findOrFail($cycleId);
        
        // Получаем все логи параметров для цикла
        $logs = CycleParameterLog::where('cycle_id', $cycleId)
            ->orderBy('log_date')
            ->get();

        // Получаем историю стадий
        $stageHistory = $cycle->stageHistory()->with('stage')->orderBy('started_at')->get();

        // Группируем данные по стадиям
        $chartData = [];
        $stages = [];

        foreach ($stageHistory as $history) {
            $stage = $history->stage;
            $stageStart = Carbon::parse($history->started_at);
            $stageEnd = $history->ended_at ? Carbon::parse($history->ended_at) : now();

            $stages[] = [
                'id' => $stage->id,
                'name' => $stage->name,
                'start' => $stageStart->toIso8601String(),
                'end' => $stageEnd->toIso8601String(),
            ];

            // Фильтруем логи для этой стадии
            $stageLogs = $logs->filter(function ($log) use ($stageStart, $stageEnd) {
                $logDate = Carbon::parse($log->log_date);
                return $logDate->between($stageStart, $stageEnd);
            });

            foreach ($stageLogs as $log) {
                $logDate = Carbon::parse($log->log_date)->toIso8601String();
                
                if (!isset($chartData[$logDate])) {
                    $chartData[$logDate] = [
                        'date' => $logDate,
                        'stage_id' => $stage->id,
                        'stage_name' => $stage->name,
                    ];
                }

                // Добавляем параметры из модели CycleParameterLog
                // Используем существующие поля: avg_ph, avg_ec, avg_temp, avg_humidity, avg_co2, avg_lux
                $paramMapping = [
                    'ph' => 'avg_ph',
                    'ec' => 'avg_ec',
                    'temp' => 'avg_temp',
                    'temperature' => 'avg_temp',
                    'humidity' => 'avg_humidity',
                    'co2' => 'avg_co2',
                    'lux' => 'avg_lux',
                ];

                foreach ($paramMapping as $paramKey => $fieldName) {
                    if (empty($parameters) || in_array($paramKey, $parameters)) {
                        $value = $log->$fieldName;
                        if ($value !== null) {
                            $chartData[$logDate][$paramKey] = $value;
                        }
                    }
                }
            }
        }

        return [
            'stages' => $stages,
            'data' => array_values($chartData),
            'parameters' => $this->extractParameters($chartData),
        ];
    }

    /**
     * Получить статистику по параметрам для цикла
     */
    public function getParameterStatistics(int $cycleId, ?int $stageId = null): array
    {
        $query = CycleParameterLog::where('cycle_id', $cycleId);

        if ($stageId) {
            // Фильтруем по стадии
            $stageHistory = CycleStageHistory::where('cycle_id', $cycleId)
                ->where('stage_id', $stageId)
                ->first();

            if ($stageHistory) {
                $start = Carbon::parse($stageHistory->started_at);
                $end = $stageHistory->ended_at 
                    ? Carbon::parse($stageHistory->ended_at) 
                    : now();

                $query->whereBetween('log_date', [$start, $end]);
            }
        }

        $logs = $query->get();

        $statistics = [];
        $paramMapping = [
            'ph' => 'avg_ph',
            'ec' => 'avg_ec',
            'temp' => 'avg_temp',
            'humidity' => 'avg_humidity',
            'co2' => 'avg_co2',
            'lux' => 'avg_lux',
        ];

        foreach ($logs as $log) {
            foreach ($paramMapping as $paramKey => $fieldName) {
                $value = $log->$fieldName;
                if ($value === null) continue;

                if (!isset($statistics[$paramKey])) {
                    $statistics[$paramKey] = [
                        'values' => [],
                        'count' => 0,
                    ];
                }

                $statistics[$paramKey]['values'][] = $value;
                $statistics[$paramKey]['count']++;
            }
        }

        // Вычисляем статистику
        $result = [];
        foreach ($statistics as $param => $data) {
            $values = $data['values'];

            $result[$param] = [
                'count' => $data['count'],
                'avg' => count($values) > 0 ? round(array_sum($values) / count($values), 2) : 0,
                'min' => count($values) > 0 ? round(min($values), 2) : 0,
                'max' => count($values) > 0 ? round(max($values), 2) : 0,
            ];
        }

        return $result;
    }

    /**
     * Сравнение циклов
     */
    public function compareCycles(array $cycleIds): array
    {
        $cycles = GrowthCycle::with(['preset', 'zone', 'culture'])
            ->whereIn('id', $cycleIds)
            ->get();

        if ($cycles->isEmpty()) {
            return [];
        }

        $comparison = [
            'cycles' => [],
            'parameters' => [],
            'stages' => [],
        ];

        foreach ($cycles as $cycle) {
            $cycleData = [
                'id' => $cycle->id,
                'name' => $cycle->name,
                'culture' => $cycle->culture->name ?? 'Unknown',
                'zone' => $cycle->zone->name ?? 'Unknown',
                'started_at' => $cycle->started_at?->toIso8601String(),
                'ended_at' => $cycle->ended_at?->toIso8601String(),
                'duration_days' => $cycle->started_at && $cycle->ended_at
                    ? $cycle->started_at->diffInDays($cycle->ended_at)
                    : null,
                'harvest_weight_kg' => $cycle->harvest_weight_kg,
                'plant_count' => $cycle->plant_count,
                'rating' => $cycle->rating,
            ];

            // Статистика параметров
            $cycleData['parameter_stats'] = $this->getParameterStatistics($cycle->id);

            // История стадий
            $stageHistory = $cycle->stageHistory()->with('stage')->get();
            $cycleData['stages'] = $stageHistory->map(function ($history) {
                return [
                    'stage_id' => $history->stage_id,
                    'stage_name' => $history->stage->name ?? 'Unknown',
                    'started_at' => $history->started_at?->toIso8601String(),
                    'ended_at' => $history->ended_at?->toIso8601String(),
                    'duration_days' => $history->actual_duration_days ?? 0,
                ];
            });

            $comparison['cycles'][] = $cycleData;
        }

        // Агрегированная статистика
        $comparison['summary'] = $this->calculateComparisonSummary($comparison['cycles']);

        return $comparison;
    }

    /**
     * Создать снимок для сравнения
     */
    public function createComparisonSnapshot(int $cycleId): CycleComparisonSnapshot
    {
        $cycle = GrowthCycle::with(['preset', 'zone', 'culture'])->findOrFail($cycleId);

        // Используем метод модели для создания снимка
        return CycleComparisonSnapshot::createFromCycle($cycle);
    }

    /**
     * Получить отчет о цикле
     */
    public function getCycleReport(int $cycleId): array
    {
        $cycle = GrowthCycle::with(['preset.stages', 'zone', 'culture', 'stageHistory.stage'])
            ->findOrFail($cycleId);

        $report = [
            'cycle' => [
                'id' => $cycle->id,
                'name' => $cycle->name,
                'culture' => $cycle->culture->name ?? 'Unknown',
                'zone' => $cycle->zone->name ?? 'Unknown',
                'preset' => $cycle->preset->name ?? 'Unknown',
                'status' => $cycle->status,
                'started_at' => $cycle->started_at?->toIso8601String(),
                'expected_harvest_at' => $cycle->expected_harvest_at?->toIso8601String(),
                'actual_harvest_at' => $cycle->actual_harvest_at?->toIso8601String(),
                'current_day' => $cycle->current_day ?? 0,
                'progress' => round($cycle->progress ?? 0, 2),
            ],
            'stages' => [],
            'parameter_statistics' => $this->getParameterStatistics($cycleId),
            'harvest' => [
                'weight_kg' => $cycle->harvest_weight_kg,
                'plant_count' => $cycle->plant_count,
                'rating' => $cycle->rating,
            ],
            'water_consumption' => $this->calculateWaterConsumption($cycleId),
            'deviations_summary' => $this->getDeviationsSummary($cycleId),
        ];

        // Детали по стадиям
        foreach ($cycle->stageHistory as $history) {
            $stage = $history->stage;
            $report['stages'][] = [
                'id' => $stage->id ?? null,
                'name' => $stage->name ?? 'Unknown',
                'started_at' => $history->started_at?->toIso8601String(),
                'ended_at' => $history->ended_at?->toIso8601String(),
                'duration_days' => $history->actual_duration_days ?? 0,
                'expected_duration_days' => $stage->duration_days ?? 0,
                'parameter_stats' => $this->getParameterStatistics($cycleId, $stage->id ?? null),
            ];
        }

        return $report;
    }

    /**
     * Извлечь список параметров из данных графика
     */
    protected function extractParameters(array $chartData): array
    {
        $parameters = [];
        foreach ($chartData as $point) {
            foreach ($point as $key => $value) {
                if (!in_array($key, ['date', 'stage_id', 'stage_name']) && 
                    !str_ends_with($key, '_deviation')) {
                    $parameters[$key] = true;
                }
            }
        }
        return array_keys($parameters);
    }

    /**
     * Вычислить сводную статистику сравнения
     */
    protected function calculateComparisonSummary(array $cycles): array
    {
        $summary = [
            'total_cycles' => count($cycles),
            'avg_duration_days' => 0,
            'avg_harvest_weight_kg' => 0,
            'avg_rating' => 0,
        ];

        $durations = array_filter(array_column($cycles, 'duration_days'));
        $harvests = array_filter(array_column($cycles, 'harvest_weight_kg'));
        $ratings = array_filter(array_column($cycles, 'rating'));

        if (!empty($durations)) {
            $summary['avg_duration_days'] = round(array_sum($durations) / count($durations), 1);
        }

        if (!empty($harvests)) {
            $summary['avg_harvest_weight_kg'] = round(array_sum($harvests) / count($harvests), 2);
        }

        if (!empty($ratings)) {
            $summary['avg_rating'] = round(array_sum($ratings) / count($ratings), 1);
        }

        return $summary;
    }

    /**
     * Вычислить потребление воды
     */
    protected function calculateWaterConsumption(int $cycleId): array
    {
        $logs = CycleParameterLog::where('cycle_id', $cycleId)
            ->whereNotNull('water_consumed_liters')
            ->get();

        $total = 0;
        $daily = [];

        foreach ($logs as $log) {
            $consumption = $log->water_consumed_liters ?? 0;
            $total += $consumption;
            
            $date = Carbon::parse($log->log_date)->format('Y-m-d');
            if (!isset($daily[$date])) {
                $daily[$date] = 0;
            }
            $daily[$date] += $consumption;
        }

        return [
            'total_liters' => round($total, 2),
            'avg_per_day' => count($daily) > 0 ? round($total / count($daily), 2) : 0,
            'daily' => $daily,
        ];
    }

    /**
     * Получить сводку по отклонениям
     * Вычисляем отклонения от целевых значений стадий
     */
    protected function getDeviationsSummary(int $cycleId): array
    {
        $cycle = GrowthCycle::with(['preset.stages', 'stageHistory.stage'])->findOrFail($cycleId);
        $logs = CycleParameterLog::where('cycle_id', $cycleId)->get();

        $summary = [];
        $paramMapping = [
            'ph' => 'avg_ph',
            'ec' => 'avg_ec',
            'temp' => 'avg_temp',
            'humidity' => 'avg_humidity',
        ];

        foreach ($logs as $log) {
            $logDate = Carbon::parse($log->log_date);
            
            // Находим стадию для этой даты
            $stageHistory = $cycle->stageHistory()
                ->where('started_at', '<=', $logDate)
                ->where(function ($query) use ($logDate) {
                    $query->whereNull('ended_at')
                        ->orWhere('ended_at', '>=', $logDate);
                })
                ->first();

            if (!$stageHistory || !$stageHistory->stage) {
                continue;
            }

            $targetParams = $stageHistory->stage->target_params ?? [];

            foreach ($paramMapping as $paramKey => $fieldName) {
                $value = $log->$fieldName;
                if ($value === null) continue;

                $target = $targetParams[$paramKey] ?? null;
                if ($target === null) continue;

                // Вычисляем отклонение
                $deviation = $value - $target;

                if (!isset($summary[$paramKey])) {
                    $summary[$paramKey] = [
                        'count' => 0,
                        'total_deviation' => 0,
                        'max_deviation' => 0,
                    ];
                }

                $absDeviation = abs($deviation);
                $summary[$paramKey]['count']++;
                $summary[$paramKey]['total_deviation'] += $absDeviation;
                $summary[$paramKey]['max_deviation'] = max($summary[$paramKey]['max_deviation'], $absDeviation);
            }
        }

        // Вычисляем средние
        foreach ($summary as $param => &$data) {
            $data['avg_deviation'] = $data['count'] > 0 
                ? round($data['total_deviation'] / $data['count'], 2) 
                : 0;
            unset($data['total_deviation']);
        }

        return $summary;
    }
}

