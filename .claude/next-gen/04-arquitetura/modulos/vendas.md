# Módulo: Vendas

> Status: **Rascunho**
> Prioridade: 1 (módulo central)
> Complexidade: **Alta**

---

## Visão Geral

O módulo de Vendas é o **coração do ERP** - todos os outros módulos existem para suportá-lo. O fluxo principal é:

```mermaid
flowchart LR
    Orcamento[Orçamento] --> Venda[Venda]
    Venda --> Compra[Compras]
    Venda --> Estoque[Estoque]
    Venda --> Financeiro[Financeiro]
    Venda --> NFe[NFe]
    Venda --> Logistica[Logística]
```

---

## Implementação Atual (C++)

### Classes

| Classe                | Arquivo                   | Finalidade                       |
| --------------------- | ------------------------- | -------------------------------- |
| `Orcamento`           | `orcamento.cpp`           | Diálogo de criação de orçamento  |
| `Venda`               | `venda.cpp`               | Diálogo de venda                 |
| `WidgetOrcamento`     | `widgetorcamento.cpp`     | Widget de listagem de orçamentos |
| `WidgetVenda`         | `widgetvenda.cpp`         | Widget de listagem de vendas     |
| `BaixaOrcamento`      | `baixaorcamento.cpp`      | Fechamento/baixa de orçamento    |
| `OrcamentoProxyModel` | `orcamentoproxymodel.cpp` | Filtros de orçamento             |
| `VendaProxyModel`     | `vendaproxymodel.cpp`     | Filtros de venda                 |

### Tabelas do Banco de Dados

#### Nível 1 (Cabeçalho + Itens do Pedido)

```text
orcamento                    venda
├── idOrcamento              ├── idVenda
├── idCliente                ├── idCliente
├── idEnderecoEntrega        ├── idEnderecoEntrega
├── idEnderecoFaturamento    ├── idEnderecoFaturamento
├── idUsuario (vendedor)     ├── idUsuario
├── idProfissional           ├── idProfissional
├── status                   ├── status
├── subTotalBru              ├── subTotalBru
├── subTotalLiq              ├── subTotalLiq
├── frete                    ├── frete
├── descontoPorc             ├── descontoPorc
├── descontoReais            ├── descontoReais
├── total                    ├── total
├── prazoEntrega             ├── prazoEntrega
├── validade                 ├── idOrcamento (FK)
└── semaforo                 └── statusFinanceiro

orcamento_itens              venda_itens (Flat structure - NEW SCHEMA)
├── id                       ├── id (PK)
├── orcamento_id (FK)        ├── venda_id (FK)
├── produto_id (FK)          ├── produto_id (FK)
├── quantidade               ├── quantidade
├── valor_unitario           ├── valor_unitario
├── desconto                 ├── desconto
└── total                    └── valor_total

#### Alocações (M:N Relationship - NEW SCHEMA)

```text
alocacoes - Links venda_items to estoque_lotes (FIFO/FEFO fulfillment)
├── id (PK)
├── venda_item_id (FK)          ← Venda item being fulfilled
├── estoque_lote_id (FK)        ← Inventory batch (estoque_lotes)
├── quantidade                  ← Allocated quantity (can be partial)
├── status                      ← ATIVO | REVERTIDA | CANCELADA
└── created_at / updated_at     ← Timestamps for audit

**Key Architecture Change**:
- **OLD**: Two-level structure (N1 + N2) with complex partial splitting
  - N1 (venda_has_produto): Product line item
  - N2 (venda_has_produto2): Fulfillment variant/batch
- **NEW**: Flat single-level items + M:N allocations
  - venda_itens: Single product line (no splitting)
  - alocacoes: M:N relationship to inventory batches (supports FIFO/FEFO)
```

### Fluxo de Estados

#### Orçamento

```mermaid
stateDiagram-v2
    [*] --> ATIVO : Criar orçamento
    ATIVO --> FECHADO : Converter para Venda
    ATIVO --> EXPIRADO : Passar validade
    ATIVO --> PERDIDO : Marcar como perdido
    EXPIRADO --> REPLICADO : Replicar orçamento
    FECHADO --> [*]
    PERDIDO --> [*]
    REPLICADO --> [*]
```

#### Venda (Cabeçalho)

```mermaid
stateDiagram-v2
    [*] --> ATIVO : Criar venda
    ATIVO --> ENTREGUE : Todos itens entregues
    ATIVO --> CANCELADO : Cancelar venda
    ENTREGUE --> DEVOLVIDO : Devolução total
    ENTREGUE --> [*]
    CANCELADO --> [*]
    DEVOLVIDO --> [*]
```

#### Item da Venda (venda_has_produto2)

```mermaid
stateDiagram-v2
    [*] --> INICIADO

    INICIADO --> EM_COMPRA : Gerar pedido
    INICIADO --> ESTOQUE : Já tem estoque
    INICIADO --> CANCELADO : Cancelar

    EM_COMPRA --> EM_FATURAMENTO : Fornecedor confirma
    EM_FATURAMENTO --> EM_ENTREGA : NFe recebida
    EM_ENTREGA --> EM_COLETA : Coleta
    EM_COLETA --> EM_RECEBIMENTO : Chegou armazém
    EM_RECEBIMENTO --> ESTOQUE : Recebido

    ESTOQUE --> ENTREGA_AGEND : Agendar entrega
    ENTREGA_AGEND --> EM_ENTREGA_CLIENTE : Saiu para entrega
    EM_ENTREGA_CLIENTE --> ENTREGUE : Entregue

    ENTREGUE --> DEVOLVIDO : Devolução

    CANCELADO --> [*]
    DEVOLVIDO --> [*]
    ENTREGUE --> [*]
