# Venda → Pedido Fornecedor → Estoque: Old vs New (ASCII Version)

## Part 1: Architecture Overview & Data Models

---

## 1. Architecture Overview

### OLD SYSTEM: Tangled L1/L2 with String References

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MASTER DATA                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────┐           ┌──────────────────────────────────┐            │
│  │ fornecedor   │           │  produto (100+ columns!)         │            │
│  │ (table)      │           │  - idEstoque (ONE ONLY!) ⚠️       │            │
│  └──────────────┘           └──────────────────────────────────┘            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                        VENDA TABLES (MESS)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                          ┌─────────────┐                                   │
│                          │   venda     │                                   │
│                          │ (cabeçalho) │                                   │
│                          └──────┬──────┘                                   │
│                                 │ 1:N                                     │
│                                 ▼                                         │
│                    ┌────────────────────────────┐                         │
│                    │ venda_has_produto (L1)     │                         │
│                    │ ⚠️ Agregado                │                         │
│                    └────────────────┬───────────┘                         │
│                                     │ 1:N                                 │
│                                     │ idVendaProdutoFK                    │
│                                     ▼                                     │
│                    ┌────────────────────────────┐                         │
│                    │ venda_has_produto2 (L2)    │                         │
│                    │ ⚠️ Detalhado               │                         │
│                    │ ⚠️ fornecedor = VARCHAR    │                         │
│                    │ ⚠️ idRelacionado (self-ref)│                         │
│                    │ ⚠️ 30+ other fields        │                         │
│                    └────────────────────────────┘                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                       COMPRA TABLES (TANGLED)                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                    ┌─────────────────────────┐                             │
│                    │  pedido_fornecedor      │                             │
│                    └────────────┬────────────┘                             │
│                                 │ 1:N                                     │
│                                 ▼                                         │
│              ┌──────────────────────────────────┐                          │
│              │ pedido_fornecedor_has_produto (L1)│                          │
│              └────────────────┬─────────────────┘                          │
│                               │ 1:N                                       │
│                               ▼                                           │
│              ┌──────────────────────────────────┐                          │
│              │ pedido_fornecedor_has_produto2(L2)│                          │
│              │ ⚠️ idRelacionado (self-ref)      │                          │
│              └──────────────────────────────────┘                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                    ESTOQUE TABLES (DESNORMALIZED)                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│              ┌──────────────────────────────┐                               │
│              │ estoque                      │                               │
│              │ ⚠️ 30+ fiscal columns!       │                               │
│              │ ⚠️ NO data_entrada           │                               │
│              │ ⚠️ idEstoque (no FK check)   │                               │
│              └────────────────┬─────────────┘                               │
│                               │ 1:N                                       │
│                               ▼                                           │
│              ┌──────────────────────────────┐                               │
│              │ estoque_has_consumo          │                               │
│              │ ⚠️ 30+ fiscal columns AGAIN! │                               │
│              │ ⚠️ Duplicated data           │                               │
│              └──────────────────────────────┘                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

CROSS-TABLE RELATIONSHIPS (BROKEN):

  venda_has_produto2 ──→ fornecedor: VARCHAR "ACME Corp" ⚠️ MAGIC STRING
                              ↓
                         ??? Need to find fornecedor_id somehow

  venda_has_produto2 ──→ pedido_fornecedor_has_produto2 via STRING MATCH ⚠️

  venda_has_produto2 ──→ estoque_has_consumo: NO CLEAR FK RELATIONSHIP

  estoque ──→ produto: product.idEstoque (only ONE!) ⚠️


PROBLEMS SUMMARY:
  ❌ Two tables for one concept (venda items)
  ❌ Always must join L1 → L2
  ❌ String magic for fornecedor (typo-prone, update 9 tables)
  ❌ Recursive idRelacionado chains (hard to follow)
  ❌ No clear hierarchy for splits
  ❌ Fiscal columns duplicated everywhere
  ❌ No timestamp for FIFO
  ❌ No audit trail
  ❌ No DB-level constraints
