# Arquitetura de Segurança

> Status: **Rascunho**
> Última atualização: 2025-12-28
> Prioridade: Alta

---

## Visão Geral

Este documento define a arquitetura de segurança para o novo sistema Laravel, abordando autenticação, autorização, proteções contra vulnerabilidades e conformidade com LGPD.

### Vulnerabilidades do Sistema Atual

O sistema C++ atual apresenta vulnerabilidades críticas que **devem ser eliminadas** na migração:

| Vulnerabilidade | Localização | Severidade | Correção Laravel |
|-----------------|-------------|------------|------------------|
| **SQL Injection** | `user.cpp:33` - concatenação de `permissao` | Crítica | Eloquent ORM |
| **SQL Injection** | `user.cpp:76` - concatenação de `user` | Crítica | Query Builder |
| **Senha em texto** | `logindialog.cpp:111` - salva senha em settings | Alta | Nunca armazenar |
| **Hash fraco** | `SHA_PASSWORD()` no MySQL | Média | bcrypt/Argon2 |
| **Sem rate limit** | Login sem proteção | Média | Laravel Throttle |

---

## Autenticação

### Decisão: Laravel Sanctum

| Opção | Uso Recomendado | Complexidade |
|-------|-----------------|--------------|
| **Sanctum** | SPA + API simples | Baixa |
| Passport | OAuth2 completo, apps terceiros | Alta |
| Fortify | UI pronta, 2FA | Média |

**Decisão**: Usar **Laravel Sanctum** com **Fortify** para UI.

**Justificativa**:
1. Sistema é SPA interno (Inertia + Vue), não precisa OAuth2
2. Sanctum é mais simples e performático
3. Fortify adiciona 2FA e recuperação de senha

### Implementação

```php
// config/auth.php
'guards' => [
    'web' => [
        'driver' => 'session',
        'provider' => 'usuarios',
    ],
    'sanctum' => [
        'driver' => 'sanctum',
        'provider' => 'usuarios',
    ],
],

'providers' => [
    'usuarios' => [
        'driver' => 'eloquent',
        'model' => App\Models\Usuario::class,
    ],
],
```

```php
// app/Models/Usuario.php
class Usuario extends Authenticatable
{
    use HasApiTokens, Notifiable;

    protected $table = 'usuarios';

    protected $hidden = [
        'password',
        'remember_token',
        'senha_uso_unico',
    ];

    protected $casts = [
        'email_verified_at' => 'datetime',
        'password' => 'hashed', // Laravel 10+ auto-hash
        'tipo' => TipoUsuario::class,
        'ativo' => 'boolean',
    ];

    public function loja(): BelongsTo
    {
        return $this->belongsTo(Loja::class);
    }

    public function permissoes(): HasOne
    {
        return $this->hasOne(UsuarioPermissao::class);
    }
}
```

### Migração de Senhas

O sistema atual usa `SHA_PASSWORD()` do MySQL. Estratégia de migração:

```php
// app/Services/Auth/LegacyPasswordService.php
class LegacyPasswordService
{
    /**
     * Verifica senha legada e migra para bcrypt
     */
    public function verificarEMigrar(Usuario $usuario, string $senha): bool
    {
        // Tenta bcrypt primeiro (já migrado)
        if (Hash::check($senha, $usuario->password)) {
            return true;
        }

        // Tenta SHA legado
        if ($this->verificarShaLegado($usuario, $senha)) {
            // Migra para bcrypt
            $usuario->update([
                'password' => Hash::make($senha),
            ]);
            return true;
        }

        return false;
    }

    private function verificarShaLegado(Usuario $usuario, string $senha): bool
    {
        // SHA_PASSWORD() do MySQL = SHA1(SHA1(senha))
        $hash = sha1(sha1($senha, true));
        return $usuario->getRawOriginal('password') === '*' . strtoupper($hash);
    }
}
```

### Autenticação de Dois Fatores (2FA)

```php
// Usar Laravel Fortify com TOTP
// config/fortify.php
'features' => [
    Features::registration(),
    Features::resetPasswords(),
    Features::emailVerification(),
    Features::updateProfileInformation(),
    Features::updatePasswords(),
    Features::twoFactorAuthentication([
        'confirm' => true,
        'confirmPassword' => true,
    ]),
],
```

### Rate Limiting

