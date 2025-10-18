<?php

namespace App\Http\Controllers;

use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

class SimpleNodeController extends Controller
{
    /**
     * Временный контроллер без БД для тестирования
     */
    public function index(Request $request): JsonResponse
    {
        // Mock данные для тестирования
        $nodes = [
            [
                'id' => 1,
                'node_id' => 'ph_ec_001',
                'node_type' => 'ph_ec',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:01',
                'online' => true,
                'is_online' => true,
                'last_seen_at' => now()->toIso8601String(),
                'status_color' => 'success',
                'icon' => 'mdi-flask',
                'last_telemetry' => [
                    'data' => [
                        'ph' => 6.5,
                        'ec' => 1.8,
                        'temp' => 24.3,
                    ],
                    'received_at' => now()->toIso8601String(),
                ],
            ],
            [
                'id' => 2,
                'node_id' => 'climate_001',
                'node_type' => 'climate',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:02',
                'online' => true,
                'is_online' => true,
                'last_seen_at' => now()->toIso8601String(),
                'status_color' => 'success',
                'icon' => 'mdi-thermometer',
                'last_telemetry' => [
                    'data' => [
                        'temp' => 25.5,
                        'humidity' => 65.0,
                        'co2' => 850,
                    ],
                    'received_at' => now()->toIso8601String(),
                ],
            ],
            [
                'id' => 3,
                'node_id' => 'water_001',
                'node_type' => 'water',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:03',
                'online' => false,
                'is_online' => false,
                'last_seen_at' => now()->subMinutes(5)->toIso8601String(),
                'status_color' => 'error',
                'icon' => 'mdi-water',
                'last_telemetry' => null,
            ],
        ];

        return response()->json($nodes);
    }

    public function show(string $nodeId): JsonResponse
    {
        return response()->json([
            'node_id' => $nodeId,
            'node_type' => 'ph_ec',
            'zone' => 'Zone 1',
            'online' => true,
            'last_seen_at' => now()->toIso8601String(),
            'telemetry' => [],
            'events' => [],
            'commands' => [],
        ]);
    }

    public function sendCommand(Request $request, string $nodeId): JsonResponse
    {
        return response()->json([
            'success' => true,
            'message' => 'Command sent (mock)',
        ]);
    }
}

