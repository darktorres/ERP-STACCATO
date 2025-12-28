# Laravel Architecture Design

> Status: **Draft**
> Last updated: 2025-12-27

---

## Current Problems in Legacy Code

### 1. SQL Injection Vulnerabilities (HIGH)

**30+ files affected** with string concatenation in queries:

```cpp
// VULNERABLE - Found in compraavulsa.cpp:350
query.exec("SELECT * FROM nfe WHERE idNFe = " + ui->itemBoxNFe->getId().toString())

// VULNERABLE - Found in cadastrofornecedor.cpp
query.exec("UPDATE produto SET fornecedor = '" + data("razaoSocial").toString() + "'")
```

Some files use parameterized queries correctly (inconsistent):
```cpp
// SAFE - Found in cadastroproduto.cpp:88-90
query.prepare("SELECT idProduto FROM produto WHERE fornecedor = :fornecedor");
query.bindValue(":fornecedor", ui->itemBoxFornecedor->text());
```

### 2. Business Logic in Widgets (MEDIUM)

Logic scattered across:
- Widget classes (`WidgetCompra*`, `WidgetEstoque*`)
- Dialog classes (`CadastroProduto`, `Venda`)
- Static SQL utility class (`Sql::contasPagar()`, `Sql::updateVendaStatus()`)
- Application class (`qApp->roundDouble()`, `qApp->sanitizeSQL()`)

**No clear service layer exists.**

### 3. Global State via `qApp` Macro (MEDIUM)

Everything accessed globally:
- Database connection
- Configuration
- Transaction state
- User session
- Error queues

Makes testing and isolation difficult.

---

## Proposed Laravel Directory Structure

```
app/
├── Models/                    # Eloquent models with relationships
│   ├── Produto.php
│   ├── Cliente.php
│   ├── Fornecedor.php
│   ├── Venda.php
│   ├── Compra.php
│   ├── Estoque.php
│   ├── Nfe.php
│   └── ...
│
├── Services/                  # Business logic layer (NEW!)
│   ├── Compras/
│   │   ├── CompraService.php
│   │   ├── ConfirmacaoCompraService.php
│   │   └── DevolucaoService.php
│   ├── Estoque/
│   │   ├── EstoqueService.php
│   │   ├── ConsumoService.php
│   │   └── InventarioService.php
│   ├── Financeiro/
│   │   ├── ContaPagarService.php
│   │   ├── ContaReceberService.php
│   │   └── ConciliacaoService.php
│   ├── NFe/
│   │   ├── NfeEmissaoService.php
│   │   ├── NfeImportacaoService.php
│   │   └── NfeValidacaoService.php
│   └── Vendas/
│       ├── VendaService.php
│       ├── OrcamentoService.php
│       └── EntregaService.php
│
├── Http/
│   ├── Controllers/           # Thin controllers, delegate to services
│   │   ├── CompraController.php
│   │   ├── VendaController.php
│   │   └── ...
│   ├── Requests/              # Form validation (replaces verifyFields)
│   │   ├── StoreProdutoRequest.php
│   │   ├── StoreClienteRequest.php
│   │   └── ...
│   └── Resources/             # API JSON responses (if needed)
│
├── Events/                    # Domain events
│   ├── CompraConfirmada.php
│   ├── VendaFinalizada.php
│   ├── EstoqueAtualizado.php
│   └── NfeEmitida.php
│
├── Listeners/                 # Event handlers
│   ├── GerarContasPagar.php
│   ├── AtualizarEstoque.php
│   └── EnviarNotificacao.php
│
├── Jobs/                      # Background tasks
│   ├── ProcessarNFeJob.php
│   ├── GerarCnabJob.php
│   └── ImportarTabelaIbptJob.php
│
├── DTOs/                      # Data transfer objects (optional)
│   ├── CompraDTO.php
│   └── ...
│
├── Enums/                     # PHP 8.1+ enums
│   ├── VendaStatus.php
│   ├── CompraStatus.php
│   ├── NfeStatus.php
│   └── ...
│
├── Exceptions/                # Custom exceptions
│   ├── BusinessException.php
│   ├── ValidationException.php
│   └── NfeException.php
│
└── Rules/                     # Custom validation rules
    ├── CnpjValido.php
    ├── CpfValido.php
    └── InscricaoEstadualValida.php
```

---

## Design Patterns to Apply

### 1. Service Layer Pattern

Thin controllers delegate to services:

