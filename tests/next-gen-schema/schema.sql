-- Next-Gen Schema Test
-- Replaces L1/L2 (venda_has_produto + venda_has_produto2) with single venda_itens

-- Drop if exists for clean testing
DROP TABLE IF EXISTS estoque_consumos CASCADE;
DROP TABLE IF EXISTS estoques CASCADE;
DROP TABLE IF EXISTS venda_itens CASCADE;
DROP TABLE IF EXISTS vendas CASCADE;
DROP TABLE IF EXISTS produtos CASCADE;
DROP TABLE IF EXISTS fornecedores CASCADE;
DROP TABLE IF EXISTS clientes CASCADE;
DROP TABLE IF EXISTS lojas CASCADE;

DROP TYPE IF EXISTS venda_item_status CASCADE;
DROP TYPE IF EXISTS venda_status CASCADE;
DROP TYPE IF EXISTS estoque_status CASCADE;
DROP TYPE IF EXISTS consumo_motivo CASCADE;

-- ENUMs
CREATE TYPE venda_status AS ENUM (
    'ABERTA',
    'PARCIAL',
    'CONCLUIDA',
    'CANCELADA'
);

CREATE TYPE venda_item_status AS ENUM (
    'PENDENTE',
    'EM_COMPRA',
    'CONFIRMADO',
    'FATURADO',
    'EM_COLETA',
    'EM_RECEBIMENTO',
    'ESTOQUE',
    'ENTREGA_AGENDADA',
    'EM_ENTREGA',
    'ENTREGUE',
    'DEVOLVIDO',
    'CANCELADO'
);

CREATE TYPE estoque_status AS ENUM (
    'DISPONIVEL',
    'RESERVADO',
    'CONSUMIDO',
    'BLOQUEADO'
);

CREATE TYPE consumo_motivo AS ENUM (
    'VENDA',
    'AJUSTE',
    'QUEBRA',
    'TRANSFERENCIA',
    'AMOSTRA'
);

-- Master Tables
CREATE TABLE lojas (
    id SERIAL PRIMARY KEY,
    nome VARCHAR(200) NOT NULL,
    cnpj VARCHAR(18) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE fornecedores (
    id SERIAL PRIMARY KEY,
    razao_social VARCHAR(200) NOT NULL,
    cnpj VARCHAR(18) UNIQUE,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE clientes (
    id SERIAL PRIMARY KEY,
    nome VARCHAR(200) NOT NULL,
    cpf_cnpj VARCHAR(18),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),
    codigo VARCHAR(100) NOT NULL,
    descricao VARCHAR(500) NOT NULL,
    unidade VARCHAR(10) DEFAULT 'UN',
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(fornecedor_id, codigo)
);

-- Transaction Tables
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    cliente_id INTEGER NOT NULL REFERENCES clientes(id),
    status venda_status NOT NULL DEFAULT 'ABERTA',
    total DECIMAL(15,2) NOT NULL DEFAULT 0,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- THE KEY TABLE: Single venda_itens replaces L1/L2
CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,
    venda_id INTEGER NOT NULL REFERENCES vendas(id) ON DELETE CASCADE,

    -- Split hierarchy (replaces idRelacionado pattern)
    parent_id INTEGER REFERENCES venda_itens(id),
    root_id INTEGER REFERENCES venda_itens(id),
    split_reason VARCHAR(50),  -- PARTIAL_NFE, PARTIAL_DELIVERY, RETURN

    -- Product (FK, not denormalized!)
    produto_id INTEGER NOT NULL REFERENCES produtos(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Quantities
    quantidade DECIMAL(15,4) NOT NULL,
    unidade VARCHAR(10) DEFAULT 'UN',

    -- Prices (snapshot at sale time)
    valor_unitario DECIMAL(15,4) NOT NULL,
    valor_total DECIMAL(15,2) NOT NULL,

    -- Status
    status venda_item_status NOT NULL DEFAULT 'PENDENTE',

    -- Dates
    data_prev_entrega DATE,
    data_real_entrega TIMESTAMP,

    -- Audit
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),

    -- Constraints
    CONSTRAINT chk_quantidade_positiva CHECK (quantidade > 0),
    CONSTRAINT chk_valid_split CHECK (
        (parent_id IS NULL AND root_id IS NULL) OR
        (parent_id IS NOT NULL AND root_id IS NOT NULL)
    )
);

