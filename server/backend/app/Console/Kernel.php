<?php

namespace App\Console;

use Illuminate\Console\Scheduling\Schedule;
use Illuminate\Foundation\Console\Kernel as ConsoleKernel;

class Kernel extends ConsoleKernel
{
    /**
     * Define the application's command schedule.
     */
    protected function schedule(Schedule $schedule): void
    {
        // Проверка статуса узлов каждую минуту
        $schedule->command('nodes:check-status --notify')
            ->everyMinute()
            ->withoutOverlapping();

        // Очистка старых записей телеметрии раз в неделю
        $schedule->command('telemetry:cleanup --days=365')
            ->weekly()
            ->sundays()
            ->at('03:00');

        // Автоматическое резолвение старых событий (24 часа)
        $schedule->call(function () {
            \App\Models\Event::active()
                ->where('created_at', '<', now()->subHours(24))
                ->where('level', '!=', \App\Models\Event::LEVEL_EMERGENCY)
                ->update([
                    'resolved_at' => now(),
                    'resolved_by' => 'auto',
                ]);
        })
        ->hourly();

        // Обработка просроченных команд каждые 2 минуты
        $schedule->command('commands:process-timeouts')
            ->everyTwoMinutes()
            ->withoutOverlapping();
    }

    /**
     * Register the commands for the application.
     */
    protected function commands(): void
    {
        $this->load(__DIR__.'/Commands');

        require base_path('routes/console.php');
    }
}

