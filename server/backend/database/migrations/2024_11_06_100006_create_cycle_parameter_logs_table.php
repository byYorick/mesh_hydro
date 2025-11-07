<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Логи параметров по дням цикла (для аналитики)
     */
    public function up(): void
    {
        Schema::create('cycle_parameter_logs', function (Blueprint $table) {
            $table->id();
            $table->foreignId('cycle_id')->constrained('growth_cycles')->onDelete('cascade');
            $table->date('log_date');
            $table->integer('day_number'); // День цикла (1, 2, 3...)
            
            // Средние значения за день
            $table->decimal('avg_ph', 5, 2)->nullable();
            $table->decimal('avg_ec', 6, 2)->nullable();
            $table->decimal('avg_temp', 5, 2)->nullable();
            $table->decimal('avg_humidity', 5, 2)->nullable();
            $table->integer('avg_co2')->nullable();
            $table->integer('avg_lux')->nullable();
            
            // Min/Max за день
            $table->decimal('min_ph', 5, 2)->nullable();
            $table->decimal('max_ph', 5, 2)->nullable();
            $table->decimal('min_temp', 5, 2)->nullable();
            $table->decimal('max_temp', 5, 2)->nullable();
            
            // Расход ресурсов
            $table->decimal('water_consumed_liters', 10, 3)->nullable();
            $table->decimal('nutrient_a_ml', 10, 2)->nullable();
            $table->decimal('nutrient_b_ml', 10, 2)->nullable();
            $table->decimal('ph_down_ml', 10, 2)->nullable();
            $table->decimal('ph_up_ml', 10, 2)->nullable();
            
            $table->timestamps();
            
            $table->unique(['cycle_id', 'log_date']);
            $table->index(['cycle_id', 'day_number']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('cycle_parameter_logs');
    }
};

