<?php

use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;
// Полные контроллеры с поддержкой БД
use App\Http\Controllers\NodeController;
use App\Http\Controllers\TelemetryController;
use App\Http\Controllers\EventController;
use App\Http\Controllers\DashboardController;
use App\Http\Controllers\NodeErrorController;
use App\Http\Controllers\PidPresetController;

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

// Fallback polling routes с более мягким rate limiting
Route::middleware('throttle:120,1')->group(function () {
    Route::get('/nodes', [NodeController::class, 'index']);
    Route::get('/events', [EventController::class, 'index']);
});

// Узлы (Nodes) - с более строгим rate limiting для write operations
Route::middleware('throttle:api')->group(function () {
    Route::prefix('nodes')->group(function () {
        Route::get('/{nodeId}', [NodeController::class, 'show']);
        Route::get('/{nodeId}/statistics', [NodeController::class, 'statistics']);
        
        // Write operations с ограниченным rate limit
        Route::middleware('throttle:60,1')->group(function () {
            Route::post('/', [NodeController::class, 'store']);
            Route::put('/{nodeId}', [NodeController::class, 'update']);
            Route::delete('/{nodeId}', [NodeController::class, 'destroy']);
            Route::post('/{nodeId}/command', [NodeController::class, 'sendCommand']);
            Route::put('/{nodeId}/config', [NodeController::class, 'updateConfig']);
            
            // Управление насосами
            Route::post('/{nodeId}/pump/run', [NodeController::class, 'runPump']);
            Route::post('/{nodeId}/pump/calibrate', [NodeController::class, 'calibratePump']);
            Route::get('/{nodeId}/pump/calibrations', [NodeController::class, 'getPumpCalibrations']);
            
            // Запрос конфигурации
            Route::get('/{nodeId}/config/request', [NodeController::class, 'requestConfig']);
            Route::get('/{nodeId}/config/history', [NodeController::class, 'getConfigHistory']);
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

// Ошибки узлов
Route::middleware('throttle:api')->group(function () {
    Route::prefix('errors')->group(function () {
        Route::get('/', [NodeErrorController::class, 'index']);
        Route::get('/statistics', [NodeErrorController::class, 'statistics']);
        Route::get('/{id}', [NodeErrorController::class, 'show']);
        
        // Write operations
        Route::middleware('throttle:30,1')->group(function () {
            Route::post('/{id}/resolve', [NodeErrorController::class, 'resolve']);
            Route::post('/resolve-bulk', [NodeErrorController::class, 'resolveBulk']);
            Route::delete('/{id}', [NodeErrorController::class, 'destroy']);
        });
    });
    
    // Ошибки для конкретного узла
    Route::get('/nodes/{nodeId}/errors', [NodeErrorController::class, 'forNode']);
});

// PID пресеты
Route::middleware('throttle:api')->group(function () {
    Route::prefix('pid-presets')->group(function () {
        Route::get('/', [PidPresetController::class, 'index']);
        Route::get('/{id}', [PidPresetController::class, 'show']);
        
        // Write operations
        Route::middleware('throttle:30,1')->group(function () {
            Route::post('/', [PidPresetController::class, 'store']);
            Route::put('/{id}', [PidPresetController::class, 'update']);
            Route::delete('/{id}', [PidPresetController::class, 'destroy']);
        });
    });
});

// Расписания (Schedules)
Route::middleware('throttle:api')->group(function () {
    Route::prefix('schedules')->group(function () {
        Route::get('/node/{nodeId}', [\App\Http\Controllers\ScheduleController::class, 'index']);
        Route::get('/node/{nodeId}/active', [\App\Http\Controllers\ScheduleController::class, 'getActive']);
        
        // Write operations
        Route::middleware('throttle:30,1')->group(function () {
            Route::post('/node/{nodeId}', [\App\Http\Controllers\ScheduleController::class, 'store']);
            Route::put('/node/{nodeId}/{scheduleId}', [\App\Http\Controllers\ScheduleController::class, 'update']);
            Route::delete('/node/{nodeId}/{scheduleId}', [\App\Http\Controllers\ScheduleController::class, 'destroy']);
        });
    });
});

// Docker управление
Route::middleware('throttle:api')->group(function () {
    Route::prefix('docker')->group(function () {
        Route::get('/status', [\App\Http\Controllers\DockerController::class, 'status']);
        Route::get('/logs', [\App\Http\Controllers\DockerController::class, 'getLogs']);
        
        // Write operations (очень строгий rate limit)
        Route::middleware('throttle:10,1')->group(function () {
            Route::post('/restart/all', [\App\Http\Controllers\DockerController::class, 'restartAll']);
            Route::post('/restart/container', [\App\Http\Controllers\DockerController::class, 'restartContainer']);
            Route::post('/start/all', [\App\Http\Controllers\DockerController::class, 'startAll']);
            Route::post('/stop/all', [\App\Http\Controllers\DockerController::class, 'stopAll']);
        });
    });
});

// Настройки системы
Route::middleware('throttle:api')->group(function () {
    Route::prefix('settings')->group(function () {
        Route::get('/', [\App\Http\Controllers\SettingsController::class, 'index']);
        Route::get('/telegram', [\App\Http\Controllers\SettingsController::class, 'getTelegram']);
        Route::get('/telegram/chat-id', [\App\Http\Controllers\SettingsController::class, 'getChatId']);
        
        // Write operations
        Route::middleware('throttle:30,1')->group(function () {
            Route::post('/telegram', [\App\Http\Controllers\SettingsController::class, 'saveTelegram']);
            Route::post('/telegram/test', [\App\Http\Controllers\SettingsController::class, 'testTelegram']);
        });
    });
});