```

### Regras de Precificação

O sistema suporta **3 níveis de desconto**:

```text
┌─────────────────────────────────────────────────────────────┐
│                    CÁLCULO DE PREÇO                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. PREÇO UNITÁRIO (prcUnitario)                           │
│     └── Preço base do produto                              │
│                                                             │
│  2. DESCONTO POR ITEM (desconto %)                         │
│     └── Aplicado sobre preço unitário                      │
│     └── parcial = quant × prcUnitario × (1 - desconto)     │
│                                                             │
│  3. DESCONTO GLOBAL (descontoPorc % ou descontoReais)      │
│     └── Aplicado proporcionalmente sobre todos itens       │
│     └── total = subTotalLiq × (1 - descontoGlobal) + frete │
│                                                             │
│  CÁLCULO FINAL:                                             │
│  subTotalBru = Σ(quant × prcUnitario)                      │
│  subTotalLiq = Σ(parcial após desconto item)               │
│  total = subTotalLiq - descontoGlobal + frete              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Funcionalidades Especiais

#### Semáforo de Orçamento

```text
🔴 FRIO    - Baixa probabilidade de fechamento
🟡 MORNO   - Média probabilidade
🟢 QUENTE  - Alta probabilidade de fechamento
```

#### Representação

Flag `representacao` indica venda via representante (comissão diferenciada).

#### RT (Comissão)

Flag `checkBoxRT` indica se venda gera comissão para profissional indicador.

---

## Implementação Laravel (V2 - Simplified)

### Models

```php
// app/Models/Orcamento.php
class Orcamento extends Model
{
    protected $fillable = [
        'loja_id', 'cliente_id', 'vendedor_id', 'profissional_id',
        'endereco_entrega_id', 'endereco_faturamento_id',
        'status', 'semaforo', 'validade',
        'subtotal_bruto', 'subtotal_liquido',
        'desconto_percentual', 'desconto_reais',
        'frete', 'frete_manual', 'total',
        'prazo_entrega', 'representacao', 'observacoes',
    ];

    protected $casts = [
        'status' => OrcamentoStatus::class,
        'semaforo' => SemaforoOrcamento::class,
        'validade' => 'date',
        'representacao' => 'boolean',
        'frete_manual' => 'boolean',
    ];

    public function cliente(): BelongsTo
    {
        return $this->belongsTo(Cliente::class);
    }

    public function vendedor(): BelongsTo
    {
        return $this->belongsTo(Usuario::class, 'vendedor_id');
    }

    public function profissional(): BelongsTo
    {
        return $this->belongsTo(Profissional::class);
    }

    public function enderecoEntrega(): BelongsTo
    {
        return $this->belongsTo(ClienteEndereco::class, 'endereco_entrega_id');
    }

    public function itens(): HasMany
    {
        return $this->hasMany(OrcamentoItem::class);
    }

    public function venda(): HasOne
    {
        return $this->hasOne(Venda::class);
    }

    public function replicadoDe(): BelongsTo
    {
        return $this->belongsTo(Orcamento::class, 'replicado_de_id');
    }
}

// app/Models/Venda.php
class Venda extends Model
{
    protected $fillable = [
        'loja_id', 'orcamento_id', 'cliente_id', 'vendedor_id', 'profissional_id',
        'endereco_entrega_id', 'endereco_faturamento_id',
        'status',
        'subtotal_bruto', 'subtotal_liquido',
        'desconto_percentual', 'desconto_reais',
        'frete', 'frete_manual', 'total',
        'prazo_entrega', 'representacao', 'rt', 'observacoes',
    ];

    protected $casts = [
        'status' => VendaStatus::class,
        'representacao' => 'boolean',
        'rt' => 'boolean',
        'frete_manual' => 'boolean',
    ];

    public function orcamento(): BelongsTo
    {
        return $this->belongsTo(Orcamento::class);
    }

    public function cliente(): BelongsTo
    {
        return $this->belongsTo(Cliente::class);
    }

    public function itens(): HasMany
    {
        return $this->hasMany(VendaItem::class);
    }

    public function parcelasReceber(): HasMany
    {
        return $this->hasMany(FinanceiroParcela::class)
            ->where('tipo', FinanceiroTipo::RECEBER);
    }

    public function entregas(): HasMany
    {
        return $this->hasMany(Entrega::class);
    }
}

// app/Models/VendaItem.php (Flat structure - no N2 level)
class VendaItem extends Model
{
    protected $table = 'venda_itens';

    protected $fillable = [
        'venda_id', 'posicao', 'produto_id', 'fornecedor_id',
        'descricao_produto', 'codigo_produto',
        'quantidade', 'quantidade_caixas', 'unidade',
        'valor_unitario', 'desconto_item_percentual', 'valor_total',
        'origem', 'status',
        'observacoes', 'orcamento_item_id', 'compra_item_id',
    ];

    protected $casts = [
        'origem' => VendaItemOrigem::class,
        'status' => VendaItemStatus::class,
        'quantidade' => 'decimal:4',
        'quantidade_caixas' => 'decimal:4',
        'valor_unitario' => 'decimal:4',
        'desconto_item_percentual' => 'decimal:2',
        'valor_total' => 'decimal:2',
    ];

    public function venda(): BelongsTo
    {
        return $this->belongsTo(Venda::class);
    }

    public function produto(): BelongsTo
    {
        return $this->belongsTo(Produto::class);
    }

    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }

    // === RELATIONSHIPS ===

    // M:N allocation relationship (venda_items ↔ estoque_lotes)
    public function alocacoes(): HasMany
    {
        return $this->hasMany(Alocacao::class, 'venda_item_id');
    }

    // Deliveries containing this item (via entrega_itens)
    public function entregas(): BelongsToMany
    {
        return $this->belongsToMany(
            Entrega::class,
            'entrega_itens',
            'venda_item_id',
            'entrega_id'
        )
        ->withPivot(['quantidade', 'status']);
    }

    // Link to quotation item (if originated from quote)
    public function orcamentoItem(): BelongsTo
    {
        return $this->belongsTo(OrcamentoItem::class)->nullable();
    }

    // Link to purchase item (if originated from purchase)
    // Used when origem = VendaItemOrigem::COMPRA
    public function compraItem(): BelongsTo
    {
        return $this->belongsTo(CompraItem::class)->nullable();
    }

    // Polymorphic origin: Can be from ORCAMENTO or COMPRA
    // origem field distinguishes: VendaItemOrigem::ORCAMENTO or VendaItemOrigem::COMPRA
    // Use getOrigemModel() helper to get the actual source model

    // Event Sourcing: Historical audit trail
    public function eventos(): HasMany
    {
        return $this->hasMany(VendaItemEvento::class, 'venda_item_id');
    }

    // === HELPERS & CALCULATIONS ===

    /**
     * Get the source model based on origem field
     * Returns either OrcamentoItem or CompraItem model
     */
    public function getOrigemModel()
    {
        return match ($this->origem) {
            VendaItemOrigem::ORCAMENTO => $this->orcamentoItem,
            VendaItemOrigem::COMPRA => $this->compraItem,
            default => null,
        };
    }

    // Calculate total allocated quantity from ATIVO allocations
    public function quantidadeAlocada(): float
    {
        return $this->alocacoes()
            ->where('status', AlocacaoStatus::ATIVO)
            ->sum('quantidade') ?? 0;
    }

    // Calculate remaining unallocated quantity
    public function quantidadePendente(): float
    {
        return max(0, $this->quantidade - $this->quantidadeAlocada());
    }

    // Check if fully allocated
    public function fullyAllocated(): bool
    {
        return $this->quantidadePendente() == 0;
    }

    // Calculate total delivered quantity
    public function quantidadeEntregue(): float
    {
        return $this->entregas()
            ->wherePivot('status', EntregaItemStatus::ENTREGUE)
            ->sum('entrega_itens.quantidade') ?? 0;
    }

    // Check if fully delivered
    public function fullyDelivered(): bool
    {
        return $this->quantidadeEntregue() >= $this->quantidade;
    }
}
```

