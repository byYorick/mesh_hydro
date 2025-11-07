<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Оценки пресетов пользователями
     */
    public function up(): void
    {
        Schema::create('preset_ratings', function (Blueprint $table) {
            $table->id();
            $table->foreignId('preset_id')->constrained('growth_presets')->onDelete('cascade');
            $table->foreignId('cycle_id')->constrained('growth_cycles')->onDelete('cascade');
            $table->unsignedBigInteger('user_id')->nullable();
            
            $table->integer('rating'); // 1-5 звезд
            $table->text('comment')->nullable();
            
            $table->timestamps();
            
            $table->unique(['preset_id', 'cycle_id']); // Один рейтинг на цикл
            $table->index(['preset_id', 'rating']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('preset_ratings');
    }
};

