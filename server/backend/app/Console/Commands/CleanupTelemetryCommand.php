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
    protected $signature = 'telemetry:cleanup {--days=30 : Number of days to keep}';

    /**
     * The console command description.
     */
    protected $description = 'Delete telemetry data older than specified days';

    /**
     * Execute the console command.
     */
    public function handle(): int
    {
        $days = (int) $this->option('days');
        
        $this->info("Cleaning up telemetry older than {$days} days...");

        $cutoffDate = now()->subDays($days);

        // Count records to be deleted
        $count = Telemetry::where('received_at', '<', $cutoffDate)->count();

        if ($count === 0) {
            $this->info('No old telemetry records found.');
            return self::SUCCESS;
        }

        if (!$this->confirm("Found {$count} records. Do you want to delete them?", true)) {
            $this->info('Cleanup cancelled.');
            return self::SUCCESS;
        }

        $bar = $this->output->createProgressBar($count);
        $bar->start();

        // Delete in chunks to avoid memory issues
        Telemetry::where('received_at', '<', $cutoffDate)
            ->chunkById(1000, function ($telemetry) use ($bar) {
                DB::transaction(function () use ($telemetry) {
                    $telemetry->each->delete();
                });
                $bar->advance($telemetry->count());
            });

        $bar->finish();
        $this->newLine();

        $this->info("Successfully deleted {$count} telemetry records.");

        return self::SUCCESS;
    }
}