### Event Sourcing (Append-Only Audit Trail)

This module uses Event Sourcing with append-only events tables for complete audit trail and state reconstruction of sales transactions.

#### Event Tables

```sql
-- Append-only events table for venda_itens state changes
CREATE TABLE venda_itens_events (
    id BIGSERIAL PRIMARY KEY,
    venda_item_id BIGINT NOT NULL,
    event_type VARCHAR(50) NOT NULL,               -- CRIADO, ALOCADO, ENTREGUE, CANCELADO, etc.
    event_data JSONB NOT NULL,                     -- Complete event payload
    usuario_id BIGINT,
    ip_address INET,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

-- Immutability constraint
CREATE TRIGGER fn_prevent_mutation_venda_itens_events
BEFORE UPDATE OR DELETE ON venda_itens_events
FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();

-- Index for query performance
CREATE INDEX idx_venda_itens_events_item_tipo
ON venda_itens_events (venda_item_id, event_type, created_at);

-- Append-only events table for allocation changes
CREATE TABLE alocacoes_events (
    id BIGSERIAL PRIMARY KEY,
    alocacao_id BIGINT NOT NULL,
    event_type VARCHAR(50) NOT NULL,               -- CRIADA, REVERTIDA, CANCELADA
    event_data JSONB NOT NULL,
    usuario_id BIGINT,
    ip_address INET,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TRIGGER fn_prevent_mutation_alocacoes_events
BEFORE UPDATE OR DELETE ON alocacoes_events
FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();

CREATE INDEX idx_alocacoes_events_alocacao_tipo
ON alocacoes_events (alocacao_id, event_type, created_at);
```

#### Event Types

```php
// app/Enums/VendaItemEventType.php
enum VendaItemEventType: string
{
    case CRIADO = 'CRIADO';                        // Item added to venda
    case VALOR_ALTERADO = 'VALOR_ALTERADO';        // Unit price or quantity changed
    case DESCONTO_APLICADO = 'DESCONTO_APLICADO';
    case ORIGEM_ALTERADA = 'ORIGEM_ALTERADA';      // Origem changed (COMPRA → ESTOQUE)
    case ALOCADO = 'ALOCADO';                      // Fully allocated
    case PARCIALMENTE_ALOCADO = 'PARCIALMENTE_ALOCADO';
    case ENTREGUE = 'ENTREGUE';
    case CANCELADO = 'CANCELADO';
    case DEVOLVIDO = 'DEVOLVIDO';
}

// app/Enums/AlocacaoEventType.php
enum AlocacaoEventType: string
{
    case CRIADA = 'CRIADA';                        // Allocation created
    case REVERTIDA = 'REVERTIDA';                  // Allocation reversed (breakage/return)
    case CANCELADA = 'CANCELADA';                  // Allocation canceled
}
```

#### Event Recording in Services

