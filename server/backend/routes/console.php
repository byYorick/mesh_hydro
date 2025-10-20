<?php

use Illuminate\Foundation\Inspiring;
use Illuminate\Support\Facades\Artisan;
use Illuminate\Support\Facades\Schedule;

/*
|--------------------------------------------------------------------------
| Console Routes
|--------------------------------------------------------------------------
|
| Консольные маршруты и планировщик задач (Scheduler)
| Laravel 11 использует этот файл вместо app/Console/Kernel.php
|
*/

// ========================================
// Scheduled Tasks (Планировщик задач)
// ========================================

// Проверка статуса узлов каждые 10 секунд
Schedule::command('nodes:check-status --notify')
    ->everyTenSeconds()
    ->withoutOverlapping();

// Очистка старых записей телеметрии раз в неделю
Schedule::command('telemetry:cleanup --days=365')
    ->weekly()
    ->sundays()
    ->at('03:00');

// Автоматическое резолвение старых событий (24 часа)
Schedule::call(function () {
    \App\Models\Event::active()
        ->where('created_at', '<', now()->subHours(24))
        ->where('level', '!=', \App\Models\Event::LEVEL_EMERGENCY)
        ->update([
            'resolved_at' => now(),
            'resolved_by' => 'auto',
        ]);
})
->hourly()
->name('auto-resolve-events')
->description('Auto-resolve non-critical events older than 24 hours');

// Обработка просроченных команд каждые 2 минуты
Schedule::command('commands:process-timeouts')
    ->everyTwoMinutes()
    ->withoutOverlapping();

// ========================================
// Custom Artisan Commands
// ========================================

Artisan::command('inspire', function () {
    $this->comment(Inspiring::quote());
})->purpose('Display an inspiring quote');