```

---

### NEW SYSTEM: Clear 1:N:N with FK Relationships

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MASTER DATA                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────┐  1:N  ┌──────────────────┐  1:N  ┌─────────────────┐    │
│  │fornecedores  │───────│   produtos       │───────│ produto_precos  │    │
│  └──────────────┘       │ (clean table)    │       │ (versioned)     │    │
│                         └────────┬─────────┘       └─────────────────┘    │
│                                  │                                        │
│                                  │ 1:1                                   │
│                                  ▼                                       │
│                         ┌────────────────────┐                           │
│                         │ produto_tributos   │                           │
│                         └────────────────────┘                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                    VENDA (SINGLE TABLE!) ✅                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ┌──────────────┐                                    │
│                        │   vendas     │                                    │
│                        │ (cabeçalho)  │                                    │
│                        └────────┬─────┘                                    │
│                                 │ 1:N                                     │
│                                 ▼                                         │
│         ┌────────────────────────────────────────┐                        │
│         │   venda_itens (TABELA ÚNICA!)          │                        │
│         │   - Sem L1/L2 separação ✅             │                        │
│         │   - produto_id FK ✅                   │                        │
│         │   - fornecedor_id FK ✅                │                        │
│         │   - parent_id (self-ref) para splits   │                        │
│         │   - root_id (self-ref) para groups     │                        │
│         │   - status ENUM ✅                     │                        │
│         │   - origem ENUM (COMPRA/ESTOQUE) ✅    │                        │
│         └────────────────────────────────────────┘                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMPRA (SINGLE TABLE!) ✅                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ┌──────────────┐                                    │
│                        │   compras    │                                    │
│                        │ (cabeçalho)  │                                    │
│                        └────────┬─────┘                                    │
│                                 │ 1:N                                     │
│                                 ▼                                         │
│         ┌────────────────────────────────────────┐                        │
│         │   compra_itens (TABELA ÚNICA!)         │                        │
│         │   - venda_item_id FK ✅                │                        │
│         │   - parent_id/root_id para splits      │                        │
│         │   - status ENUM ✅                     │                        │
│         └────────────────────────────────────────┘                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      ESTOQUE (CLEAN!) ✅                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ┌──────────────┐                                    │
│                        │   nfes       │                                    │
│                        └────────┬─────┘                                    │
│                                 │ 1:N                                     │
│                                 ▼                                         │
│    ┌─────────────────────────────────────────┐                            │
│    │ nfe_itens                               │                            │
│    │ - dados JSONB (all fiscal) ✅           │                            │
│    │ - compra_item_id FK ✅                  │                            │
│    └──────────────────┬──────────────────────┘                            │
│                       │ 1:N                                              │
│                       ▼                                                  │
│    ┌─────────────────────────────────────────┐                            │
│    │ estoques                                │                            │
│    │ - quantidade_original                   │                            │
│    │ - quantidade_disponivel                 │                            │
│    │ - data_entrada (FIFO!) ✅               │                            │
│    │ - status ENUM ✅                        │                            │
│    └──────────────────┬──────────────────────┘                            │
│                       │ 1:1 (UNIQUE constraint)                          │
│                       ▼                                                  │
│    ┌─────────────────────────────────────────┐                            │
│    │ estoque_consumos                        │                            │
│    │ - venda_item_id FK (1:1) ✅             │                            │
│    │ - estoque_id FK (1:1) ✅                │                            │
│    │ - quantidade (must match!) ✅           │                            │
│    │ - is_estornado (soft delete) ✅         │                            │
│    └─────────────────────────────────────────┘                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

CLEAN RELATIONSHIPS (WITH FK INTEGRITY) ✅:

  venda_itens.produto_id FK ──→ produtos ✅
  venda_itens.fornecedor_id FK ──→ fornecedores ✅

  venda_itens.origem = 'COMPRA' ──→ compra_itens (via venda_item_id FK) ✅
  venda_itens.origem = 'ESTOQUE' ──→ estoque_consumos (direct FK) ✅

  compra_itens FK ──→ nfe_itens (1:1 link) ✅
  nfe_itens FK ──→ estoques (1:N, one per line) ✅
  estoques FK ──→ estoque_consumos (1:1 unique) ✅

  estoque_consumos.venda_item_id FK ──→ venda_itens ✅

ADVANTAGES:
  ✅ ONE table = ONE concept (DRY principle)
  ✅ All FKs enforced at DB level
  ✅ No string magic, no typos
  ✅ Change supplier name = update 1 record
  ✅ Clear split hierarchy (parent_id/root_id)
  ✅ Fiscal data isolated in JSONB (nfe_itens.dados)
  ✅ FIFO support via data_entrada timestamp
  ✅ 1:1 pairing enforced with UNIQUE constraints
  ✅ Audit trail via audit_log table
  ✅ Status as ENUMs (type-safe)
```