```php
// app/Services/Vendas/VendaService.php
class VendaService
{
    public function adicionarItem(Venda $venda, array $itemData): VendaItem
    {
        return DB::transaction(function () use ($venda, $itemData) {
            $item = $venda->itens()->create($itemData);

            // Record CRIADO event
            DB::table('venda_itens_events')->insert([
                'venda_item_id' => $item->id,
                'event_type' => VendaItemEventType::CRIADO->value,
                'event_data' => json_encode([
                    'produto_id' => $item->produto_id,
                    'quantidade' => $item->quantidade,
                    'valor_unitario' => $item->valor_unitario,
                    'origem' => $item->origem,
                    'status' => $item->status,
                ]),
                'usuario_id' => auth()->id(),
                'ip_address' => request()->ip(),
                'created_at' => now(),
            ]);

            event(new VendaItemAdicionado($item));

            return $item;
        });
    }

    public function cancelarItem(VendaItem $item, string $motivo): void
    {
        DB::transaction(function () use ($item, $motivo) {
            // Record allocation reversals first
            $item->alocacoes()
                ->where('status', AlocacaoStatus::ATIVO)
                ->update(['status' => AlocacaoStatus::REVERTIDA]);

            foreach ($item->alocacoes as $alocacao) {
                DB::table('alocacoes_events')->insert([
                    'alocacao_id' => $alocacao->id,
                    'event_type' => AlocacaoEventType::REVERTIDA->value,
                    'event_data' => json_encode([
                        'motivo' => 'Item cancelado: ' . $motivo,
                        'quantidade_revertida' => $alocacao->quantidade,
                    ]),
                    'usuario_id' => auth()->id(),
                    'ip_address' => request()->ip(),
                    'created_at' => now(),
                ]);
            }

            // Then record item cancellation
            DB::table('venda_itens_events')->insert([
                'venda_item_id' => $item->id,
                'event_type' => VendaItemEventType::CANCELADO->value,
                'event_data' => json_encode([
                    'motivo' => $motivo,
                    'quantidade_cancelada' => $item->quantidade,
                    'alocacoes_revertidas' => $item->alocacoes()->count(),
                ]),
                'usuario_id' => auth()->id(),
                'ip_address' => request()->ip(),
                'created_at' => now(),
            ]);

            $item->update(['status' => VendaItemStatus::CANCELADO]);

            event(new VendaItemCancelado($item));
        });
    }
}

// app/Services/Estoque/AlocacaoService.php
class AlocacaoService
{
    public function alocar(int $vendaItemId, int $estoqueLoteId, float $quantidade): Alocacao
    {
        return DB::transaction(function () use ($vendaItemId, $estoqueLoteId, $quantidade) {
            $alocacao = Alocacao::create([
                'venda_item_id' => $vendaItemId,
                'estoque_lote_id' => $estoqueLoteId,
                'quantidade' => $quantidade,
                'status' => AlocacaoStatus::ATIVO,
            ]);

            // Record CRIADA event
            DB::table('alocacoes_eventos')->insert([
                'alocacao_id' => $alocacao->id,
                'event_type' => AlocacaoEventType::CRIADA->value,
                'event_data' => json_encode([
                    'venda_item_id' => $vendaItemId,
                    'estoque_lote_id' => $estoqueLoteId,
                    'quantidade' => $quantidade,
                    'custo_unitario' => $alocacao->estoqueLote->custo_unitario,
                    'valor_total' => $quantidade * $alocacao->estoqueLote->custo_unitario,
                ]),
                'usuario_id' => auth()->id(),
                'ip_address' => request()->ip(),
                'created_at' => now(),
            ]);

            // Update venda_item allocation status
            DB::table('venda_itens_events')->insert([
                'venda_item_id' => $vendaItemId,
                'event_type' => VendaItemEventType::ALOCADO->value,
                'event_data' => json_encode([
                    'alocacao_id' => $alocacao->id,
                    'quantidade_alocada' => $quantidade,
                ]),
                'usuario_id' => auth()->id(),
                'ip_address' => request()->ip(),
                'created_at' => now(),
            ]);

            event(new AlocacaoCriada($alocacao));

            return $alocacao;
        });
    }
}
```

#### Audit Trail Queries

```php
// Get complete history of a venda_item
$historia = DB::table('venda_itens_events')
    ->where('venda_item_id', $itemId)
    ->orderBy('created_at')
    ->get();

// Get all allocation events for an item
$alocacoes_historia = DB::table('alocacoes_events')
    ->whereIn('alocacao_id', function ($query) use ($itemId) {
        $query->select('id')->from('alocacoes')
            ->where('venda_item_id', $itemId);
    })
    ->orderBy('created_at')
    ->get();

// Reconstruct state at specific timestamp
$estado_em_data = DB::table('venda_itens_events')
    ->where('venda_item_id', $itemId)
    ->where('created_at', '<=', $data)
    ->orderBy('created_at', 'desc')
    ->first();
```

#### Key Benefits

- **Complete Audit Trail**: Every change is recorded with user, timestamp, and IP
- **Immutable History**: Cannot modify or delete events (fn_prevent_mutation enforced)
- **State Reconstruction**: Can replay events to see state at any point in time
- **Compliance**: Meets regulatory requirements for transaction audit logs
- **Debugging**: Trace exact sequence of allocation and delivery events
- **Analytics**: Query event log for insights (e.g., most frequently canceled items)

---

### Enums

