# Módulo: Compras

> Status: **Rascunho**
> Prioridade: 2 (após Cadastros)
> Complexidade: Média

---

## Implementação Atual (C++)

### Classes
| Classe | Arquivo | Finalidade |
|--------|---------|------------|
| `TabCompras` | `tabcompras.cpp` | Container principal da aba |
| `WidgetCompraGerar` | `widgetcompragerar.cpp` | Gerar pedidos de compra |
| `WidgetCompraConfirmar` | `widgetcompraconfirmar.cpp` | Confirmar pedidos |
| `WidgetCompraFaturar` | `widgetcomprafaturar.cpp` | Faturar pedidos |
| `WidgetCompraPendentes` | `widgetcomprapendentes.cpp` | Lista de pedidos pendentes |
| `WidgetCompraResumo` | `widgetcompraresumo.cpp` | Resumo do pedido |
| `WidgetCompraDevolucao` | `widgetcompradevolucao.cpp` | Devoluções |
| `WidgetCompraHistorico` | `widgetcomprahistorico.cpp` | Histórico de pedidos |
| `CompraAvulsa` | `compraavulsa.cpp` | Compras avulsas |

### Fluxo Atual

```
Venda Criada
    ↓
[GERAR] Gerar Pedido de Compra
    ↓ status: PENDENTE
[CONFIRMAR] Confirmar com Fornecedor
    ↓ status: CONFIRMADO
    ↓ → Cria Contas a Pagar
[FATURAR] Receber Nota Fiscal (NFe)
    ↓ status: FATURADO
[RECEBER] Receber Mercadorias
    ↓ status: RECEBIDO
    ↓ → Atualiza Estoque
```

### Tabelas do Banco de Dados (Atual)
- `pedido_fornecedor_has_produto` (Nível 1)
- `pedido_fornecedor_has_produto2` (Nível 2)
- `compra_avulsa` (Compras avulsas)
- `conta_a_pagar_has_pagamento`
- `conta_a_pagar_has_idcompra`

---

## Implementação Laravel

### Models

```php
// app/Models/Compra.php
class Compra extends Model
{
    protected $fillable = [
        'loja_id', 'fornecedor_id', 'venda_id', 'status',
        'subtotal', 'frete', 'total',
        'data_prev_compra', 'data_real_compra',
        'data_prev_entrega', 'data_real_entrega',
        'nfe_id', 'observacoes',
    ];

    protected $casts = [
        'status' => CompraStatus::class,
        'data_prev_compra' => 'date',
        'data_real_compra' => 'date',
        'data_prev_entrega' => 'date',
        'data_real_entrega' => 'date',
    ];

    public function loja(): BelongsTo
    {
        return $this->belongsTo(Loja::class);
    }

    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }

    public function venda(): BelongsTo
    {
        return $this->belongsTo(Venda::class);
    }

    public function itens(): HasMany
    {
        return $this->hasMany(CompraItem::class);
    }

    public function contasPagar(): HasMany
    {
        return $this->hasMany(ContaPagar::class);
    }

    public function nfe(): BelongsTo
    {
        return $this->belongsTo(Nfe::class);
    }
}

// app/Models/CompraItem.php
class CompraItem extends Model
{
    protected $fillable = [
        'compra_id', 'produto_id', 'venda_item_id',
        'quantidade', 'preco_unitario', 'desconto',
        'descricao_produto', 'unidade',
    ];

    public function compra(): BelongsTo
    {
        return $this->belongsTo(Compra::class);
    }

    public function produto(): BelongsTo
    {
        return $this->belongsTo(Produto::class);
    }

    public function vendaItem(): BelongsTo
    {
        return $this->belongsTo(VendaItem::class);
    }
}
```

### Enum de Status