CREATE INDEX idx_venda_itens_venda ON venda_itens(venda_id);
CREATE INDEX idx_venda_itens_status ON venda_itens(status);
CREATE INDEX idx_venda_itens_parent ON venda_itens(parent_id) WHERE parent_id IS NOT NULL;
CREATE INDEX idx_venda_itens_root ON venda_itens(root_id) WHERE root_id IS NOT NULL;

-- Stock Tables
CREATE TABLE estoques (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    produto_id INTEGER NOT NULL REFERENCES produtos(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    quantidade_original DECIMAL(15,4) NOT NULL,
    quantidade_disponivel DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,

    lote VARCHAR(50),
    status estoque_status NOT NULL DEFAULT 'DISPONIVEL',
    data_entrada TIMESTAMP NOT NULL DEFAULT NOW(),

    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),

    CONSTRAINT chk_estoque_quantidade CHECK (quantidade_original > 0 AND quantidade_disponivel >= 0),
    CONSTRAINT chk_disponivel_nao_excede CHECK (quantidade_disponivel <= quantidade_original)
);

CREATE INDEX idx_estoques_disponivel ON estoques(produto_id, loja_id, data_entrada)
    WHERE quantidade_disponivel > 0;

-- Stock Consumption (1:1 link between venda_item and estoque)
CREATE TABLE estoque_consumos (
    id SERIAL PRIMARY KEY,
    venda_item_id INTEGER NOT NULL REFERENCES venda_itens(id),
    estoque_id INTEGER NOT NULL REFERENCES estoques(id),

    quantidade DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,

    motivo consumo_motivo NOT NULL DEFAULT 'VENDA',

    -- Reversal tracking
    is_estornado BOOLEAN DEFAULT FALSE,
    estornado_em TIMESTAMP,
    estorno_motivo VARCHAR(200),

    created_at TIMESTAMP DEFAULT NOW(),

    CONSTRAINT chk_consumo_quantidade CHECK (quantidade > 0)
);

-- 1:1 constraint: Only one active consumption per venda_item
CREATE UNIQUE INDEX idx_consumos_venda_item_ativo
    ON estoque_consumos(venda_item_id)
    WHERE NOT is_estornado;

-- 1:1 constraint: Each estoque can only be consumed once
CREATE UNIQUE INDEX idx_consumos_estoque_ativo
    ON estoque_consumos(estoque_id)
    WHERE NOT is_estornado;


-- ============================================================================
-- TRIGGERS
-- ============================================================================

-- Validate consumption before allowing pairing
CREATE OR REPLACE FUNCTION fn_validar_consumo()
RETURNS TRIGGER AS $$
DECLARE
    v_qtd_item DECIMAL(15,4);
    v_qtd_disponivel DECIMAL(15,4);
    v_status_item venda_item_status;
    v_produto_item INTEGER;
    v_produto_estoque INTEGER;
    v_fornecedor_item INTEGER;
    v_fornecedor_estoque INTEGER;
BEGIN
    -- Get venda_item data
    SELECT quantidade, status, produto_id, fornecedor_id
    INTO v_qtd_item, v_status_item, v_produto_item, v_fornecedor_item
    FROM venda_itens WHERE id = NEW.venda_item_id;

    -- Get estoque data
    SELECT quantidade_disponivel, produto_id, fornecedor_id
    INTO v_qtd_disponivel, v_produto_estoque, v_fornecedor_estoque
    FROM estoques WHERE id = NEW.estoque_id;

    -- RULE 1: Consumption quantity must equal item quantity
    IF NEW.quantidade != v_qtd_item THEN
        RAISE EXCEPTION 'Quantidade do consumo (%) deve ser igual a do item (%)',
            NEW.quantidade, v_qtd_item;
    END IF;

    -- RULE 2: Stock must have sufficient quantity
    IF v_qtd_disponivel < NEW.quantidade THEN
        RAISE EXCEPTION 'Estoque insuficiente: disponivel=%, solicitado=%',
            v_qtd_disponivel, NEW.quantidade;
    END IF;

    -- RULE 3: Product must match
    IF v_produto_item != v_produto_estoque THEN
        RAISE EXCEPTION 'Produto do item (%) diferente do estoque (%)',
            v_produto_item, v_produto_estoque;
    END IF;

    -- RULE 4: Supplier must match
    IF v_fornecedor_item != v_fornecedor_estoque THEN
        RAISE EXCEPTION 'Fornecedor do item (%) diferente do estoque (%)',
            v_fornecedor_item, v_fornecedor_estoque;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_validar_consumo
    BEFORE INSERT ON estoque_consumos
    FOR EACH ROW EXECUTE FUNCTION fn_validar_consumo();


