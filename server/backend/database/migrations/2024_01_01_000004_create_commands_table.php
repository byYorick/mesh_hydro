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
        Schema::create('commands', function (Blueprint $table) {
            $table->id();
            $table->string('node_id');
            $table->string('command');            // "calibrate", "open_windows", "pump_on"
            $table->json('params')->nullable();  // Параметры команды
            $table->enum('status', ['pending', 'sent', 'acknowledged', 'completed', 'failed'])
                ->default('pending');
            $table->timestamp('sent_at')->nullable();
            $table->timestamp('acknowledged_at')->nullable();
            $table->timestamp('completed_at')->nullable();
            $table->json('response')->nullable(); // Ответ от узла
            $table->text('error')->nullable();     // Ошибка если не выполнена
            $table->string('user_id')->nullable(); // Кто отправил команду
            $table->timestamps();

            // Индексы
            $table->index('node_id');
            $table->index('status');
            $table->index('created_at');
            $table->index(['node_id', 'status']); // Составной индекс
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('commands');
    }
};

