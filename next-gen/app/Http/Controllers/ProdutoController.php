<?php

namespace App\Http\Controllers;

use App\Models\Produto;
use App\Http\Requests\StoreProdutoRequest;
use App\Http\Requests\UpdateProdutoRequest;
use App\Http\Resources\ProdutoResource;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

class ProdutoController extends Controller
{
    /**
     * Display a listing of products.
     */
    public function index(Request $request)
    {
        $query = Produto::query();

        // Filter by fornecedor_id if provided
        if ($request->has('fornecedor_id')) {
            $query->where('fornecedor_id', $request->input('fornecedor_id'));
        }

        // Filter by is_ativo if provided
        if ($request->has('is_ativo')) {
            $query->where('is_ativo', $request->boolean('is_ativo'));
        }

        // Filter by categoria if provided
        if ($request->has('categoria')) {
            $query->where('categoria', $request->input('categoria'));
        }

        // Search by description or codes
        if ($request->has('search')) {
            $search = $request->input('search');
            $query->where(function ($q) use ($search) {
                $q->where('descricao', 'ilike', "%{$search}%")
                    ->orWhere('codigo_comercial', 'ilike', "%{$search}%")
                    ->orWhere('codigo_barras', 'ilike', "%{$search}%");
            });
        }

        // Pagination
        $perPage = $request->input('per_page', 15);
        $produtos = $query->with('fornecedor')->paginate($perPage);

        return ProdutoResource::collection($produtos);
    }

    /**
     * Store a newly created product.
     */
    public function store(StoreProdutoRequest $request)
    {
        $produto = Produto::create($request->validated());
        $produto->load('fornecedor');

        return (new ProdutoResource($produto))
            ->response()
            ->setStatusCode(201);
    }

    /**
     * Display the specified product.
     */
    public function show(Produto $produto)
    {
        $produto->load('fornecedor');

        return new ProdutoResource($produto);
    }

    /**
     * Update the specified product.
     */
    public function update(UpdateProdutoRequest $request, Produto $produto)
    {
        $produto->update($request->validated());
        $produto->load('fornecedor');

        return new ProdutoResource($produto);
    }

    /**
     * Delete the specified product.
     */
    public function destroy(Produto $produto): JsonResponse
    {
        $produto->delete();

        return response()->json(null, 204);
    }

    /**
     * Bulk delete products.
     */
    public function bulkDelete(Request $request)
    {
        $ids = $request->input('ids', []);

        if (empty($ids)) {
            return response()->json(['error' => 'No IDs provided'], 422);
        }

        $deleted = Produto::whereIn('id', $ids)->delete();

        return response()->json([
            'message' => "{$deleted} product(s) deleted successfully",
            'deleted_count' => $deleted,
        ]);
    }

    /**
     * Get products by supplier.
     */
    public function byFornecedor(int $fornecedorId, Request $request)
    {
        $query = Produto::where('fornecedor_id', $fornecedorId);

        if ($request->has('is_ativo')) {
            $query->where('is_ativo', $request->boolean('is_ativo'));
        }

        $perPage = $request->input('per_page', 15);
        $produtos = $query->with('fornecedor')->paginate($perPage);

        return ProdutoResource::collection($produtos);
    }
}
