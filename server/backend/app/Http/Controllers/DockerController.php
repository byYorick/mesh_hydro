<?php

namespace App\Http\Controllers;

use App\Services\DockerService;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\Log;

class DockerController extends Controller
{
    /**
     * Получить статус контейнеров
     */
    public function status(DockerService $docker): JsonResponse
    {
        $status = $docker->getStatus();

        return response()->json([
            'success' => true,
            'docker' => $status,
        ]);
    }

    /**
     * Перезапустить все контейнеры
     */
    public function restartAll(DockerService $docker): JsonResponse
    {
        if (!config('docker.allowed_operations.restart', true)) {
            return response()->json([
                'success' => false,
                'error' => 'Restart operation is disabled in config',
            ], 403);
        }

        $result = $docker->restartAll();

        Log::info("Docker restart all requested", [
            'success' => $result['success'],
            'user' => request()->user()?->email ?? 'api',
        ]);

        return response()->json($result, $result['success'] ? 200 : 500);
    }

    /**
     * Перезапустить конкретный контейнер
     */
    public function restartContainer(Request $request, DockerService $docker): JsonResponse
    {
        if (!config('docker.allowed_operations.restart', true)) {
            return response()->json([
                'success' => false,
                'error' => 'Restart operation is disabled',
            ], 403);
        }

        $validated = $request->validate([
            'container' => 'required|string|in:backend,frontend,websocket,mqtt,postgres,redis',
        ]);

        $result = $docker->restartContainer($validated['container']);

        Log::info("Docker restart container requested", [
            'container' => $validated['container'],
            'success' => $result['success'],
            'user' => request()->user()?->email ?? 'api',
        ]);

        return response()->json($result, $result['success'] ? 200 : 500);
    }

    /**
     * Запустить все контейнеры
     */
    public function startAll(DockerService $docker): JsonResponse
    {
        if (!config('docker.allowed_operations.start', true)) {
            return response()->json([
                'success' => false,
                'error' => 'Start operation is disabled',
            ], 403);
        }

        $result = $docker->startAll();

        return response()->json($result, $result['success'] ? 200 : 500);
    }

    /**
     * Остановить все контейнеры
     */
    public function stopAll(DockerService $docker): JsonResponse
    {
        if (!config('docker.allowed_operations.stop', false)) {
            return response()->json([
                'success' => false,
                'error' => 'Stop operation is disabled for safety',
            ], 403);
        }

        $result = $docker->stopAll();

        Log::warning("Docker stop all requested", [
            'success' => $result['success'],
            'user' => request()->user()?->email ?? 'api',
        ]);

        return response()->json($result, $result['success'] ? 200 : 500);
    }

    /**
     * Получить логи контейнера
     */
    public function getLogs(Request $request, DockerService $docker): JsonResponse
    {
        $validated = $request->validate([
            'container' => 'required|string',
            'lines' => 'integer|min:10|max:1000',
        ]);

        $result = $docker->getLogs(
            $validated['container'],
            $validated['lines'] ?? 100
        );

        return response()->json($result);
    }
}