```php
// app/Enums/CompraStatus.php
enum CompraStatus: string
{
    case PENDENTE = 'PENDENTE';
    case CONFIRMADO = 'CONFIRMADO';
    case FATURADO = 'FATURADO';
    case RECEBIDO = 'RECEBIDO';
    case CANCELADO = 'CANCELADO';

    public function label(): string
    {
        return match($this) {
            self::PENDENTE => 'Pendente',
            self::CONFIRMADO => 'Confirmado',
            self::FATURADO => 'Faturado',
            self::RECEBIDO => 'Recebido',
            self::CANCELADO => 'Cancelado',
        };
    }

    public function color(): string
    {
        return match($this) {
            self::PENDENTE => 'yellow',
            self::CONFIRMADO => 'blue',
            self::FATURADO => 'purple',
            self::RECEBIDO => 'green',
            self::CANCELADO => 'red',
        };
    }

    public function canTransitionTo(self $new): bool
    {
        return match($this) {
            self::PENDENTE => in_array($new, [self::CONFIRMADO, self::CANCELADO]),
            self::CONFIRMADO => in_array($new, [self::FATURADO, self::CANCELADO]),
            self::FATURADO => in_array($new, [self::RECEBIDO, self::CANCELADO]),
            self::RECEBIDO, self::CANCELADO => false,
        };
    }
}
```

### Service

```php
// app/Services/Compras/CompraService.php
class CompraService
{
    public function __construct(
        private ContaPagarService $contaPagarService,
        private EstoqueService $estoqueService,
    ) {}

    /**
     * Gerar pedidos de compra a partir de uma venda
     */
    public function gerarDeVenda(Venda $venda): Collection
    {
        return DB::transaction(function () use ($venda) {
            // Agrupar itens da venda por fornecedor
            $itensPorFornecedor = $venda->itens
                ->groupBy('fornecedor_id');

            $compras = collect();

            foreach ($itensPorFornecedor as $fornecedorId => $itens) {
                $compra = Compra::create([
                    'loja_id' => $venda->loja_id,
                    'fornecedor_id' => $fornecedorId,
                    'venda_id' => $venda->id,
                    'status' => CompraStatus::PENDENTE,
                    'data_prev_compra' => now(),
                ]);

                foreach ($itens as $vendaItem) {
                    $compra->itens()->create([
                        'produto_id' => $vendaItem->produto_id,
                        'venda_item_id' => $vendaItem->id,
                        'quantidade' => $vendaItem->quantidade,
                        'preco_unitario' => $vendaItem->custo_estimado,
                        'descricao_produto' => $vendaItem->descricao_produto,
                        'unidade' => $vendaItem->unidade,
                    ]);
                }

                $compra->recalcularTotais();
                $compras->push($compra);
            }

            return $compras;
        });
    }

    /**
     * Confirmar pedido de compra com fornecedor
     */
    public function confirmar(Compra $compra): void
    {
        DB::transaction(function () use ($compra) {
            $this->validarTransicao($compra, CompraStatus::CONFIRMADO);

            $compra->update([
                'status' => CompraStatus::CONFIRMADO,
                'data_real_compra' => now(),
            ]);

            // Gerar contas a pagar
            $this->contaPagarService->gerarDeCompra($compra);

            event(new CompraConfirmada($compra));
        });
    }

    /**
     * Receber nota fiscal (NFe)
     */
    public function faturar(Compra $compra, Nfe $nfe): void
    {
        DB::transaction(function () use ($compra, $nfe) {
            $this->validarTransicao($compra, CompraStatus::FATURADO);

            $compra->update([
                'status' => CompraStatus::FATURADO,
                'nfe_id' => $nfe->id,
            ]);

            event(new CompraFaturada($compra));
        });
    }

    /**
     * Receber mercadorias no estoque
     */
    public function receber(Compra $compra, array $recebimento): void
    {
        DB::transaction(function () use ($compra, $recebimento) {
            $this->validarTransicao($compra, CompraStatus::RECEBIDO);

            // Criar entradas no estoque
            foreach ($compra->itens as $item) {
                $this->estoqueService->darEntrada([
                    'loja_id' => $compra->loja_id,
                    'compra_id' => $compra->id,
                    'produto_id' => $item->produto_id,
                    'fornecedor_id' => $compra->fornecedor_id,
                    'quantidade' => $item->quantidade,
                    'custo_unitario' => $item->preco_unitario,
                    'bloco_id' => $recebimento['bloco_id'] ?? null,
                ]);
            }

            $compra->update([
                'status' => CompraStatus::RECEBIDO,
                'data_real_entrega' => now(),
            ]);

            event(new CompraRecebida($compra));
        });
    }

    private function validarTransicao(Compra $compra, CompraStatus $novoStatus): void
    {
        if (!$compra->status->canTransitionTo($novoStatus)) {
            throw new BusinessException(
                "Não é possível alterar status de {$compra->status->label()} para {$novoStatus->label()}"
            );
        }
    }
}
```