---

## 2. Data Model Comparison

### OLD: Table Structure (venda_has_produto / venda_has_produto2)

```
┌──────────────────────────────────────────────────────────────────────────┐
│ venda_has_produto (L1 - Level 1)                                         │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  idVendaProduto (PK, INT)                                                │
│  idVenda (FK) ──→ venda                                                  │
│  idVendaProduto2FK (FK) ──→ venda_has_produto2                           │
│                                                                          │
│  Purpose: ???                                                            │
│  Why separate from L2? ❌ Not clear!                                     │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

                                    │
                                    │ 1:N
                                    │ Must join through FK
                                    ▼

┌──────────────────────────────────────────────────────────────────────────┐
│ venda_has_produto2 (L2 - Level 2 - THE ACTUAL DATA)                      │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  idVendaProduto2 (PK, INT)                                               │
│  idVendaProduto (FK) ──→ venda_has_produto (L1)                          │
│  idProduto (FK) ──→ produto                                              │
│                                                                          │
│  descricaoProduto (VARCHAR 500) - DESNORMALIZED ⚠️                       │
│  fornecedor (VARCHAR 100) - MAGIC STRING ⚠️                              │
│  quantidade (DECIMAL)                                                   │
│  valor_unitario (DECIMAL)                                               │
│  desconto (DECIMAL)                                                     │
│  valor_total (DECIMAL)                                                  │
│                                                                          │
│  idRelacionado (FK) ──→ venda_has_produto2 (RECURSIVE!) ⚠️               │
│  split_reason (VARCHAR)                                                 │
│                                                                          │
│  ... 30+ other fields ...                                               │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

KEY PROBLEMS WITH THIS STRUCTURE:

  ❌ TWO tables for ONE concept (venda items)
     → Every query must join L1 + L2
     → Can't select directly from L2
     → Maintenance nightmare

  ❌ fornecedor stored as VARCHAR "ACME Corp"
     → Typo-prone (ACME Corp vs Acme Corp)
     → If supplier name changes, must update L2 records + 9 other tables
     → Can't join with fornecedor table reliably
     → No referential integrity

  ❌ descricaoProduto desnormalized
     → If product description changes, L2 records stay old
     → No way to know if L2 is stale

  ❌ idRelacionado for splits
     → Recursive reference to same table
     → Unclear semantics (parent? child? both?)
     → Hard to trace chains of splits
     → No constraint (1:1? 1:N? Both?)

  ❌ Can't track which items are splits
     → Is this the original? or a split?
     → If original, which splits belong to it?
     → Only idRelacionado gives hints (ambiguous)

  ❌ No constraints at DB level
     → Application must enforce all rules
     → If app has bug → data corruption


EXAMPLE QUERY: Get all items for venda 100

    -- Complex join required
    SELECT vp1.*, vp2.*
    FROM venda_has_produto vp1
    JOIN venda_has_produto2 vp2
        ON vp1.idVendaProduto = vp2.idVendaProdutoFK
    WHERE vp1.idVenda = 100;

    -- Problem: Must go through L1, can't join venda directly to L2
```

---

### NEW: Table Structure (venda_itens - Single Table)

