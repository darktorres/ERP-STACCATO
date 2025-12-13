<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\OrcamentoView;
use App\Models\Loja;
use App\Models\Usuario;
use Illuminate\Http\Request;

class OrcamentoController extends Controller
{
    /**
     * List orcamentos with filtering - matches WidgetOrcamento::montaFiltro()
     */
    public function list(Request $request)
    {
        $user = auth('api')->user();

        if (!$user) {
            return response()->json(['error' => 'Unauthorized'], 401);
        }

        $filters = $request->all();

        // Start query
        $query = OrcamentoView::query();

        // Role-based filtering
        if (in_array($user->tipo, ['GERENTE LOJA', 'GERENTE DEPARTAMENTO'])) {
            $query->where('idLoja', $user->idLoja);
        } elseif (isset($filters['idLoja'])) {
            $query->where('idLoja', $filters['idLoja']);
        }

        // Month filter
        if (!empty($filters['mesAno'])) {
            $query->where('data2', $filters['mesAno']);
        }

        // Vendor filter
        if (!empty($filters['idVendedor'])) {
            $query->where(function ($q) use ($filters) {
                $q->where('idUsuario', $filters['idVendedor'])
                  ->orWhere('idUsuarioConsultor', $filters['idVendedor']);
            });
        }

        // Supplier filter
        if (!empty($filters['fornecedor'])) {
            $query->whereRaw('FIND_IN_SET(?, fornecedores)', [$filters['fornecedor']]);
        }

        // Status filter
        if (!empty($filters['statuses']) && is_array($filters['statuses'])) {
            $query->whereIn('status', $filters['statuses']);
        }

        // Semaforo filter
        if (isset($filters['semaforo'])) {
            $query->where('semaforo', $filters['semaforo']);
        }

        // "Próprios" filter (only for vendedores)
        if (in_array($user->tipo, ['VENDEDOR', 'VENDEDOR ESPECIAL']) &&
            !empty($filters['apenasPropriosOrcamentos'])) {
            $query->where(function ($q) use ($user) {
                $q->where('vendedor', $user->nome)
                  ->orWhere('consultor', $user->nome);
            });
        }

        // Search filter
        if (!empty($filters['search'])) {
            $search = $filters['search'];
            $query->where(function ($q) use ($search) {
                $q->where('idOrcamento', 'like', "%$search%")
                  ->orWhere('vendedor', 'like', "%$search%")
                  ->orWhere('cliente', 'like', "%$search%")
                  ->orWhere('profissional', 'like', "%$search%");
            });
        }

        // Order by
        $query->orderByDesc('data');

        // Get results
        $orcamentos = $query->get();

        // Map results to match frontend expectations
        $result = $orcamentos->map(function ($o) {
            return [
                'idOrcamento' => $o->idOrcamento,
                'idLoja' => $o->idLoja,
                'idUsuario' => $o->idUsuario,
                'idUsuarioConsultor' => $o->idUsuarioConsultor,
                'status' => $o->status,
                'diasRestantes' => $o->diasRestantes ? (string)$o->diasRestantes : '-',
                'vendedor' => $o->vendedor,
                'consultor' => $o->consultor,
                'cliente' => $o->cliente,
                'profissional' => $o->profissional,
                'tel' => $o->tel,
                'telCel' => $o->telCel,
                'telProf' => $o->telProf,
                'data' => $o->data,
                'data2' => $o->data2,
                'total' => $o->total,
                'dataFollowup' => $o->dataFollowup,
                'dataProxFollowup' => $o->dataProxFollowup,
                'observacao' => $o->observacao,
                'semaforo' => $o->semaforo,
                'fornecedores' => $o->fornecedores,
            ];
        });

        return response()->json([
            'success' => true,
            'data' => $result,
        ]);
    }

    /**
     * Get lojas for filter dropdown
     */
    public function lojas()
    {
        $user = auth('api')->user();

        if (!$user) {
            return response()->json(['error' => 'Unauthorized'], 401);
        }

        $lojas = Loja::where('desativado', false)
            ->orderBy('idLoja')
            ->get()
            ->map(function ($loja) {
                return [
                    'idLoja' => $loja->idLoja,
                    'descricao' => $loja->descricao,
                    'nomeFantasia' => $loja->nomeFantasia,
                ];
            });

        return response()->json([
            'success' => true,
            'data' => $lojas,
        ]);
    }

    /**
     * Get vendedores for filter dropdown
     */
    public function vendedores(Request $request)
    {
        $user = auth('api')->user();

        if (!$user) {
            return response()->json(['error' => 'Unauthorized'], 401);
        }

        $query = Usuario::whereIn('tipo', ['VENDEDOR', 'VENDEDOR ESPECIAL'])
            ->where('desativado', false);

        if ($request->has('idLoja')) {
            $query->where('idLoja', $request->idLoja);
        }

        $vendedores = $query->orderBy('nome')
            ->get()
            ->map(function ($u) {
                return [
                    'idUsuario' => $u->idUsuario,
                    'nome' => $u->nome,
                ];
            });

        return response()->json([
            'success' => true,
            'data' => $vendedores,
        ]);
    }

    /**
     * Get fornecedores for filter dropdown
     */
    public function fornecedores()
    {
        $user = auth('api')->user();

        if (!$user) {
            return response()->json(['error' => 'Unauthorized'], 401);
        }

        $fornecedoresSet = OrcamentoView::whereNotNull('fornecedores')
            ->where('fornecedores', '!=', '')
            ->pluck('fornecedores')
            ->flatMap(function ($fornecedores) {
                return array_map('trim', explode(',', $fornecedores));
            })
            ->unique()
            ->sort()
            ->values();

        $fornecedores = $fornecedoresSet->map(function ($razaoSocial) {
            return ['razaoSocial' => $razaoSocial];
        });

        return response()->json([
            'success' => true,
            'data' => $fornecedores,
        ]);
    }
}
