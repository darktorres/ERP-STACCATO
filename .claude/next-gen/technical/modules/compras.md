# Module: Compras (Purchases)

> Status: **Draft**
> Priority: 2 (after Cadastros)
> Complexity: Medium

---

## Current Implementation (C++)

### Classes
| Class | File | Purpose |
|-------|------|---------|
| `TabCompras` | `tabcompras.cpp` | Main tab container |
| `WidgetCompraGerar` | `widgetcompragerar.cpp` | Generate purchase orders |
| `WidgetCompraConfirmar` | `widgetcompraconfirmar.cpp` | Confirm orders |
| `WidgetCompraFaturar` | `widgetcomprafaturar.cpp` | Invoice orders |
| `WidgetCompraPendentes` | `widgetcomprapendentes.cpp` | Pending orders list |
| `WidgetCompraResumo` | `widgetcompraresumo.cpp` | Order summary |
| `WidgetCompraDevolucao` | `widgetcompradevolucao.cpp` | Returns |
| `WidgetCompraHistorico` | `widgetcomprahistorico.cpp` | Order history |
| `CompraAvulsa` | `compraavulsa.cpp` | Ad-hoc purchases |

### Current Workflow

```
Venda Created
    ↓
[GERAR] Generate Purchase Order
    ↓ status: PENDENTE
[CONFIRMAR] Confirm with Supplier
    ↓ status: CONFIRMADO
    ↓ → Creates Contas a Pagar
[FATURAR] Receive Invoice (NFe)
    ↓ status: FATURADO
[RECEBER] Receive Goods
    ↓ status: RECEBIDO
    ↓ → Updates Estoque
```

### Database Tables (Current)
- `pedido_fornecedor_has_produto` (Level 1)
- `pedido_fornecedor_has_produto2` (Level 2)
- `compra_avulsa` (Ad-hoc purchases)
- `conta_a_pagar_has_pagamento`
- `conta_a_pagar_has_idcompra`

---

## Laravel Implementation

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

### Status Enum

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
     * Generate purchase orders from a sale
     */
    public function gerarDeVenda(Venda $venda): Collection
    {
        return DB::transaction(function () use ($venda) {
            // Group sale items by supplier
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
     * Confirm purchase order with supplier
     */
    public function confirmar(Compra $compra): void
    {
        DB::transaction(function () use ($compra) {
            $this->validarTransicao($compra, CompraStatus::CONFIRMADO);

            $compra->update([
                'status' => CompraStatus::CONFIRMADO,
                'data_real_compra' => now(),
            ]);

            // Generate accounts payable
            $this->contaPagarService->gerarDeCompra($compra);

            event(new CompraConfirmada($compra));
        });
    }

    /**
     * Receive invoice (NFe)
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
     * Receive goods into inventory
     */
    public function receber(Compra $compra, array $recebimento): void
    {
        DB::transaction(function () use ($compra, $recebimento) {
            $this->validarTransicao($compra, CompraStatus::RECEBIDO);

            // Create stock entries
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

### Routes

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

## UI Components Needed

### List View
- Filterable data table (status, supplier, date range)
- Quick actions (confirm, view)
- Status badges with colors
- Pagination

### Detail View
- Header with order info
- Items table with edit capability
- Status timeline
- Action buttons based on current status
- Related documents (NFe, Contas a Pagar)

### Form (Ad-hoc Purchase)
- Supplier selection (searchable)
- Product line items
- Payment terms
- Delivery info

---

## Events

| Event | Triggers |
|-------|----------|
| `CompraConfirmada` | Generate Contas a Pagar, Notify supplier |
| `CompraFaturada` | Link NFe, Update financials |
| `CompraRecebida` | Create Estoque entries, Update Venda items |
| `CompraCancelada` | Reverse Contas a Pagar, Notify |

---

## Migration Considerations

### Data Migration
1. Map `pedido_fornecedor_has_produto` → `compras`
2. Map `pedido_fornecedor_has_produto2` → `compra_itens`
3. Normalize supplier references (name → FK)
4. Map date fields to new naming

### Breaking Changes
- Two-level table structure simplified to one level
- Status values may differ
- Supplier reference is now FK only