```
┌──────────────────────────────────────────────────────────────────────────┐
│ venda_itens (TABELA ÚNICA!) - No L1/L2 separation                        │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  id (PK, INT) ✅                                                         │
│  venda_id (FK) ──→ vendas ✅                                             │
│                                                                          │
│  HIERARCHY (for splits):                                                │
│  ├─ parent_id (FK self-ref) ──→ venda_itens ✅                           │
│  └─ root_id (FK self-ref) ──→ venda_itens ✅                             │
│     split_reason (VARCHAR) = 'PARTIAL_NFE' | 'SUPPLIER_SPLIT' | ...     │
│                                                                          │
│  PRODUCT DATA (Foreign Keys, not desnormalized):                         │
│  ├─ produto_id (FK) ──→ produtos ✅                                      │
│  ├─ fornecedor_id (FK) ──→ fornecedores ✅                               │
│  └─ origem (ENUM) = 'COMPRA' | 'ESTOQUE' ✅                              │
│                                                                          │
│  QUANTITIES:                                                            │
│  ├─ quantidade (DECIMAL)                                                │
│  ├─ quantidade_caixas (DECIMAL)                                         │
│  └─ unidade (VARCHAR) = 'UN' | 'CX' | ...                               │
│                                                                          │
│  PRICING (snapshot at time of sale):                                    │
│  ├─ valor_unitario (DECIMAL)                                            │
│  ├─ desconto_item_percentual (DECIMAL)                                  │
│  ├─ valor_com_desconto (DECIMAL)                                        │
│  └─ valor_total (DECIMAL) = quantidade * valor_com_desconto             │
│                                                                          │
│  DENORMALIZED FOR DISPLAY (snapshot):                                   │
│  ├─ descricao_produto (VARCHAR 500) ✅                                   │
│  └─ codigo_comercial (VARCHAR)                                          │
│                                                                          │
│  STATUS TRACKING:                                                       │
│  ├─ status (ENUM venda_item_status) ✅                                   │
│  │   = PENDENTE | EM_COMPRA | CONFIRMADO | FATURADO | ...              │
│  ├─ data_prev_entrega (DATE)                                            │
│  └─ data_real_entrega (TIMESTAMP)                                       │
│                                                                          │
│  DELIVERY INFO:                                                         │
│  ├─ entregue_por (VARCHAR)                                              │
│  └─ recebido_por (VARCHAR)                                              │
│                                                                          │
│  NFe LINK:                                                              │
│  └─ nfe_saida_id (FK) ──→ nfes ✅                                        │
│                                                                          │
│  AUDIT:                                                                 │
│  ├─ created_at (TIMESTAMP)                                              │
│  └─ updated_at (TIMESTAMP)                                              │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

KEY ADVANTAGES:

  ✅ SINGLE table = Direct access
     → SELECT * FROM venda_itens WHERE venda_id = 100
     → No L1/L2 join needed
     → Faster, simpler, clearer

  ✅ FKs for produto and fornecedor
     → No string magic
     → Change supplier name = 1 update
     → Referential integrity enforced by DB
     → JOINs are fast and reliable

  ✅ Desnormalized display fields
     → descricao_produto is SNAPSHOT at time of sale
     → When product description changes later, venda_itens keeps original
     → Can see exactly what customer ordered
     → Still can join to current product if needed

  ✅ parent_id/root_id for splits
     → Clear hierarchy (parent = original, children = splits)
     → root_id groups all related items
     → Query all splits: WHERE root_id = 1 OR id = 1
     → Constraint on split_reason explains WHY split happened

  ✅ origen ENUM clarifies flow
     → origen = 'COMPRA' → must create compra_itens
     → origen = 'ESTOQUE' → consume from existing stock
     → Application logic is obvious from data

  ✅ status ENUM is type-safe
     → Can't have typos like 'PENDNE' or 'pENDENTE'
     → Application can switch on enum values
     → PostgreSQL validates transitions

  ✅ All constraints at DB level
     → Data integrity even if app has bugs
     → Triggers enforce business rules
     → Can't get into impossible states


EXAMPLE QUERY: Get all items for venda 100

    -- Simple, direct!
    SELECT * FROM venda_itens
    WHERE venda_id = 100;

    -- Get all splits of an original item?
    SELECT * FROM venda_itens
    WHERE id = 1 OR root_id = 1;

    -- Get with product names?
    SELECT vi.*, p.descricao
    FROM venda_itens vi
    JOIN produtos p ON vi.produto_id = p.id
    WHERE vi.venda_id = 100;

    -- Much simpler and more performant!
```

