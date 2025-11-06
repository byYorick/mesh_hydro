<?php

namespace App\Http\Controllers;

use Illuminate\Http\JsonResponse;

class StatusController extends Controller
{
    /**
     * Получить пороги конфигурации статусов
     */
    public function thresholds(): JsonResponse
    {
        return response()->json([
            'heartbeat_interval' => config('hydro.heartbeat_interval', 10),
            'node_offline_timeout' => config('hydro.node_offline_timeout', 30),
            'status_colors' => [
                'online' => 'success',
                'warning' => 'warning',
                'offline' => 'error',
                'unknown' => 'grey',
            ],
        ]);
    }
}
