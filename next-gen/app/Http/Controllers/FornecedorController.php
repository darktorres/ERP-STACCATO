<?php

namespace App\Http\Controllers;

use App\Models\Fornecedor;
use App\Http\Requests\StoreFornecedorRequest;
use App\Http\Requests\UpdateFornecedorRequest;
use App\Http\Resources\FornecedorResource;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

class FornecedorController extends Controller
{
    /**
     * Display a listing of suppliers.
     */
    public function index(Request $request)
    {
        $query = Fornecedor::query();

        // Filter by is_ativo if provided
        if ($request->has('is_ativo')) {
            $query->where('is_ativo', $request->boolean('is_ativo'));
        }

        // Search by name or cnpj
        if ($request->has('search')) {
            $search = $request->input('search');
            $query->where(function ($q) use ($search) {
                $q->where('razao_social', 'ilike', "%{$search}%")
                    ->orWhere('nome_fantasia', 'ilike', "%{$search}%")
                    ->orWhere('cnpj', 'ilike', "%{$search}%");
            });
        }

        // Pagination
        $perPage = $request->input('per_page', 15);
        $fornecedores = $query->paginate($perPage);

        return FornecedorResource::collection($fornecedores);
    }

    /**
     * Store a newly created supplier.
     */
    public function store(StoreFornecedorRequest $request)
    {
        $fornecedor = Fornecedor::create($request->validated());

        return (new FornecedorResource($fornecedor))
            ->response()
            ->setStatusCode(201);
    }

    /**
     * Display the specified supplier.
     */
    public function show(Fornecedor $fornecedor)
    {
        $fornecedor->load('produtos');

        return new FornecedorResource($fornecedor);
    }

    /**
     * Update the specified supplier.
     */
    public function update(UpdateFornecedorRequest $request, Fornecedor $fornecedor)
    {
        $fornecedor->update($request->validated());

        return new FornecedorResource($fornecedor);
    }

    /**
     * Delete the specified supplier.
     */
    public function destroy(Fornecedor $fornecedor): JsonResponse
    {
        $fornecedor->delete();

        return response()->json(null, 204);
    }

    /**
     * Bulk delete suppliers.
     */
    public function bulkDelete(Request $request)
    {
        $ids = $request->input('ids', []);

        if (empty($ids)) {
            return response()->json(['error' => 'No IDs provided'], 422);
        }

        $deleted = Fornecedor::whereIn('id', $ids)->delete();

        return response()->json([
            'message' => "{$deleted} supplier(s) deleted successfully",
            'deleted_count' => $deleted,
        ]);
    }
}
