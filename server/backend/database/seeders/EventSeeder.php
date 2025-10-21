<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\Event;

class EventSeeder extends Seeder
{
    public function run(): void
    {
        $events = [
            [
                'node_id' => 'ph_001',
                'level' => 'warning',
                'message' => 'pH is below optimal range',
                'data' => ['ph' => 5.8, 'threshold' => 6.0],
                'created_at' => now()->subHours(5),
                'resolved_at' => now()->subHours(4),
                'resolved_by' => 'auto',
            ],
            [
                'node_id' => 'climate_001',
                'level' => 'info',
                'message' => 'Temperature stabilized',
                'data' => ['temp' => 24.5],
                'created_at' => now()->subHours(3),
                'resolved_at' => now()->subHours(3),
                'resolved_by' => 'auto',
            ],
            [
                'node_id' => 'water_001',
                'level' => 'critical',
                'message' => 'Node went offline',
                'data' => ['last_seen' => now()->subMinutes(10)->toDateTimeString()],
                'created_at' => now()->subMinutes(10),
                'resolved_at' => null,
            ],
            [
                'node_id' => 'climate_001',
                'level' => 'warning',
                'message' => 'CO2 level elevated',
                'data' => ['co2' => 1150, 'threshold' => 1200],
                'created_at' => now()->subHour(),
                'resolved_at' => null,
            ],
            [
                'node_id' => 'ec_001',
                'level' => 'warning',
                'message' => 'EC above optimal range',
                'data' => ['ec' => 2.6, 'threshold' => 2.5],
                'created_at' => now()->subHours(2),
                'resolved_at' => now()->subHour(),
                'resolved_by' => 'auto',
            ],
            [
                'node_id' => 'ph_001',
                'level' => 'emergency',
                'message' => 'pH critically low - immediate action required',
                'data' => ['ph' => 5.2, 'threshold' => 5.5],
                'created_at' => now()->subMinutes(30),
                'resolved_at' => null,
            ],
        ];

        foreach ($events as $eventData) {
            Event::create($eventData);
        }

        $this->command->info('Created ' . count($events) . ' test events');
    }
}

