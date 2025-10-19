<?php

use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;
// Полные контроллеры с поддержкой БД
use App\Http\Controllers\NodeController;
use App\Http\Controllers\TelemetryController;
use App\Http\Controllers\EventController;
use App\Http\Controllers\DashboardController;

/*
|--------------------------------------------------------------------------
| API Routes
|--------------------------------------------------------------------------
|
| Все маршруты для REST API Mesh Hydro System
| Префикс: /api
|
*/

// Public routes with rate limiting
Route::middleware('throttle:api')->group(function () {
    // Dashboard и система
    Route::get('/dashboard/summary', [DashboardController::class, 'summary']);
    Route::get('/status', [DashboardController::class, 'status']);
    Route::get('/health', function () {
        return response()->json([
            'status' => 'ok',
            'timestamp' => now()->toIso8601String(),
        ]);
    });
});

// Узлы (Nodes) - с более строгим rate limiting для write operations
Route::middleware('throttle:api')->group(function () {
    Route::prefix('nodes')->group(function () {
        Route::get('/', [NodeController::class, 'index']);
        Route::get('/{nodeId}', [NodeController::class, 'show']);
        Route::get('/{nodeId}/statistics', [NodeController::class, 'statistics']);
        
        // Write operations с ограниченным rate limit
        Route::middleware('throttle:30,1')->group(function () {
            Route::post('/', [NodeController::class, 'store']);
            Route::put('/{nodeId}', [NodeController::class, 'update']);
            Route::delete('/{nodeId}', [NodeController::class, 'destroy']);
            Route::post('/{nodeId}/command', [NodeController::class, 'sendCommand']);
            Route::put('/{nodeId}/config', [NodeController::class, 'updateConfig']);
        });
    });
});

// Телеметрия
Route::middleware('throttle:api')->group(function () {
    Route::prefix('telemetry')->group(function () {
        Route::get('/', [TelemetryController::class, 'index']);
        Route::get('/latest', [TelemetryController::class, 'latest']);
        Route::get('/aggregate', [TelemetryController::class, 'aggregate']);
        Route::get('/export', [TelemetryController::class, 'export']);
    });
});

// События
Route::middleware('throttle:api')->group(function () {
    Route::prefix('events')->group(function () {
        Route::get('/', [EventController::class, 'index']);
        Route::get('/statistics', [EventController::class, 'statistics']);
        Route::get('/{id}', [EventController::class, 'show']);
        
        // Write operations
        Route::middleware('throttle:30,1')->group(function () {
            Route::post('/{id}/resolve', [EventController::class, 'resolve']);
            Route::post('/resolve-bulk', [EventController::class, 'resolveBulk']);
            Route::delete('/{id}', [EventController::class, 'destroy']);
        });
    });
});

