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
        Schema::table('commands', function (Blueprint $table) {
            $table->timestamp('timeout_at')->nullable()->after('completed_at');
            $table->integer('timeout_seconds')->default(300)->after('timeout_at');
            
            // Индекс для быстрого поиска просроченных команд
            $table->index(['timeout_at', 'status'], 'idx_commands_timeout_status');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::table('commands', function (Blueprint $table) {
            $table->dropIndex('idx_commands_timeout_status');
            $table->dropColumn(['timeout_at', 'timeout_seconds']);
        });
    }
};
