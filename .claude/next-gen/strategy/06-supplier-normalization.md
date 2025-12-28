# Supplier Reference Normalization

> Status: **Analysis**
> Last updated: 2025-12-27
> Focus: Replace denormalized supplier names with proper FK references

---

## Table of Contents

1. [Current Problem](#1-current-problem)
2. [Affected Tables](#2-affected-tables)
3. [Impact Analysis](#3-impact-analysis)
4. [Proposed Solution](#4-proposed-solution)
5. [Migration Strategy](#5-migration-strategy)
6. [Code Changes](#6-code-changes)

---

## 1. Current Problem

### 1.1 What's Wrong

Supplier name is stored as **VARCHAR** in multiple tables instead of **FK to fornecedor**:

```sql
-- Current: Name duplicated everywhere
venda_has_produto2 (fornecedor VARCHAR)     -- "ACME Corp"
estoque (fornecedor VARCHAR)                 -- "ACME Corp"
estoque_has_consumo (fornecedor VARCHAR)     -- "ACME Corp"
pedido_fornecedor_has_produto2 (fornecedor VARCHAR) -- "ACME Corp"
produto (fornecedor VARCHAR)                 -- "ACME Corp"

-- Should be: FK reference
venda_has_produto2 (fornecedor_id INT FK)   -- 123
estoque (fornecedor_id INT FK)              -- 123
```

### 1.2 Why It's a Problem

| Issue | Impact |
|-------|--------|
| **Data inconsistency** | Typos, variations ("ACME Corp" vs "Acme Corp") |
| **Rename nightmare** | If supplier renames, must update 5+ tables |
| **No referential integrity** | Can have orphan references |
| **Query inefficiency** | String comparison slower than INT |
| **Storage waste** | VARCHAR repeated vs single INT FK |
| **No cascade** | Delete supplier leaves orphan records |

### 1.3 Real Example from Code

```cpp
// inputdialogproduto.cpp:215 - Searching by name (fragile!)
query.bindValue(":razaoSocial", modelPedidoFornecedor.data(0, "fornecedor"));

// If name has typo or variation, this fails silently
```

---

## 2. Affected Tables

### 2.1 Tables with Denormalized Supplier

| Table | Column | Usage Count in Code |
|-------|--------|---------------------|
| `produto` | `fornecedor` | ~15 files |
| `venda_has_produto` | `fornecedor` | ~10 files |
| `venda_has_produto2` | `fornecedor` | ~20 files |
| `pedido_fornecedor_has_produto` | `fornecedor` | ~8 files |
| `pedido_fornecedor_has_produto2` | `fornecedor` | ~12 files |
| `estoque` | `fornecedor` | ~8 files |
| `estoque_has_consumo` | `fornecedor` | ~5 files |
| `compra_avulsa` | `fornecedor` | ~3 files |
| `orcamento_has_produto` | `fornecedor` | ~5 files |

**Total**: ~9 tables, ~85+ code references

### 2.2 Tables That Already Have FK

```sql
-- These are correct
produto.idFornecedor → fornecedor.idFornecedor
pedido_fornecedor.idFornecedor → fornecedor.idFornecedor
```

**Problem**: Both `idFornecedor` (FK) AND `fornecedor` (VARCHAR) exist!

---

## 3. Impact Analysis

### 3.1 Current Data Issues

```sql
-- Find inconsistencies
SELECT DISTINCT p.fornecedor, f.razaoSocial
FROM produto p
LEFT JOIN fornecedor f ON p.idFornecedor = f.idFornecedor
WHERE p.fornecedor != f.razaoSocial
  AND p.fornecedor IS NOT NULL;

-- Common issues found:
-- "TRAMONTINA " vs "TRAMONTINA" (trailing space)
-- "Tok&Stok" vs "TOK&STOK" (case difference)
-- Old name vs new name after rename
```

### 3.2 Code Patterns to Fix

**Pattern 1: Setting supplier name manually**
```cpp
// Current: Copy name string
modelEstoque.setData(newRow, "fornecedor", xml.xNome);

// Should be: Use FK
modelEstoque.setData(newRow, "fornecedor_id", fornecedorId);
```

**Pattern 2: Comparing by name**
```cpp
// Current: String comparison
if (modelItem.data(0, "fornecedor").toString() == "ATELIER STACCATO") { ... }

// Should be: Compare FK or use constant
if (modelItem.data(0, "fornecedor_id").toInt() == ATELIER_STACCATO_ID) { ... }
```

**Pattern 3: Grouping by supplier**
```cpp
// Current: Group by name (slow, error-prone)
for (row : rows) { fornecedores << modelItem.data(row, "fornecedor").toString(); }

// Should be: Group by FK
for (row : rows) { fornecedorIds << modelItem.data(row, "fornecedor_id").toInt(); }
```

---

## 4. Proposed Solution

### 4.1 New Schema

```sql
-- Keep fornecedor table as-is (already normalized)
-- fornecedor (idFornecedor PK, razaoSocial, cnpj, ...)

-- Add FK columns to affected tables
ALTER TABLE venda_has_produto2
    ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);

ALTER TABLE estoque
    ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);

ALTER TABLE estoque_has_consumo
    ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);

-- etc for other tables

-- Create indexes
CREATE INDEX idx_vhp2_fornecedor ON venda_has_produto2(fornecedor_id);
CREATE INDEX idx_estoque_fornecedor ON estoque(fornecedor_id);
```

### 4.2 PostgreSQL Schema (New System)

```sql
-- Suppliers table
CREATE TABLE fornecedores (
    id SERIAL PRIMARY KEY,
    razao_social VARCHAR(200) NOT NULL,
    nome_fantasia VARCHAR(200),
    cnpj VARCHAR(18) UNIQUE,
    inscricao_estadual VARCHAR(20),

    -- Contact
    email VARCHAR(200),
    telefone VARCHAR(20),

    -- Banking
    banco VARCHAR(100),
    agencia VARCHAR(20),
    conta VARCHAR(20),

    -- Business rules
    comissao_percentual DECIMAL(5,2) DEFAULT 0,
    frete_pago_loja BOOLEAN DEFAULT FALSE,
    representacao BOOLEAN DEFAULT FALSE,

    -- Status
    ativo BOOLEAN DEFAULT TRUE,

    -- Audit
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- All related tables use FK
CREATE TABLE venda_itens (
    -- ...
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),
    -- Remove: fornecedor VARCHAR
);

CREATE TABLE estoques (
    -- ...
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),
    -- Remove: fornecedor VARCHAR
);
```

### 4.3 View for Backwards Compatibility

```sql
-- During transition, create view that includes supplier name
CREATE VIEW venda_itens_com_fornecedor AS
SELECT
    vi.*,
    f.razao_social as fornecedor,
    f.nome_fantasia as fornecedor_fantasia
FROM venda_itens vi
JOIN fornecedores f ON vi.fornecedor_id = f.id;
```

---

## 5. Migration Strategy

### Phase 1: Add FK Columns (Non-Breaking)

```sql
-- Add nullable FK columns
ALTER TABLE venda_has_produto2
    ADD COLUMN fornecedor_id INTEGER;

ALTER TABLE estoque
    ADD COLUMN fornecedor_id INTEGER;

-- Populate from existing names
UPDATE venda_has_produto2 vp
SET fornecedor_id = f.idFornecedor
FROM fornecedor f
WHERE UPPER(TRIM(vp.fornecedor)) = UPPER(TRIM(f.razaoSocial));

UPDATE estoque e
SET fornecedor_id = f.idFornecedor
FROM fornecedor f
WHERE UPPER(TRIM(e.fornecedor)) = UPPER(TRIM(f.razaoSocial));

-- Check for unmatched records
SELECT DISTINCT fornecedor
FROM venda_has_produto2
WHERE fornecedor_id IS NULL
  AND fornecedor IS NOT NULL;
```

### Phase 2: Handle Unmatched Names

```sql
-- Option A: Create missing suppliers
INSERT INTO fornecedor (razaoSocial)
SELECT DISTINCT vp.fornecedor
FROM venda_has_produto2 vp
WHERE vp.fornecedor_id IS NULL
  AND vp.fornecedor IS NOT NULL
  AND NOT EXISTS (
      SELECT 1 FROM fornecedor f
      WHERE UPPER(TRIM(f.razaoSocial)) = UPPER(TRIM(vp.fornecedor))
  );

-- Option B: Manual review of variations
-- Export list for manual matching
COPY (
    SELECT DISTINCT fornecedor, COUNT(*) as count
    FROM venda_has_produto2
    WHERE fornecedor_id IS NULL
    GROUP BY fornecedor
    ORDER BY count DESC
) TO '/tmp/unmatched_suppliers.csv' CSV HEADER;
```

### Phase 3: Add Constraints

```sql
-- After all data migrated
ALTER TABLE venda_has_produto2
    ALTER COLUMN fornecedor_id SET NOT NULL,
    ADD CONSTRAINT fk_vhp2_fornecedor
        FOREIGN KEY (fornecedor_id)
        REFERENCES fornecedor(idFornecedor);

-- Create index
CREATE INDEX idx_vhp2_fornecedor ON venda_has_produto2(fornecedor_id);
```

### Phase 4: Dual-Write in Application

```php
// During transition: write to both columns
class VendaItemService
{
    public function create(array $data): VendaItem
    {
        return VendaItem::create([
            'fornecedor_id' => $data['fornecedor_id'],
            // Keep writing to old column for backwards compatibility
            'fornecedor' => Fornecedor::find($data['fornecedor_id'])->razao_social,
            // ...
        ]);
    }
}
```

### Phase 5: Drop Old Columns

```sql
-- After all code migrated
ALTER TABLE venda_has_produto2 DROP COLUMN fornecedor;
ALTER TABLE estoque DROP COLUMN fornecedor;
-- etc.
```

---

## 6. Code Changes

### 6.1 Laravel Models

```php
<?php

namespace App\Models;

class VendaItem extends Model
{
    protected $fillable = [
        'venda_id',
        'produto_id',
        'fornecedor_id',  // FK instead of name
        'quantidade',
        // ...
    ];

    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }

    // Accessor for backwards compatibility
    public function getFornecedorNomeAttribute(): string
    {
        return $this->fornecedor->razao_social;
    }
}

class Estoque extends Model
{
    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }
}

class Fornecedor extends Model
{
    public function produtos(): HasMany
    {
        return $this->hasMany(Produto::class);
    }

    public function vendaItens(): HasMany
    {
        return $this->hasMany(VendaItem::class);
    }

    public function estoques(): HasMany
    {
        return $this->hasMany(Estoque::class);
    }

    // Helper: find by name (for migration/import)
    public static function findByName(string $name): ?self
    {
        return static::whereRaw(
            'UPPER(TRIM(razao_social)) = ?',
            [strtoupper(trim($name))]
        )->first();
    }

    // Helper: find or create by name
    public static function findOrCreateByName(string $name): self
    {
        return static::findByName($name)
            ?? static::create(['razao_social' => trim($name)]);
    }
}
```

### 6.2 Import Service (NFe)

```php
<?php

namespace App\Services\NFe;

class NfeImportService
{
    public function importarEstoque(NfeXml $xml): Estoque
    {
        // Get or create supplier by CNPJ (preferred) or name
        $fornecedor = Fornecedor::where('cnpj', $xml->emitente->cnpj)->first()
            ?? Fornecedor::findOrCreateByName($xml->emitente->razaoSocial);

        return Estoque::create([
            'fornecedor_id' => $fornecedor->id,  // FK!
            'quantidade' => $xml->quantidade,
            // ...
        ]);
    }
}
```

### 6.3 Query Examples

```php
// Old: Group by name (slow, error-prone)
$vendas->groupBy('fornecedor');

// New: Group by FK (fast, reliable)
$vendas->groupBy('fornecedor_id');

// Old: Filter by name
VendaItem::where('fornecedor', 'ACME Corp')->get();

// New: Filter by FK
VendaItem::where('fornecedor_id', $acmeId)->get();

// Or with relationship
VendaItem::whereHas('fornecedor', fn($q) =>
    $q->where('razao_social', 'like', '%ACME%')
)->get();

// Eager load supplier name
VendaItem::with('fornecedor:id,razao_social')->get();
```

### 6.4 Display in UI

```php
// Blade template
{{ $item->fornecedor->razao_social }}

// Or with accessor
{{ $item->fornecedor_nome }}

// Vue/Inertia
<td>{{ item.fornecedor.razao_social }}</td>
```

---

## 7. Special Cases

### 7.1 "ATELIER STACCATO" Check

```cpp
// Current: Magic string comparison
if (modelItem.data(0, "fornecedor").toString() == "ATELIER STACCATO") { ... }
```

```php
// New: Use constant or config
class Fornecedor extends Model
{
    // Known supplier IDs
    public const ATELIER_STACCATO_ID = 1;  // Or from config

    public function isAtelierStaccato(): bool
    {
        return $this->id === self::ATELIER_STACCATO_ID;
    }
}

// Usage
if ($item->fornecedor->isAtelierStaccato()) { ... }
```

### 7.2 Supplier Name in Reports

```php
// For reports/exports, join supplier name
$items = VendaItem::query()
    ->select('venda_itens.*', 'f.razao_social as fornecedor_nome')
    ->join('fornecedores as f', 'f.id', '=', 'venda_itens.fornecedor_id')
    ->get();
```

### 7.3 Historical Data

```php
// For audit purposes, may want to snapshot supplier name at transaction time
CREATE TABLE venda_itens (
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    fornecedor_nome_snapshot VARCHAR(200),  -- Name at time of sale
);

// Set on creation
$item = VendaItem::create([
    'fornecedor_id' => $fornecedor->id,
    'fornecedor_nome_snapshot' => $fornecedor->razao_social,
]);
```

---

## 8. Benefits Summary

| Aspect | Before | After |
|--------|--------|-------|
| **Storage** | VARCHAR repeated in 9 tables | Single INT FK |
| **Rename** | Update 9 tables manually | Update 1 table |
| **Integrity** | None (orphans possible) | FK constraint |
| **Query speed** | String comparison | INT comparison |
| **Typos** | Silently break queries | Impossible |
| **Joins** | By string (slow) | By FK (fast) |

---

## Related Documents

- [03-improvements.md](./03-improvements.md) - Full improvements list
- [04-l1l2-simplification.md](./04-l1l2-simplification.md) - Table flattening (includes fornecedor_id)
- [../business/04-cadastros-flows.md](../business/04-cadastros-flows.md) - Supplier registration flow
