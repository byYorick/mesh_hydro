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
        Schema::create('schedules', function (Blueprint $table) {
            $table->id();
            $table->string('node_id', 32)->index();
            $table->string('name', 100)->comment('Название режима: "День", "Ночь", и т.д.');
            $table->time('time_start')->comment('Время начала (08:00)');
            $table->time('time_end')->comment('Время окончания (20:00)');
            $table->jsonb('config')->comment('Конфигурация для этого периода');
            $table->boolean('enabled')->default(true);
            $table->integer('priority')->default(0)->comment('Приоритет (для пересекающихся расписаний)');
            $table->json('days_of_week')->nullable()->comment('Дни недели [1,2,3,4,5,6,7] или null = каждый день');
            $table->timestamps();
            
            // Индексы
            $table->index(['node_id', 'enabled']);
            $table->index('time_start');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('schedules');
    }
};