```php
// app/Enums/OrcamentoStatus.php
enum OrcamentoStatus: string
{
    case ATIVO = 'ATIVO';
    case FECHADO = 'FECHADO';
    case EXPIRADO = 'EXPIRADO';
    case PERDIDO = 'PERDIDO';
    case REPLICADO = 'REPLICADO';

    public function label(): string
    {
        return match($this) {
            self::ATIVO => 'Ativo',
            self::FECHADO => 'Fechado',
            self::EXPIRADO => 'Expirado',
            self::PERDIDO => 'Perdido',
            self::REPLICADO => 'Replicado',
        };
    }

    public function color(): string
    {
        return match($this) {
            self::ATIVO => 'green',
            self::FECHADO => 'blue',
            self::EXPIRADO => 'gray',
            self::PERDIDO => 'red',
            self::REPLICADO => 'purple',
        };
    }

    public function canConvertToSale(): bool
    {
        return $this === self::ATIVO;
    }
}

// app/Enums/VendaItemStatus.php
enum VendaItemStatus: string
{
    case INICIADO = 'INICIADO';
    case PENDENTE = 'PENDENTE';
    case EM_COMPRA = 'EM COMPRA';
    case EM_FATURAMENTO = 'EM FATURAMENTO';
    case EM_ENTREGA = 'EM ENTREGA';
    case EM_COLETA = 'EM COLETA';
    case EM_RECEBIMENTO = 'EM RECEBIMENTO';
    case ESTOQUE = 'ESTOQUE';
    case ENTREGA_AGENDADA = 'ENTREGA AGEND.';
    case ENTREGUE = 'ENTREGUE';
    case CANCELADO = 'CANCELADO';
    case DEVOLVIDO = 'DEVOLVIDO';

    public function canTransitionTo(self $new): bool
    {
        return match($this) {
            self::INICIADO => in_array($new, [
                self::EM_COMPRA, self::ESTOQUE, self::CANCELADO
            ]),
            self::EM_COMPRA => in_array($new, [
                self::EM_FATURAMENTO, self::CANCELADO
            ]),
            self::EM_FATURAMENTO => in_array($new, [
                self::EM_ENTREGA, self::CANCELADO
            ]),
            self::EM_ENTREGA => in_array($new, [
                self::EM_COLETA, self::EM_RECEBIMENTO, self::CANCELADO
            ]),
            self::EM_COLETA => in_array($new, [
                self::EM_RECEBIMENTO
            ]),
            self::EM_RECEBIMENTO => in_array($new, [
                self::ESTOQUE
            ]),
            self::ESTOQUE => in_array($new, [
                self::ENTREGA_AGENDADA, self::CANCELADO
            ]),
            self::ENTREGA_AGENDADA => in_array($new, [
                self::EM_ENTREGA, self::CANCELADO
            ]),
            self::ENTREGUE => in_array($new, [
                self::DEVOLVIDO
            ]),
            self::CANCELADO, self::DEVOLVIDO => false,
            default => false,
        };
    }
}

// app/Enums/SemaforoOrcamento.php
enum SemaforoOrcamento: string
{
    case FRIO = 'FRIO';
    case MORNO = 'MORNO';
    case QUENTE = 'QUENTE';

    public function emoji(): string
    {
        return match($this) {
            self::FRIO => '🔴',
            self::MORNO => '🟡',
            self::QUENTE => '🟢',
        };
    }
}
```

## Service Layer Architecture

### Overview

The service layer is organized by **module boundaries** with **event-driven decoupling**:

```
┌─────────────────────────────────────────────────────────────┐
│ Presentation Layer (Controllers)                            │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ Application Layer (Services)                                │
│                                                              │
│  VendaService (Vendas module owner)                         │
│  ├── AlocacaoService (Estoque module owner)                 │
│  ├── FinanceiroParcelaService (Financeiro module owner)     │
│  └── EntregaService (Logística module owner)                │
│                                                              │
│  ⚠️ Direct dependencies create TIGHT COUPLING               │
│  Solution: Use EVENTS for cross-module communication        │
│                                                              │
└─────────────────────────┬───────────────────────────────────┘
                          │ EVENTS
┌─────────────────────────▼───────────────────────────────────┐
│ Event Listeners (Decoupled handlers)                        │
│                                                              │
│  VendaCriada → Create financial accounts                    │
│  VendaCriada → Initialize allocation                        │
│  AlocacaoCriada → Update inventory                          │
│  EntregueConfirmada → Generate invoice                      │
│                                                              │
└─────────────────────────┬───────────────────────────────────┘
                          │
┌─────────────────────────▼───────────────────────────────────┐
│ Domain Models & Database                                    │
└─────────────────────────────────────────────────────────────┘
```

### Service Boundaries

**Module Owners** (each module owns its service):

| Module     | Service                | Responsibilities |
| ---------- | ---------------------- | --------------- |
| Vendas     | VendaService, OrcamentoService | Quote/Sales management, item tracking |
| Estoque    | EstoqueService, AlocacaoService | Inventory batches, allocation logic, FIFO/FEFO |
| Financeiro | FinanceiroParcelaService | Accounts receivable/payable, CNAB processing |
| Logística  | EntregaService | Delivery scheduling, route optimization |

### Avoiding Tight Coupling

**❌ ANTI-PATTERN (DO NOT DO):**
```php
class VendaService {
    public function __construct(
        private EstoqueService $estoqueService,              // ⚠️ Creates dependency
        private FinanceiroParcelaService $financeiroService, // ⚠️ Creates dependency
        private EntregaService $entregaService,             // ⚠️ Creates dependency
    ) {}

    public function criar($dados) {
        $venda = Venda::create($dados);

        // ⚠️ Direct calls across modules
        $this->estoqueService->reservar($venda);
        $this->financeiroService->gerarParcelas($venda, FinanceiroTipo::RECEBER);
        $this->entregaService->agendar($venda);

        return $venda;
    }
}
```

**✅ SOLUTION (USE EVENTS):**
```php
class VendaService {
    public function criar($dados) {
        $venda = Venda::create($dados);

        // ✅ Emit event - let other modules subscribe
        event(new VendaCriada($venda));

        return $venda;
    }
}

// In separate module listeners:
class GerarParcelaRecebidaListener {
    public function __construct(private FinanceiroParcelaService $financeiroService) {}

    public function handle(VendaCriada $event) {
        $this->financeiroService->gerarParcelas($event->venda, FinanceiroTipo::RECEBER);
    }
}

class ReservarEstoqueListener {
    public function handle(VendaCriada $event) {
        $this->estoqueService->reservar($event->venda);
    }
}
```

### Dependency Direction

Services should **depend inward** toward the domain:

```
Controllers
    ↓
Services (should NOT depend on each other)
    ↓
Models / Events
    ↓
Database
```

**✅ GOOD**: Service calls Service in same module or Models
**⚠️ OK**: Service publishes Event that other modules subscribe to
**❌ BAD**: Service injects Service from different module

### Cross-Module Communication Pattern

When services need to communicate across module boundaries:

