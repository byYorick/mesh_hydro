<?php

namespace App\Http\Controllers;

use App\Models\Zone;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\DB;

/**
 * ⭐ ZONING: Контроллер статистики зон
 */
class ZoneStatisticsController extends Controller
{
    /**
     * Получить агрегированную статистику зоны
     */
    public function getAggregatedStats(Zone $zone, Request $request): JsonResponse
    {
        $hours = $request->get('hours', 24);
        $interval = $request->get('interval', '1hour'); // 5min, 15min, 1hour, 1day

        // Получаем все узлы зоны
        $nodes = $zone->getAllNodes();
        $nodeIds = $nodes->pluck('node_id')->toArray();

        if (empty($nodeIds)) {
            return response()->json([
                'message' => 'No nodes in zone',
                'data' => [],
            ]);
        }

        // Определение группировки по времени (PostgreSQL)
        $groupBy = match($interval) {
            '5min' => "date_trunc('minute', received_at) + INTERVAL '5 min' * FLOOR(EXTRACT(MINUTE FROM received_at) / 5)",
            '15min' => "date_trunc('minute', received_at) + INTERVAL '15 min' * FLOOR(EXTRACT(MINUTE FROM received_at) / 15)",
            '1hour' => "date_trunc('hour', received_at)",
            '1day' => "date_trunc('day', received_at)",
            default => "date_trunc('hour', received_at)",
        };

        // Агрегированные данные
        $stats = DB::table('telemetry')
            ->select(
                DB::raw("{$groupBy} as time_bucket"),
                DB::raw("AVG((data->>'ph')::numeric) as avg_ph"),
                DB::raw("AVG((data->>'ec')::numeric) as avg_ec"),
                DB::raw("AVG((data->>'temp')::numeric) as avg_temp"),
                DB::raw("AVG((data->>'temperature')::numeric) as avg_temperature"),
                DB::raw("AVG((data->>'humidity')::numeric) as avg_humidity"),
                DB::raw("AVG((data->>'co2')::numeric) as avg_co2"),
                DB::raw("AVG((data->>'lux')::numeric) as avg_lux"),
                DB::raw("COUNT(*) as data_points")
            )
            ->whereIn('node_id', $nodeIds)
            ->where('received_at', '>', now()->subHours($hours))
            ->groupBy('time_bucket')
            ->orderBy('time_bucket', 'asc')
            ->get();

        return response()->json([
            'zone' => $zone->name,
            'interval' => $interval,
            'period_hours' => $hours,
            'data' => $stats,
        ]);
    }

    /**
     * Получить последние значения параметров зоны
     */
    public function getLatestValues(Zone $zone): JsonResponse
    {
        $nodes = $zone->getAllNodes();
        $nodeIds = $nodes->pluck('node_id')->toArray();

        if (empty($nodeIds)) {
            return response()->json([
                'message' => 'No nodes in zone',
                'data' => null,
            ]);
        }

        // Получаем последнюю телеметрию от каждого узла
        $latestTelemetry = DB::table('telemetry')
            ->select('node_id', 'data', 'received_at')
            ->whereIn('node_id', $nodeIds)
            ->orderBy('received_at', 'desc')
            ->limit(count($nodeIds) * 2) // Берем с запасом
            ->get()
            ->groupBy('node_id')
            ->map(fn($group) => $group->first());

        // Агрегируем параметры
        $aggregated = [
            'ph' => [],
            'ec' => [],
            'temp' => [],
            'humidity' => [],
            'co2' => [],
            'lux' => [],
        ];

        foreach ($latestTelemetry as $t) {
            $data = is_string($t->data) ? json_decode($t->data, true) : $t->data;
            
            foreach ($aggregated as $param => &$values) {
                if (isset($data[$param])) {
                    $values[] = (float) $data[$param];
                } elseif ($param === 'temp' && isset($data['temperature'])) {
                    $values[] = (float) $data['temperature'];
                }
            }
        }

        // Вычисляем средние
        $result = [];
        foreach ($aggregated as $param => $values) {
            $result[$param] = !empty($values) 
                ? round(array_sum($values) / count($values), 2) 
                : null;
        }

        return response()->json([
            'zone' => $zone->name,
            'timestamp' => now()->toIso8601String(),
            'values' => $result,
            'nodes_count' => count($latestTelemetry),
        ]);
    }

    /**
     * Сравнить зоны по эффективности
     */
    public function compareZones(Request $request): JsonResponse
    {
        $validated = $request->validate([
            'zone_ids' => 'required|array',
            'zone_ids.*' => 'integer|exists:zones,id',
            'metric' => 'nullable|in:harvest,water_efficiency,cycle_duration',
        ]);

        $metric = $validated['metric'] ?? 'harvest';
        $zones = Zone::whereIn('id', $validated['zone_ids'])->get();

        $comparison = [];

        foreach ($zones as $zone) {
            // Получаем завершенные циклы зоны
            $completedCycles = $zone->cycles()
                ->where('status', 'harvested')
                ->with('preset', 'culture')
                ->get();

            $zoneStats = [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'total_cycles' => $completedCycles->count(),
                'avg_harvest_kg' => $completedCycles->avg('harvest_weight_kg'),
                'total_harvest_kg' => $completedCycles->sum('harvest_weight_kg'),
                'avg_cycle_days' => null, // TODO: вычислить
                'avg_rating' => $completedCycles->avg('rating'),
            ];

            $comparison[] = $zoneStats;
        }

        return response()->json([
            'metric' => $metric,
            'zones' => $comparison,
        ]);
    }
}

