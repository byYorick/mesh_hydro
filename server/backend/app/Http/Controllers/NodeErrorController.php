<?php

namespace App\Http\Controllers;

use App\Models\NodeError;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\DB;

class NodeErrorController extends Controller
{
    /**
     * Список всех ошибок с фильтрами
     */
    public function index(Request $request): JsonResponse
    {
        $query = NodeError::with('node');

        // Фильтр по узлу
        if ($request->has('node_id')) {
            $query->where('node_id', $request->node_id);
        }

        // Фильтр по типу ошибки
        if ($request->has('error_type')) {
            $query->where('error_type', $request->error_type);
        }

        // Фильтр по severity
        if ($request->has('severity')) {
            $query->where('severity', $request->severity);
        }

        // Фильтр по статусу (active/resolved)
        if ($request->has('status')) {
            if ($request->status === 'active') {
                $query->unresolved();
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
            $query->where('occurred_at', '>', now()->subHours($hours));
        }

        // Сортировка
        $query->orderBy('occurred_at', 'desc');

        // Пагинация или лимит
        if ($request->has('paginate')) {
            $perPage = $request->get('per_page', 50);
            $errors = $query->paginate($perPage);
        } else {
            $limit = $request->get('limit', 100);
            $errors = $query->limit($limit)->get();
        }

        // Добавление вычисляемых полей
        if (!$request->has('paginate')) {
            $errors->each(function ($error) {
                $error->severity_color = $error->severity_color;
                $error->type_icon = $error->type_icon;
            });
        }

        return response()->json($errors);
    }

    /**
     * Детали ошибки
     */
    public function show(int $id): JsonResponse
    {
        $error = NodeError::with('node')->findOrFail($id);

        $error->severity_color = $error->severity_color;
        $error->type_icon = $error->type_icon;

        return response()->json($error);
    }

    /**
     * Ошибки для конкретного узла
     */
    public function forNode(string $nodeId, Request $request): JsonResponse
    {
        $query = NodeError::forNode($nodeId);

        // Фильтр по статусу
        if ($request->has('status')) {
            if ($request->status === 'active') {
                $query->unresolved();
            } elseif ($request->status === 'resolved') {
                $query->resolved();
            }
        }

        // Фильтр по периоду
        if ($request->has('hours')) {
            $hours = (int) $request->hours;
            $query->where('occurred_at', '>', now()->subHours($hours));
        }

        $query->orderBy('occurred_at', 'desc');

        $limit = $request->get('limit', 50);
        $errors = $query->limit($limit)->get();

        return response()->json([
            'node_id' => $nodeId,
            'count' => $errors->count(),
            'errors' => $errors,
        ]);
    }

    /**
     * Статистика по ошибкам
     */
    public function statistics(Request $request): JsonResponse
    {
        $hours = $request->get('hours', 24);

        $query = NodeError::where('occurred_at', '>', now()->subHours($hours));

        $total = $query->count();
        $active = (clone $query)->unresolved()->count();
        $resolved = (clone $query)->resolved()->count();
        $critical = (clone $query)->critical()->count();

        // По severity
        $bySeverity = (clone $query)
            ->selectRaw('severity, COUNT(*) as count')
            ->groupBy('severity')
            ->pluck('count', 'severity');

        // По типу
        $byType = (clone $query)
            ->selectRaw('error_type, COUNT(*) as count')
            ->groupBy('error_type')
            ->pluck('count', 'error_type');

        // По узлам (топ 10)
        $byNode = (clone $query)
            ->selectRaw('node_id, COUNT(*) as count')
            ->groupBy('node_id')
            ->orderByDesc('count')
            ->limit(10)
            ->get();

        // Топ error codes
        $topErrorCodes = (clone $query)
            ->selectRaw('error_code, COUNT(*) as count')
            ->groupBy('error_code')
            ->orderByDesc('count')
            ->limit(10)
            ->get();

        return response()->json([
            'period_hours' => $hours,
            'total' => $total,
            'active' => $active,
            'resolved' => $resolved,
            'critical' => $critical,
            'by_severity' => $bySeverity,
            'by_type' => $byType,
            'top_nodes' => $byNode,
            'top_error_codes' => $topErrorCodes,
        ]);
    }

    /**
     * Пометить ошибку как решенную
     */
    public function resolve(Request $request, int $id): JsonResponse
    {
        $error = NodeError::findOrFail($id);

        if ($error->isResolved()) {
            return response()->json([
                'success' => false,
                'error' => 'Error is already resolved',
            ], 400);
        }

        $validated = $request->validate([
            'resolved_by' => 'nullable|string|max:100',
            'resolution_notes' => 'nullable|string|max:1000',
        ]);

        $error->resolve(
            $validated['resolved_by'] ?? 'manual',
            $validated['resolution_notes'] ?? null
        );

        return response()->json([
            'success' => true,
            'message' => 'Error resolved',
            'error' => $error,
        ]);
    }

    /**
     * Массовое резолвение ошибок
     */
    public function resolveBulk(Request $request): JsonResponse
    {
        $validated = $request->validate([
            'error_ids' => 'required|array',
            'error_ids.*' => 'integer|exists:node_errors,id',
            'resolved_by' => 'nullable|string|max:100',
            'resolution_notes' => 'nullable|string|max:1000',
        ]);

        $resolvedBy = $validated['resolved_by'] ?? 'bulk';
        $notes = $validated['resolution_notes'] ?? null;

        $count = NodeError::whereIn('id', $validated['error_ids'])
            ->whereNull('resolved_at')
            ->update([
                'resolved_at' => now(),
                'resolved_by' => $resolvedBy,
                'resolution_notes' => $notes,
            ]);

        return response()->json([
            'success' => true,
            'message' => "Resolved {$count} errors",
            'count' => $count,
        ]);
    }

    /**
     * Удаление ошибки (только для resolved)
     */
    public function destroy(int $id): JsonResponse
    {
        $error = NodeError::findOrFail($id);

        if (!$error->isResolved()) {
            return response()->json([
                'success' => false,
                'error' => 'Can only delete resolved errors',
            ], 400);
        }

        $error->delete();

        return response()->json([
            'success' => true,
            'message' => 'Error deleted',
        ]);
    }
}