```php
<?php

namespace App\Http\Controllers;

use App\Services\Compras\CompraService;
use App\Http\Requests\StoreCompraRequest;

class CompraController extends Controller
{
    public function __construct(
        private CompraService $compraService
    ) {}

    public function store(StoreCompraRequest $request)
    {
        $compra = $this->compraService->criar($request->validated());

        return redirect()->route('compras.show', $compra)
            ->with('success', 'Compra criada com sucesso');
    }

    public function confirmar(Compra $compra)
    {
        $this->compraService->confirmar($compra);

        return back()->with('success', 'Compra confirmada');
    }
}
```

Service contains business logic:

```php
<?php

namespace App\Services\Compras;

use App\Models\Compra;
use App\Events\CompraConfirmada;
use App\Exceptions\BusinessException;
use Illuminate\Support\Facades\DB;

class CompraService
{
    public function __construct(
        private ContaPagarService $contaPagarService,
        private EstoqueService $estoqueService,
    ) {}

    public function confirmar(Compra $compra): void
    {
        DB::transaction(function () use ($compra) {
            $this->validarParaConfirmacao($compra);

            $compra->update([
                'status' => CompraStatus::CONFIRMADO,
                'data_real_conf' => now(),
            ]);

            $this->contaPagarService->gerarParcelas($compra);
            $this->estoqueService->darEntrada($compra);

            event(new CompraConfirmada($compra));
        });
    }

    private function validarParaConfirmacao(Compra $compra): void
    {
        if ($compra->status !== CompraStatus::PENDENTE) {
            throw new BusinessException('Compra não está pendente');
        }

        if ($compra->itens->isEmpty()) {
            throw new BusinessException('Compra não possui itens');
        }
    }
}
```

### 2. PHP 8.1+ Enums for Status

Replace magic strings with type-safe enums:

```php
<?php

namespace App\Enums;

enum VendaStatus: string
{
    case ORCAMENTO = 'ORCAMENTO';
    case PENDENTE = 'PENDENTE';
    case ESTOQUE = 'ESTOQUE';
    case EM_ENTREGA = 'EM_ENTREGA';
    case ENTREGUE = 'ENTREGUE';
    case FINALIZADO = 'FINALIZADO';
    case CANCELADO = 'CANCELADO';

    public function label(): string
    {
        return match($this) {
            self::ORCAMENTO => 'Orçamento',
            self::PENDENTE => 'Pendente',
            self::ESTOQUE => 'Em Estoque',
            self::EM_ENTREGA => 'Em Entrega',
            self::ENTREGUE => 'Entregue',
            self::FINALIZADO => 'Finalizado',
            self::CANCELADO => 'Cancelado',
        };
    }

    public function color(): string
    {
        return match($this) {
            self::ORCAMENTO => 'gray',
            self::PENDENTE => 'yellow',
            self::ESTOQUE => 'blue',
            self::EM_ENTREGA => 'orange',
            self::ENTREGUE => 'green',
            self::FINALIZADO => 'emerald',
            self::CANCELADO => 'red',
        };
    }

    public function canTransitionTo(self $new): bool
    {
        return match($this) {
            self::ORCAMENTO => in_array($new, [self::PENDENTE, self::CANCELADO]),
            self::PENDENTE => in_array($new, [self::ESTOQUE, self::CANCELADO]),
            self::ESTOQUE => in_array($new, [self::EM_ENTREGA, self::CANCELADO]),
            self::EM_ENTREGA => in_array($new, [self::ENTREGUE, self::CANCELADO]),
            self::ENTREGUE => in_array($new, [self::FINALIZADO]),
            self::FINALIZADO, self::CANCELADO => false,
        };
    }
}
```

Usage in model:

```php
<?php

namespace App\Models;

use App\Enums\VendaStatus;

class Venda extends Model
{
    protected $casts = [
        'status' => VendaStatus::class,
    ];
}
```

### 3. Event-Driven Workflows

Decouple operations with events:

```php
<?php
// app/Events/CompraConfirmada.php
namespace App\Events;

class CompraConfirmada
{
    public function __construct(
        public Compra $compra
    ) {}
}

// app/Providers/EventServiceProvider.php
protected $listen = [
    CompraConfirmada::class => [
        GerarContasPagarListener::class,
        AtualizarEstoqueListener::class,
        EnviarNotificacaoCompraListener::class,
        LogAtividadeListener::class,
    ],
];

// app/Listeners/GerarContasPagarListener.php
namespace App\Listeners;

class GerarContasPagarListener
{
    public function handle(CompraConfirmada $event): void
    {
        $compra = $event->compra;

        foreach ($compra->parcelas as $parcela) {
            ContaPagar::create([
                'compra_id' => $compra->id,
                'fornecedor_id' => $compra->fornecedor_id,
                'valor' => $parcela->valor,
                'vencimento' => $parcela->vencimento,
                'status' => ContaPagarStatus::PENDENTE,
            ]);
        }
    }
}
```