1. **Publisher**: Module A publishes an Event
2. **Event**: Contains all necessary data for subscribers
3. **Listeners**: Module B subscribes to event via Listener
4. **Handler**: Listener calls Module B's service independently

Example flow:
```
VendaService::criar()
    └─> event(new VendaCriada($venda))
        ├─> Listener 1: CreateFinancialAccountsListener
        │   └─> FinanceiroParcelaService::gerarDeVenda()
        ├─> Listener 2: InitializeAllocationListener
        │   └─> AlocacaoService::inicializar()
        └─> Listener 3: ScheduleDeliveryListener
            └─> EntregaService::agendar()
```

### Services

```php
// app/Services/Vendas/OrcamentoService.php
class OrcamentoService
{
    public function __construct(
        private FreteService $freteService,
    ) {}

    /**
     * Criar novo orçamento
     */
    public function criar(array $dados): Orcamento
    {
        return DB::transaction(function () use ($dados) {
            $orcamento = Orcamento::create([
                'loja_id' => $dados['loja_id'],
                'cliente_id' => $dados['cliente_id'],
                'vendedor_id' => $dados['vendedor_id'] ?? auth()->id(),
                'profissional_id' => $dados['profissional_id'] ?? null,
                'endereco_entrega_id' => $dados['endereco_entrega_id'],
                'status' => OrcamentoStatus::ATIVO,
                'semaforo' => SemaforoOrcamento::MORNO,
                'validade' => now()->addDays(30),
            ]);

            foreach ($dados['itens'] as $item) {
                $this->adicionarItem($orcamento, $item);
            }

            $this->recalcularTotais($orcamento);

            return $orcamento;
        });
    }

    /**
     * Adicionar item ao orçamento
     */
    public function adicionarItem(Orcamento $orcamento, array $item): OrcamentoItem
    {
        $produto = Produto::findOrFail($item['produto_id']);

        return $orcamento->itens()->create([
            'produto_id' => $produto->id,
            'fornecedor_id' => $item['fornecedor_id'] ?? $produto->fornecedor_padrao_id,
            'quantidade' => $item['quantidade'],
            'preco_unitario' => $item['preco_unitario'] ?? $produto->preco_venda,
            'desconto_percentual' => $item['desconto'] ?? 0,
            'descricao_produto' => $produto->descricao,
            'unidade' => $produto->unidade,
            'codigo_comercial' => $produto->codigo_comercial,
        ]);
    }

    /**
     * Recalcular totais do orçamento
     */
    public function recalcularTotais(Orcamento $orcamento): void
    {
        $subtotalBruto = $orcamento->itens->sum(fn($item) =>
            $item->quantidade * $item->preco_unitario
        );

        $subtotalLiquido = $orcamento->itens->sum(fn($item) =>
            $item->quantidade * $item->preco_unitario * (1 - $item->desconto_percentual / 100)
        );

        // Calcular frete automaticamente se não for manual
        $frete = $orcamento->frete_manual
            ? $orcamento->frete
            : $this->freteService->calcular($orcamento);

        $descontoGlobal = $orcamento->desconto_percentual > 0
            ? $subtotalLiquido * $orcamento->desconto_percentual / 100
            : $orcamento->desconto_reais;

        $total = $subtotalLiquido - $descontoGlobal + $frete;

        $orcamento->update([
            'subtotal_bruto' => $subtotalBruto,
            'subtotal_liquido' => $subtotalLiquido,
            'frete' => $frete,
            'total' => $total,
        ]);
    }

    /**
     * Replicar orçamento expirado
     */
    public function replicar(Orcamento $orcamento): Orcamento
    {
        return DB::transaction(function () use ($orcamento) {
            $novo = $orcamento->replicate([
                'status', 'created_at', 'updated_at'
            ]);
            $novo->status = OrcamentoStatus::ATIVO;
            $novo->validade = now()->addDays(30);
            $novo->replicado_de_id = $orcamento->id;
            $novo->save();

            foreach ($orcamento->itens as $item) {
                $novoItem = $item->replicate();
                $novoItem->orcamento_id = $novo->id;
                $novoItem->save();
            }

            $orcamento->update(['status' => OrcamentoStatus::REPLICADO]);

            return $novo;
        });
    }
}

// app/Services/Vendas/VendaService.php
// ✅ DECOUPLED SERVICE - No cross-module dependencies
class VendaService
{
    public function __construct() {
        // ✅ No injected services from other modules
        // Cross-module concerns handled via event listeners
    }

    /**
     * Converter orçamento em venda
     */
    public function criarDeOrcamento(Orcamento $orcamento): Venda
    {
        $this->validarConversao($orcamento);

        return DB::transaction(function () use ($orcamento) {
            // Criar cabeçalho da venda
            $venda = Venda::create([
                'loja_id' => $orcamento->loja_id,
                'orcamento_id' => $orcamento->id,
                'cliente_id' => $orcamento->cliente_id,
                'vendedor_id' => $orcamento->vendedor_id,
                'profissional_id' => $orcamento->profissional_id,
                'endereco_entrega_id' => $orcamento->endereco_entrega_id,
                'endereco_faturamento_id' => $orcamento->endereco_faturamento_id,
                'status' => VendaStatus::ATIVO,
                'status_financeiro' => VendaStatusFinanceiro::PENDENTE,
                'subtotal_bruto' => $orcamento->subtotal_bruto,
                'subtotal_liquido' => $orcamento->subtotal_liquido,
                'desconto_percentual' => $orcamento->desconto_percentual,
                'desconto_reais' => $orcamento->desconto_reais,
                'frete' => $orcamento->frete,
                'total' => $orcamento->total,
                'prazo_entrega' => $orcamento->prazo_entrega,
                'representacao' => $orcamento->representacao,
            ]);

            // Copiar itens de orçamento para venda (flat structure)
            foreach ($orcamento->itens as $orcItem) {
                $vendaItem = $venda->itens()->create([
                    'produto_id' => $orcItem->produto_id,
                    'fornecedor_id' => $orcItem->fornecedor_id,
                    'quantidade' => $orcItem->quantidade,
                    'preco_unitario' => $orcItem->preco_unitario,
                    'desconto_percentual' => $orcItem->desconto_percentual,
                    'desconto_global_percentual' => $orcamento->desconto_percentual,
                    'subtotal' => $orcItem->subtotal,
                    'total' => $orcItem->total,
                    'descricao_produto' => $orcItem->descricao_produto,
                    'unidade' => $orcItem->unidade,
                ]);

                // Initialize delivery/fulfillment (allocations handled separately)
                $this->inicializarAtendimento($venda, $vendaItem);
            }

            // Fechar orçamento
            $orcamento->update(['status' => OrcamentoStatus::FECHADO]);

            event(new VendaCriada($venda));

            return $venda;
        });
    }

    /**
     * Inicializar atendimento/fulfillment
     * ✅ DECOUPLED: No EstoqueService dependency
     * NEW SCHEMA: Allocations (alocacoes) created via separate listeners/services
     * This method: Default to INICIADO status, let event listeners check stock
     */
    private function inicializarAtendimento(Venda $venda, VendaItem $vendaItem): void
    {
        // ✅ Set default status - availability will be determined by listeners
        $status = VendaItemStatus::INICIADO;

        // Atualizar status do item de venda
        $vendaItem->update([
            'status' => $status,
        ]);

        // Event Sourcing: Registrar evento de criação
        DB::table('venda_itens_events')->insert([
            'venda_item_id' => $vendaItem->id,
            'event_type' => 'CRIADO',
            'event_data' => json_encode([
                'quantidade' => $vendaItem->quantidade,
                'produto_id' => $vendaItem->produto_id,
            ]),
            'created_at' => now(),
        ]);

        // ✅ Emit event - let EstoqueService listener check availability
        event(new VendaItemCriado($vendaItem, $venda));
    }

    /**
     * Cancelar venda
     */
    public function cancelar(Venda $venda, string $motivo): void
    {
        DB::transaction(function () use ($venda, $motivo) {
            // Cancelar todos os itens de atendimento
            foreach ($venda->itensAtendimento as $item) {
                $this->cancelarItemAtendimento($item);
            }

            // Cancelar parcelas a receber
            $venda->parcelasReceber()->update([
                'status' => FinanceiroStatus::CANCELADO,
            ]);

            // Atualizar venda
            $venda->update([
                'status' => VendaStatus::CANCELADO,
                'motivo_cancelamento' => $motivo,
            ]);

            // Reativar orçamento
            if ($venda->orcamento) {
                $venda->orcamento->update([
                    'status' => OrcamentoStatus::ATIVO,
                ]);
            }

            event(new VendaCancelada($venda));
        });
    }

    /**
     * Cancelar item de atendimento específico
     */
    private function cancelarItemAtendimento(VendaItemAtendimento $item): void
    {
        // Desfazer consumo de estoque
        $this->estoqueService->desfazerConsumo($item);

        // Desvincular de compra
        if ($item->compra_id) {
            $item->compraItem?->update([
                'venda_id' => null,
                'venda_item_atendimento_id' => null,
            ]);
        }

        $item->update([
            'status' => VendaItemStatus::CANCELADO,
            'compra_id' => null,
            'lote' => null,
        ]);
    }

    private function validarConversao(Orcamento $orcamento): void
    {
        if (!$orcamento->status->canConvertToSale()) {
            throw new BusinessException(
                "Orçamento com status {$orcamento->status->label()} não pode ser convertido"
            );
        }

        if (!$orcamento->endereco_entrega_id) {
            throw new BusinessException('Endereço de entrega é obrigatório');
        }

        if ($orcamento->itens->isEmpty()) {
            throw new BusinessException('Orçamento não possui itens');
        }
    }
}
```

