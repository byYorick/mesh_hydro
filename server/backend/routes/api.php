<?php

use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;
// Используем упрощенные контроллеры без БД для быстрого старта
use App\Http\Controllers\SimpleNodeController as NodeController;
use App\Http\Controllers\SimpleEventController as EventController;
use App\Http\Controllers\SimpleDashboardController as DashboardController;
// use App\Http\Controllers\TelemetryController;

/*
|--------------------------------------------------------------------------
| API Routes
|--------------------------------------------------------------------------
|
| Все маршруты для REST API Mesh Hydro System
| Префикс: /api
|
*/

// Dashboard и система
Route::get('/dashboard/summary', [DashboardController::class, 'summary']);
Route::get('/status', [DashboardController::class, 'status']);

// Узлы (Nodes)
Route::prefix('nodes')->group(function () {
    Route::get('/', [NodeController::class, 'index']);
    Route::post('/', [NodeController::class, 'store']);
    Route::get('/{nodeId}', [NodeController::class, 'show']);
    Route::delete('/{nodeId}', [NodeController::class, 'destroy']);
    
    // Управление узлами
    Route::post('/{nodeId}/command', [NodeController::class, 'sendCommand']);
    Route::put('/{nodeId}/config', [NodeController::class, 'updateConfig']);
    Route::get('/{nodeId}/statistics', [NodeController::class, 'statistics']);
});

// Телеметрия (временно отключена - требует БД)
Route::prefix('telemetry')->group(function () {
    Route::get('/', function() {
        return response()->json(['count' => 0, 'data' => []]);
    });
    Route::get('/latest', function() {
        return response()->json([]);
    });
});

// События
Route::prefix('events')->group(function () {
    Route::get('/', [EventController::class, 'index']);
    Route::get('/statistics', [EventController::class, 'statistics']);
    Route::get('/{id}', [EventController::class, 'show']);
    Route::post('/{id}/resolve', [EventController::class, 'resolve']);
    Route::post('/resolve-bulk', [EventController::class, 'resolveBulk']);
    Route::delete('/{id}', [EventController::class, 'destroy']);
});

// Health check
Route::get('/health', function () {
    return response()->json([
        'status' => 'ok',
        'timestamp' => now()->toIso8601String(),
    ]);
});

