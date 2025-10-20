<?php

namespace App\Console\Commands;

use App\Models\Command as CommandModel;
use Illuminate\Console\Command;
use Illuminate\Support\Facades\Log;

class ProcessTimedOutCommandsCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'commands:process-timeouts';

    /**
     * The console command description.
     */
    protected $description = 'Process timed out commands and mark them as failed';

    /**
     * Execute the console command.
     */
    public function handle()
    {
        $timedOutCommands = CommandModel::timedOut()->get();
        
        if ($timedOutCommands->isEmpty()) {
            $this->info('No timed out commands found');
            return 0;
        }

        $this->info("Found {$timedOutCommands->count()} timed out commands");

        foreach ($timedOutCommands as $command) {
            $command->markAsTimedOut();
            
            Log::warning("Command timed out", [
                'command_id' => $command->id,
                'node_id' => $command->node_id,
                'command' => $command->command,
                'timeout_seconds' => $command->timeout_seconds,
                'sent_at' => $command->sent_at,
            ]);

            $this->line("Marked command {$command->id} ({$command->command}) as timed out");
        }

        $this->info("Processed {$timedOutCommands->count()} timed out commands");
        return 0;
    }
}
