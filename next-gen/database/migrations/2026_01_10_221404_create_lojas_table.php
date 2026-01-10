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
        Schema::create('lojas', function (Blueprint $table) {
            $table->id();

            // Identification
            $table->string('codigo', 10)->unique();
            $table->string('nome', 200);
            $table->string('cnpj', 18)->unique();
            $table->string('inscricao_estadual', 20)->nullable();

            // Configuration
            $table->jsonb('config')->default('{}');

            // Status
            $table->boolean('is_ativo')->default(true);

            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('lojas');
    }
};
