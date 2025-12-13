<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Usuario;
use App\Models\Maintenance;
use Illuminate\Http\Request;
use Tymon\JwtAuth\JwtGuard;

class AuthController extends Controller
{
    /**
     * Login endpoint matching C++ WidgetAcesso logic
     */
    public function login(Request $request)
    {
        // Validate input
        $validated = $request->validate([
            'user' => 'required|string',
            'password' => 'required|string',
        ]);

        // Check maintenance mode
        $maintenance = Maintenance::find(1);
        if ($maintenance && $maintenance->emManutencao) {
            return response()->json([
                'success' => false,
                'error' => 'Sistema em manutenção!',
            ], 503);
        }

        // Query user with SHA_PASSWORD verification
        $usuario = Usuario::whereRaw('UPPER(user) = UPPER(?)', [$validated['user']])
            ->whereRaw('password = SHA_PASSWORD(?)', [$validated['password']])
            ->where('desativado', false)
            ->with('loja')
            ->first();

        if (!$usuario) {
            return response()->json([
                'success' => false,
                'error' => 'Login inválido!',
            ], 401);
        }

        // Block OPERACIONAL users
        if ($usuario->tipo === 'OPERACIONAL') {
            return response()->json([
                'success' => false,
                'error' => 'Operacional bloqueado!',
            ], 403);
        }

        // Generate JWT token using auth guard
        $guard = auth('api');
        $token = $guard->login($usuario);

        return response()->json([
            'success' => true,
            'token' => $token,
            'user' => [
                'idUsuario' => $usuario->idUsuario,
                'idLoja' => $usuario->idLoja,
                'user' => strtolower($usuario->user),
                'nome' => $usuario->nome,
                'tipo' => $usuario->tipo,
                'loja' => $usuario->loja ? [
                    'idLoja' => $usuario->loja->idLoja,
                    'descricao' => $usuario->loja->descricao,
                    'nomeFantasia' => $usuario->loja->nomeFantasia,
                ] : null,
            ],
        ]);
    }

    /**
     * Get authenticated user info
     */
    public function me(Request $request)
    {
        $usuario = auth('api')->user();

        if (!$usuario) {
            return response()->json(['error' => 'Unauthorized'], 401);
        }

        $usuario->load('loja');

        return response()->json([
            'idUsuario' => $usuario->idUsuario,
            'idLoja' => $usuario->idLoja,
            'user' => strtolower($usuario->user),
            'nome' => $usuario->nome,
            'tipo' => $usuario->tipo,
            'loja' => $usuario->loja ? [
                'idLoja' => $usuario->loja->idLoja,
                'descricao' => $usuario->loja->descricao,
                'nomeFantasia' => $usuario->loja->nomeFantasia,
            ] : null,
        ]);
    }

    /**
     * Logout endpoint
     */
    public function logout()
    {
        auth('api')->logout();
        return response()->json(['message' => 'Logged out successfully']);
    }
}
