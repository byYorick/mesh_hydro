<?php

namespace App\Http\Controllers;

use App\Models\Telemetry;
use App\Models\Node;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\DB;

class TelemetryController extends Controller
{
    /**
     * Получить телеметрию с фильтрами
     */
    public function index(Request $request): JsonResponse
    {
        $query = Telemetry::query();

        // Фильтр по узлу
        if ($request->has('node_id')) {
            $query->where('node_id', $request->node_id);
        }

        // Фильтр по типу узла
        if ($request->has('node_type')) {
            $query->where('node_type', $request->node_type);
        }

        // Фильтр по периоду
        if ($request->has('hours')) {
            $hours = (int) $request->hours;
            $query->where('received_at', '>', now()->subHours($hours));
        } elseif ($request->has('from') && $request->has('to')) {
            $query->whereBetween('received_at', [
                $request->from,
                $request->to,
            ]);
        } else {
            // По умолчанию последние 24 часа
            $query->where('received_at', '>', now()->subHours(24));
        }

        // Сортировка
        $query->orderBy('received_at', 'desc');

        // Лимит
        $limit = $request->get('limit', 1000);
        $telemetry = $query->limit($limit)->get();

        return response()->json([
            'count' => $telemetry->count(),
            'data' => $telemetry,
        ]);
    }

    /**
     * Последняя телеметрия для всех узлов
     */
    public function latest(): JsonResponse
    {
        $nodes = Node::with('lastTelemetry')->get();

        $data = $nodes->map(function ($node) {
            return [
                'node_id' => $node->node_id,
                'node_type' => $node->node_type,
                'zone' => $node->zone,
                'online' => $node->isOnline(),
                'last_seen_at' => $node->last_seen_at,
                'telemetry' => $node->lastTelemetry,
            ];
        });

        return response()->json($data);
    }

    /**
     * Агрегированные данные (среднее, мин, макс)
     */
    public function aggregate(Request $request): JsonResponse
    {
        $validated = $request->validate([
            'node_id' => 'required|string',
            'field' => 'required|string',
            'hours' => 'nullable|integer|min:1|max:8760',
            'interval' => 'nullable|string|in:5min,15min,1hour,1day',
        ]);

        $nodeId = $validated['node_id'];
        $field = $validated['field'];
        $hours = $validated['hours'] ?? 24;
        $interval = $validated['interval'] ?? '1hour';

        // Определение группировки по времени
        $groupBy = match($interval) {
            '5min' => "date_trunc('minute', received_at) + INTERVAL '5 min' * FLOOR(EXTRACT(MINUTE FROM received_at) / 5)",
            '15min' => "date_trunc('minute', received_at) + INTERVAL '15 min' * FLOOR(EXTRACT(MINUTE FROM received_at) / 15)",
            '1hour' => "date_trunc('hour', received_at)",
            '1day' => "date_trunc('day', received_at)",
            default => "date_trunc('hour', received_at)",
        };

        $results = DB::table('telemetry')
            ->select(
                DB::raw("{$groupBy} as time_bucket"),
                DB::raw("AVG((data->>'{$field}')::numeric) as avg"),
                DB::raw("MIN((data->>'{$field}')::numeric) as min"),
                DB::raw("MAX((data->>'{$field}')::numeric) as max"),
                DB::raw("COUNT(*) as count")
            )
            ->where('node_id', $nodeId)
            ->where('received_at', '>', now()->subHours($hours))
            ->whereNotNull(DB::raw("data->>'{$field}'"))
            ->groupBy('time_bucket')
            ->orderBy('time_bucket', 'asc')
            ->get();

        return response()->json([
            'node_id' => $nodeId,
            'field' => $field,
            'interval' => $interval,
            'period_hours' => $hours,
            'data' => $results,
        ]);
    }

    /**
     * Экспорт телеметрии в CSV
     */
    public function export(Request $request): \Symfony\Component\HttpFoundation\StreamedResponse
    {
        $validated = $request->validate([
            'node_id' => 'nullable|string',
            'hours' => 'nullable|integer|min:1',
        ]);

        $query = Telemetry::query();

        if (isset($validated['node_id'])) {
            $query->where('node_id', $validated['node_id']);
        }

        if (isset($validated['hours'])) {
            $query->where('received_at', '>', now()->subHours($validated['hours']));
        }

        $query->orderBy('received_at', 'desc');

        $headers = [
            'Content-Type' => 'text/csv',
            'Content-Disposition' => 'attachment; filename="telemetry_export.csv"',
        ];

        return response()->stream(function () use ($query) {
            $handle = fopen('php://output', 'w');
            
            // Заголовки CSV
            fputcsv($handle, ['id', 'node_id', 'node_type', 'data', 'received_at']);

            // Данные
            $query->chunk(100, function ($telemetries) use ($handle) {
                foreach ($telemetries as $telemetry) {
                    fputcsv($handle, [
                        $telemetry->id,
                        $telemetry->node_id,
                        $telemetry->node_type,
                        json_encode($telemetry->data),
                        $telemetry->received_at->toDateTimeString(),
                    ]);
                }
            });

            fclose($handle);
        }, 200, $headers);
    }
}

