<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Стадии роста в пресете
     */
    public function up(): void
    {
        Schema::create('growth_stages', function (Blueprint $table) {
            $table->id();
            $table->foreignId('preset_id')->constrained('growth_presets')->onDelete('cascade');
            
            $table->string('name'); // "Прорастание", "Вегетация", "Цветение"
            $table->integer('order')->default(0); // Порядок стадии (1, 2, 3...)
            $table->integer('duration_days'); // Длительность стадии в днях
            
            // ⭐ Целевые параметры для этой стадии
            $table->json('target_params'); // {ph_min, ph_max, ec_min, ec_max, temp, humidity, co2, light_intensity...}
            
            // Описание
            $table->text('description')->nullable();
            $table->text('care_instructions')->nullable(); // Инструкции по уходу
            
            // Автоматические действия при переходе
            $table->json('auto_actions')->nullable(); // [{action: "update_ph", params: {target: 6.0}}]
            
            $table->timestamps();
            
            $table->index(['preset_id', 'order']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('growth_stages');
    }
};

