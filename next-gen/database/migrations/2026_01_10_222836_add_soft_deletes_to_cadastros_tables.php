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
        Schema::table('lojas', function (Blueprint $table) {
            $table->softDeletes();
        });

        Schema::table('clientes', function (Blueprint $table) {
            $table->softDeletes();
        });

        Schema::table('fornecedores', function (Blueprint $table) {
            $table->softDeletes();
        });

        Schema::table('produtos', function (Blueprint $table) {
            $table->softDeletes();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::table('lojas', function (Blueprint $table) {
            $table->dropSoftDeletes();
        });

        Schema::table('clientes', function (Blueprint $table) {
            $table->dropSoftDeletes();
        });

        Schema::table('fornecedores', function (Blueprint $table) {
            $table->dropSoftDeletes();
        });

        Schema::table('produtos', function (Blueprint $table) {
            $table->dropSoftDeletes();
        });
    }
};