---

## 3. Supplier Reference Comparison

### OLD: Fornecedor as VARCHAR String

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    THE STRING MAGIC PROBLEM                              │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  WHERE DATA LIVES (VARCHAR 'ACME Corp'):                                 │
│                                                                          │
│  venda_has_produto2.fornecedor          = 'ACME Corp' ⚠️                 │
│  estoque.fornecedor                     = 'ACME Corp' ⚠️                 │
│  estoque_has_consumo.fornecedor         = 'ACME Corp' ⚠️                 │
│  pedido_fornecedor_has_produto2.fornecedor = 'ACME Corp' ⚠️              │
│  compra_avulsa.fornecedor               = 'ACME Corp' ⚠️                 │
│  ... and 4 more tables ...                                              │
│                                                                          │
│  TOTAL: 9 tables with VARCHAR fornecedor ⚠️⚠️⚠️                           │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                        CONSISTENCY PROBLEM                               │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Data entry creates records:                                            │
│                                                                          │
│    venda_has_produto2: 'ACME Corp'         ← Correct                    │
│    estoque:            'ACME corp'         ← Typo! (lowercase 'c')      │
│    pedido_fornecedor:  'Acme Corp'         ← Different capitalization   │
│    compra_avulsa:      'ACME Corporation'  ← Different name             │
│                                                                          │
│  Result: 4 different string values for same supplier! ⚠️⚠️⚠️             │
│  Can't reliably join. Reports show 4 different suppliers.              │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                        UPDATE NIGHTMARE                                  │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Supplier changes name: 'ACME Corp' → 'ACME Corporation'                 │
│                                                                          │
│  Must update 9 tables:                                                  │
│                                                                          │
│    UPDATE venda_has_produto2                                            │
│    SET fornecedor = 'ACME Corporation'                                  │
│    WHERE fornecedor = 'ACME Corp';                                      │
│                                                                          │
│    UPDATE estoque                                                       │
│    SET fornecedor = 'ACME Corporation'                                  │
│    WHERE fornecedor = 'ACME Corp';                                      │
│                                                                          │
│    UPDATE estoque_has_consumo ...                                       │
│    UPDATE pedido_fornecedor_has_produto2 ...                            │
│    UPDATE compra_avulsa ...                                             │
│    UPDATE ... (4 more tables) ...                                       │
│                                                                          │
│  If you miss one table → Data inconsistency! ⚠️⚠️⚠️                      │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                        NO REFERENTIAL INTEGRITY                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  CREATE venda_has_produto2 with:                                        │
│    fornecedor = 'ACME Corp'                                             │
│                                                                          │
│  Database doesn't check:                                                │
│    - Does 'ACME Corp' exist in fornecedor table? ⚠️                       │
│    - Is it spelled correctly?                                           │
│    - Is it active (not deleted)?                                        │
│                                                                          │
│  Result: Orphaned records with supplier names that don't exist! ⚠️       │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

COMMON ERRORS CAUSED BY STRING REFERENCES:

  ❌ Typos in manual data entry
     "ACE Corp" instead of "ACME Corp" → Can't find items

  ❌ Abbreviations in some tables, full names in others
     "ACME" vs "ACME Corp" → Multiple entries for same supplier

  ❌ Historical name changes not propagated
     Supplier renamed 5 years ago, some old records still use old name

  ❌ Orphaned records
     Item has fornecedor = 'ACME Inc' but this supplier was deleted/renamed

  ❌ Can't write queries with confidence
     SELECT * FROM estoque WHERE fornecedor = 'ACME Corp'
     But are there other variations? Unknown!

  ❌ Reports are incomplete or wrong
     "Total from ACME" might miss items if spellings differ

  ❌ Can't enforce relationships in code
     No way to validate: Is this a real supplier?
