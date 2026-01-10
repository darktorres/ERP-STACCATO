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
        Schema::create('produtos', function (Blueprint $table) {
            $table->id();

            // Supplier Link
            $table->foreignId('fornecedor_id')->constrained('fornecedores');

            // Identification
            $table->string('codigo_comercial', 100);
            $table->string('codigo_barras', 50)->nullable()->unique();
            $table->string('descricao', 500);
            $table->string('descricao_curta', 100)->nullable();

            // Units & Measurements
            $table->string('unidade', 10)->default('UN');
            $table->decimal('unidades_por_caixa', 10, 4)->default(1);
            $table->decimal('peso_kg', 10, 4)->nullable();

            // Classification & Attributes
            $table->string('ncm', 20)->nullable(); // NCM code
            $table->string('categoria', 100)->nullable();
            $table->boolean('tem_lote')->default(false);

            // Pricing (will be managed separately with product_prices table for multi-shop)
            $table->decimal('preco_custo', 15, 2)->default(0);
            $table->decimal('preco_tabela', 15, 2)->default(0);

            // Status
            $table->boolean('is_ativo')->default(true);

            $table->timestamps();

            // Indexes
            $table->index('codigo_comercial');
            $table->index('fornecedor_id');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('produtos');
    }
};
