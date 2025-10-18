<?php

namespace App\Http\Controllers;

use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

class SimpleEventController extends Controller
{
    public function index(Request $request): JsonResponse
    {
        $events = [
            [
                'id' => 1,
                'node_id' => 'ph_ec_001',
                'level' => 'warning',
                'message' => 'pH slightly low',
                'created_at' => now()->subHours(2)->toIso8601String(),
                'resolved_at' => null,
            ],
            [
                'id' => 2,
                'node_id' => 'climate_001',
                'level' => 'info',
                'message' => 'Temperature optimal',
                'created_at' => now()->subHours(1)->toIso8601String(),
                'resolved_at' => now()->toIso8601String(),
            ],
        ];

        return response()->json($events);
    }

    public function resolve(Request $request, int $id): JsonResponse
    {
        return response()->json([
            'success' => true,
            'message' => 'Event resolved (mock)',
        ]);
    }
}

