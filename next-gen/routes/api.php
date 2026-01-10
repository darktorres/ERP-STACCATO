<?php

use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;
use App\Http\Controllers\ClienteController;
use App\Http\Controllers\FornecedorController;
use App\Http\Controllers\ProdutoController;
use App\Http\Controllers\LojaController;

/*
|--------------------------------------------------------------------------
| API Routes
|--------------------------------------------------------------------------
|
| Here is where you can register API routes for your application. These
| routes are loaded by the RouteServiceProvider and all of them will
| be assigned to the "api" middleware group. Make something great!
|
*/

Route::middleware('auth:sanctum')->get('/user', function (Request $request) {
    return $request->user();
});

// Cadastros Module Routes
Route::middleware('auth:sanctum')->prefix('cadastros')->group(function () {
    // Lojas routes
    Route::apiResource('lojas', LojaController::class, ['parameters' => ['loja' => 'loja']]);
    Route::post('lojas/bulk-delete', [LojaController::class, 'bulkDelete']);

    // Clientes routes
    Route::apiResource('clientes', ClienteController::class, ['parameters' => ['cliente' => 'cliente']]);
    Route::post('clientes/bulk-delete', [ClienteController::class, 'bulkDelete']);
    Route::get('lojas/{loja}/clientes', [ClienteController::class, 'byLoja']);

    // Fornecedores routes - explicit to avoid pluralization issues
    Route::get('fornecedores', [FornecedorController::class, 'index'])->name('fornecedores.index');
    Route::post('fornecedores', [FornecedorController::class, 'store'])->name('fornecedores.store');
    Route::get('fornecedores/{fornecedor}', [FornecedorController::class, 'show'])->name('fornecedores.show');
    Route::put('fornecedores/{fornecedor}', [FornecedorController::class, 'update'])->name('fornecedores.update');
    Route::patch('fornecedores/{fornecedor}', [FornecedorController::class, 'update'])->name('fornecedores.update');
    Route::delete('fornecedores/{fornecedor}', [FornecedorController::class, 'destroy'])->name('fornecedores.destroy');
    Route::post('fornecedores/bulk-delete', [FornecedorController::class, 'bulkDelete']);

    // Produtos routes - explicit to avoid pluralization issues
    Route::get('produtos', [ProdutoController::class, 'index'])->name('produtos.index');
    Route::post('produtos', [ProdutoController::class, 'store'])->name('produtos.store');
    Route::get('produtos/{produto}', [ProdutoController::class, 'show'])->name('produtos.show');
    Route::put('produtos/{produto}', [ProdutoController::class, 'update'])->name('produtos.update');
    Route::patch('produtos/{produto}', [ProdutoController::class, 'update'])->name('produtos.update');
    Route::delete('produtos/{produto}', [ProdutoController::class, 'destroy'])->name('produtos.destroy');
    Route::post('produtos/bulk-delete', [ProdutoController::class, 'bulkDelete']);
    Route::get('fornecedores/{fornecedor}/produtos', [ProdutoController::class, 'byFornecedor']);
});
