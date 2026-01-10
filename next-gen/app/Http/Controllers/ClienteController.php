<?php

namespace App\Http\Controllers;

use App\Models\Cliente;
use App\Http\Requests\StoreClienteRequest;
use App\Http\Requests\UpdateClienteRequest;
use App\Http\Resources\ClienteResource;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

class ClienteController extends Controller
{
    /**
     * Display a listing of customers.
     */
    public function index(Request $request)
    {
        $query = Cliente::query();

        // Filter by loja_id if provided
        if ($request->has('loja_id')) {
            $query->where('loja_id', $request->input('loja_id'));
        }

        // Filter by tipo if provided
        if ($request->has('tipo')) {
            $query->where('tipo', $request->input('tipo'));
        }

        // Filter by is_ativo if provided
        if ($request->has('is_ativo')) {
            $query->where('is_ativo', $request->boolean('is_ativo'));
        }

        // Search by name or cpf_cnpj
        if ($request->has('search')) {
            $search = $request->input('search');
            $query->where(function ($q) use ($search) {
                $q->where('nome_razao', 'ilike', "%{$search}%")
                    ->orWhere('cpf_cnpj', 'ilike', "%{$search}%");
            });
        }

        // Pagination
        $perPage = $request->input('per_page', 15);
        $clientes = $query->with(['loja', 'vendedor'])->paginate($perPage);

        return ClienteResource::collection($clientes);
    }

    /**
     * Store a newly created customer.
     */
    public function store(StoreClienteRequest $request)
    {
        $cliente = Cliente::create($request->validated());
        $cliente->load(['loja', 'vendedor']);

        return (new ClienteResource($cliente))
            ->response()
            ->setStatusCode(201);
    }

    /**
     * Display the specified customer.
     */
    public function show(Cliente $cliente)
    {
        $cliente->load(['loja', 'vendedor']);

        return new ClienteResource($cliente);
    }

    /**
     * Update the specified customer.
     */
    public function update(UpdateClienteRequest $request, Cliente $cliente)
    {
        $cliente->update($request->validated());
        $cliente->load(['loja', 'vendedor']);

        return new ClienteResource($cliente);
    }

    /**
     * Delete the specified customer.
     */
    public function destroy(Cliente $cliente): JsonResponse
    {
        $cliente->delete();

        return response()->json(null, 204);
    }

    /**
     * Bulk delete customers.
     */
    public function bulkDelete(Request $request)
    {
        $ids = $request->input('ids', []);

        if (empty($ids)) {
            return response()->json(['error' => 'No IDs provided'], 422);
        }

        $deleted = Cliente::whereIn('id', $ids)->delete();

        return response()->json([
            'message' => "{$deleted} customer(s) deleted successfully",
            'deleted_count' => $deleted,
        ]);
    }

    /**
     * Get customers by loja.
     */
    public function byLoja(int $lojaId, Request $request)
    {
        $query = Cliente::where('loja_id', $lojaId);

        if ($request->has('is_ativo')) {
            $query->where('is_ativo', $request->boolean('is_ativo'));
        }

        $perPage = $request->input('per_page', 15);
        $clientes = $query->with('vendedor')->paginate($perPage);

        return response()->json($clientes);
    }
}