```php
// app/Providers/RouteServiceProvider.php
RateLimiter::for('login', function (Request $request) {
    $key = Str::transliterate(
        Str::lower($request->input('email')) . '|' . $request->ip()
    );

    return Limit::perMinute(5)->by($key);
});

// routes/auth.php
Route::post('/login', [LoginController::class, 'store'])
    ->middleware('throttle:login');
```

---

## Autorização

### Sistema Atual

O sistema atual usa dois mecanismos:

1. **RBAC (Role-Based)**: Coluna `tipo` com valores fixos
2. **PBAC (Permission-Based)**: Tabela `usuario_has_permissao` com flags booleanas

```sql
-- Tipos de usuário (RBAC)
tipo IN (
    'ADMINISTRADOR',
    'DIRETOR',
    'ADMINISTRATIVO',
    'GERENTE LOJA',
    'GERENTE DEPARTAMENTO',
    'GERENTE FINANCEIRO',
    'VENDEDOR',
    'VENDEDOR ESPECIAL',
    'OPERACIONAL',
    'ASSISTENTE ADMINISTRATIVO'
)

-- Permissões granulares (PBAC)
usuario_has_permissao (
    view_tab_orcamento,
    view_tab_venda,
    view_tab_compra,
    view_tab_logistica,
    view_tab_nfe,
    view_tab_estoque,
    view_tab_galpao,
    view_tab_financeiro,
    view_tab_relatorio,
    ...
)
```

### Decisão: Spatie Permission + Policies

Usar **spatie/laravel-permission** para roles/permissions + **Policies** do Laravel para lógica complexa.

```bash
composer require spatie/laravel-permission
```

### Implementação

#### Enums de Tipo

```php
// app/Enums/TipoUsuario.php
enum TipoUsuario: string
{
    case ADMINISTRADOR = 'ADMINISTRADOR';
    case DIRETOR = 'DIRETOR';
    case ADMINISTRATIVO = 'ADMINISTRATIVO';
    case GERENTE_LOJA = 'GERENTE_LOJA';
    case GERENTE_DEPARTAMENTO = 'GERENTE_DEPARTAMENTO';
    case GERENTE_FINANCEIRO = 'GERENTE_FINANCEIRO';
    case VENDEDOR = 'VENDEDOR';
    case VENDEDOR_ESPECIAL = 'VENDEDOR_ESPECIAL';
    case OPERACIONAL = 'OPERACIONAL';
    case ASSISTENTE_ADMINISTRATIVO = 'ASSISTENTE_ADMINISTRATIVO';

    public function isAdmin(): bool
    {
        return in_array($this, [self::ADMINISTRADOR, self::DIRETOR]);
    }

    public function isGerente(): bool
    {
        return in_array($this, [
            self::GERENTE_LOJA,
            self::GERENTE_DEPARTAMENTO,
            self::GERENTE_FINANCEIRO,
        ]);
    }
}
```

#### Modelo com Roles

```php
// app/Models/Usuario.php
use Spatie\Permission\Traits\HasRoles;

class Usuario extends Authenticatable
{
    use HasApiTokens, HasRoles, Notifiable;

    // Guard name para Spatie
    protected $guard_name = 'web';

    /**
     * Verifica se é admin (compatibilidade com legado)
     */
    public function isAdmin(): bool
    {
        return $this->tipo->isAdmin() || $this->hasRole('admin');
    }

    /**
     * Verifica permissão (compatibilidade com legado)
     */
    public function temPermissao(string $permissao): bool
    {
        // Admins têm todas as permissões
        if ($this->isAdmin()) {
            return true;
        }

        // Verifica via Spatie
        return $this->hasPermissionTo($permissao);
    }
}
```

#### Seeder de Permissões

