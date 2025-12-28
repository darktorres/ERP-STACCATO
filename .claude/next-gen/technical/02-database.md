# Database Schema Redesign

> Status: **Draft**
> Last updated: 2025-12-27
> Target: PostgreSQL 16

---

## Why PostgreSQL

| Feature | Benefit for This Project |
|---------|-------------------------|
| **Native JSONB** | Flexible tax data, product attributes, audit logs |
| **Native ENUM** | Type-safe status fields |
| **CHECK constraints** | Business rule enforcement at DB level |
| **Full-text search** | Built-in `tsvector` for product search |
| **Better concurrency** | MVCC handles concurrent users well |
| **Schemas** | Multi-tenancy option (schema per loja) |
| **Partitioning** | Table partitioning for large transaction tables |

---

## Current Schema Problems

### 1. Denormalized Supplier Names

Supplier names stored as VARCHAR in multiple tables instead of FK:

| Table | Column |
|-------|--------|
| `venda_has_produto2` | `fornecedor` |
| `estoque` | `fornecedor` |
| `estoque_has_consumo` | `fornecedor` |
| `compra_avulsa` | `fornecedor` |
| `pedido_fornecedor_has_produto2` | `fornecedor` |

**Impact**: If supplier name changes, requires updating 5+ tables.

**Fix**: Replace with `fornecedor_id` FK.

---

### 2. Mega-Table: `produto`

The `produto` table has **100+ columns** including:
- Core product data
- Multiple `*Upd` tracking flags for each field
- Calculated fields (`estoqueRestante`)
- Historical values (`oldPrecoVenda`)
- Multiple boolean flags scattered throughout

**Fix**: Split into normalized tables:

```sql
-- Core product data only
CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    cod_comercial VARCHAR(100),
    descricao VARCHAR(500) NOT NULL,
    ncm_id INTEGER REFERENCES ncms(id),
    unidade VARCHAR(10) DEFAULT 'UN',
    ativo BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Versioned pricing (history preserved)
CREATE TABLE produto_precos (
    id SERIAL PRIMARY KEY,
    produto_id INTEGER REFERENCES produtos(id) ON DELETE CASCADE,
    custo DECIMAL(15,2),
    preco_venda DECIMAL(15,2),
    margem DECIMAL(7,4),
    vigencia_inicio DATE NOT NULL DEFAULT CURRENT_DATE,
    vigencia_fim DATE,
    created_at TIMESTAMP DEFAULT NOW(),

    -- Only one active price per product at a time
    CONSTRAINT one_active_price EXCLUDE USING gist (
        produto_id WITH =,
        daterange(vigencia_inicio, vigencia_fim, '[]') WITH &&
    ) WHERE (vigencia_fim IS NOT NULL)
);

-- Flexible attributes (dimensions, colors, specs)
CREATE TABLE produto_atributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id) ON DELETE CASCADE,
    atributos JSONB DEFAULT '{}'::jsonb,
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Get current price view
CREATE VIEW produto_preco_atual AS
SELECT DISTINCT ON (produto_id) *
FROM produto_precos
WHERE vigencia_inicio <= CURRENT_DATE
  AND (vigencia_fim IS NULL OR vigencia_fim >= CURRENT_DATE)
ORDER BY produto_id, vigencia_inicio DESC;
```

---

### 3. Two-Level Detail Tables

Current pattern uses two levels:
- `venda_has_produto` (Level 1 - aggregated)
- `venda_has_produto2` (Level 2 - detailed)

**Problem**: Complexity, sync issues.

**Fix**: Single `venda_itens` table with proper relationships:

```sql
CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,
    venda_id INTEGER REFERENCES vendas(id) ON DELETE CASCADE,
    produto_id INTEGER REFERENCES produtos(id),
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    quantidade DECIMAL(15,4) NOT NULL,
    preco_unitario DECIMAL(15,2) NOT NULL,
    desconto DECIMAL(7,4) DEFAULT 0,
    -- Denormalized for performance (captured at time of sale)
    descricao_produto VARCHAR(500),
    unidade VARCHAR(10),
    -- Tracking
    estoque_id INTEGER REFERENCES estoques(id), -- which stock was consumed
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_venda_itens_venda ON venda_itens(venda_id);
CREATE INDEX idx_venda_itens_produto ON venda_itens(produto_id);
```

---

### 4. Tax Fields Explosion

Current: 35 inline fields for IBS/CBS/IS in `estoque` and `estoque_has_consumo`.

**Fix**: Separate table with JSONB:

```sql
CREATE TYPE tributo_tipo AS ENUM ('ICMS', 'IPI', 'PIS', 'COFINS', 'IBS', 'CBS', 'IS');

CREATE TABLE item_tributos (
    id SERIAL PRIMARY KEY,
    item_type VARCHAR(50) NOT NULL, -- 'estoque', 'venda_item', 'compra_item'
    item_id INTEGER NOT NULL,
    tributo tributo_tipo NOT NULL,
    valores JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT NOW(),

    UNIQUE(item_type, item_id, tributo)
);

-- Example JSONB structure for ICMS:
-- {
--   "cst": "00",
--   "orig": "0",
--   "vBC": 1000.00,
--   "pICMS": 18.00,
--   "vICMS": 180.00
-- }

-- Example for IBS (Reforma Tributária):
-- {
--   "cst": "01",
--   "cClassTrib": "123456",
--   "vBC": 1000.00,
--   "pIBSUF": 9.5,
--   "pIBSMun": 3.5,
--   "vTribOp": 130.00
-- }

CREATE INDEX idx_item_tributos_item ON item_tributos(item_type, item_id);
```

