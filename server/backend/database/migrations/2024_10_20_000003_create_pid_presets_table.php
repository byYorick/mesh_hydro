<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::create('pid_presets', function (Blueprint $table) {
            $table->id();
            $table->string('name');
            $table->text('description')->nullable();
            $table->jsonb('ph_config');
            $table->jsonb('ec_config');
            $table->boolean('is_default')->default(false);
            $table->integer('user_id')->nullable();
            $table->timestamps();

            $table->index('name');
            $table->index('is_default');
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('pid_presets');
    }
};

