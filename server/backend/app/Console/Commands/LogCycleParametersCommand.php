<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\GrowthCycle;
use App\Models\CycleParameterLog;
use App\Models\Zone;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Log;
use Carbon\Carbon;

class LogCycleParametersCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'growth:log-parameters 
                            {--cycle-id= : ID конкретного цикла (опционально)}';

    /**
     * The console command description.
     */
    protected $description = 'Ежедневное логирование параметров для активных циклов роста';

    /**
     * Execute the console command.
     */
    public function handle(): int
    {
        $this->info('📊 Логирование параметров для активных циклов...');
        $this->newLine();

        $cycleId = $this->option('cycle-id');

        if ($cycleId) {
            $cycles = GrowthCycle::where('id', $cycleId)
                ->where('status', 'active')
                ->with(['zone', 'preset'])
                ->get();
        } else {
            $cycles = GrowthCycle::where('status', 'active')
                ->with(['zone', 'preset'])
                ->get();
        }

        if ($cycles->isEmpty()) {
            $this->info('✅ Нет активных циклов для логирования');
            return 0;
        }

        $this->info("Найдено {$cycles->count()} активных циклов");
        $this->newLine();

        $loggedCount = 0;

        foreach ($cycles as $cycle) {
            try {
                $this->line("Обработка цикла #{$cycle->id}: {$cycle->name}");

                $log = $this->logCycleParameters($cycle);
                
                if ($log) {
                    $loggedCount++;
                    $this->info("  ✅ Параметры залогированы");
                } else {
                    $this->warn("  ⚠️  Не удалось получить данные для логирования");
                }
            } catch (\Exception $e) {
                $this->error("  ❌ Ошибка: {$e->getMessage()}");
                Log::error("Error logging cycle parameters", [
                    'cycle_id' => $cycle->id,
                    'error' => $e->getMessage(),
                ]);
            }
        }

        $this->newLine();
        $this->info("✅ Логирование завершено. Обработано циклов: {$loggedCount}");

        return 0;
    }

    /**
     * Логирование параметров для цикла
     */
    protected function logCycleParameters(GrowthCycle $cycle): ?CycleParameterLog
    {
        $zone = $cycle->zone;
        if (!$zone) {
            return null;
        }

        // Получаем все узлы зоны
        $zoneNodes = $zone->getAllNodes();
        if ($zoneNodes->isEmpty()) {
            return null;
        }

        // Получаем телеметрию за последние 24 часа для узлов зоны
        $nodeIds = $zoneNodes->pluck('node_id')->toArray();
        
        $telemetry = DB::table('telemetry')
            ->whereIn('node_id', $nodeIds)
            ->where('received_at', '>=', now()->subDay())
            ->orderBy('received_at', 'desc')
            ->get();

        if ($telemetry->isEmpty()) {
            return null;
        }

        // Агрегируем параметры
        $parameters = $this->aggregateParameters($telemetry);

        // Вычисляем день цикла
        $dayNumber = $cycle->current_day ?? 1;

        // Проверяем, не залогирован ли уже сегодня
        $today = now()->startOfDay();
        $existingLog = CycleParameterLog::where('cycle_id', $cycle->id)
            ->where('log_date', $today)
            ->first();

        if ($existingLog) {
            // Обновляем существующий лог
            $existingLog->update($parameters);
            return $existingLog;
        }

        // Создаем новый лог
        return CycleParameterLog::create(array_merge($parameters, [
            'cycle_id' => $cycle->id,
            'log_date' => $today,
            'day_number' => $dayNumber,
        ]));
    }

    /**
     * Агрегация параметров из телеметрии
     */
    protected function aggregateParameters($telemetry): array
    {
        $phValues = [];
        $ecValues = [];
        $tempValues = [];
        $humidityValues = [];
        $co2Values = [];
        $luxValues = [];

        foreach ($telemetry as $t) {
            $data = is_string($t->data) ? json_decode($t->data, true) : $t->data;
            
            if (isset($data['ph'])) {
                $phValues[] = (float) $data['ph'];
            }
            if (isset($data['ec'])) {
                $ecValues[] = (float) $data['ec'];
            }
            if (isset($data['temp']) || isset($data['temperature'])) {
                $tempValues[] = (float) ($data['temp'] ?? $data['temperature'] ?? 0);
            }
            if (isset($data['humidity'])) {
                $humidityValues[] = (float) $data['humidity'];
            }
            if (isset($data['co2'])) {
                $co2Values[] = (int) $data['co2'];
            }
            if (isset($data['lux']) || isset($data['light'])) {
                $luxValues[] = (int) ($data['lux'] ?? $data['light'] ?? 0);
            }
        }

        return [
            'avg_ph' => !empty($phValues) ? round(array_sum($phValues) / count($phValues), 2) : null,
            'avg_ec' => !empty($ecValues) ? round(array_sum($ecValues) / count($ecValues), 2) : null,
            'avg_temp' => !empty($tempValues) ? round(array_sum($tempValues) / count($tempValues), 2) : null,
            'avg_humidity' => !empty($humidityValues) ? round(array_sum($humidityValues) / count($humidityValues), 2) : null,
            'avg_co2' => !empty($co2Values) ? (int) round(array_sum($co2Values) / count($co2Values)) : null,
            'avg_lux' => !empty($luxValues) ? (int) round(array_sum($luxValues) / count($luxValues)) : null,
            'min_ph' => !empty($phValues) ? round(min($phValues), 2) : null,
            'max_ph' => !empty($phValues) ? round(max($phValues), 2) : null,
            'min_temp' => !empty($tempValues) ? round(min($tempValues), 2) : null,
            'max_temp' => !empty($tempValues) ? round(max($tempValues), 2) : null,
            // TODO: Добавить логирование потребления воды и удобрений из узлов
            'water_consumed_liters' => null,
            'nutrient_a_ml' => null,
            'nutrient_b_ml' => null,
            'ph_down_ml' => null,
            'ph_up_ml' => null,
        ];
    }
}


