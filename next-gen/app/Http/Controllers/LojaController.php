<?php

namespace App\Http\Controllers;

use App\Models\Loja;
use App\Http\Requests\StoreLojaRequest;
use App\Http\Requests\UpdateLojaRequest;
use App\Http\Resources\LojaResource;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

class LojaController extends Controller
{
    /**
     * Display a listing of stores.
     */
    public function index(Request $request)
    {
        $query = Loja::query();

        // Filter by is_ativo if provided
        if ($request->has('is_ativo')) {
            $query->where('is_ativo', $request->boolean('is_ativo'));
        }

        // Search by name or code
        if ($request->has('search')) {
            $search = $request->input('search');
            $query->where(function ($q) use ($search) {
                $q->where('nome', 'ilike', "%{$search}%")
                    ->orWhere('codigo', 'ilike', "%{$search}%")
                    ->orWhere('cnpj', 'ilike', "%{$search}%");
            });
        }

        // Pagination
        $perPage = $request->input('per_page', 15);
        $lojas = $query->paginate($perPage);

        return LojaResource::collection($lojas);
    }

    /**
     * Store a newly created store.
     */
    public function store(StoreLojaRequest $request)
    {
        $loja = Loja::create($request->validated());

        return (new LojaResource($loja))
            ->response()
            ->setStatusCode(201);
    }

    /**
     * Display the specified store.
     */
    public function show(Loja $loja)
    {
        $loja->load('clientes.loja', 'clientes.vendedor');

        return new LojaResource($loja);
    }

    /**
     * Update the specified store.
     */
    public function update(UpdateLojaRequest $request, Loja $loja)
    {
        $loja->update($request->validated());

        return new LojaResource($loja);
    }

    /**
     * Delete the specified store.
     */
    public function destroy(Loja $loja): JsonResponse
    {
        $loja->delete();

        return response()->json(null, 204);
    }

    /**
     * Bulk delete stores.
     */
    public function bulkDelete(Request $request)
    {
        $ids = $request->input('ids', []);

        if (empty($ids)) {
            return response()->json(['error' => 'No IDs provided'], 422);
        }

        $deleted = Loja::whereIn('id', $ids)->delete();

        return response()->json([
            'message' => "{$deleted} store(s) deleted successfully",
            'deleted_count' => $deleted,
        ]);
    }
}