```php
// database/seeders/PermissaoSeeder.php
class PermissaoSeeder extends Seeder
{
    public function run(): void
    {
        // Reset cache
        app()[\Spatie\Permission\PermissionRegistrar::class]->forgetCachedPermissions();

        // Permissões de módulo (migradas do legado)
        $modulos = [
            'orcamento', 'venda', 'compra', 'logistica',
            'nfe', 'estoque', 'galpao', 'financeiro',
            'relatorio', 'grafico', 'rh',
        ];

        foreach ($modulos as $modulo) {
            Permission::create(['name' => "view_{$modulo}"]);
            Permission::create(['name' => "create_{$modulo}"]);
            Permission::create(['name' => "edit_{$modulo}"]);
            Permission::create(['name' => "delete_{$modulo}"]);
        }

        // Permissões especiais
        Permission::create(['name' => 'ajuste_frete']);
        Permission::create(['name' => 'autorizar_desconto']);
        Permission::create(['name' => 'cancelar_nfe']);
        Permission::create(['name' => 'emitir_nfe']);

        // Roles
        $admin = Role::create(['name' => 'admin']);
        $gerente = Role::create(['name' => 'gerente']);
        $vendedor = Role::create(['name' => 'vendedor']);
        $operacional = Role::create(['name' => 'operacional']);

        // Atribuir permissões
        $admin->givePermissionTo(Permission::all());

        $gerente->givePermissionTo([
            'view_orcamento', 'create_orcamento', 'edit_orcamento',
            'view_venda', 'create_venda', 'edit_venda',
            'view_compra', 'view_estoque', 'view_financeiro',
            'view_relatorio', 'ajuste_frete',
        ]);

        $vendedor->givePermissionTo([
            'view_orcamento', 'create_orcamento', 'edit_orcamento',
            'view_venda', 'create_venda',
        ]);

        $operacional->givePermissionTo([
            'view_logistica', 'edit_logistica',
            'view_estoque', 'view_galpao',
        ]);
    }
}
```

#### Policies

```php
// app/Policies/VendaPolicy.php
class VendaPolicy
{
    /**
     * Determine whether the user can view any models.
     */
    public function viewAny(Usuario $user): bool
    {
        return $user->temPermissao('view_venda');
    }

    /**
     * Determine whether the user can view the model.
     */
    public function view(Usuario $user, Venda $venda): bool
    {
        if (!$user->temPermissao('view_venda')) {
            return false;
        }

        // Vendedor só vê suas próprias vendas
        if ($user->tipo === TipoUsuario::VENDEDOR) {
            return $venda->vendedor_id === $user->id;
        }

        // Gerente vê vendas da sua loja
        if ($user->tipo->isGerente()) {
            return $venda->loja_id === $user->loja_id;
        }

        // Admin vê tudo
        return true;
    }

    /**
     * Determine whether the user can create models.
     */
    public function create(Usuario $user): bool
    {
        return $user->temPermissao('create_venda');
    }

    /**
     * Determine whether the user can update the model.
     */
    public function update(Usuario $user, Venda $venda): bool
    {
        if (!$user->temPermissao('edit_venda')) {
            return false;
        }

        // Não pode editar venda finalizada
        if ($venda->status->isFinalizado()) {
            return $user->isAdmin();
        }

        return $this->view($user, $venda);
    }

    /**
     * Determine whether the user can delete the model.
     */
    public function delete(Usuario $user, Venda $venda): bool
    {
        // Apenas admin pode excluir
        return $user->isAdmin() && !$venda->status->isFinalizado();
    }

    /**
     * Autorização especial para desconto acima do limite
     */
    public function autorizarDesconto(Usuario $user): bool
    {
        return $user->temPermissao('autorizar_desconto');
    }
}
```

#### Gates

```php
// app/Providers/AuthServiceProvider.php
public function boot(): void
{
    // Gate para autorização de desconto (senha de uso único)
    Gate::define('autorizar-desconto-extra', function (Usuario $user, float $percentual) {
        // Desconto até 10% qualquer gerente pode dar
        if ($percentual <= 10 && $user->tipo->isGerente()) {
            return true;
        }

        // Desconto acima de 10% precisa de admin
        return $user->isAdmin();
    });

    // Gate para acessar dados de outra loja
    Gate::define('acessar-outra-loja', function (Usuario $user) {
        return $user->isAdmin();
    });

    // Gate para operações sensíveis
    Gate::define('operacao-sensivel', function (Usuario $user) {
        return $user->isAdmin() || $user->tipo === TipoUsuario::DIRETOR;
    });
}
```

### Middleware de Autorização

```php
// app/Http/Middleware/CheckPermission.php
class CheckPermission
{
    public function handle(Request $request, Closure $next, string $permission): Response
    {
        if (!$request->user()?->temPermissao($permission)) {
            if ($request->expectsJson()) {
                return response()->json(['error' => 'Não autorizado'], 403);
            }

            abort(403, 'Você não tem permissão para acessar este recurso.');
        }

        return $next($request);
    }
}

// Uso nas rotas
Route::middleware(['auth', 'permission:view_financeiro'])
    ->prefix('financeiro')
    ->group(function () {
        // ...
    });
```

---

## Validação de Entrada

### Form Requests