### 4. Form Request Validation

Replace `verifyFields()` with dedicated request classes:

```php
<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use App\Rules\CnpjValido;

class StoreFornecedorRequest extends FormRequest
{
    public function rules(): array
    {
        return [
            'razao_social' => ['required', 'string', 'max:255'],
            'nome_fantasia' => ['nullable', 'string', 'max:255'],
            'cnpj' => ['required', new CnpjValido, 'unique:fornecedores,cnpj'],
            'inscricao_estadual' => ['nullable', 'string', 'max:20'],
            'email' => ['nullable', 'email'],
            'telefone' => ['nullable', 'string', 'max:20'],
            'endereco' => ['required', 'array'],
            'endereco.cep' => ['required', 'string', 'size:8'],
            'endereco.logradouro' => ['required', 'string', 'max:255'],
            'endereco.numero' => ['required', 'string', 'max:20'],
            'endereco.cidade_id' => ['required', 'exists:cidades,id'],
        ];
    }

    public function messages(): array
    {
        return [
            'razao_social.required' => 'Razão social é obrigatória',
            'cnpj.required' => 'CNPJ é obrigatório',
            'endereco.cep.required' => 'CEP é obrigatório',
            'endereco.cep.size' => 'CEP deve ter 8 dígitos',
        ];
    }
}
```

---

## Module to Controller/Service Mapping

| C++ Class | Laravel Controller | Laravel Service |
|-----------|-------------------|-----------------|
| `TabCompras` | `CompraController` | `CompraService` |
| `WidgetCompraGerar` | `CompraController@create` | `CompraService@gerar()` |
| `WidgetCompraConfirmar` | `CompraController@confirmar` | `CompraService@confirmar()` |
| `WidgetCompraFaturar` | `CompraController@faturar` | `CompraService@faturar()` |
| `TabEstoque` | `EstoqueController` | `EstoqueService` |
| `Estoque` (dialog) | `EstoqueController@show` | `EstoqueService@visualizar()` |
| `TabFinanceiro` | `ContaPagarController`, `ContaReceberController` | `FinanceiroService` |
| `TabNFe` | `NfeController` | `NfeService` |
| `CadastroNFe` | `NfeController@store` | `NfeEmissaoService` |
| `TabLogistica` | `EntregaController` | `LogisticaService` |
| `TabGalpao` | `ArmazemController` | `ArmazemService` |
| `CadastroProduto` | `ProdutoController` | `ProdutoService` |
| `CadastroCliente` | `ClienteController` | - (simple CRUD) |
| `CadastroFornecedor` | `FornecedorController` | - (simple CRUD) |

---

## Background Jobs

| Current Implementation | Laravel Job |
|-----------------------|-------------|
| ACBr NFe signing | `ProcessarNFeJob` |
| CNAB generation | `GerarCnabJob` |
| IBPT table import | `ImportarIbptJob` |
| Report generation | `GerarRelatorioJob` |

---

## Scheduled Tasks

```php
// app/Console/Kernel.php
protected function schedule(Schedule $schedule)
{
    // Check for NFe status updates
    $schedule->job(new ConsultarStatusNFeJob)->everyFiveMinutes();

    // Daily inventory reconciliation
    $schedule->job(new ReconciliarEstoqueJob)->dailyAt('06:00');

    // Banking holidays sync
    $schedule->job(new SincronizarFeriadosJob)->weekly();

    // Clean up old sessions/logs
    $schedule->command('sanctum:prune-expired --hours=24')->daily();
}
```

---

## Testing Strategy

```
tests/
├── Unit/
│   ├── Services/
│   │   ├── CompraServiceTest.php
│   │   └── EstoqueServiceTest.php
│   ├── Models/
│   │   └── VendaStatusTransitionTest.php
│   └── Rules/
│       ├── CnpjValidoTest.php
│       └── CpfValidoTest.php
│
├── Feature/
│   ├── Compras/
│   │   ├── CriarCompraTest.php
│   │   └── ConfirmarCompraTest.php
│   ├── Vendas/
│   │   └── FluxoVendaTest.php
│   └── NFe/
│       └── EmitirNfeTest.php
│
└── Integration/
    └── NFe/
        └── SefazIntegrationTest.php
```
