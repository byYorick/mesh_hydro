<?php

namespace App\Http\Controllers;

use Illuminate\Http\JsonResponse;

class SimpleDashboardController extends Controller
{
    public function summary(): JsonResponse
    {
        return response()->json([
            'nodes' => [
                'total' => 3,
                'online' => 2,
                'offline' => 1,
                'by_type' => [
                    'ph_ec' => 1,
                    'climate' => 1,
                    'water' => 1,
                ],
            ],
            'events' => [
                'active' => 1,
                'critical' => 0,
                'recent' => [],
            ],
            'commands' => [
                'today' => 5,
                'pending' => 0,
            ],
            'telemetry' => [
                'last_hour' => 120,
                'latest' => [],
            ],
            'timestamp' => now()->toIso8601String(),
        ]);
    }

    public function status(): JsonResponse
    {
        return response()->json([
            'status' => 'running',
            'database' => 'not configured',
            'mqtt' => 'not configured',
            'telegram' => 'disabled',
            'system' => [
                'php_version' => PHP_VERSION,
                'laravel_version' => app()->version(),
                'server_time' => now()->toDateTimeString(),
            ],
        ]);
    }
}