```php
// app/Http/Requests/CriarClienteRequest.php
class CriarClienteRequest extends FormRequest
{
    public function authorize(): bool
    {
        return $this->user()->temPermissao('create_cadastro');
    }

    public function rules(): array
    {
        return [
            'tipo_pessoa' => ['required', Rule::enum(TipoPessoa::class)],
            'razao_social' => ['required', 'string', 'max:255'],
            'nome_fantasia' => ['nullable', 'string', 'max:255'],

            // CPF ou CNPJ conforme tipo
            'cpf' => [
                'required_if:tipo_pessoa,PF',
                'nullable',
                'string',
                'size:11',
                new CpfValido(),
                Rule::unique('clientes', 'cpf')->ignore($this->cliente),
            ],
            'cnpj' => [
                'required_if:tipo_pessoa,PJ',
                'nullable',
                'string',
                'size:14',
                new CnpjValido(),
                Rule::unique('clientes', 'cnpj')->ignore($this->cliente),
            ],

            'email' => ['nullable', 'email:rfc,dns', 'max:255'],
            'telefone' => ['nullable', 'string', 'max:20', new TelefoneValido()],

            // Endereços
            'enderecos' => ['nullable', 'array'],
            'enderecos.*.cep' => ['required', 'string', 'size:8'],
            'enderecos.*.logradouro' => ['required', 'string', 'max:255'],
            'enderecos.*.numero' => ['required', 'string', 'max:20'],
            'enderecos.*.bairro' => ['required', 'string', 'max:100'],
            'enderecos.*.cidade' => ['required', 'string', 'max:100'],
            'enderecos.*.uf' => ['required', 'string', 'size:2'],
        ];
    }

    public function messages(): array
    {
        return [
            'cpf.required_if' => 'CPF é obrigatório para pessoa física.',
            'cnpj.required_if' => 'CNPJ é obrigatório para pessoa jurídica.',
            'cpf.unique' => 'Este CPF já está cadastrado.',
            'cnpj.unique' => 'Este CNPJ já está cadastrado.',
        ];
    }

    protected function prepareForValidation(): void
    {
        // Limpar máscaras
        $this->merge([
            'cpf' => preg_replace('/\D/', '', $this->cpf ?? ''),
            'cnpj' => preg_replace('/\D/', '', $this->cnpj ?? ''),
            'telefone' => preg_replace('/\D/', '', $this->telefone ?? ''),
        ]);
    }
}
```

### Regras de Validação Customizadas

```php
// app/Rules/CpfValido.php
class CpfValido implements ValidationRule
{
    public function validate(string $attribute, mixed $value, Closure $fail): void
    {
        $cpf = preg_replace('/\D/', '', $value);

        if (strlen($cpf) !== 11) {
            $fail('O :attribute deve ter 11 dígitos.');
            return;
        }

        // Verifica se todos os dígitos são iguais
        if (preg_match('/^(\d)\1*$/', $cpf)) {
            $fail('O :attribute é inválido.');
            return;
        }

        // Validação dos dígitos verificadores
        for ($t = 9; $t < 11; $t++) {
            $d = 0;
            for ($c = 0; $c < $t; $c++) {
                $d += $cpf[$c] * (($t + 1) - $c);
            }
            $d = ((10 * $d) % 11) % 10;
            if ($cpf[$t] != $d) {
                $fail('O :attribute é inválido.');
                return;
            }
        }
    }
}

// app/Rules/CnpjValido.php
class CnpjValido implements ValidationRule
{
    public function validate(string $attribute, mixed $value, Closure $fail): void
    {
        $cnpj = preg_replace('/\D/', '', $value);

        if (strlen($cnpj) !== 14) {
            $fail('O :attribute deve ter 14 dígitos.');
            return;
        }

        if (preg_match('/^(\d)\1*$/', $cnpj)) {
            $fail('O :attribute é inválido.');
            return;
        }

        // Validação dos dígitos verificadores
        $multiplicadores1 = [5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2];
        $multiplicadores2 = [6, 5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2];

        $soma = 0;
        for ($i = 0; $i < 12; $i++) {
            $soma += $cnpj[$i] * $multiplicadores1[$i];
        }
        $resto = $soma % 11;
        $digito1 = $resto < 2 ? 0 : 11 - $resto;

        if ($cnpj[12] != $digito1) {
            $fail('O :attribute é inválido.');
            return;
        }

        $soma = 0;
        for ($i = 0; $i < 13; $i++) {
            $soma += $cnpj[$i] * $multiplicadores2[$i];
        }
        $resto = $soma % 11;
        $digito2 = $resto < 2 ? 0 : 11 - $resto;

        if ($cnpj[13] != $digito2) {
            $fail('O :attribute é inválido.');
            return;
        }
    }
}
```

