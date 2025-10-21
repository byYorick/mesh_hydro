<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        Schema::create('pump_calibrations', function (Blueprint $table) {
            $table->id();
            $table->string('node_id', 32)->index();
            $table->integer('pump_id')->comment('0, 1, 2...');
            $table->float('ml_per_second')->default(1.0)->comment('Производительность насоса (мл/сек)');
            $table->float('calibration_volume_ml')->nullable()->comment('Объем при калибровке (мл)');
            $table->integer('calibration_time_ms')->nullable()->comment('Время калибровки (мс)');
            $table->boolean('is_calibrated')->default(false)->comment('Флаг калибровки');
            $table->timestamp('calibrated_at')->nullable()->comment('Дата калибровки');
            $table->timestamps();
            
            // Уникальный индекс: один pump_id на node_id
            $table->unique(['node_id', 'pump_id']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('pump_calibrations');
    }
};

