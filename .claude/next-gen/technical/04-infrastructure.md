# Infrastructure Architecture

> Status: **Draft**
> Last updated: 2025-12-27
> Focus: Audit, temporal data, search, pg_ivm materialized views

---

## Table of Contents

1. [Audit Trail Architecture](#1-audit-trail-architecture)
2. [Temporal Data / Point-in-Time Queries](#2-temporal-data--point-in-time-queries)
3. [Search Architecture](#3-search-architecture)
4. [Materialized Views](#4-materialized-views)

---

## 1. Audit Trail Architecture

### Requirements
- Track ALL changes to critical tables
- Know WHO made the change
- Know WHEN it happened
- Know WHAT changed (old → new values)
- Ability to query historical state

### Recommended: Audit Log Table

```sql
CREATE TABLE audit_log (
    id BIGSERIAL PRIMARY KEY,

    -- What changed
    table_name VARCHAR(100) NOT NULL,
    record_id INTEGER NOT NULL,
    action VARCHAR(20) NOT NULL, -- INSERT, UPDATE, DELETE

    -- The changes
    old_values JSONB,
    new_values JSONB,
    changed_fields TEXT[], -- which columns changed

    -- Who and when
    user_id INTEGER,
    user_name VARCHAR(100), -- denormalized for history
    ip_address INET,
    user_agent TEXT,

    -- Context
    transaction_id VARCHAR(100), -- group related changes
    module VARCHAR(50), -- 'compras', 'vendas', etc.
    reason TEXT, -- optional justification

    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_audit_table_record ON audit_log(table_name, record_id);
CREATE INDEX idx_audit_created ON audit_log(created_at);
CREATE INDEX idx_audit_user ON audit_log(user_id);
CREATE INDEX idx_audit_transaction ON audit_log(transaction_id);
```

### Generic Audit Trigger

```sql
CREATE OR REPLACE FUNCTION audit_trigger_func()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'DELETE' THEN
        INSERT INTO audit_log (table_name, record_id, action, old_values, user_id)
        VALUES (TG_TABLE_NAME, OLD.id, 'DELETE', to_jsonb(OLD),
                current_setting('app.user_id', true)::int);
        RETURN OLD;
    ELSIF TG_OP = 'UPDATE' THEN
        INSERT INTO audit_log (table_name, record_id, action, old_values, new_values, user_id)
        VALUES (TG_TABLE_NAME, NEW.id, 'UPDATE', to_jsonb(OLD), to_jsonb(NEW),
                current_setting('app.user_id', true)::int);
        RETURN NEW;
    ELSIF TG_OP = 'INSERT' THEN
        INSERT INTO audit_log (table_name, record_id, action, new_values, user_id)
        VALUES (TG_TABLE_NAME, NEW.id, 'INSERT', to_jsonb(NEW),
                current_setting('app.user_id', true)::int);
        RETURN NEW;
    END IF;
END;
$$ LANGUAGE plpgsql;

-- Apply to important tables
CREATE TRIGGER audit_vendas
    AFTER INSERT OR UPDATE OR DELETE ON vendas
    FOR EACH ROW EXECUTE FUNCTION audit_trigger_func();
```

---

## 2. Temporal Data / Point-in-Time Queries

### Use Cases

| Query | Example |
|-------|---------|
| Point-in-time | "What was the stock of product X on Jan 15?" |
| History | "Show all price changes for product X in 2024" |
| Audit | "Who changed this record and when?" |
| Rollback | "What did this sale look like before the edit?" |
| Reporting | "What was our total receivables on Dec 31?" |

### Option A: Temporal Tables with History

```sql
-- Main table (current state)
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,
    cliente_id INTEGER,
    status VARCHAR(50),
    total DECIMAL(15,2),
    -- ... other fields

    -- Temporal metadata
    valid_from TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    valid_to TIMESTAMPTZ NOT NULL DEFAULT 'infinity',

    -- Audit metadata
    created_by INTEGER,
    updated_by INTEGER,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- History table (past states)
CREATE TABLE vendas_history (
    history_id BIGSERIAL PRIMARY KEY,
    id INTEGER NOT NULL, -- original record id
    cliente_id INTEGER,
    status VARCHAR(50),
    total DECIMAL(15,2),
    -- ... mirror all fields

    valid_from TIMESTAMPTZ NOT NULL,
    valid_to TIMESTAMPTZ NOT NULL,

    operation VARCHAR(10), -- UPDATE, DELETE
    changed_by INTEGER,
    changed_at TIMESTAMPTZ DEFAULT NOW()
);

-- Trigger to maintain history
CREATE OR REPLACE FUNCTION vendas_history_trigger()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'UPDATE' THEN
        -- Close the old record
        INSERT INTO vendas_history
        SELECT nextval('vendas_history_history_id_seq'),
               OLD.*,
               OLD.valid_from, NOW(), 'UPDATE',
               current_setting('app.user_id', true)::int, NOW();

        -- Update valid_from on new record
        NEW.valid_from := NOW();
        NEW.updated_at := NOW();
        NEW.updated_by := current_setting('app.user_id', true)::int;
        RETURN NEW;

    ELSIF TG_OP = 'DELETE' THEN
        INSERT INTO vendas_history
        SELECT nextval('vendas_history_history_id_seq'),
               OLD.*,
               OLD.valid_from, NOW(), 'DELETE',
               current_setting('app.user_id', true)::int, NOW();
        RETURN OLD;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER vendas_history
    BEFORE UPDATE OR DELETE ON vendas
    FOR EACH ROW EXECUTE FUNCTION vendas_history_trigger();
```

### Querying at Point in Time

```sql
-- What was venda 123 on 2025-01-15?
SELECT * FROM vendas WHERE id = 123
  AND valid_from <= '2025-01-15' AND valid_to > '2025-01-15'
UNION ALL
SELECT id, cliente_id, status, total, ... FROM vendas_history WHERE id = 123
  AND valid_from <= '2025-01-15' AND valid_to > '2025-01-15';
```

### Option B: Snapshot Tables

For specific reporting needs, take periodic snapshots:

```sql
CREATE TABLE estoque_snapshots (
    id SERIAL PRIMARY KEY,
    snapshot_date DATE NOT NULL,
    produto_id INTEGER NOT NULL,
    quantidade DECIMAL(15,4),
    valor_total DECIMAL(15,2),
    created_at TIMESTAMPTZ DEFAULT NOW(),

    UNIQUE(snapshot_date, produto_id)
);

-- Daily job to capture snapshot
INSERT INTO estoque_snapshots (snapshot_date, produto_id, quantidade, valor_total)
SELECT
    CURRENT_DATE,
    produto_id,
    SUM(quantidade_disponivel),
    SUM(quantidade_disponivel * custo_unitario)
FROM estoques
GROUP BY produto_id;
```

---

## 3. Search Architecture

### Current State
- MySQL FULLTEXT indexes
- LIKE queries with wildcards
- Slow on large datasets
- Limited features (no fuzzy, no synonyms, no ranking)

### Option A: PostgreSQL Full-Text Search (Recommended Start)

```sql
-- Add search vector column
ALTER TABLE produtos ADD COLUMN search_vector tsvector;

-- Create GIN index
CREATE INDEX idx_produtos_search ON produtos USING GIN(search_vector);

-- Update trigger
CREATE OR REPLACE FUNCTION produtos_search_trigger() RETURNS trigger AS $$
BEGIN
    NEW.search_vector :=
        setweight(to_tsvector('portuguese', COALESCE(NEW.descricao, '')), 'A') ||
        setweight(to_tsvector('portuguese', COALESCE(NEW.cod_comercial, '')), 'B') ||
        setweight(to_tsvector('portuguese', COALESCE(NEW.marca, '')), 'C');
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER produtos_search_update
    BEFORE INSERT OR UPDATE ON produtos
    FOR EACH ROW EXECUTE FUNCTION produtos_search_trigger();

-- Search query
SELECT
    id, descricao,
    ts_rank(search_vector, query) as rank
FROM produtos, plainto_tsquery('portuguese', 'mesa escritorio madeira') query
WHERE search_vector @@ query
ORDER BY rank DESC
LIMIT 20;
```

**Features:**
- Stemming (Portuguese)
- Ranking
- Phrase search
- Prefix matching
- Weights by field

### Option B: Elasticsearch (If Needed Later)

```
┌─────────────┐     sync      ┌─────────────────┐
│ PostgreSQL  │ ───────────►  │  Elasticsearch  │
│  (source)   │               │    (search)     │
└─────────────┘               └─────────────────┘
```

**When to upgrade:**
- Need typo tolerance ("meza" finds "mesa")
- Need autocomplete with fuzzy
- Dataset > 1M products
- Complex faceted search needed

---

## 4. Materialized Views

### Problem
Views recalculate on every query - slow for dashboards.

### Solution: Materialized Views with pg_ivm (Recommended)

**pg_ivm** (Incremental View Maintenance) provides auto-updating materialized views that only refresh changed rows - not the entire view.

#### Installing pg_ivm

```sql
-- Install the extension
CREATE EXTENSION pg_ivm;
```

#### Creating Incrementally-Maintained Views

```sql
-- Use create_immv instead of CREATE MATERIALIZED VIEW
SELECT create_immv('immv_produto_estoque', $$
    SELECT
        p.id as produto_id,
        p.descricao,
        p.fornecedor_id,
        f.razao_social as fornecedor_nome,
        COALESCE(SUM(e.quantidade_disponivel), 0) as estoque_total,
        COALESCE(AVG(e.custo_unitario), 0) as custo_medio,
        MAX(e.data_entrada) as ultima_entrada
    FROM produtos p
    LEFT JOIN fornecedores f ON p.fornecedor_id = f.id
    LEFT JOIN estoques e ON p.id = e.produto_id AND e.quantidade_disponivel > 0
    GROUP BY p.id, p.descricao, p.fornecedor_id, f.razao_social
$$);

-- The view automatically updates when produtos, fornecedores, or estoques change!
-- No manual REFRESH needed.
```

#### pg_ivm vs Standard Materialized Views

| Feature | Standard MV | pg_ivm (IMMV) |
|---------|-------------|---------------|
| Auto-update | No (manual REFRESH) | Yes (automatic) |
| Update speed | Full rebuild | Incremental (only changes) |
| Consistency | Stale until refresh | Always current |
| Overhead | None between refreshes | Slight on each DML |
| Best for | Large, rarely-changing | Frequently-changing data |

#### Supported Query Features

pg_ivm supports most common query patterns:

```sql
-- Aggregates (SUM, COUNT, AVG, MIN, MAX)
SELECT create_immv('immv_vendas_por_cliente', $$
    SELECT
        cliente_id,
        COUNT(*) as total_vendas,
        SUM(total) as valor_total
    FROM vendas
    WHERE status = 'completed'
    GROUP BY cliente_id
$$);

-- JOINs (INNER, LEFT, RIGHT)
SELECT create_immv('immv_itens_com_produto', $$
    SELECT
        vi.id,
        vi.venda_id,
        p.descricao as produto_nome,
        vi.quantidade,
        vi.preco_unitario
    FROM venda_itens vi
    JOIN produtos p ON p.id = vi.produto_id
$$);

-- DISTINCT
SELECT create_immv('immv_fornecedores_ativos', $$
    SELECT DISTINCT fornecedor_id
    FROM estoques
    WHERE quantidade_disponivel > 0
$$);
```

#### Limitations

pg_ivm does NOT support:
- Window functions (`ROW_NUMBER`, `RANK`, etc.)
- CTEs (`WITH` clauses)
- Subqueries in `FROM`
- `UNION`, `INTERSECT`, `EXCEPT`
- `HAVING` (use CTE workaround in app layer)

For these, use standard materialized views with scheduled refresh.

#### Managing IMMVs

```sql
-- List all incrementally-maintained materialized views
SELECT * FROM pg_ivm_immv;

-- Drop an IMMV
SELECT drop_immv('immv_produto_estoque');

-- Temporarily disable auto-refresh (for bulk operations)
SELECT immv_set_pause('immv_produto_estoque', true);

-- Re-enable
SELECT immv_set_pause('immv_produto_estoque', false);

-- Manual refresh if needed
REFRESH MATERIALIZED VIEW immv_produto_estoque;
```

#### Best Practices

1. **Use IMMVs for dashboards** - Always-current data without polling
2. **Pause during bulk imports** - Avoid overhead during large data loads
3. **Index the IMMV** - Create indexes just like regular tables
4. **Monitor overhead** - Check if DML operations slow down

```sql
-- Create indexes on IMMV
CREATE INDEX idx_immv_estoque_fornecedor ON immv_produto_estoque(fornecedor_id);
CREATE INDEX idx_immv_estoque_total ON immv_produto_estoque(estoque_total);
```

---

### Standard Materialized Views (When pg_ivm Doesn't Apply)

For queries with window functions, CTEs, or other unsupported features, use standard materialized views with scheduled refresh:

```sql
-- Create materialized view
CREATE MATERIALIZED VIEW mv_produto_estoque AS
SELECT
    p.id as produto_id,
    p.descricao,
    p.fornecedor_id,
    f.razao_social as fornecedor_nome,
    COALESCE(SUM(e.quantidade_disponivel), 0) as estoque_total,
    COALESCE(AVG(e.custo_unitario), 0) as custo_medio,
    MAX(e.data_entrada) as ultima_entrada
FROM produtos p
LEFT JOIN fornecedores f ON p.fornecedor_id = f.id
LEFT JOIN estoques e ON p.id = e.produto_id AND e.quantidade_disponivel > 0
GROUP BY p.id, p.descricao, p.fornecedor_id, f.razao_social;

-- Create indexes on materialized view
CREATE UNIQUE INDEX ON mv_produto_estoque(produto_id);
CREATE INDEX ON mv_produto_estoque(fornecedor_id);
CREATE INDEX ON mv_produto_estoque(estoque_total);

-- Refresh strategies:
-- 1. Concurrent refresh (non-blocking, requires unique index)
REFRESH MATERIALIZED VIEW CONCURRENTLY mv_produto_estoque;

-- 2. Scheduled refresh (via pg_cron)
SELECT cron.schedule('refresh-estoque', '*/5 * * * *',
    'REFRESH MATERIALIZED VIEW CONCURRENTLY mv_produto_estoque');
```

### Candidate Views for Materialization

| View | Type | Refresh | Reason |
|------|------|---------|--------|
| `immv_produto_estoque` | **pg_ivm** | Auto | Stock levels need real-time accuracy |
| `immv_vendas_dashboard` | **pg_ivm** | Auto | Dashboard must be current |
| `immv_order_totals` | **pg_ivm** | Auto | Order totals change with items |
| `immv_cliente_stats` | **pg_ivm** | Auto | Customer purchase history |
| `mv_financeiro_resumo` | Standard | 1 hour | Complex queries, less frequent |
| `mv_fornecedor_performance` | Standard | Daily | Historical analysis, window functions |
| `mv_produto_ranking` | Standard | Daily | Uses RANK() window function |

### Refresh with Logging

```sql
CREATE TABLE mv_refresh_log (
    id SERIAL PRIMARY KEY,
    view_name VARCHAR(100) NOT NULL,
    started_at TIMESTAMPTZ NOT NULL,
    finished_at TIMESTAMPTZ,
    duration_ms INTEGER,
    rows_affected INTEGER,
    status VARCHAR(20) DEFAULT 'running'
);

CREATE OR REPLACE FUNCTION refresh_mv_with_logging(view_name TEXT)
RETURNS void AS $$
DECLARE
    start_time TIMESTAMPTZ;
    log_id INTEGER;
BEGIN
    start_time := NOW();

    INSERT INTO mv_refresh_log (view_name, started_at)
    VALUES (view_name, start_time)
    RETURNING id INTO log_id;

    EXECUTE 'REFRESH MATERIALIZED VIEW CONCURRENTLY ' || view_name;

    UPDATE mv_refresh_log
    SET finished_at = NOW(),
        duration_ms = EXTRACT(MILLISECONDS FROM NOW() - start_time),
        status = 'completed'
    WHERE id = log_id;
END;
$$ LANGUAGE plpgsql;
```

---

## Summary Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        APPLICATION                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│  │   Current   │  │   History   │  │     Audit Log           │ │
│  │   Tables    │  │   Tables    │  │  (who did what when)    │ │
│  │             │  │  (temporal) │  │                         │ │
│  └──────┬──────┘  └──────┬──────┘  └────────────┬────────────┘ │
│         │                │                      │               │
│         └────────────────┼──────────────────────┘               │
│                          │                                      │
│  ┌───────────────────────▼───────────────────────────────────┐ │
│  │                    PostgreSQL                              │ │
│  │  • Temporal tables (valid_from/valid_to)                  │ │
│  │  • pg_ivm (auto-updating materialized views)              │ │
│  │  • Standard MVs (complex queries, scheduled refresh)      │ │
│  │  • Full-text search (tsvector)                            │ │
│  │  • JSONB (flexible attributes, tax data)                  │ │
│  │  • ENUMs (type-safe status)                               │ │
│  └───────────────────────────────────────────────────────────┘ │
│                          │                                      │
│                          │ (optional, if needed)                │
│                          ▼                                      │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                   Elasticsearch                            │ │
│  │  • Fuzzy search                                           │ │
│  │  • Autocomplete                                           │ │
│  │  • Faceted search                                         │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Related Documents

- [02-database.md](./02-database.md) - Core schema design
- [../business/02-stock-flows.md](../business/02-stock-flows.md) - Stock data integrity rules
