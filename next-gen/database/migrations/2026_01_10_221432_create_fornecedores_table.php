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
        Schema::create('fornecedores', function (Blueprint $table) {
            $table->id();

            // Names
            $table->string('razao_social', 200);
            $table->string('nome_fantasia', 200)->nullable();

            // Identification
            $table->string('cnpj', 18)->unique()->nullable();
            $table->string('inscricao_estadual', 20)->nullable();

            // Contact
            $table->string('email', 200)->nullable();
            $table->string('telefone', 20)->nullable();

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
        Schema::dropIfExists('fornecedores');
    }
};
