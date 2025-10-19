<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\Telemetry;
use Carbon\Carbon;

class TelemetrySeeder extends Seeder
{
    public function run(): void
    {
        $count = 0;
        
        // Generate telemetry for last 24 hours
        $startTime = now()->subHours(24);
        
        // pH/EC telemetry (every 5 minutes)
        for ($i = 0; $i < 288; $i++) {
            $time = $startTime->copy()->addMinutes($i * 5);
            
            Telemetry::create([
                'node_id' => 'ph_ec_001',
                'node_type' => 'ph_ec',
                'data' => [
                    'ph' => round(6.0 + (sin($i / 20) * 0.5) + (rand(-10, 10) / 100), 2),
                    'ec' => round(1.5 + (cos($i / 15) * 0.3) + (rand(-10, 10) / 100), 2),
                    'temp' => round(22.0 + (sin($i / 30) * 3) + (rand(-5, 5) / 10), 1),
                ],
                'received_at' => $time,
                'created_at' => $time,
            ]);
            $count++;
        }

        // Climate telemetry (every 5 minutes)
        for ($i = 0; $i < 288; $i++) {
            $time = $startTime->copy()->addMinutes($i * 5);
            
            Telemetry::create([
                'node_id' => 'climate_001',
                'node_type' => 'climate',
                'data' => [
                    'temp' => round(23.0 + (sin($i / 25) * 4) + (rand(-5, 5) / 10), 1),
                    'humidity' => round(60.0 + (cos($i / 20) * 15) + rand(-2, 2), 1),
                    'co2' => round(600 + (sin($i / 18) * 200) + rand(-50, 50)),
                    'light' => rand(0, 1) > 0.5 ? rand(30000, 50000) : rand(0, 5000),
                ],
                'received_at' => $time,
                'created_at' => $time,
            ]);
            $count++;
        }

        $this->command->info('Created ' . $count . ' telemetry records (24 hours)');
    }
}

