<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Снимки для сравнения циклов (аналитика)
     */
    public function up(): void
    {
        Schema::create('cycle_comparison_snapshots', function (Blueprint $table) {
            $table->id();
            $table->foreignId('cycle_id')->constrained('growth_cycles')->onDelete('cascade');
            
            // Итоговые метрики
            $table->integer('total_days');
            $table->decimal('total_harvest_kg', 10, 3)->nullable();
            $table->decimal('avg_ph', 5, 2)->nullable();
            $table->decimal('avg_ec', 6, 2)->nullable();
            $table->decimal('avg_temp', 5, 2)->nullable();
            $table->decimal('avg_humidity', 5, 2)->nullable();
            
            // Расход ресурсов
            $table->decimal('total_water_liters', 10, 2)->nullable();
            $table->decimal('total_nutrient_a_ml', 10, 2)->nullable();
            $table->decimal('total_nutrient_b_ml', 10, 2)->nullable();
            $table->decimal('total_ph_correction_ml', 10, 2)->nullable();
            
            // Эффективность
            $table->decimal('yield_per_plant', 8, 3)->nullable();
            $table->decimal('water_per_kg_harvest', 8, 2)->nullable();
            
            // Проблемы
            $table->integer('total_issues_count')->default(0);
            $table->integer('critical_issues_count')->default(0);
            
            $table->timestamps();
            
            $table->index(['cycle_id', 'created_at']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('cycle_comparison_snapshots');
    }
};

