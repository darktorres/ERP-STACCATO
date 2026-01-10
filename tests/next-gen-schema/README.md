# Next-Gen Schema Test

Interactive demo to validate the new PostgreSQL schema that replaces L1/L2 tables.

## What This Tests

| Old (L1/L2)                                       | New (Unified)                    |
| ------------------------------------------------- | -------------------------------- |
| `venda_has_produto` + `venda_has_produto2`        | Single `venda_itens` table       |
| `idRelacionado` chains for splits                 | `parent_id` + `root_id` columns  |
| VARCHAR status strings                            | PostgreSQL ENUMs                 |
| Manual sync between L1 and L2                     | No sync needed - single table    |
| Complex JOIN queries                              | Simple queries + aggregated view |

## Quick Start (Docker)

**Prerequisites:** Docker Desktop

```bash
cd tests/next-gen-schema
run-demo.bat
```

This will:
1. Start PostgreSQL 16 + PHP 8.3 containers
2. Open browser at **http://localhost:8080**

Or manually:
```bash
docker-compose up -d
# Open http://localhost:8080
```

## Web Demo Features

- **View Data:** Vendas, Venda Itens, Estoques, Consumos
- **Operations:**
  - Create Venda (ENUM status)
  - Add Item (single INSERT, no L2 trigger!)
  - Split Item (parent_id/root_id hierarchy)
  - Pair Item with Stock (1:1 consumption)
  - Reverse Consumption (estorno)
- **Analysis:**
  - Aggregated View (replaces L1 queries)
  - Split Hierarchy visualization

## Manual Commands

```bash
# Start containers
docker-compose up -d

# Run interactive demo
docker exec -it erp_nextgen_php php demo.php

# Run SQL tests directly
docker exec -i erp_nextgen_postgres psql -U postgres -d erp_nextgen_test < test.sql

# Stop containers
docker-compose down

# Stop and remove data
docker-compose down -v
```

## What Gets Tested

1. **Create Venda** - Basic venda creation with ENUM status
2. **Add Venda Item** - Single INSERT (no L2 trigger copy!)
3. **Split Item** - Using `split_venda_item()` function with `parent_id`/`root_id`
4. **Cascading Split** - `root_id` correctly traces back to original
5. **Estoque Consumo** - 1:1 pairing between item and stock
6. **Auto-Updates** - Triggers update stock and item status
7. **Estorno** - Reversal restores stock correctly
8. **Validations**:
   - Product mismatch rejected
   - Quantity mismatch rejected
   - 1:1 constraint (no double pairing)
   - Insufficient stock rejected
9. **Aggregated View** - Replaces complex L1+L2 JOIN queries

## Key Schema Differences

### Split Hierarchy (replaces idRelacionado)

```
OLD: idRelacionado chains can get lost in cascading splits
     Item 100 -> Item 101 (idRelacionado=100) -> Item 102 (idRelacionado=101)
     Need recursive query to find original!

NEW: root_id always points to original
     Item 1 (parent=NULL, root=NULL)      -- original
     Item 2 (parent=1, root=1)            -- first split
     Item 3 (parent=2, root=1)            -- cascade split still points to 1!

     Simple query: WHERE id = 1 OR root_id = 1
```

### Stock Consumption (1:1)

```sql
-- 1:1 constraint enforced by partial unique indexes
CREATE UNIQUE INDEX idx_consumos_venda_item_ativo
    ON estoque_consumos(venda_item_id)
    WHERE NOT is_estornado;

CREATE UNIQUE INDEX idx_consumos_estoque_ativo
    ON estoque_consumos(estoque_id)
    WHERE NOT is_estornado;
```

### Status as ENUM

```sql
-- Type-safe, no magic strings
CREATE TYPE venda_item_status AS ENUM (
    'PENDENTE', 'EM_COMPRA', 'CONFIRMADO', 'FATURADO',
    'EM_COLETA', 'EM_RECEBIMENTO', 'ESTOQUE',
    'ENTREGA_AGENDADA', 'EM_ENTREGA', 'ENTREGUE',
    'DEVOLVIDO', 'CANCELADO'
);
```
