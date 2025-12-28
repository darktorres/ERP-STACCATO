# FIFO Stock Consumption Fix - Deep Exploration

> Status: **Analysis**
> Last updated: 2025-12-27
> Focus: Fix stock consumption to properly follow First-In-First-Out

---

## Table of Contents

1. [Current Problem](#1-current-problem)
2. [Root Cause Analysis](#2-root-cause-analysis)
3. [Proposed Solution](#3-proposed-solution)
4. [Implementation Details](#4-implementation-details)
5. [Edge Cases](#5-edge-cases)
6. [Migration Strategy](#6-migration-strategy)

---

## 1. Current Problem

### 1.1 What FIFO Means

**First-In-First-Out (FIFO)**: Oldest stock should be consumed first.

```
Received Jan 1:  100 units @ R$10.00  ← Should be consumed FIRST
Received Jan 15:  50 units @ R$12.00  ← Should be consumed SECOND
Received Jan 30:  75 units @ R$11.00  ← Should be consumed THIRD
```

### 1.2 Why It Matters

| Issue | Impact |
|-------|--------|
| **Inventory valuation** | COGS calculated incorrectly |
| **Perishable goods** | Old stock never used, expires |
| **Tax compliance** | Brazilian tax law assumes FIFO for some calcs |
| **Audit trail** | Can't trace which batch was sold |
| **Stock rotation** | Old stock accumulates |

### 1.3 Current Behavior

The system does **NOT** implement FIFO correctly:

```cpp
// venda.cpp:1046 - Current stock consumption
query.prepare(
    "SELECT p.idEstoque, vp2.idVendaProduto2, vp2.quant "
    "FROM venda_has_produto2 vp2 "
    "LEFT JOIN produto p ON vp2.idProduto = p.idProduto "
    "WHERE vp2.idVenda = :idVenda AND vp2.estoque > 0"
);
```

**Problem**: Uses `p.idEstoque` - a single pre-set stock ID stored on the `produto` table.

---

## 2. Root Cause Analysis

### 2.1 The `produto.idEstoque` Column

The `produto` table has an `idEstoque` column that:
- Points to ONE specific stock record
- Is manually set (or set by last NFe import)
- Has no FIFO logic

```sql
-- Current: produto points to a specific stock
produto (idProduto=123, idEstoque=456)
        └── estoque (idEstoque=456)  -- Just this one, regardless of age

-- Multiple stock records exist, but only one is linked
estoque (idEstoque=455, idProduto=123, data_entrada='2025-01-01')  -- OLDEST, ignored!
estoque (idEstoque=456, idProduto=123, data_entrada='2025-01-15')  -- Used (by chance)
estoque (idEstoque=457, idProduto=123, data_entrada='2025-01-30')  -- NEWEST, ignored!
```

### 2.2 When Stock is Consumed

**Path 1: Via NFe Import** (`importarxml.cpp`)
- Stock IS properly linked to specific purchase/sale
- Creates `estoque_has_consumo` with specific `idEstoque`
- This is OK - it's deterministic

**Path 2: Via Stock Sale** (`venda.cpp:criarConsumos`)
- Uses `produto.idEstoque` (the problem!)
- No ORDER BY, no FIFO selection
- Just takes whatever is pre-set

### 2.3 Code Flow

```
Customer buys product from existing stock:
    │
    ├── Sale created with vp2.estoque > 0
    │
    ├── Venda::criarConsumos() called
    │
    ├── Query: SELECT p.idEstoque ...
    │       │
    │       └── Gets SINGLE idEstoque from produto table
    │           (No FIFO consideration!)
    │
    └── Estoque::criarConsumo(idEstoque, quantity)
            │
            └── Consumes from that ONE stock record
```

### 2.4 Related Issues

1. **No automatic stock selection**: User must manually set `produto.idEstoque`
2. **One stock per product**: Can't easily consume from multiple batches
3. **No batch tracking**: Lost traceability to original NFe/lot

---

## 3. Proposed Solution

### 3.1 Overview

Replace single `produto.idEstoque` with **dynamic FIFO selection**:

```sql
-- NEW: Query oldest available stock
SELECT id, quantidade_disponivel, custo_unitario, data_entrada
FROM estoques
WHERE produto_id = :produto_id
  AND loja_id = :loja_id
  AND quantidade_disponivel > 0
ORDER BY data_entrada ASC  -- FIFO: oldest first
FOR UPDATE;  -- Lock for concurrent safety
```

### 3.2 Key Changes

| Aspect | Current | Proposed |
|--------|---------|----------|
| Stock selection | Manual via `produto.idEstoque` | Automatic FIFO |
| Multiple batches | No | Yes - consume from multiple |
| Locking | None | `FOR UPDATE` during consumption |
| Traceability | Lost | Full batch tracking |

### 3.3 New Schema

```sql
-- Remove produto.idEstoque (no longer needed)
ALTER TABLE produtos DROP COLUMN idEstoque;

-- Ensure estoque has proper indexes
CREATE INDEX idx_estoques_fifo
    ON estoques(produto_id, loja_id, data_entrada)
    WHERE quantidade_disponivel > 0;

-- Add batch/lot tracking
ALTER TABLE estoques ADD COLUMN lote VARCHAR(50);
ALTER TABLE estoques ADD COLUMN data_validade DATE;
ALTER TABLE estoques ADD COLUMN data_entrada TIMESTAMP DEFAULT NOW();
```

---

## 4. Implementation Details

### 4.1 PostgreSQL Function

```sql
CREATE OR REPLACE FUNCTION consumir_estoque_fifo(
    p_produto_id INTEGER,
    p_loja_id INTEGER,
    p_quantidade DECIMAL,
    p_venda_item_id INTEGER,
    p_motivo VARCHAR DEFAULT 'VENDA'
) RETURNS TABLE (
    estoque_id INTEGER,
    quantidade_consumida DECIMAL,
    custo_unitario DECIMAL
) AS $$
DECLARE
    v_restante DECIMAL := p_quantidade;
    v_estoque RECORD;
    v_consumir DECIMAL;
BEGIN
    -- Lock and iterate through available stock (FIFO order)
    FOR v_estoque IN
        SELECT id, quantidade_disponivel, custo_unitario
        FROM estoques
        WHERE produto_id = p_produto_id
          AND loja_id = p_loja_id
          AND quantidade_disponivel > 0
        ORDER BY data_entrada ASC  -- FIFO
        FOR UPDATE
    LOOP
        EXIT WHEN v_restante <= 0;

        -- Calculate how much to take from this batch
        v_consumir := LEAST(v_restante, v_estoque.quantidade_disponivel);

        -- Update stock
        UPDATE estoques
        SET quantidade_disponivel = quantidade_disponivel - v_consumir,
            updated_at = NOW()
        WHERE id = v_estoque.id;

        -- Create consumption record
        INSERT INTO estoque_consumos (
            estoque_id, venda_item_id, quantidade,
            custo_unitario, motivo, created_at
        ) VALUES (
            v_estoque.id, p_venda_item_id, v_consumir,
            v_estoque.custo_unitario, p_motivo, NOW()
        );

        -- Return consumed batch info
        estoque_id := v_estoque.id;
        quantidade_consumida := v_consumir;
        custo_unitario := v_estoque.custo_unitario;
        RETURN NEXT;

        v_restante := v_restante - v_consumir;
    END LOOP;

    -- Check if we fulfilled the entire request
    IF v_restante > 0 THEN
        RAISE EXCEPTION 'Estoque insuficiente. Faltam % unidades', v_restante;
    END IF;
END;
$$ LANGUAGE plpgsql;
```

### 4.2 Laravel Service

```php
<?php

namespace App\Services\Estoque;

use App\Models\Estoque;
use App\Models\EstoqueConsumo;
use App\Models\VendaItem;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\DB;

class EstoqueConsumoService
{
    /**
     * Consume stock using FIFO
     *
     * @param int $produtoId
     * @param int $lojaId
     * @param float $quantidade
     * @param int|null $vendaItemId
     * @param string $motivo
     * @return Collection<EstoqueConsumo>
     * @throws \Exception
     */
    public function consumirFifo(
        int $produtoId,
        int $lojaId,
        float $quantidade,
        ?int $vendaItemId = null,
        string $motivo = 'VENDA'
    ): Collection {
        return DB::transaction(function () use ($produtoId, $lojaId, $quantidade, $vendaItemId, $motivo) {
            $consumos = collect();
            $restante = $quantidade;

            // Get available stock in FIFO order with lock
            $estoques = Estoque::where('produto_id', $produtoId)
                ->where('loja_id', $lojaId)
                ->where('quantidade_disponivel', '>', 0)
                ->orderBy('data_entrada', 'asc')  // FIFO
                ->lockForUpdate()
                ->get();

            foreach ($estoques as $estoque) {
                if ($restante <= 0) break;

                $consumir = min($restante, $estoque->quantidade_disponivel);

                // Update stock
                $estoque->decrement('quantidade_disponivel', $consumir);

                // Create consumption record
                $consumo = EstoqueConsumo::create([
                    'estoque_id' => $estoque->id,
                    'venda_item_id' => $vendaItemId,
                    'quantidade' => $consumir,
                    'custo_unitario' => $estoque->custo_unitario,
                    'motivo' => $motivo,
                ]);

                $consumos->push($consumo);
                $restante -= $consumir;
            }

            if ($restante > 0) {
                throw new \Exception(
                    "Estoque insuficiente para produto {$produtoId}. Faltam {$restante} unidades."
                );
            }

            return $consumos;
        });
    }

    /**
     * Check available stock (FIFO preview)
     */
    public function verificarDisponibilidade(int $produtoId, int $lojaId): array
    {
        $estoques = Estoque::where('produto_id', $produtoId)
            ->where('loja_id', $lojaId)
            ->where('quantidade_disponivel', '>', 0)
            ->orderBy('data_entrada', 'asc')
            ->get();

        return [
            'total_disponivel' => $estoques->sum('quantidade_disponivel'),
            'custo_medio' => $estoques->avg('custo_unitario'),
            'lotes' => $estoques->map(fn($e) => [
                'id' => $e->id,
                'quantidade' => $e->quantidade_disponivel,
                'custo' => $e->custo_unitario,
                'data_entrada' => $e->data_entrada,
                'lote' => $e->lote,
            ])->toArray(),
        ];
    }

    /**
     * Reverse consumption (for returns)
     */
    public function estornarConsumo(EstoqueConsumo $consumo): void
    {
        DB::transaction(function () use ($consumo) {
            // Return quantity to stock
            Estoque::where('id', $consumo->estoque_id)
                ->increment('quantidade_disponivel', $consumo->quantidade);

            // Mark consumption as reversed
            $consumo->update([
                'estornado' => true,
                'estornado_at' => now(),
            ]);
        });
    }
}
```

### 4.3 Updated VendaService

```php
<?php

namespace App\Services\Venda;

use App\Models\Venda;
use App\Models\VendaItem;
use App\Services\Estoque\EstoqueConsumoService;

class VendaService
{
    public function __construct(
        private EstoqueConsumoService $estoqueService
    ) {}

    /**
     * Create stock consumptions for sale items marked as from stock
     */
    public function criarConsumos(Venda $venda): void
    {
        $itensEstoque = $venda->itens()
            ->where('origem', 'ESTOQUE')  // Only items from existing stock
            ->whereNull('consumido_at')   // Not yet consumed
            ->get();

        foreach ($itensEstoque as $item) {
            $consumos = $this->estoqueService->consumirFifo(
                produtoId: $item->produto_id,
                lojaId: $venda->loja_id,
                quantidade: $item->quantidade,
                vendaItemId: $item->id,
                motivo: 'VENDA'
            );

            // Mark item as consumed
            $item->update([
                'consumido_at' => now(),
                'custo_real' => $consumos->sum(fn($c) => $c->quantidade * $c->custo_unitario),
            ]);
        }
    }
}
```

### 4.4 Schema Changes

```sql
-- New consumption table (replaces estoque_has_consumo)
CREATE TABLE estoque_consumos (
    id SERIAL PRIMARY KEY,
    estoque_id INTEGER NOT NULL REFERENCES estoques(id),
    venda_item_id INTEGER REFERENCES venda_itens(id),
    compra_item_id INTEGER REFERENCES compra_itens(id),

    quantidade DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,
    motivo VARCHAR(50) NOT NULL,  -- VENDA, AJUSTE, QUEBRA, TRANSFERENCIA

    -- Reversal tracking
    estornado BOOLEAN DEFAULT FALSE,
    estornado_at TIMESTAMP,
    estornado_por INTEGER REFERENCES usuarios(id),

    -- Audit
    created_at TIMESTAMP DEFAULT NOW(),
    created_by INTEGER REFERENCES usuarios(id)
);

CREATE INDEX idx_consumos_estoque ON estoque_consumos(estoque_id);
CREATE INDEX idx_consumos_venda ON estoque_consumos(venda_item_id);
```

---

## 5. Edge Cases

### 5.1 Insufficient Stock

```php
// Current: Silent failure or cryptic error
// Proposed: Clear exception with details

try {
    $this->estoqueService->consumirFifo($produtoId, $lojaId, 100);
} catch (EstoqueInsuficienteException $e) {
    // $e->getQuantidadeFaltante() = 25
    // $e->getQuantidadeDisponivel() = 75
    // Show user-friendly message
}
```

### 5.2 Concurrent Consumption

```php
// FOR UPDATE lock prevents race conditions
// If two sales try to consume same stock simultaneously:
// - First transaction locks rows
// - Second transaction waits
// - After first commits, second sees updated quantities
```

### 5.3 Multiple Warehouses (Lojas)

```php
// FIFO per warehouse
$consumos = $this->estoqueService->consumirFifo(
    produtoId: $produtoId,
    lojaId: $venda->loja_id,  // Only from this warehouse
    quantidade: $quantidade
);

// Cross-warehouse transfer if needed
$this->estoqueService->transferir(
    produtoId: $produtoId,
    lojaOrigem: 1,
    lojaDestino: 2,
    quantidade: 50
);
```

### 5.4 Lot/Batch Specific Consumption

```php
// Sometimes need specific lot (quality issue, customer request)
public function consumirLoteEspecifico(
    int $estoqueId,
    float $quantidade,
    int $vendaItemId
): EstoqueConsumo {
    // Bypass FIFO for specific batch
    $estoque = Estoque::lockForUpdate()->findOrFail($estoqueId);

    if ($estoque->quantidade_disponivel < $quantidade) {
        throw new EstoqueInsuficienteException();
    }

    $estoque->decrement('quantidade_disponivel', $quantidade);

    return EstoqueConsumo::create([
        'estoque_id' => $estoqueId,
        'venda_item_id' => $vendaItemId,
        'quantidade' => $quantidade,
        'custo_unitario' => $estoque->custo_unitario,
        'motivo' => 'VENDA_LOTE_ESPECIFICO',
    ]);
}
```

### 5.5 Expiration Date Priority

```php
// FEFO: First-Expired, First-Out (for perishables)
public function consumirFefo(int $produtoId, int $lojaId, float $quantidade): Collection
{
    $estoques = Estoque::where('produto_id', $produtoId)
        ->where('loja_id', $lojaId)
        ->where('quantidade_disponivel', '>', 0)
        ->orderByRaw('COALESCE(data_validade, DATE "9999-12-31") ASC')  // FEFO
        ->orderBy('data_entrada', 'asc')  // Then FIFO
        ->lockForUpdate()
        ->get();

    // ... rest same as FIFO
}
```

---

## 6. Migration Strategy

### Phase 1: Add New Columns (Non-Breaking)

```sql
-- Add data_entrada if missing
ALTER TABLE estoque ADD COLUMN IF NOT EXISTS data_entrada TIMESTAMP;

-- Populate from NFe date for existing records
UPDATE estoque e
SET data_entrada = n.dataEmissao
FROM nfe n
WHERE e.idNFe = n.idNFe
  AND e.data_entrada IS NULL;

-- Default for any remaining
UPDATE estoque
SET data_entrada = created_at
WHERE data_entrada IS NULL;

-- Make non-nullable going forward
ALTER TABLE estoque ALTER COLUMN data_entrada SET NOT NULL;
ALTER TABLE estoque ALTER COLUMN data_entrada SET DEFAULT NOW();
```

### Phase 2: Create New Service (Parallel)

```php
// Create new service alongside old code
class EstoqueConsumoService { ... }

// Feature flag for gradual rollout
if (config('features.fifo_consumption')) {
    $this->newService->consumirFifo(...);
} else {
    $this->legacyConsumption(...);
}
```

### Phase 3: Migrate Consumption Logic

```php
// Replace venda.cpp criarConsumos() equivalent
// Old: Use produto.idEstoque
// New: Use EstoqueConsumoService::consumirFifo()
```

### Phase 4: Remove produto.idEstoque

```sql
-- After all code migrated
ALTER TABLE produto DROP COLUMN idEstoque;
```

---

## Summary

### Problem
- `produto.idEstoque` points to ONE stock record (no FIFO)
- No automatic stock selection
- Oldest stock may never be consumed

### Solution
- Remove `produto.idEstoque`
- Add `estoques.data_entrada` for FIFO ordering
- Create `EstoqueConsumoService` with FIFO query
- Lock rows during consumption to prevent races
- Support multiple batches per consumption

### Benefits
- Proper FIFO compliance
- Batch traceability
- Correct inventory valuation
- Concurrent-safe consumption
- Flexible (FIFO, FEFO, specific lot)

---

## Related Documents

- [03-improvements.md](./03-improvements.md) - Full improvements list
- [../business/02-stock-flows.md](../business/02-stock-flows.md) - Stock flow analysis
- [04-l1l2-simplification.md](./04-l1l2-simplification.md) - Table flattening (affects consumption)
