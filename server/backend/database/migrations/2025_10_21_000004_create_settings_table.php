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
        Schema::create('settings', function (Blueprint $table) {
            $table->id();
            $table->string('key')->unique();
            $table->text('value')->nullable();
            $table->string('type')->default('string'); // string, integer, boolean, json
            $table->text('description')->nullable();
            $table->string('group')->default('general'); // general, telegram, docker, etc
            $table->boolean('is_sensitive')->default(false); // для паролей, токенов
            $table->timestamps();

            $table->index('key');
            $table->index('group');
        });

        // Начальные настройки Telegram
        DB::table('settings')->insert([
            [
                'key' => 'telegram.enabled',
                'value' => 'false',
                'type' => 'boolean',
                'description' => 'Включить Telegram уведомления',
                'group' => 'telegram',
                'is_sensitive' => false,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'key' => 'telegram.bot_token',
                'value' => '',
                'type' => 'string',
                'description' => 'Токен Telegram бота',
                'group' => 'telegram',
                'is_sensitive' => true,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'key' => 'telegram.chat_id',
                'value' => '',
                'type' => 'string',
                'description' => 'Chat ID для уведомлений',
                'group' => 'telegram',
                'is_sensitive' => false,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'key' => 'telegram.notify_critical',
                'value' => 'true',
                'type' => 'boolean',
                'description' => 'Уведомления о критических событиях',
                'group' => 'telegram',
                'is_sensitive' => false,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'key' => 'telegram.notify_warnings',
                'value' => 'true',
                'type' => 'boolean',
                'description' => 'Уведомления о предупреждениях',
                'group' => 'telegram',
                'is_sensitive' => false,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'key' => 'telegram.notify_info',
                'value' => 'false',
                'type' => 'boolean',
                'description' => 'Информационные уведомления',
                'group' => 'telegram',
                'is_sensitive' => false,
                'created_at' => now(),
                'updated_at' => now(),
            ],
        ]);
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('settings');
    }
};

