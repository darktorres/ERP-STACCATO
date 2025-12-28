# Flow & Schema Improvements

> Status: **Brainstorming**
> Last updated: 2025-12-27
> Purpose: Identify pain points and improvement opportunities for the web migration

---

## Table of Contents

1. [Current Pain Points](#1-current-pain-points)
2. [Improvement Options](#2-improvement-options)
3. [New Capabilities](#3-new-capabilities)
4. [Priority Matrix](#4-priority-matrix)
5. [Decisions Needed](#5-decisions-needed)

---

## 1. Current Pain Points

### 1.1 Two-Level Tables (L1/L2 Architecture)

**Problem**: The system uses paired tables for sales and purchases:
- `venda_has_produto` (L1) + `venda_has_produto2` (L2)
- `pedido_fornecedor_has_produto` (L1) + `pedido_fornecedor_has_produto2` (L2)

**Issues**:
- Hard to reason about which level to query
- Sync issues between levels
- Database triggers required to maintain consistency
- Complex JOINs for simple queries
- `idRelacionado` self-referential links for splits add more complexity

**Current Purpose**:
| Aspect | Level 1 | Level 2 |
|--------|---------|---------|
| Purpose | What was ordered | How it's being fulfilled |
| Granularity | Aggregated | Per-delivery/per-NFe |
| Status | Order status | Item fulfillment status |

**Root Cause**: Designed to handle partial deliveries and order splits, but implementation became complex.

---

### 1.2 FIFO Not Properly Implemented

**Problem**: Stock consumption doesn't follow First-In-First-Out properly.

**Current Code** (simplified):
```cpp
// Just grabs whatever idEstoque is pre-set on the product
query.exec("SELECT * FROM estoque WHERE idEstoque = " + produto.idEstoque);
```

**Should Be**:
```sql
SELECT * FROM estoque
WHERE produto_id = :produto_id
  AND quantidade_disponivel > 0
ORDER BY data_entrada ASC  -- FIFO: oldest first
LIMIT 1
```

**Impact**:
- Inventory valuation incorrect
- Older stock may never get consumed
- Audit/compliance issues for perishable goods

---

### 1.3 Denormalized Supplier Names

**Problem**: Supplier name stored as VARCHAR in multiple tables instead of FK.

**Affected Tables**:
| Table | Column |
|-------|--------|
| `venda_has_produto2` | `fornecedor` |
| `estoque` | `fornecedor` |
| `estoque_has_consumo` | `fornecedor` |
| `compra_avulsa` | `fornecedor` |
| `pedido_fornecedor_has_produto2` | `fornecedor` |

**Impact**:
- If supplier renames, need to update 5+ tables
- No referential integrity
- Inconsistent data possible (typos, variations)
- Can't easily query "all transactions for supplier X"

---

### 1.4 Returns (Devolução) Flow Incomplete

**Problem**: The returns flow has multiple bugs and missing features.

**Issues Found**:
1. **No automatic NFe Devolução**: Should generate return invoice to supplier
2. **Financial records wrong**: Marked as `RECEBIDO` immediately instead of pending
3. **Empty observação**: No reason captured for the return
4. **Stock not properly reversed**: `restante` field manipulation is fragile
5. **Commission reversal incomplete**: RT clawback logic has edge cases

**Code Evidence**:
```cpp
// From devolucao.cpp - TODOs found
// TODO: gerar NFe de devolução
// TODO: verificar se precisa estornar financeiro
```

---

### 1.5 Status as Magic Strings

**Problem**: Status values are hardcoded strings throughout the codebase.

**Examples**:
```cpp
if (status == "PENDENTE") ...
if (status == "EM ENTREGA") ...
if (status == "PEND. APROV.") ...
```

**Issues**:
- Typos cause silent bugs
- No compile-time checking
- Different status sets for different tables (inconsistent)
- Hard to find all places that check a status

**Status Variations Found**:
| Table | Statuses |
|-------|----------|
| `venda_has_produto2` | PENDENTE, ESTOQUE, ENTREGA AGEND., EM ENTREGA, ENTREGUE, DEVOLVIDO |
| `pedido_fornecedor_has_produto2` | PENDENTE, CONFIRMADO, FATURADO, EM COLETA, EM RECEBIMENTO, ESTOQUE |
| `conta_a_pagar` | PENDENTE, CONFERIDO, AGENDADO, PAGO, CANCELADO |
| `nfe` | NOTA PENDENTE, AUTORIZADA, CANCELADA, DENEGADA |

---

### 1.6 Mega-Table: `produto` (100+ Columns)

**Problem**: The `produto` table has grown to 100+ columns mixing different concerns.

**Column Categories**:
| Category | Example Columns | Count |
|----------|-----------------|-------|
| Core data | descricao, codComercial, codBarras | ~10 |
| Pricing | custo, precoVenda, markup, oldPrecoVenda | ~8 |
| Tax | ncm, cst, icms, st, mva, ipi, pis, cofins | ~15 |
| Tax Reform (IBS/CBS) | cClassTribIBSCBS, pAliqEfetIBSUF, etc. | ~35 |
| Stock | estoqueRestante, quantCaixa, temLote | ~5 |
| Tracking flags | *Upd columns for each field | ~20+ |
| Dimensions | m2, altura, largura, profundidade | ~5 |

**Issues**:
- Hard to maintain
- Many NULL columns for irrelevant fields
- Tax columns change with legislation (2025 reform added 35 columns)
- Tracking flags pollute the table

---

### 1.7 No Audit Trail

**Problem**: Limited tracking of who changed what and when.

**Current State**:
- Some `*Upd` boolean flags exist
- No user tracking
- No timestamp of changes
- No old value preservation

**Impact**:
- Can't answer "who changed this price?"
- Can't reconstruct historical state
- Compliance/audit issues

---

### 1.8 Junction Tables Complexity

**Problem**: Multiple junction tables with overlapping purposes.

**Current Junction Tables**:
```
estoque_has_compra      - Links stock to purchase order
estoque_has_consumo     - Links stock to sale order
conta_a_pagar_has_idcompra - Links payment to purchase
veiculo_has_produto     - Links delivery to products
```

**Issue**: Hard to trace the full chain from customer order → purchase → NFe → stock → delivery.

---

## 2. Improvement Options

### 2.1 Simplify L1/L2 Tables

**Option A: Flatten to Single Table**
```sql
CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,
    venda_id INTEGER REFERENCES vendas(id),
    parent_item_id INTEGER REFERENCES venda_itens(id), -- For splits
    produto_id INTEGER,
    quantidade_pedida DECIMAL,
    quantidade_entregue DECIMAL,
    status venda_item_status,
    -- All other fields...
);
```
- Pros: Simpler queries, no sync issues
- Cons: Need to handle splits via self-reference

**Option B: Keep L2 Only, Derive L1**
```sql
-- L2 is the source of truth
CREATE TABLE venda_itens (...);

-- L1 is a view/materialized view
CREATE VIEW venda_itens_agregado AS
SELECT venda_id, produto_id, SUM(quantidade) as total
FROM venda_itens
GROUP BY venda_id, produto_id;
```
- Pros: Single source of truth, L1 always consistent
- Cons: Aggregation overhead

**Option C: Event Sourcing**
```sql
CREATE TABLE venda_item_events (
    id SERIAL,
    venda_item_id INTEGER,
    event_type VARCHAR, -- CREATED, SPLIT, DELIVERED, RETURNED
    payload JSONB,
    created_at TIMESTAMP
);
```
- Pros: Full history, can replay state
- Cons: More complex, needs CQRS

---

### 2.2 Fix FIFO Consumption

**Simple Fix**:
```sql
-- Proper FIFO query
SELECT id, quantidade_disponivel
FROM estoques
WHERE produto_id = :produto_id
  AND loja_id = :loja_id
  AND quantidade_disponivel > 0
ORDER BY data_entrada ASC
FOR UPDATE;  -- Lock for consumption
```

**Better: Consumption Service**
```php
class EstoqueConsumoService
{
    public function consumir(int $produtoId, float $quantidade): Collection
    {
        $consumidos = collect();
        $restante = $quantidade;

        $estoques = Estoque::where('produto_id', $produtoId)
            ->where('quantidade_disponivel', '>', 0)
            ->orderBy('data_entrada', 'asc')  // FIFO
            ->lockForUpdate()
            ->get();

        foreach ($estoques as $estoque) {
            if ($restante <= 0) break;

            $consumir = min($restante, $estoque->quantidade_disponivel);
            $estoque->decrement('quantidade_disponivel', $consumir);

            $consumidos->push([
                'estoque_id' => $estoque->id,
                'quantidade' => $consumir,
                'custo' => $estoque->custo_unitario,
            ]);

            $restante -= $consumir;
        }

        return $consumidos;
    }
}
```

---

### 2.3 Normalize Supplier References

**Migration**:
```sql
-- Add FK columns
ALTER TABLE venda_has_produto2 ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);
ALTER TABLE estoque ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);
-- etc.

-- Populate from existing names
UPDATE venda_has_produto2 v
SET fornecedor_id = f.id
FROM fornecedores f
WHERE v.fornecedor = f.razao_social;

-- Eventually drop VARCHAR columns
ALTER TABLE venda_has_produto2 DROP COLUMN fornecedor;
```

---

### 2.4 Fix Returns Flow

**Complete Returns Flow**:
```
1. User initiates return
   ├── Capture reason (observação required)
   ├── Validate quantities
   └── Check if within return window

2. Stock reversal
   ├── Create negative consumption record
   ├── Update estoque.quantidade_disponivel
   └── Record which original consumption is being reversed

3. Financial reversal
   ├── Create credit note (conta_a_receber with negative value)
   ├── DO NOT mark as RECEBIDO immediately
   └── Link to original payment records

4. NFe Devolução
   ├── Generate return NFe XML
   ├── Reference original NFe (chave)
   └── Submit to SEFAZ

5. Commission reversal
   ├── Calculate proportional RT clawback
   ├── Create conta_a_pagar for salesperson
   └── Schedule for next payment cycle
```

---

### 2.5 Split Produto Table

**Proposed Structure**:
```sql
-- Core product data only
CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    cod_comercial VARCHAR(100),
    descricao VARCHAR(500),
    unidade VARCHAR(10),
    ativo BOOLEAN DEFAULT true
);

-- Versioned pricing
CREATE TABLE produto_precos (
    id SERIAL PRIMARY KEY,
    produto_id INTEGER REFERENCES produtos(id),
    custo DECIMAL(15,2),
    preco_venda DECIMAL(15,2),
    vigencia_inicio DATE,
    vigencia_fim DATE
);

-- Tax configuration (JSONB for flexibility)
CREATE TABLE produto_tributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id),
    ncm_id INTEGER REFERENCES ncms(id),
    config JSONB  -- {icms: {...}, ipi: {...}, ibs: {...}}
);

-- Flexible attributes
CREATE TABLE produto_atributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id),
    atributos JSONB  -- {m2: 1.5, cor: "branco", ...}
);
```

---

### 2.6 Redesign Status Handling

**PostgreSQL ENUMs**:
```sql
CREATE TYPE venda_item_status AS ENUM (
    'PENDENTE',
    'ESTOQUE',
    'ENTREGA_AGENDADA',
    'EM_ENTREGA',
    'ENTREGUE',
    'DEVOLVIDO',
    'CANCELADO'
);

CREATE TYPE compra_status AS ENUM (
    'PENDENTE',
    'CONFIRMADO',
    'FATURADO',
    'EM_COLETA',
    'EM_RECEBIMENTO',
    'RECEBIDO',
    'CANCELADO'
);
```

**PHP Enums**:
```php
enum VendaItemStatus: string
{
    case PENDENTE = 'PENDENTE';
    case ESTOQUE = 'ESTOQUE';
    case ENTREGA_AGENDADA = 'ENTREGA_AGENDADA';
    // ...

    public function canTransitionTo(self $new): bool
    {
        return match($this) {
            self::PENDENTE => in_array($new, [self::ESTOQUE, self::CANCELADO]),
            self::ESTOQUE => in_array($new, [self::ENTREGA_AGENDADA, self::CANCELADO]),
            // ...
        };
    }
}
```

---

## 3. New Capabilities

The migration enables features not possible in the current system:

### 3.1 Proper Audit Trail
- Track all changes with user, timestamp, old/new values
- Query historical state at any point in time
- Compliance-ready logging

### 3.2 Temporal Queries
- "What was the stock level on Dec 31?"
- "What was the price of product X last month?"
- Point-in-time reporting for audits

### 3.3 Better Search
- PostgreSQL full-text search with Portuguese stemming
- Fuzzy matching for product names
- Faceted search (by supplier, category, price range)

### 3.4 Real-Time Updates
- WebSocket notifications for status changes
- Live dashboard updates
- Multi-user collaboration without refresh

### 3.5 API-First Design
- Mobile app possibility
- Third-party integrations
- Webhook notifications

### 3.6 Background Processing
- NFe processing in queues
- CNAB generation async
- Report generation without blocking UI

---

## 4. Priority Matrix

| # | Improvement | Impact | Complexity | Priority |
|---|-------------|--------|------------|----------|
| 1 | Fix FIFO consumption | Medium | Low | **High** |
| 2 | Normalize supplier refs | Medium | Medium | **High** |
| 3 | Redesign status handling | Low | Low | **High** |
| 4 | Add audit trail | High | Medium | **High** |
| 5 | Fix returns flow | Medium | Medium | **Medium** |
| 6 | Split produto table | Medium | Medium | **Medium** |
| 7 | Simplify L1/L2 tables | High | High | **Medium** |
| 8 | Add temporal queries | Medium | High | **Low** |

### Recommended Order

**Phase 1 - Quick Wins** (implement immediately):
- FIFO fix
- Status ENUMs
- Supplier FK normalization

**Phase 2 - Foundation** (during migration):
- Audit trail
- Split produto table
- Returns flow fix

**Phase 3 - Architecture** (careful planning):
- L1/L2 simplification
- Temporal queries

---

## 5. Decisions Needed

### Decision 1: L1/L2 Strategy
- [ ] Option A: Flatten to single table
- [ ] Option B: Keep L2 only, derive L1
- [ ] Option C: Event sourcing
- [ ] Option D: Keep current structure (just clean up)

### Decision 2: Stock Consumption Method
- [ ] Simple FIFO by date
- [ ] FIFO by date + location (warehouse block)
- [ ] Configurable (FIFO/LIFO/specific lot)

### Decision 3: Audit Trail Scope
- [ ] All tables
- [ ] Critical tables only (vendas, compras, estoque, financeiro)
- [ ] Configurable per table

### Decision 4: Returns NFe
- [ ] Generate automatically on return confirmation
- [ ] Manual generation with pre-filled data
- [ ] Optional (some returns may not need NFe)

---

## Related Documents

- [business/02-stock-flows.md](../business/02-stock-flows.md) - Current stock flow analysis
- [business/01-flows-overview.md](../business/01-flows-overview.md) - Complete flow documentation
- [technical/02-database.md](../technical/02-database.md) - Database schema proposal
- [technical/04-infrastructure.md](../technical/04-infrastructure.md) - Audit/temporal architecture
