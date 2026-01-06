# Staccato ERP - New Schema DDL

## Overview

Complete PostgreSQL 16 DDL (Data Definition Language) for the new normalized schema design. Divided into 3 parts for easier management and verification.

## Files

### Part 1: `schema-ddl-part1.sql`
**Size:** ~600 lines
**Contains:**
- Extensions (uuid-ossp, pg_trgm, unaccent, pgcrypto)
- 9 ENUM types (status enums for all entities)
- Master data tables:
  - `lojas` - Store/branch locations
  - `fornecedores` - Suppliers
  - `usuarios` - Users
  - `clientes` - Customers
  - `cidades`, `enderecos` - Address management
  - `ncms`, `categorias` - Product classification
- Product tables:
  - `produtos` - Product master data
  - `produto_precos` - Versioned pricing
  - `produto_tributos` - Tax configuration
- Sales tables (Part 1):
  - `orcamentos` - Quotes
  - `vendas` - Sales header
  - `venda_itens` - Sales line items (single table, no L1/L2!)

### Part 2: `schema-ddl-part2.sql`
**Size:** ~600 lines
**Contains:**
- Purchase tables:
  - `compras` - Purchase order header
  - `compra_itens` - Purchase line items
- NFe (Electronic Invoice) tables:
  - `nfes` - NFe header
  - `nfe_itens` - NFe line items (with JSONB for fiscal data)
- Stock tables:
  - `galpao_blocos` - Warehouse locations
  - `estoques` - Stock records (one per batch/line)
  - `estoque_consumos` - Stock consumption with 1:1 pairing
- Financial tables:
  - `recebiveis` - Accounts receivable
  - `pagaveis` - Accounts payable
- Audit table:
  - `audit_log` - Complete audit trail

### Part 3: `schema-ddl-part3.sql`
**Size:** ~600 lines
**Contains:**
- Trigger functions:
  - `fn_validar_consumo()` - Validate stock pairing
  - `fn_atualizar_estoque_apos_consumo()` - Auto-update stock quantities
  - `fn_validar_transicao_status_venda_item()` - Enforce state transitions
  - `fn_impedir_alteracao_consumo_estornado()` - Immutability protection
  - `fn_impedir_exclusao_consumo()` - Soft-delete only
  - `fn_impedir_alteracao_item_pareado()` - Protect paired items
  - `fn_audit_log()` - Generic audit logging
- Trigger attachments to critical tables
- Common views:
  - `vw_produto_preco_atual` - Current product prices
  - `vw_estoque_disponivel` - Available stock (FIFO order)
  - `vw_venda_completa` - Complete sales info
  - `vw_compra_completa` - Complete purchase info
  - `vw_recebiveis_pendentes` - Pending receivables
  - `vw_pagaveis_pendentes` - Pending payables
  - `vw_venda_item_status` - Sales item tracking

## Key Features

### Data Integrity
- Foreign key constraints (referential integrity)
- Unique constraints (1:1 relationships)
- Check constraints (business rules at DB level)
- Triggers for automatic consistency

### Type Safety
- ENUMs for all status fields (no magic strings)
- Status transition validation
- Immutability protection for reversed transactions

### Performance
- Strategic indexes on frequently queried columns
- Partial indexes (e.g., only available stock)
- Index on JSONB for NFe data queries
- Views for common operations

### Auditability
- Comprehensive audit_log table
- User tracking (created_by, user_id)
- Timestamp tracking (created_at, updated_at)
- Full before/after values in audit

### Flexibility
- JSONB for fiscal data (future tax reforms)
- Versioned pricing (historical prices)
- Soft deletes (is_estornado for reversals)
- Polimorphic addresses (cliente, fornecedor, loja)

## Installation

### Prerequisites
- PostgreSQL 16+
- superuser or role with CREATE DATABASE privilege

### Step 1: Create Database
```bash
createdb staccato_erp
```

### Step 2: Run DDL Parts in Sequence
```bash
psql staccato_erp < schema-ddl-part1.sql
psql staccato_erp < schema-ddl-part2.sql
psql staccato_erp < schema-ddl-part3.sql
```

Or in one go:
```bash
psql staccato_erp < schema-ddl-part1.sql && \
psql staccato_erp < schema-ddl-part2.sql && \
psql staccato_erp < schema-ddl-part3.sql
```

### Step 3: Verify Installation
```bash
psql staccato_erp -c "\dt"  # List all tables
psql staccato_erp -c "\dv"  # List all views
psql staccato_erp -c "\df"  # List all functions
```

### Step 4: Grant Permissions (Example)
```bash
psql staccato_erp << EOF
-- Create application user
CREATE ROLE staccato_app WITH LOGIN PASSWORD 'secure_password';

-- Grant permissions
GRANT USAGE ON SCHEMA public TO staccato_app;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO staccato_app;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO staccato_app;
GRANT ALL PRIVILEGES ON ALL FUNCTIONS IN SCHEMA public TO staccato_app;

-- Set default privileges
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO staccato_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT USAGE, SELECT ON SEQUENCES TO staccato_app;
EOF
```