### Controller

```php
// app/Http/Controllers/CompraController.php
class CompraController extends Controller
{
    public function __construct(
        private CompraService $compraService
    ) {}

    public function index(Request $request)
    {
        $compras = Compra::query()
            ->with(['fornecedor:id,razao_social', 'venda:id'])
            ->when($request->status, fn($q) => $q->where('status', $request->status))
            ->when($request->fornecedor_id, fn($q) => $q->where('fornecedor_id', $request->fornecedor_id))
            ->latest()
            ->paginate(20);

        return Inertia::render('Compras/Index', [
            'compras' => $compras,
            'filters' => $request->only(['status', 'fornecedor_id']),
        ]);
    }

    public function confirmar(Compra $compra)
    {
        $this->compraService->confirmar($compra);

        return back()->with('success', 'Compra confirmada com sucesso');
    }

    public function receber(Compra $compra, ReceberCompraRequest $request)
    {
        $this->compraService->receber($compra, $request->validated());

        return back()->with('success', 'Compra recebida no estoque');
    }
}
```

### Rotas

```php
// routes/web.php
Route::middleware(['auth'])->group(function () {
    Route::resource('compras', CompraController::class);

    Route::post('compras/{compra}/confirmar', [CompraController::class, 'confirmar'])
        ->name('compras.confirmar');

    Route::post('compras/{compra}/faturar', [CompraController::class, 'faturar'])
        ->name('compras.faturar');

    Route::post('compras/{compra}/receber', [CompraController::class, 'receber'])
        ->name('compras.receber');

    Route::post('compras/{compra}/cancelar', [CompraController::class, 'cancelar'])
        ->name('compras.cancelar');
});
```

---

## Componentes de UI Necessários

### Visualização em Lista
- Tabela de dados filtrável (status, fornecedor, período)
- Ações rápidas (confirmar, visualizar)
- Badges de status com cores
- Paginação

### Visualização de Detalhes
- Cabeçalho com informações do pedido
- Tabela de itens com capacidade de edição
- Linha do tempo de status
- Botões de ação baseados no status atual
- Documentos relacionados (NFe, Contas a Pagar)

### Formulário (Compra Avulsa)
- Seleção de fornecedor (com busca)
- Itens de produto
- Condições de pagamento
- Informações de entrega

---

## Eventos

| Evento | Dispara |
|--------|---------|
| `CompraConfirmada` | Gerar Contas a Pagar, Notificar fornecedor |
| `CompraFaturada` | Vincular NFe, Atualizar financeiro |
| `CompraRecebida` | Criar entradas no Estoque, Atualizar itens da Venda |
| `CompraCancelada` | Reverter Contas a Pagar, Notificar |

---

## Considerações de Migração

### Migração de Dados
1. Mapear `pedido_fornecedor_has_produto` → `compras`
2. Mapear `pedido_fornecedor_has_produto2` → `compra_itens`
3. Normalizar referências de fornecedor (nome → FK)
4. Mapear campos de data para nova nomenclatura

### Mudanças Incompatíveis
- Estrutura de tabela em dois níveis simplificada para um nível
- Valores de status podem diferir
- Referência ao fornecedor agora é apenas FK
