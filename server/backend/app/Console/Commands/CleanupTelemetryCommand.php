<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\Telemetry;
use Illuminate\Support\Facades\DB;

class CleanupTelemetryCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'telemetry:cleanup
                            {--days=365 : Delete telemetry older than N days}
                            {--dry-run : Show what would be deleted without actually deleting}';

    /**
     * The console command description.
     */
    protected $description = 'Clean up old telemetry data';

    /**
     * Execute the console command.
     */
    public function handle(): int
    {
        $days = (int) $this->option('days');
        $dryRun = $this->option('dry-run');
        
        $this->info("Cleaning up telemetry older than {$days} days...");
        
        $cutoffDate = now()->subDays($days);
        
        // Подсчет записей для удаления
        $count = Telemetry::where('received_at', '<', $cutoffDate)->count();
        
        if ($count === 0) {
            $this->info('No old telemetry data found.');
            return 0;
        }

        $this->warn("Found {$count} records to delete (before {$cutoffDate->toDateTimeString()})");

        if ($dryRun) {
            $this->info('DRY RUN: No data was deleted.');
            return 0;
        }

        if (!$this->confirm('Do you want to continue?', true)) {
            $this->info('Cleanup cancelled.');
            return 0;
        }

        // Удаление старых записей пакетами (чтобы не заблокировать БД)
        $deleted = 0;
        $batchSize = 1000;

        $this->output->progressStart($count);

        while (true) {
            $batch = Telemetry::where('received_at', '<', $cutoffDate)
                ->limit($batchSize)
                ->delete();
            
            if ($batch === 0) {
                break;
            }

            $deleted += $batch;
            $this->output->progressAdvance($batch);
            
            // Небольшая пауза между пакетами
            usleep(100000); // 0.1 секунды
        }

        $this->output->progressFinish();
        
        $this->info("✅ Deleted {$deleted} telemetry records.");

        // Оптимизация таблицы (для PostgreSQL)
        $this->info('Optimizing table...');
        DB::statement('VACUUM ANALYZE telemetry');
        $this->info('✅ Table optimized.');

        return 0;
    }
}

