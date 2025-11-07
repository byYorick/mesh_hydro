<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Культуры растений
     */
    public function up(): void
    {
        Schema::create('growth_cultures', function (Blueprint $table) {
            $table->id();
            $table->string('name')->unique(); // "Салат Романо", "Клубника Альбион"
            $table->string('slug')->unique(); // "salat-romano", "klubnika-albion"
            $table->string('category'); // "leafy_greens", "berries", "vegetables", "herbs"
            $table->text('description')->nullable();
            
            // Общие характеристики
            $table->integer('typical_cycle_days')->nullable(); // Типичная длительность цикла
            $table->decimal('optimal_temp_min', 5, 2)->nullable();
            $table->decimal('optimal_temp_max', 5, 2)->nullable();
            $table->decimal('optimal_humidity_min', 5, 2)->nullable();
            $table->decimal('optimal_humidity_max', 5, 2)->nullable();
            
            // Метаданные
            $table->string('image_url')->nullable();
            $table->boolean('is_system')->default(false); // Системная культура (нельзя удалить)
            $table->boolean('is_active')->default(true);
            
            $table->timestamps();
            
            $table->index('category');
            $table->index('is_active');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('growth_cultures');
    }
};