---

## Proteções de Segurança

### CSRF

```php
// Automático para formulários Blade
// Para Inertia, usar o middleware VerifyCsrfToken

// app/Http/Middleware/VerifyCsrfToken.php
class VerifyCsrfToken extends Middleware
{
    protected $except = [
        // Apenas webhooks externos (se necessário)
        'webhooks/*',
    ];
}
```

### XSS

```php
// Vue.js escapa automaticamente com {{ }}
// Para HTML raw, usar v-html apenas com dados sanitizados

// Backend: usar Purifier para conteúdo HTML
// composer require mews/purifier

$limpo = Purifier::clean($request->input('descricao'));
```

### SQL Injection

```php
// NUNCA concatenar SQL - usar Eloquent ou Query Builder

// ❌ ERRADO (vulnerável)
DB::select("SELECT * FROM usuarios WHERE nome = '" . $nome . "'");

// ✅ CORRETO
Usuario::where('nome', $nome)->get();
DB::select("SELECT * FROM usuarios WHERE nome = ?", [$nome]);
```

### Mass Assignment

```php
// Definir $fillable ou $guarded em todos os models

class Venda extends Model
{
    protected $fillable = [
        'cliente_id',
        'vendedor_id',
        'endereco_entrega_id',
        'observacao',
    ];

    // Campos que NUNCA podem ser mass-assigned
    protected $guarded = [
        'id',
        'status',
        'total',
        'created_at',
        'updated_at',
    ];
}
```

### Headers de Segurança

```php
// app/Http/Middleware/SecurityHeaders.php
class SecurityHeaders
{
    public function handle(Request $request, Closure $next): Response
    {
        $response = $next($request);

        $response->headers->set('X-Content-Type-Options', 'nosniff');
        $response->headers->set('X-Frame-Options', 'SAMEORIGIN');
        $response->headers->set('X-XSS-Protection', '1; mode=block');
        $response->headers->set('Referrer-Policy', 'strict-origin-when-cross-origin');
        $response->headers->set(
            'Content-Security-Policy',
            "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'"
        );

        return $response;
    }
}
```

---

## Auditoria

### Logging de Ações

```php
// composer require spatie/laravel-activitylog

// app/Models/Venda.php
use Spatie\Activitylog\Traits\LogsActivity;
use Spatie\Activitylog\LogOptions;

class Venda extends Model
{
    use LogsActivity;

    public function getActivitylogOptions(): LogOptions
    {
        return LogOptions::defaults()
            ->logOnly(['status', 'total', 'cliente_id', 'vendedor_id'])
            ->logOnlyDirty()
            ->dontSubmitEmptyLogs()
            ->setDescriptionForEvent(fn(string $eventName) => "Venda foi {$eventName}");
    }
}
```

### Log de Login/Logout

```php
// app/Listeners/LogSuccessfulLogin.php
class LogSuccessfulLogin
{
    public function handle(Login $event): void
    {
        activity()
            ->causedBy($event->user)
            ->withProperties([
                'ip' => request()->ip(),
                'user_agent' => request()->userAgent(),
            ])
            ->log('Login realizado');
    }
}

// app/Listeners/LogSuccessfulLogout.php
class LogSuccessfulLogout
{
    public function handle(Logout $event): void
    {
        activity()
            ->causedBy($event->user)
            ->log('Logout realizado');
    }
}
```

### Consulta de Auditoria

```php
// app/Http/Controllers/AuditoriaController.php
class AuditoriaController extends Controller
{
    public function index(Request $request)
    {
        $this->authorize('view_auditoria');

        $logs = Activity::query()
            ->with('causer')
            ->when($request->usuario_id, fn($q) => $q->causedBy(Usuario::find($request->usuario_id)))
            ->when($request->modelo, fn($q) => $q->where('subject_type', $request->modelo))
            ->when($request->data_inicio, fn($q) => $q->whereDate('created_at', '>=', $request->data_inicio))
            ->when($request->data_fim, fn($q) => $q->whereDate('created_at', '<=', $request->data_fim))
            ->orderByDesc('created_at')
            ->paginate(50);

        return Inertia::render('Auditoria/Index', [
            'logs' => $logs,
        ]);
    }
}
```