```

---

### NEW: Fornecedor as Foreign Key

```
┌──────────────────────────────────────────────────────────────────────────┐
│                   CLEAN FK RELATIONSHIP ✅                               │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  fornecedores table:                                                    │
│  ┌─────────────────────────────────────────┐                            │
│  │ id (PK) │ razao_social │ nome_fantasia   │                            │
│  ├─────────┼──────────────┼─────────────────┤                            │
│  │ 2       │ ACME Corp    │ ACME            │ ← SINGLE SOURCE OF TRUTH  │
│  │ 5       │ BRICKS Inc   │ BRICKS          │                            │
│  │ 7       │ STEEL Ltd    │ STEEL           │                            │
│  └─────────┴──────────────┴─────────────────┘                            │
│                                                                          │
│  Now in all other tables: Use FK fornecedor_id (INT), not string        │
│                                                                          │
│  venda_itens.fornecedor_id    = 2 (FK → fornecedores.id) ✅             │
│  estoques.fornecedor_id       = 2 (FK → fornecedores.id) ✅             │
│  compra_itens                 = 2 (FK → fornecedores.id) ✅             │
│  ... all other tables ...                                              │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                      CONSISTENCY = AUTOMATIC ✅                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  All references point to same record:                                   │
│                                                                          │
│  venda_itens.fornecedor_id = 2 → ACME Corp                              │
│  estoques.fornecedor_id = 2 → ACME Corp                                 │
│  compra_itens.fornecedor_id = 2 → ACME Corp                             │
│                                                                          │
│  No typos possible (INT fields can't have typos)                        │
│  No inconsistencies (all resolve to same supplier)                      │
│  No orphaned records (FK constraint prevents invalid IDs)              │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                        UPDATE = SIMPLE ✅                                │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Supplier changes name: 'ACME Corp' → 'ACME Corporation'                 │
│                                                                          │
│  Update ONE record:                                                     │
│                                                                          │
│    UPDATE fornecedores                                                  │
│    SET razao_social = 'ACME Corporation'                                │
│    WHERE id = 2;                                                        │
│                                                                          │
│  That's it! All 100+ items automatically reflect new name. ✅            │
│  All joins automatically work. ✅                                        │
│  No risk of forgetting a table. ✅                                       │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                    REFERENTIAL INTEGRITY = ENFORCED ✅                    │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Try to create venda_itens with invalid fornecedor_id:                  │
│                                                                          │
│    INSERT INTO venda_itens (                                            │
│      fornecedor_id = 999  ← Doesn't exist!                              │
│    )                                                                    │
│                                                                          │
│  Database error: FOREIGN KEY constraint fails ✅                         │
│  (Can't create item for non-existent supplier)                          │
│                                                                          │
│  Try to delete supplier while items exist:                              │
│                                                                          │
│    DELETE FROM fornecedores WHERE id = 2;                               │
│                                                                          │
│  Database error: FK constraint fails ✅                                  │
│  (Can't delete supplier with active items)                              │
│                                                                          │
│  (Or use ON DELETE CASCADE if you want cascading deletes)               │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

IMPACT SUMMARY:

  ✅ Data Integrity
     - No typos (INT IDs are exact)
     - No orphaned records (FK constraint)
     - No stale data (change supplier once, everywhere updates)

  ✅ Query Simplicity
     - JOIN fornecedores ON venda_itens.fornecedor_id = fornecedores.id
     - Get supplier names with no string matching

  ✅ Update Efficiency
     - Change supplier data = 1 UPDATE statement
     - No risk of missing a table

  ✅ Reporting Accuracy
     - "Total from ACME" is accurate (no spelling variations)
     - Supplier consolidation works (can group by fornecedor_id)

  ✅ Database Performance
     - INT joins are faster than string joins
     - Indexes on INT IDs are more efficient
```

---

**End of Part 1**

Next up in Part 2:
- Scenario 1: Simple Sale (No Splits)
- Scenario 2: Sale with Supplier Split
- Scenario 3: Partial NFe (Split Across Shipments)