### Controllers

```php
// app/Http/Controllers/OrcamentoController.php
class OrcamentoController extends Controller
{
    public function __construct(
        private OrcamentoService $orcamentoService
    ) {}

    public function index(Request $request)
    {
        $orcamentos = Orcamento::query()
            ->with(['cliente:id,razao_social', 'vendedor:id,nome'])
            ->when($request->status, fn($q) => $q->where('status', $request->status))
            ->when($request->semaforo, fn($q) => $q->where('semaforo', $request->semaforo))
            ->when($request->vendedor_id, fn($q) => $q->where('vendedor_id', $request->vendedor_id))
            ->when($request->search, fn($q) => $q->whereHas('cliente', fn($q) =>
                $q->where('razao_social', 'like', "%{$request->search}%")
            ))
            ->latest()
            ->paginate(20);

        return Inertia::render('Orcamentos/Index', [
            'orcamentos' => $orcamentos,
            'filters' => $request->only(['status', 'semaforo', 'vendedor_id', 'search']),
        ]);
    }

    public function store(CriarOrcamentoRequest $request)
    {
        $orcamento = $this->orcamentoService->criar($request->validated());

        return redirect()->route('orcamentos.show', $orcamento)
            ->with('success', 'Orçamento criado com sucesso');
    }

    public function gerarVenda(Orcamento $orcamento, VendaService $vendaService)
    {
        $venda = $vendaService->criarDeOrcamento($orcamento);

        return redirect()->route('vendas.show', $venda)
            ->with('success', 'Venda gerada com sucesso');
    }

    public function replicar(Orcamento $orcamento)
    {
        $novo = $this->orcamentoService->replicar($orcamento);

        return redirect()->route('orcamentos.show', $novo)
            ->with('success', 'Orçamento replicado com sucesso');
    }
}

// app/Http/Controllers/VendaController.php
class VendaController extends Controller
{
    public function __construct(
        private VendaService $vendaService
    ) {}

    public function index(Request $request)
    {
        $vendas = Venda::query()
            ->with(['cliente:id,razao_social', 'vendedor:id,nome'])
            ->when($request->status, fn($q) => $q->where('status', $request->status))
            ->when($request->cliente_id, fn($q) => $q->where('cliente_id', $request->cliente_id))
            ->when($request->periodo, function($q) use ($request) {
                [$inicio, $fim] = explode(',', $request->periodo);
                $q->whereBetween('created_at', [$inicio, $fim]);
            })
            ->latest()
            ->paginate(20);

        return Inertia::render('Vendas/Index', [
            'vendas' => $vendas,
            'filters' => $request->only(['status', 'cliente_id', 'periodo']),
        ]);
    }

    public function show(Venda $venda)
    {
        $venda->load([
            'cliente',
            'vendedor',
            'profissional',
            'enderecoEntrega',
            'itens.produto',
            'itensAtendimento.compra',
            'parcelasReceber',
        ]);

        return Inertia::render('Vendas/Show', [
            'venda' => $venda,
        ]);
    }

    public function cancelar(Venda $venda, CancelarVendaRequest $request)
    {
        $this->vendaService->cancelar($venda, $request->motivo);

        return back()->with('success', 'Venda cancelada com sucesso');
    }
}
```