---

## Gerenciamento de Sessão

```php
// config/session.php
return [
    'driver' => env('SESSION_DRIVER', 'database'),
    'lifetime' => 120, // minutos
    'expire_on_close' => false,
    'encrypt' => true,
    'secure' => env('SESSION_SECURE_COOKIE', true),
    'same_site' => 'lax',
];

// Logout de todas as sessões
public function logoutOutrasSessoes(Request $request)
{
    $request->validate([
        'password' => ['required', 'current_password'],
    ]);

    Auth::logoutOtherDevices($request->password);

    return back()->with('success', 'Outras sessões foram encerradas.');
}
```

---

## LGPD Compliance

### Consentimento

```php
// app/Models/ConsentimentoLgpd.php
class ConsentimentoLgpd extends Model
{
    protected $table = 'consentimentos_lgpd';

    protected $fillable = [
        'cliente_id',
        'tipo_consentimento',
        'aceito',
        'ip',
        'user_agent',
        'versao_termos',
    ];

    protected $casts = [
        'aceito' => 'boolean',
    ];
}
```

### Exportação de Dados

```php
// app/Services/Lgpd/ExportacaoDadosService.php
class ExportacaoDadosService
{
    /**
     * Exportar todos os dados de um cliente (Direito de Portabilidade)
     */
    public function exportar(Cliente $cliente): array
    {
        return [
            'dados_cadastrais' => $cliente->only([
                'razao_social', 'nome_fantasia', 'cpf', 'cnpj',
                'email', 'telefone', 'celular',
            ]),
            'enderecos' => $cliente->enderecos->map->only([
                'cep', 'logradouro', 'numero', 'bairro', 'cidade', 'uf',
            ]),
            'orcamentos' => $cliente->orcamentos->map->only([
                'id', 'created_at', 'total', 'status',
            ]),
            'vendas' => $cliente->vendas->map->only([
                'id', 'created_at', 'total', 'status',
            ]),
            'exportado_em' => now()->toIso8601String(),
        ];
    }
}
```

### Anonimização

```php
// app/Services/Lgpd/AnonimizacaoService.php
class AnonimizacaoService
{
    /**
     * Anonimizar dados de cliente (Direito ao Esquecimento)
     */
    public function anonimizar(Cliente $cliente): void
    {
        DB::transaction(function () use ($cliente) {
            // Anonimizar dados pessoais
            $cliente->update([
                'razao_social' => 'CLIENTE ANONIMIZADO #' . $cliente->id,
                'nome_fantasia' => null,
                'cpf' => null,
                'cnpj' => null,
                'email' => null,
                'telefone' => null,
                'celular' => null,
                'anonimizado_em' => now(),
            ]);

            // Anonimizar endereços
            $cliente->enderecos()->update([
                'logradouro' => 'ANONIMIZADO',
                'numero' => '0',
                'complemento' => null,
            ]);

            // Log de auditoria
            activity()
                ->causedBy(auth()->user())
                ->performedOn($cliente)
                ->log('Dados anonimizados por solicitação LGPD');
        });
    }
}
```

---

## Considerações de Migração

### Migração de Permissões

```php
// database/seeders/MigrarPermissoesLegadoSeeder.php
class MigrarPermissoesLegadoSeeder extends Seeder
{
    public function run(): void
    {
        // Mapear permissões legadas para novas
        $mapeamento = [
            'view_tab_orcamento' => 'view_orcamento',
            'view_tab_venda' => 'view_venda',
            'view_tab_compra' => 'view_compra',
            // ...
        ];

        $usuarios = DB::connection('mysql_legado')
            ->table('usuario_has_permissao')
            ->get();

        foreach ($usuarios as $legado) {
            $usuario = Usuario::find($legado->idUsuario);
            if (!$usuario) continue;

            foreach ($mapeamento as $antiga => $nova) {
                if ($legado->{$antiga}) {
                    $usuario->givePermissionTo($nova);
                }
            }
        }
    }
}
```

### Checklist de Segurança

- [ ] Todas as senhas migradas para bcrypt
- [ ] Rate limiting configurado em login
- [ ] 2FA habilitado para admins
- [ ] Policies criadas para todos os models principais
- [ ] Form Requests em todos os controllers
- [ ] Activity log configurado
- [ ] Headers de segurança aplicados
- [ ] HTTPS forçado em produção
- [ ] Backup de dados criptografado
- [ ] Consentimento LGPD implementado
