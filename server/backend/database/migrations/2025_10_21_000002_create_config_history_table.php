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
        Schema::create('config_history', function (Blueprint $table) {
            $table->id();
            $table->string('node_id', 32)->index();
            $table->string('user_id', 50)->nullable()->comment('Кто изменил (email или "system")');
            $table->jsonb('old_config')->nullable()->comment('Старая конфигурация');
            $table->jsonb('new_config')->comment('Новая конфигурация');
            $table->jsonb('changes')->nullable()->comment('Детали изменений (diff)');
            $table->string('change_type', 50)->comment('update_config, calibrate_pump, set_target, etc');
            $table->text('comment')->nullable()->comment('Комментарий пользователя');
            $table->timestamp('changed_at')->useCurrent();
            
            // Индексы
            $table->index('changed_at');
            $table->index(['node_id', 'changed_at']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('config_history');
    }
};