### Rotas

```php
// routes/web.php
Route::middleware(['auth'])->group(function () {
    // Orçamentos
    Route::resource('orcamentos', OrcamentoController::class);
    Route::post('orcamentos/{orcamento}/gerar-venda', [OrcamentoController::class, 'gerarVenda'])
        ->name('orcamentos.gerar-venda');
    Route::post('orcamentos/{orcamento}/replicar', [OrcamentoController::class, 'replicar'])
        ->name('orcamentos.replicar');
    Route::post('orcamentos/{orcamento}/marcar-perdido', [OrcamentoController::class, 'marcarPerdido'])
        ->name('orcamentos.marcar-perdido');

    // Vendas
    Route::resource('vendas', VendaController::class);
    Route::post('vendas/{venda}/cancelar', [VendaController::class, 'cancelar'])
        ->name('vendas.cancelar');
    Route::get('vendas/{venda}/pdf', [VendaController::class, 'gerarPdf'])
        ->name('vendas.pdf');
    Route::get('vendas/{venda}/excel', [VendaController::class, 'gerarExcel'])
        ->name('vendas.excel');
});
```

---

## Componentes de UI

### Lista de Orçamentos

- Filtros: Status, Semáforo, Vendedor, Período, Busca
- Colunas: ID, Cliente, Vendedor, Total, Validade, Semáforo, Status
- Ações: Visualizar, Editar, Gerar Venda, Replicar, PDF

### Lista de Vendas

- Filtros: Status, Status Financeiro, Cliente, Vendedor, Período
- Colunas: ID, Cliente, Vendedor, Total, Status, Status Financeiro
- Ações: Visualizar, PDF, Excel, Cancelar

### Formulário de Orçamento

- Seleção de cliente (com cadastro rápido)
- Seleção de endereço de entrega
- Seleção de profissional (opcional)
- Tabela de itens com:
  - Busca de produto
  - Quantidade, preço, desconto por item
  - Subtotal automático
- Frete (automático ou manual)
- Desconto global (% ou R$)
- Total calculado
- Observações

### Visualização de Venda

- Cabeçalho com dados do cliente e endereços
- Timeline de status
- Tabela de itens com status individual
- Aba de pagamentos (formas de pagamento)
- Aba de documentos (NFes vinculadas)
- Botões de ação baseados no status

---

## Eventos

| Evento              | Dispara                                               |
| ------------------- | ----------------------------------------------------- |
| `OrcamentoCriado`   | Notificar vendedor, log de auditoria                  |
| `VendaCriada`       | Gerar contas a receber, notificar logística           |
| `VendaCancelada`    | Reverter financeiro, reativar orçamento, notificar    |
| `VendaEntregue`     | Atualizar status, disparar faturamento se automático  |
| `ItemStatusChanged` | Atualizar status da venda pai, notificar interessados |

---

## Considerações de Migração

### Migração de Dados

**Architecture Change: Two-level (N1+N2) → Flat (items + M:N allocations)**

1. **Orçamentos**: `orcamento` → `orcamentos` (direto)
2. **Orçamento Itens**: `orcamento_has_produto` → `orcamento_itens` (normalizar fornecedor)
3. **Vendas**: `venda` → `vendas` (direto)
4. **Venda Itens**: `venda_has_produto` + `venda_has_produto2` → `venda_itens` (FLAT, no N2 splitting)
   - Merge N1 + N2 in migration to single venda_itens per product/supplier combination
   - Remove "partial division" logic - allocations handle fulfillment
5. **Allocations**: `estoque_has_consumo` → `alocacoes` (M:N, supports FIFO/FEFO)

### Mudanças Incompatíveis

- Two-level structure (N1 + N2) → Single level + M:N relationships
- `fornecedor` VARCHAR → `fornecedor_id` FK (normalização obrigatória)
- Status como strings → Status como Enum
- Old `venda_has_produto2` complex partial logic → Simple `alocacoes` M:N model

### Scripts de Migração

```sql
-- Normalizar fornecedor em venda_has_produto
UPDATE venda_has_produto vhp
SET fornecedor_id = (
    SELECT id FROM fornecedores WHERE razao_social = vhp.fornecedor LIMIT 1
)
WHERE fornecedor_id IS NULL AND fornecedor IS NOT NULL;

-- Verificar itens sem fornecedor válido
SELECT COUNT(*) FROM venda_has_produto
WHERE fornecedor IS NOT NULL AND fornecedor_id IS NULL;
```
