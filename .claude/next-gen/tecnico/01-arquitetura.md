# Design de Arquitetura Laravel

> Status: **Rascunho**
> Última atualização: 2025-12-27

---

## Problemas Atuais no Código Legado

### 1. Vulnerabilidades de SQL Injection (ALTA)

**30+ arquivos afetados** com concatenação de strings em queries:

```cpp
// VULNERÁVEL - Encontrado em compraavulsa.cpp:350
query.exec("SELECT * FROM nfe WHERE idNFe = " + ui->itemBoxNFe->getId().toString())

// VULNERÁVEL - Encontrado em cadastrofornecedor.cpp
query.exec("UPDATE produto SET fornecedor = '" + data("razaoSocial").toString() + "'")
```

Alguns arquivos usam queries parametrizadas corretamente (inconsistente):
```cpp
// SEGURO - Encontrado em cadastroproduto.cpp:88-90
query.prepare("SELECT idProduto FROM produto WHERE fornecedor = :fornecedor");
query.bindValue(":fornecedor", ui->itemBoxFornecedor->text());
```

### 2. Lógica de Negócio em Widgets (MÉDIA)

Lógica espalhada em:
- Classes de Widget (`WidgetCompra*`, `WidgetEstoque*`)
- Classes de Dialog (`CadastroProduto`, `Venda`)
- Classe utilitária SQL estática (`Sql::contasPagar()`, `Sql::updateVendaStatus()`)
- Classe Application (`qApp->roundDouble()`, `qApp->sanitizeSQL()`)

**Não existe uma camada de serviço clara.**

### 3. Estado Global via Macro `qApp` (MÉDIA)

Tudo acessado globalmente:
- Conexão com banco de dados
- Configuração
- Estado de transação
- Sessão do usuário
- Filas de erro

Dificulta testes e isolamento.

---

## Estrutura de Diretórios Laravel Proposta

```
app/
├── Models/                    # Modelos Eloquent com relacionamentos
│   ├── Produto.php
│   ├── Cliente.php
│   ├── Fornecedor.php
│   ├── Venda.php
│   ├── Compra.php
│   ├── Estoque.php
│   ├── Nfe.php
│   └── ...
│
├── Services/                  # Camada de lógica de negócio (NOVO!)
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
│   ├── Controllers/           # Controllers enxutos, delegam para services
│   │   ├── CompraController.php
│   │   ├── VendaController.php
│   │   └── ...
│   ├── Requests/              # Validação de formulário (substitui verifyFields)
│   │   ├── StoreProdutoRequest.php
│   │   ├── StoreClienteRequest.php
│   │   └── ...
│   └── Resources/             # Respostas JSON da API (se necessário)
│
├── Events/                    # Eventos de domínio
│   ├── CompraConfirmada.php
│   ├── VendaFinalizada.php
│   ├── EstoqueAtualizado.php
│   └── NfeEmitida.php
│
├── Listeners/                 # Manipuladores de eventos
│   ├── GerarContasPagar.php
│   ├── AtualizarEstoque.php
│   └── EnviarNotificacao.php
│
├── Jobs/                      # Tarefas em background
│   ├── ProcessarNFeJob.php
│   ├── GerarCnabJob.php
│   └── ImportarTabelaIbptJob.php
│
├── DTOs/                      # Objetos de transferência de dados (opcional)
│   ├── CompraDTO.php
│   └── ...
│
├── Enums/                     # Enums PHP 8.1+
│   ├── VendaStatus.php
│   ├── CompraStatus.php
│   ├── NfeStatus.php
│   └── ...
│
├── Exceptions/                # Exceções customizadas
│   ├── BusinessException.php
│   ├── ValidationException.php
│   └── NfeException.php
│
└── Rules/                     # Regras de validação customizadas
    ├── CnpjValido.php
    ├── CpfValido.php
    └── InscricaoEstadualValida.php
```

---

## Padrões de Design a Aplicar

### 1. Padrão Service Layer

Controllers enxutos delegam para services:

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

Service contém lógica de negócio:

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

### 2. Enums PHP 8.1+ para Status

Substituir strings mágicas por enums com type-safety:

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

Uso no modelo:

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

### 3. Fluxos de Trabalho Orientados a Eventos

Desacoplar operações com eventos:

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

### 4. Validação com Form Request

Substituir `verifyFields()` por classes de request dedicadas:

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

## Mapeamento de Módulo para Controller/Service

| Classe C++ | Controller Laravel | Service Laravel |
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
| `CadastroCliente` | `ClienteController` | - (CRUD simples) |
| `CadastroFornecedor` | `FornecedorController` | - (CRUD simples) |

---

## Jobs em Background

| Implementação Atual | Job Laravel |
|-----------------------|-------------|
| Assinatura NFe via ACBr | `ProcessarNFeJob` |
| Geração de CNAB | `GerarCnabJob` |
| Importação tabela IBPT | `ImportarIbptJob` |
| Geração de relatórios | `GerarRelatorioJob` |

---

## Tarefas Agendadas

```php
// app/Console/Kernel.php
protected function schedule(Schedule $schedule)
{
    // Verificar atualizações de status de NFe
    $schedule->job(new ConsultarStatusNFeJob)->everyFiveMinutes();

    // Reconciliação diária de estoque
    $schedule->job(new ReconciliarEstoqueJob)->dailyAt('06:00');

    // Sincronização de feriados bancários
    $schedule->job(new SincronizarFeriadosJob)->weekly();

    // Limpar sessões/logs antigos
    $schedule->command('sanctum:prune-expired --hours=24')->daily();
}
```

---

## Estratégia de Testes

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