---

### 5. Status as VARCHAR

Current: Magic strings like `"PENDENTE"`, `"PEND. APROV."`, `"EM ENTREGA"`.

**Fix**: PostgreSQL ENUMs:

```sql
CREATE TYPE venda_status AS ENUM (
    'ORCAMENTO',
    'PENDENTE',
    'ESTOQUE',
    'EM_ENTREGA',
    'ENTREGUE',
    'FINALIZADO',
    'CANCELADO'
);

CREATE TYPE compra_status AS ENUM (
    'PENDENTE',
    'CONFIRMADO',
    'FATURADO',
    'RECEBIDO',
    'CANCELADO'
);

CREATE TYPE nfe_status AS ENUM (
    'PENDENTE',
    'AUTORIZADA',
    'CANCELADA',
    'DENEGADA',
    'INUTILIZADA'
);

-- Usage in table
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,
    status venda_status NOT NULL DEFAULT 'ORCAMENTO',
    -- ...
);
```

---

### 6. No Audit Trail

Current: Some `*Upd` flags but no real audit trail.

**Fix**: Audit log table with triggers:

```sql
CREATE TABLE audit_log (
    id BIGSERIAL PRIMARY KEY,
    table_name VARCHAR(100) NOT NULL,
    record_id INTEGER NOT NULL,
    action VARCHAR(20) NOT NULL, -- INSERT, UPDATE, DELETE
    old_values JSONB,
    new_values JSONB,
    changed_fields TEXT[], -- list of changed column names
    user_id INTEGER REFERENCES usuarios(id),
    ip_address INET,
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_audit_log_table_record ON audit_log(table_name, record_id);
CREATE INDEX idx_audit_log_created ON audit_log(created_at);

-- Generic audit trigger function
CREATE OR REPLACE FUNCTION audit_trigger_func()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'DELETE' THEN
        INSERT INTO audit_log (table_name, record_id, action, old_values, user_id)
        VALUES (TG_TABLE_NAME, OLD.id, 'DELETE', to_jsonb(OLD), current_setting('app.user_id', true)::int);
        RETURN OLD;
    ELSIF TG_OP = 'UPDATE' THEN
        INSERT INTO audit_log (table_name, record_id, action, old_values, new_values, user_id)
        VALUES (TG_TABLE_NAME, NEW.id, 'UPDATE', to_jsonb(OLD), to_jsonb(NEW), current_setting('app.user_id', true)::int);
        RETURN NEW;
    ELSIF TG_OP = 'INSERT' THEN
        INSERT INTO audit_log (table_name, record_id, action, new_values, user_id)
        VALUES (TG_TABLE_NAME, NEW.id, 'INSERT', to_jsonb(NEW), current_setting('app.user_id', true)::int);
        RETURN NEW;
    END IF;
END;
$$ LANGUAGE plpgsql;

-- Apply to important tables
CREATE TRIGGER audit_vendas
    AFTER INSERT OR UPDATE OR DELETE ON vendas
    FOR EACH ROW EXECUTE FUNCTION audit_trigger_func();

CREATE TRIGGER audit_compras
    AFTER INSERT OR UPDATE OR DELETE ON compras
    FOR EACH ROW EXECUTE FUNCTION audit_trigger_func();
```

---

## Proposed Core Schema

### Master Tables

```sql
-- Lojas (stores/branches)
CREATE TABLE lojas (
    id SERIAL PRIMARY KEY,
    cnpj VARCHAR(14) UNIQUE NOT NULL,
    razao_social VARCHAR(255) NOT NULL,
    nome_fantasia VARCHAR(255),
    inscricao_estadual VARCHAR(20),
    configuracoes JSONB DEFAULT '{}',
    ativo BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Usuarios
CREATE TABLE usuarios (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id),
    nome VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    tipo VARCHAR(50) NOT NULL, -- admin, vendedor, comprador, etc
    permissoes JSONB DEFAULT '{}',
    ativo BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Fornecedores
CREATE TABLE fornecedores (
    id SERIAL PRIMARY KEY,
    cnpj VARCHAR(14) UNIQUE,
    razao_social VARCHAR(255) NOT NULL,
    nome_fantasia VARCHAR(255),
    inscricao_estadual VARCHAR(20),
    email VARCHAR(255),
    telefone VARCHAR(20),
    ativo BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Clientes
CREATE TABLE clientes (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id),
    cpf_cnpj VARCHAR(14),
    nome VARCHAR(255) NOT NULL,
    email VARCHAR(255),
    telefone VARCHAR(20),
    ativo BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Enderecos (polymorphic)
CREATE TABLE enderecos (
    id SERIAL PRIMARY KEY,
    enderecavel_type VARCHAR(50) NOT NULL, -- 'cliente', 'fornecedor', 'loja'
    enderecavel_id INTEGER NOT NULL,
    tipo VARCHAR(50) DEFAULT 'principal', -- principal, entrega, cobranca
    cep VARCHAR(8),
    logradouro VARCHAR(255),
    numero VARCHAR(20),
    complemento VARCHAR(100),
    bairro VARCHAR(100),
    cidade_id INTEGER REFERENCES cidades(id),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),

    UNIQUE(enderecavel_type, enderecavel_id, tipo)
);

CREATE INDEX idx_enderecos_enderecavel ON enderecos(enderecavel_type, enderecavel_id);
```

