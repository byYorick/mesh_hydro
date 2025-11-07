<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Пресеты (шаблоны) циклов роста
     */
    public function up(): void
    {
        Schema::create('growth_presets', function (Blueprint $table) {
            $table->id();
            $table->foreignId('culture_id')->constrained('growth_cultures')->onDelete('cascade');
            
            $table->string('name'); // "Салат NFT быстрый", "Клубника стандарт"
            $table->string('slug')->unique();
            $table->text('description')->nullable();
            
            // Тип пресета
            $table->enum('preset_type', ['system', 'custom'])->default('custom');
            $table->unsignedBigInteger('created_by_user_id')->nullable(); // TODO: FK когда будет таблица users
            
            // Характеристики
            $table->integer('total_days'); // Общая длительность цикла
            $table->enum('difficulty', ['easy', 'medium', 'hard'])->default('medium');
            $table->enum('recommended_system', ['nft', 'dwc', 'drip', 'ebb_flow', 'any'])->default('any');
            
            // Статистика использования
            $table->integer('usage_count')->default(0);
            $table->decimal('avg_rating', 3, 2)->nullable();
            
            // Статус
            $table->boolean('is_public')->default(false);
            $table->boolean('is_active')->default(true);
            
            $table->timestamps();
            
            $table->index(['culture_id', 'preset_type']);
            $table->index('is_active');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('growth_presets');
    }
};

