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
        Schema::create('clientes', function (Blueprint $table) {
            $table->id();

            // Type (Pessoa Física or Jurídica)
            $table->char('tipo', 2); // PF or PJ

            // Names
            $table->string('nome_razao', 200);
            $table->string('nome_fantasia', 200)->nullable();

            // Identification
            $table->string('cpf_cnpj', 18)->unique();
            $table->string('inscricao_estadual', 20)->nullable();

            // Contact
            $table->string('email', 200)->nullable();
            $table->string('telefone', 20)->nullable();

            // Credit
            $table->decimal('limite_credito', 15, 2)->default(0);
            $table->decimal('saldo_credito', 15, 2)->default(0);

            // Links
            $table->unsignedBigInteger('loja_id')->nullable(); // Will be added as foreign key in separate migration
            $table->foreignId('vendedor_id')->nullable()->constrained('users');

            // Status
            $table->boolean('is_incompleto')->default(false);
            $table->boolean('is_ativo')->default(true);

            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('clientes');
    }
};