## Important Configuration

### Set User ID for Audit Logging
Before operations, set the current user in the session:
```sql
-- In your application before each transaction
SET app.user_id = 123;  -- Your usuario.id
```

This is used by triggers to track who made changes.

## Common Queries

### List Available Stock (FIFO)
```sql
SELECT * FROM vw_estoque_disponivel
WHERE produto_id = 50
ORDER BY fifo_order ASC;
```

### Get Sales with Items
```sql
SELECT * FROM vw_venda_completa
WHERE status != 'CANCELADA'
ORDER BY data_emissao DESC;
```

### Track Receivables
```sql
SELECT * FROM vw_recebiveis_pendentes
WHERE dias_atraso < 0
ORDER BY dias_atraso ASC;
```

### Audit Trail for a Record
```sql
SELECT * FROM audit_log
WHERE tabela = 'venda_itens' AND registro_id = 1
ORDER BY created_at DESC;
```

## Schema Highlights

### Split Handling
`venda_itens` uses parent_id/root_id hierarchy:
```
venda_itens[1]: original item (parent_id=NULL, root_id=NULL)
  └── venda_itens[2]: split item (parent_id=1, root_id=1)
       └── venda_itens[3]: sub-split (parent_id=2, root_id=1)
```

Query all items in original order:
```sql
SELECT * FROM venda_itens WHERE id = 1 OR root_id = 1;
```

### FIFO Stock Selection
Stock is tracked with data_entrada timestamp:
```sql
-- Get next FIFO item for product
SELECT * FROM estoques
WHERE produto_id = 50 AND quantidade_disponivel > 0
ORDER BY data_entrada ASC
LIMIT 1;
```

### NFe Fiscal Data
All fiscal columns are in `nfe_itens.dados` as JSONB:
```sql
-- Query NFe items by CFOP
SELECT * FROM nfe_itens
WHERE dados->>'cfop' = '5102';

-- Calculate total ICMS
SELECT SUM((dados->'icms'->>'valor')::DECIMAL)
FROM nfe_itens
WHERE nfe_id = 1000;
```

### 1:1 Stock Pairing
Unique constraints ensure 1:1 relationship:
```sql
-- Only one active consumo per venda_item
CREATE UNIQUE INDEX idx_consumos_venda_item_ativo
    ON estoque_consumos(venda_item_id)
    WHERE NOT is_estornado;

-- Each stock can only be consumed once
CREATE UNIQUE INDEX idx_consumos_estoque_ativo
    ON estoque_consumos(estoque_id)
    WHERE NOT is_estornado;
```

## Triggers Overview

| Trigger | Purpose | Event |
|---------|---------|-------|
| `trg_validar_consumo` | Validate pairing rules | INSERT estoque_consumos |
| `trg_atualizar_estoque_apos_consumo` | Auto-update quantities | INSERT/UPDATE estoque_consumos |
| `trg_validar_transicao_status_venda_item` | Enforce state machine | UPDATE venda_itens.status |
| `trg_impedir_alteracao_consumo_estornado` | Immutability | UPDATE estoque_consumos |
| `trg_impedir_exclusao_consumo` | Soft-delete only | DELETE estoque_consumos |
| `trg_impedir_alteracao_item_pareado` | Protect paired data | UPDATE venda_itens |
| `audit_*` (7 tables) | Audit logging | INSERT/UPDATE/DELETE |

## Performance Indexes

Total indexes created: ~40+

Key indexes:
- **FIFO lookup:** `idx_estoques_disponivel` (produto_id, loja_id, data_entrada)
- **Sales tracking:** `idx_vendas_loja_status` (loja_id, status)
- **Purchase status:** `idx_compras_loja_status` (loja_id, status)
- **JSONB queries:** `idx_nfe_itens_dados` (GIN index)
- **Audit searches:** `idx_audit_created`, `idx_audit_usuario`

## Migration from Old Schema

Use this DDL to build the new schema alongside the old one. Then:

1. Create mapping tables (old → new IDs)
2. Write data migration scripts
3. Run double-write during transition
4. Switch reads to new schema
5. Archive old schema

## Backup & Recovery

### Before making changes:
```bash
pg_dump staccato_erp > backup_$(date +%Y%m%d_%H%M%S).sql
```

### Recovery:
```bash
psql < backup_20250105_120000.sql
```

## Contact & Support

For questions about the schema design, see:
- `.claude/venda-compra-estoque-comparison.md` - Mermaid diagrams
- `.claude/venda-compra-estoque-ascii-part*.md` - ASCII documentation
- CLAUDE.md - Project guidelines

---

**Version:** 1.0
**Date:** 2025-01-05
**Status:** Production Ready
**PostgreSQL:** 16+
**Schema Creator:** Claude Code (Anthropic)