### Transaction Tables

```sql
-- Vendas
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id) NOT NULL,
    cliente_id INTEGER REFERENCES clientes(id),
    vendedor_id INTEGER REFERENCES usuarios(id),
    status venda_status NOT NULL DEFAULT 'ORCAMENTO',

    -- Totals (denormalized for performance)
    subtotal DECIMAL(15,2) DEFAULT 0,
    desconto DECIMAL(15,2) DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) DEFAULT 0,

    -- Dates
    data_orcamento TIMESTAMP,
    data_venda TIMESTAMP,
    data_previsao_entrega DATE,
    data_entrega TIMESTAMP,

    observacoes TEXT,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_vendas_loja_status ON vendas(loja_id, status);
CREATE INDEX idx_vendas_cliente ON vendas(cliente_id);
CREATE INDEX idx_vendas_data ON vendas(data_venda);

-- Compras
CREATE TABLE compras (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id) NOT NULL,
    fornecedor_id INTEGER REFERENCES fornecedores(id) NOT NULL,
    venda_id INTEGER REFERENCES vendas(id), -- if linked to a sale
    status compra_status NOT NULL DEFAULT 'PENDENTE',

    -- Totals
    subtotal DECIMAL(15,2) DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) DEFAULT 0,

    -- Planned vs Actual dates
    data_prev_compra DATE,
    data_real_compra DATE,
    data_prev_entrega DATE,
    data_real_entrega DATE,

    nfe_id INTEGER REFERENCES nfes(id),
    observacoes TEXT,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_compras_loja_status ON compras(loja_id, status);
CREATE INDEX idx_compras_fornecedor ON compras(fornecedor_id);
```

### Inventory Tables

```sql
-- Estoque (stock receipts/entries)
CREATE TABLE estoques (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id) NOT NULL,
    compra_id INTEGER REFERENCES compras(id),
    produto_id INTEGER REFERENCES produtos(id) NOT NULL,
    fornecedor_id INTEGER REFERENCES fornecedores(id),

    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_disponivel DECIMAL(15,4) NOT NULL, -- current available
    custo_unitario DECIMAL(15,2),

    -- Location
    bloco_id INTEGER REFERENCES blocos(id), -- warehouse location

    -- Dates
    data_entrada TIMESTAMP DEFAULT NOW(),
    validade DATE,

    observacoes TEXT,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_estoques_produto ON estoques(produto_id);
CREATE INDEX idx_estoques_disponivel ON estoques(produto_id, quantidade_disponivel)
    WHERE quantidade_disponivel > 0;

-- Estoque consumos (stock consumption tracking)
CREATE TABLE estoque_consumos (
    id SERIAL PRIMARY KEY,
    estoque_id INTEGER REFERENCES estoques(id) NOT NULL,
    venda_item_id INTEGER REFERENCES venda_itens(id),
    quantidade DECIMAL(15,4) NOT NULL,
    data_consumo TIMESTAMP DEFAULT NOW(),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_estoque_consumos_estoque ON estoque_consumos(estoque_id);
```

---

## Migration Strategy

### Phase 1: New Tables
Create new normalized tables alongside existing ones.

### Phase 2: Dual-Write
Write to both old and new tables during transition.

### Phase 3: Backfill
Migrate historical data from old to new tables.

### Phase 4: Switch Reads
Point application reads to new tables.

### Phase 5: Cleanup
Remove old tables after validation.

---

## Full-Text Search

```sql
-- Add search vector to produtos
ALTER TABLE produtos ADD COLUMN search_vector tsvector;

CREATE INDEX idx_produtos_search ON produtos USING GIN(search_vector);

-- Update trigger
CREATE OR REPLACE FUNCTION produtos_search_update() RETURNS trigger AS $$
BEGIN
    NEW.search_vector :=
        setweight(to_tsvector('portuguese', COALESCE(NEW.descricao, '')), 'A') ||
        setweight(to_tsvector('portuguese', COALESCE(NEW.cod_comercial, '')), 'B');
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER produtos_search_trigger
    BEFORE INSERT OR UPDATE ON produtos
    FOR EACH ROW EXECUTE FUNCTION produtos_search_update();

-- Search query
SELECT * FROM produtos
WHERE search_vector @@ plainto_tsquery('portuguese', 'mesa escritorio')
ORDER BY ts_rank(search_vector, plainto_tsquery('portuguese', 'mesa escritorio')) DESC;
```