-- Auto-update stock after consumption/reversal
CREATE OR REPLACE FUNCTION fn_atualizar_estoque_apos_consumo()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' AND NOT NEW.is_estornado THEN
        -- Consumption: decrease available quantity
        UPDATE estoques
        SET quantidade_disponivel = quantidade_disponivel - NEW.quantidade,
            status = CASE
                WHEN quantidade_disponivel - NEW.quantidade = 0 THEN 'CONSUMIDO'::estoque_status
                ELSE status
            END,
            updated_at = NOW()
        WHERE id = NEW.estoque_id;

        -- Update venda_item status to ESTOQUE
        UPDATE venda_itens
        SET status = 'ESTOQUE',
            updated_at = NOW()
        WHERE id = NEW.venda_item_id;

    ELSIF TG_OP = 'UPDATE' AND NEW.is_estornado AND NOT OLD.is_estornado THEN
        -- Reversal: restore quantity
        UPDATE estoques
        SET quantidade_disponivel = quantidade_disponivel + OLD.quantidade,
            status = 'DISPONIVEL'::estoque_status,
            updated_at = NOW()
        WHERE id = OLD.estoque_id;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_atualizar_estoque_apos_consumo
    AFTER INSERT OR UPDATE ON estoque_consumos
    FOR EACH ROW EXECUTE FUNCTION fn_atualizar_estoque_apos_consumo();


-- Split function
CREATE OR REPLACE FUNCTION split_venda_item(
    p_item_id INTEGER,
    p_quantidade_manter DECIMAL,
    p_split_reason VARCHAR(50)
) RETURNS INTEGER AS $$
DECLARE
    v_original RECORD;
    v_novo_id INTEGER;
    v_quantidade_split DECIMAL;
BEGIN
    SELECT * INTO v_original FROM venda_itens WHERE id = p_item_id FOR UPDATE;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Item nao encontrado: %', p_item_id;
    END IF;

    v_quantidade_split := v_original.quantidade - p_quantidade_manter;

    IF v_quantidade_split <= 0 THEN
        RAISE EXCEPTION 'Quantidade de split invalida';
    END IF;

    -- Update original with reduced quantity
    UPDATE venda_itens
    SET quantidade = p_quantidade_manter,
        valor_total = valor_unitario * p_quantidade_manter,
        updated_at = NOW()
    WHERE id = p_item_id;

    -- Create split item
    INSERT INTO venda_itens (
        venda_id, parent_id, root_id, split_reason,
        produto_id, fornecedor_id,
        quantidade, unidade,
        valor_unitario, valor_total,
        status, data_prev_entrega
    )
    SELECT
        venda_id,
        p_item_id,
        COALESCE(root_id, p_item_id),
        p_split_reason,
        produto_id, fornecedor_id,
        v_quantidade_split, unidade,
        valor_unitario, valor_unitario * v_quantidade_split,
        v_original.status, data_prev_entrega
    FROM venda_itens WHERE id = p_item_id
    RETURNING id INTO v_novo_id;

    RETURN v_novo_id;
END;
$$ LANGUAGE plpgsql;


-- ============================================================================
-- VIEWS
-- ============================================================================

-- Aggregated view (replaces L1 functionality)
CREATE VIEW venda_itens_agregado AS
SELECT
    venda_id,
    produto_id,
    fornecedor_id,

    -- Original order (from root items)
    SUM(quantidade) FILTER (WHERE parent_id IS NULL) as quantidade_pedida,
    SUM(valor_total) FILTER (WHERE parent_id IS NULL) as total_pedido,

    -- Current state (all active items)
    SUM(quantidade) FILTER (WHERE status NOT IN ('CANCELADO', 'DEVOLVIDO')) as quantidade_ativa,

    -- By status
    SUM(quantidade) FILTER (WHERE status = 'PENDENTE') as quantidade_pendente,
    SUM(quantidade) FILTER (WHERE status = 'ESTOQUE') as quantidade_estoque,
    SUM(quantidade) FILTER (WHERE status = 'ENTREGUE') as quantidade_entregue,

    -- Line counts (for splits)
    COUNT(*) as total_linhas,
    COUNT(*) FILTER (WHERE parent_id IS NOT NULL) as linhas_split

FROM venda_itens
GROUP BY venda_id, produto_id, fornecedor_id;
