# Schema Alternativo: 2 Entidades (Brainstorm)

> Status: **Brainstorm**
> Última atualização: 2025-12-28
> Propósito: Avaliar simplificação do modelo removendo tabela `estoques`

---

## Motivação

O modelo atual tem 3 entidades principais para o fluxo de mercadorias:
- `venda_itens` - o que o cliente quer
- `compra_itens` - o que compramos do fornecedor
- `estoques` - o inventário físico

A proposta é **unificar compra_itens + estoques** em uma única entidade, onde:
- `compra_item` com status=PENDENTE é um pedido de compra
- `compra_item` com status=RECEBIDO é inventário (estoque)

---

## Comparação de Modelos

### Modelo Atual (3 Entidades)

```
venda_itens ──────> compra_itens ──────> estoques
              "buy for"          "receives"

                    estoque_consumos
                    (fulfillment link)
```

### Modelo Proposto (2 Entidades)

```
venda_itens <────── alocacoes ──────> compra_itens
                (fulfillment)     (order + inventory)
```

---

## Conceito Principal

> "Um `compra_item` em status RECEBIDO **É** o inventário"

O `compra_item` evolui através de estados:
- `PENDENTE` → é um pedido de compra
- `CONFIRMADO` → fornecedor confirmou
- `FATURADO` → NFe recebida
- `RECEBIDO` → está no galpão (é "estoque")

---

## Cenários Detalhados

### Cenário 1: Fluxo Normal (compra 100, recebe 100)

```
ANTES DA NFe:
┌─────────────────┐         ┌─────────────────┐
│ venda_item      │         │ compra_item     │
│ id=1, qty=100   │────────>│ id=1, qty=100   │
│ status=PENDENTE │ origem  │ status=PENDENTE │
└─────────────────┘         └─────────────────┘

DEPOIS DA NFe (100un):
┌─────────────────┐         ┌─────────────────────────┐
│ venda_item      │         │ compra_item             │
│ id=1, qty=100   │         │ id=1, qty=100           │
│ status=ESTOQUE  │         │ status=RECEBIDO         │
└────────┬────────┘         │ nfe_item_id=X           │
         │                  │ custo=50.00             │
         │                  │ lote='ABC123'           │
         │    ┌─────────────┤ data_entrada=2025-01-15 │
         │    │             └─────────────────────────┘
         │    │ alocacao (1:1)
         └────┴─────────────────────────────────────────
```

### Cenário 2: Entrega Parcial (compra 100, recebe 60)

```
ANTES DA NFe:
  venda_item (id=1, qty=100)
  compra_item (id=1, qty=100, venda_origem_id=1, status=PENDENTE)

DEPOIS DA NFe com 60un:

  SPLIT de ambos:
  ┌─────────────────────────────────────────────────────────┐
  │ venda_item (id=1, qty=60)                               │
  │ venda_item (id=2, qty=40, parent_id=1)  ← SPLIT         │
  │                                                         │
  │ compra_item (id=1, qty=60, status=RECEBIDO)             │
  │ compra_item (id=2, qty=40, parent_id=1, status=PENDENTE)│
  └─────────────────────────────────────────────────────────┘

  alocacao: venda_item=1 ↔ compra_item=1 (60un recebidos)

  venda_item=2 aguarda compra_item=2 ser recebido
```

### Cenário 3: Atendimento com Estoque Existente

```
ESTOQUE EXISTENTE (reposição):
  compra_item (id=50, qty=150, venda_origem_id=NULL, status=RECEBIDO)
  ↑ comprado para reposição, não para cliente específico

CLIENTE PEDE 100un:
  venda_item (id=1, qty=100)

  SPLIT do compra_item existente:
    compra_item (id=50, qty=100)  -- para este cliente
    compra_item (id=51, qty=50, parent_id=50)  -- sobra

  alocacao: venda_item=1 ↔ compra_item=50
```

### Cenário 4: Cancelamento e Reatribuição

```
SITUAÇÃO INICIAL:
  venda_item_A (id=1, qty=100)
  compra_item (id=1, venda_origem_id=1, status=RECEBIDO)
  SEM alocacao ainda (mercadoria chegou mas não foi pareada)

CLIENTE A CANCELA:
  venda_item_A (id=1, status=CANCELADO)
  compra_item (id=1, venda_origem_id=1, status=RECEBIDO)  -- inalterado!

NOVO CLIENTE B:
  venda_item_B (id=2, qty=100)
  alocacao: venda_item=2 ↔ compra_item=1

TRILHA DE AUDITORIA:
  - compra_item.venda_origem_id = 1 (compramos para A)
  - alocacao.venda_item_id = 2 (foi para B)
```

### Cenário 5: Atendimento Misto (60 do estoque + 40 nova compra)

```
ESTOQUE EXISTENTE:
  compra_item (id=50, qty=60, status=RECEBIDO)

CLIENTE PEDE 100un:
  venda_item (id=1, qty=100)

  SPLIT venda_item:
    venda_item (id=1, qty=60)  -- do estoque
    venda_item (id=2, qty=40, parent_id=1)  -- precisa comprar

  Alocação imediata:
    alocacao: venda_item=1 ↔ compra_item=50

  Nova compra para o restante:
    compra_item (id=100, qty=40, venda_origem_id=2, status=PENDENTE)

  Quando NFe chegar:
    compra_item (id=100, status=RECEBIDO)
    alocacao: venda_item=2 ↔ compra_item=100
```

---

## Schema Proposto

### ENUMs

```sql
-- Status do compra_item (ordem → inventário)
CREATE TYPE compra_item_status AS ENUM (
    'PENDENTE',           -- Pedido criado
    'CONFIRMADO',         -- Fornecedor confirmou
    'FATURADO',           -- NFe recebida
    'EM_COLETA',          -- Aguardando coleta
    'EM_RECEBIMENTO',     -- Sendo conferido
    'RECEBIDO',           -- No galpão (é "estoque")
    'ALOCADO',            -- Totalmente alocado para vendas
    'CANCELADO'           -- Cancelado
);

-- Tipo de entrada no inventário
CREATE TYPE compra_item_tipo AS ENUM (
    'COMPRA',             -- Compra normal de fornecedor
    'DEVOLUCAO',          -- Devolução de cliente
    'AJUSTE_ENTRADA',     -- Ajuste manual (entrada)
    'TRANSFERENCIA',      -- Transferência entre lojas
    'INVENTARIO'          -- Inventário inicial / contagem
);
```

### Tabela compra_itens (unificada)

```sql
CREATE TABLE compra_itens (
    id SERIAL PRIMARY KEY,
    compra_id INTEGER REFERENCES compras(id),  -- NULL para não-compras
    loja_id INTEGER NOT NULL REFERENCES lojas(id),

    -- Hierarquia (splits)
    parent_id INTEGER REFERENCES compra_itens(id),
    root_id INTEGER REFERENCES compra_itens(id),
    split_reason VARCHAR(50),

    -- Produto
    produto_id INTEGER NOT NULL REFERENCES produtos(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Origem (por que compramos - trilha de auditoria)
    venda_item_origem_id INTEGER REFERENCES venda_itens(id),

    -- Quantidades
    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_alocada DECIMAL(15,4) DEFAULT 0,

    -- Info do pedido
    valor_unitario DECIMAL(15,4),
    valor_total DECIMAL(15,2),

    -- Info de inventário (preenchido quando RECEBIDO)
    nfe_entrada_id INTEGER REFERENCES nfes(id),
    nfe_item_id INTEGER REFERENCES nfe_itens(id),
    custo_unitario DECIMAL(15,4),
    lote VARCHAR(50),
    data_validade DATE,
    bloco_id INTEGER REFERENCES galpao_blocos(id),
    data_entrada TIMESTAMP,

    -- Status e tipo
    status compra_item_status NOT NULL DEFAULT 'PENDENTE',
    tipo compra_item_tipo NOT NULL DEFAULT 'COMPRA',

    -- Auditoria
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Índice para busca de inventário disponível
CREATE INDEX idx_compra_itens_disponivel
    ON compra_itens(produto_id, loja_id, data_entrada)
    WHERE status = 'RECEBIDO' AND quantidade > quantidade_alocada;

-- 1:1 entre venda origem e compra
CREATE UNIQUE INDEX idx_compra_itens_venda_origem_unique
    ON compra_itens(venda_item_origem_id)
    WHERE venda_item_origem_id IS NOT NULL;
```

### Tabela alocacoes (link de fulfillment)

```sql
-- Renomeado de estoque_consumos
CREATE TABLE alocacoes (
    id SERIAL PRIMARY KEY,

    -- Link 1:1
    venda_item_id INTEGER NOT NULL REFERENCES venda_itens(id),
    compra_item_id INTEGER NOT NULL REFERENCES compra_itens(id),

    -- Quantidade e custo (snapshot)
    quantidade DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,
    custo_total DECIMAL(15,2) GENERATED ALWAYS AS (quantidade * custo_unitario) STORED,

    -- Estorno
    is_estornado BOOLEAN DEFAULT FALSE,
    estornado_em TIMESTAMP,
    estorno_motivo VARCHAR(200),
    estornado_por INTEGER REFERENCES usuarios(id),

    -- Auditoria
    created_at TIMESTAMP DEFAULT NOW(),
    created_by INTEGER REFERENCES usuarios(id)
);

-- Constraint 1:1: um venda_item só pode ter uma alocação ativa
CREATE UNIQUE INDEX idx_alocacoes_venda_item_ativo
    ON alocacoes(venda_item_id)
    WHERE NOT is_estornado;

-- Constraint 1:1: um compra_item só pode ser alocado uma vez
CREATE UNIQUE INDEX idx_alocacoes_compra_item_ativo
    ON alocacoes(compra_item_id)
    WHERE NOT is_estornado;
```

---

## Triggers de Integridade

### Validar Alocação

```sql
CREATE OR REPLACE FUNCTION fn_validar_alocacao()
RETURNS TRIGGER AS $$
DECLARE
    v_qtd_venda DECIMAL(15,4);
    v_qtd_compra DECIMAL(15,4);
    v_qtd_alocada DECIMAL(15,4);
    v_status_compra compra_item_status;
    v_produto_venda INTEGER;
    v_produto_compra INTEGER;
    v_fornecedor_venda INTEGER;
    v_fornecedor_compra INTEGER;
BEGIN
    -- Buscar dados do venda_item
    SELECT quantidade, produto_id, fornecedor_id
    INTO v_qtd_venda, v_produto_venda, v_fornecedor_venda
    FROM venda_itens WHERE id = NEW.venda_item_id;

    -- Buscar dados do compra_item
    SELECT quantidade, quantidade_alocada, status, produto_id, fornecedor_id
    INTO v_qtd_compra, v_qtd_alocada, v_status_compra, v_produto_compra, v_fornecedor_compra
    FROM compra_itens WHERE id = NEW.compra_item_id;

    -- REGRA 1: Quantidades devem ser iguais
    IF NEW.quantidade != v_qtd_venda THEN
        RAISE EXCEPTION 'Quantidade da alocação (%) deve ser igual à do venda_item (%)',
            NEW.quantidade, v_qtd_venda;
    END IF;

    -- REGRA 2: compra_item deve estar RECEBIDO
    IF v_status_compra != 'RECEBIDO' THEN
        RAISE EXCEPTION 'compra_item deve estar RECEBIDO para alocação (status atual: %)',
            v_status_compra;
    END IF;

    -- REGRA 3: compra_item deve ter quantidade disponível
    IF (v_qtd_compra - v_qtd_alocada) < NEW.quantidade THEN
        RAISE EXCEPTION 'compra_item sem quantidade disponível: total=%, alocada=%, solicitada=%',
            v_qtd_compra, v_qtd_alocada, NEW.quantidade;
    END IF;

    -- REGRA 4: Mesmo produto
    IF v_produto_venda != v_produto_compra THEN
        RAISE EXCEPTION 'Produto do venda_item (%) diferente do compra_item (%)',
            v_produto_venda, v_produto_compra;
    END IF;

    -- REGRA 5: Mesmo fornecedor
    IF v_fornecedor_venda != v_fornecedor_compra THEN
        RAISE EXCEPTION 'Fornecedor do venda_item (%) diferente do compra_item (%)',
            v_fornecedor_venda, v_fornecedor_compra;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_validar_alocacao
    BEFORE INSERT ON alocacoes
    FOR EACH ROW EXECUTE FUNCTION fn_validar_alocacao();
```

### Atualizar Quantidades Automaticamente

```sql
CREATE OR REPLACE FUNCTION fn_atualizar_apos_alocacao()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' AND NOT NEW.is_estornado THEN
        -- Alocação: aumentar quantidade_alocada
        UPDATE compra_itens
        SET quantidade_alocada = quantidade_alocada + NEW.quantidade,
            status = CASE
                WHEN quantidade_alocada + NEW.quantidade >= quantidade THEN 'ALOCADO'::compra_item_status
                ELSE status
            END,
            updated_at = NOW()
        WHERE id = NEW.compra_item_id;

        -- Atualizar status do venda_item
        UPDATE venda_itens
        SET status = 'ESTOQUE',
            updated_at = NOW()
        WHERE id = NEW.venda_item_id;

    ELSIF TG_OP = 'UPDATE' AND NEW.is_estornado AND NOT OLD.is_estornado THEN
        -- Estorno: diminuir quantidade_alocada
        UPDATE compra_itens
        SET quantidade_alocada = quantidade_alocada - OLD.quantidade,
            status = 'RECEBIDO'::compra_item_status,
            updated_at = NOW()
        WHERE id = OLD.compra_item_id;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_atualizar_apos_alocacao
    AFTER INSERT OR UPDATE ON alocacoes
    FOR EACH ROW EXECUTE FUNCTION fn_atualizar_apos_alocacao();
```

---

## Queries Comuns

### Listar Inventário Disponível

```sql
-- Inventário disponível para alocação
SELECT
    ci.id,
    ci.produto_id,
    p.descricao as produto,
    ci.fornecedor_id,
    f.razao_social as fornecedor,
    ci.quantidade,
    ci.quantidade_alocada,
    (ci.quantidade - ci.quantidade_alocada) as disponivel,
    ci.lote,
    ci.custo_unitario,
    ci.data_entrada
FROM compra_itens ci
JOIN produtos p ON p.id = ci.produto_id
JOIN fornecedores f ON f.id = ci.fornecedor_id
WHERE ci.status = 'RECEBIDO'
  AND ci.quantidade > ci.quantidade_alocada
  AND ci.loja_id = :loja_id
ORDER BY ci.data_entrada;  -- FIFO suggestion
```

### Listar Itens de Venda com Alocação

```sql
SELECT
    vi.id as venda_item_id,
    vi.quantidade,
    vi.status,
    p.descricao as produto,
    ci.id as compra_item_id,
    ci.lote,
    ci.custo_unitario,
    a.created_at as data_alocacao
FROM venda_itens vi
JOIN produtos p ON p.id = vi.produto_id
LEFT JOIN alocacoes a ON a.venda_item_id = vi.id AND NOT a.is_estornado
LEFT JOIN compra_itens ci ON ci.id = a.compra_item_id
WHERE vi.venda_id = :venda_id
ORDER BY vi.id;
```

### Trilha de Auditoria Completa

```sql
-- Ver histórico: para quem compramos vs para quem foi
SELECT
    ci.id as compra_item_id,
    ci.quantidade,
    ci.lote,

    -- Para quem compramos originalmente
    vi_origem.id as venda_origem_id,
    v_origem.id as venda_origem,
    c_origem.nome_razao as cliente_origem,

    -- Para quem realmente foi
    a.id as alocacao_id,
    vi_dest.id as venda_destino_id,
    v_dest.id as venda_destino,
    c_dest.nome_razao as cliente_destino,

    CASE
        WHEN vi_origem.id = vi_dest.id THEN 'Normal'
        WHEN vi_origem.id IS NULL THEN 'Reposição'
        ELSE 'Reatribuído'
    END as tipo_alocacao

FROM compra_itens ci
LEFT JOIN venda_itens vi_origem ON vi_origem.id = ci.venda_item_origem_id
LEFT JOIN vendas v_origem ON v_origem.id = vi_origem.venda_id
LEFT JOIN clientes c_origem ON c_origem.id = v_origem.cliente_id
LEFT JOIN alocacoes a ON a.compra_item_id = ci.id AND NOT a.is_estornado
LEFT JOIN venda_itens vi_dest ON vi_dest.id = a.venda_item_id
LEFT JOIN vendas v_dest ON v_dest.id = vi_dest.venda_id
LEFT JOIN clientes c_dest ON c_dest.id = v_dest.cliente_id
WHERE ci.id = :compra_item_id;
```

---

## Comparação Detalhada: 3-Entidades vs 2-Entidades

### Visão Geral da Estrutura

#### Modelo 3-Entidades (Atual)

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ venda_itens │     │compra_itens │     │  estoques   │
├─────────────┤     ├─────────────┤     ├─────────────┤
│ id          │     │ id          │     │ id          │
│ venda_id    │     │ compra_id   │     │ compra_item │
│ produto_id  │     │ venda_item  │     │ nfe_item_id │
│ quantidade  │     │ produto_id  │     │ produto_id  │
│ valor_*     │     │ quantidade  │     │ quantidade  │
│ status      │     │ valor_*     │     │ custo_*     │
│ parent_id   │     │ status      │     │ lote        │
└──────┬──────┘     │ parent_id   │     │ bloco_id    │
       │            └──────┬──────┘     │ status      │
       │                   │            └──────┬──────┘
       │                   │                   │
       │            ┌──────┴───────────────────┘
       │            │
       ▼            ▼
┌─────────────────────────┐
│   estoque_consumos      │
├─────────────────────────┤
│ venda_item_id           │
│ estoque_id              │
│ quantidade              │
│ is_estornado            │
└─────────────────────────┘

Tabelas: 4
Relacionamentos: 5
```

#### Modelo 2-Entidades (Proposto)

```
┌─────────────┐                    ┌─────────────────────┐
│ venda_itens │                    │    compra_itens     │
├─────────────┤                    │  (order+inventory)  │
│ id          │                    ├─────────────────────┤
│ venda_id    │                    │ id                  │
│ produto_id  │                    │ compra_id           │
│ quantidade  │                    │ venda_item_origem   │
│ valor_*     │                    │ produto_id          │
│ status      │                    │ quantidade          │
│ parent_id   │                    │ quantidade_alocada  │
└──────┬──────┘                    │ valor_* / custo_*   │
       │                           │ nfe_item_id         │
       │                           │ lote, bloco_id      │
       │                           │ status, tipo        │
       │                           │ parent_id           │
       │                           └──────────┬──────────┘
       │                                      │
       │            ┌─────────────────────────┘
       │            │
       ▼            ▼
┌─────────────────────────┐
│      alocacoes          │
├─────────────────────────┤
│ venda_item_id           │
│ compra_item_id          │
│ quantidade              │
│ is_estornado            │
└─────────────────────────┘

Tabelas: 3
Relacionamentos: 3
```

---

### Comparação de Tabelas

| Aspecto | 3-Entidades | 2-Entidades |
|---------|-------------|-------------|
| **Total de tabelas** | 4 | 3 |
| **Tabelas principais** | venda_itens, compra_itens, estoques | venda_itens, compra_itens |
| **Tabela de link** | estoque_consumos | alocacoes |
| **Colunas em compra_itens** | ~15 | ~25 |
| **Colunas em estoques** | ~15 | N/A (merged) |

---

### Comparação de Relacionamentos

#### 3-Entidades

| De | Para | Cardinalidade | Propósito |
|----|------|---------------|-----------|
| compra_itens | venda_itens | N:1 | "compramos para esta venda" |
| estoques | compra_itens | N:1 | "veio desta compra" |
| estoques | nfe_itens | N:1 | "documento fiscal" |
| estoque_consumos | venda_itens | 1:1 | "alocado para esta venda" |
| estoque_consumos | estoques | 1:1 | "consumindo deste estoque" |

#### 2-Entidades

| De | Para | Cardinalidade | Propósito |
|----|------|---------------|-----------|
| compra_itens | venda_itens | N:1 | "compramos para esta venda" (origem) |
| compra_itens | nfe_itens | N:1 | "documento fiscal" |
| alocacoes | venda_itens | 1:1 | "alocado para esta venda" |
| alocacoes | compra_itens | 1:1 | "consumindo deste item" |

---

### Comparação de Queries

#### Query 1: Listar Inventário Disponível

**3-Entidades:**
```sql
SELECT
    e.id,
    e.produto_id,
    p.descricao,
    e.quantidade_disponivel,
    e.lote,
    e.custo_unitario,
    e.data_entrada
FROM estoques e
JOIN produtos p ON p.id = e.produto_id
WHERE e.status = 'DISPONIVEL'
  AND e.quantidade_disponivel > 0
  AND e.loja_id = :loja_id
ORDER BY e.data_entrada;
```
**JOINs: 1** | **Complexidade: Baixa**

**2-Entidades:**
```sql
SELECT
    ci.id,
    ci.produto_id,
    p.descricao,
    (ci.quantidade - ci.quantidade_alocada) as disponivel,
    ci.lote,
    ci.custo_unitario,
    ci.data_entrada
FROM compra_itens ci
JOIN produtos p ON p.id = ci.produto_id
WHERE ci.status = 'RECEBIDO'
  AND ci.quantidade > ci.quantidade_alocada
  AND ci.loja_id = :loja_id
ORDER BY ci.data_entrada;
```
**JOINs: 1** | **Complexidade: Baixa**

**Veredito: Empate** ✓

---

#### Query 2: Ver Itens de Venda com Estoque Alocado

**3-Entidades:**
```sql
SELECT
    vi.id,
    vi.quantidade,
    vi.status,
    e.lote,
    e.custo_unitario,
    ec.created_at as data_alocacao
FROM venda_itens vi
LEFT JOIN estoque_consumos ec ON ec.venda_item_id = vi.id AND NOT ec.is_estornado
LEFT JOIN estoques e ON e.id = ec.estoque_id
WHERE vi.venda_id = :venda_id;
```
**JOINs: 2** | **Complexidade: Média**

**2-Entidades:**
```sql
SELECT
    vi.id,
    vi.quantidade,
    vi.status,
    ci.lote,
    ci.custo_unitario,
    a.created_at as data_alocacao
FROM venda_itens vi
LEFT JOIN alocacoes a ON a.venda_item_id = vi.id AND NOT a.is_estornado
LEFT JOIN compra_itens ci ON ci.id = a.compra_item_id
WHERE vi.venda_id = :venda_id;
```
**JOINs: 2** | **Complexidade: Média**

**Veredito: Empate** ✓

---

#### Query 3: Rastrear Origem do Estoque (de qual compra veio)

**3-Entidades:**
```sql
SELECT
    e.id as estoque_id,
    e.lote,
    ci.id as compra_item_id,
    c.id as compra_id,
    c.data_emissao as data_compra,
    f.razao_social as fornecedor
FROM estoques e
JOIN compra_itens ci ON ci.id = e.compra_item_id
JOIN compras c ON c.id = ci.compra_id
JOIN fornecedores f ON f.id = c.fornecedor_id
WHERE e.id = :estoque_id;
```
**JOINs: 3** | **Complexidade: Média**

**2-Entidades:**
```sql
SELECT
    ci.id as compra_item_id,
    ci.lote,
    c.id as compra_id,
    c.data_emissao as data_compra,
    f.razao_social as fornecedor
FROM compra_itens ci
JOIN compras c ON c.id = ci.compra_id
JOIN fornecedores f ON f.id = c.fornecedor_id
WHERE ci.id = :compra_item_id;
```
**JOINs: 2** | **Complexidade: Baixa**

**Veredito: 2-Entidades melhor** ✓

---

#### Query 4: Relatório de Custo Médio por Produto

**3-Entidades:**
```sql
SELECT
    p.id,
    p.descricao,
    SUM(e.quantidade_disponivel) as qtd_total,
    SUM(e.quantidade_disponivel * e.custo_unitario) /
        NULLIF(SUM(e.quantidade_disponivel), 0) as custo_medio
FROM produtos p
JOIN estoques e ON e.produto_id = p.id
WHERE e.status = 'DISPONIVEL'
  AND e.quantidade_disponivel > 0
GROUP BY p.id, p.descricao;
```
**JOINs: 1** | **Complexidade: Média**

**2-Entidades:**
```sql
SELECT
    p.id,
    p.descricao,
    SUM(ci.quantidade - ci.quantidade_alocada) as qtd_total,
    SUM((ci.quantidade - ci.quantidade_alocada) * ci.custo_unitario) /
        NULLIF(SUM(ci.quantidade - ci.quantidade_alocada), 0) as custo_medio
FROM produtos p
JOIN compra_itens ci ON ci.produto_id = p.id
WHERE ci.status = 'RECEBIDO'
  AND ci.quantidade > ci.quantidade_alocada
GROUP BY p.id, p.descricao;
```
**JOINs: 1** | **Complexidade: Média** (cálculo quantidade-alocada mais verboso)

**Veredito: 3-Entidades levemente melhor** (quantidade_disponivel é mais direto)

---

#### Query 5: Trilha de Auditoria - Para Quem Compramos vs Para Quem Foi

**3-Entidades:**
```sql
SELECT
    e.id as estoque_id,
    -- Origem (para quem compramos)
    ci.venda_item_id as venda_origem_id,
    v_origem.id as venda_origem,
    c_origem.nome_razao as cliente_origem,
    -- Destino (para quem foi)
    ec.venda_item_id as venda_destino_id,
    v_dest.id as venda_destino,
    c_dest.nome_razao as cliente_destino
FROM estoques e
JOIN compra_itens ci ON ci.id = e.compra_item_id
LEFT JOIN venda_itens vi_origem ON vi_origem.id = ci.venda_item_id
LEFT JOIN vendas v_origem ON v_origem.id = vi_origem.venda_id
LEFT JOIN clientes c_origem ON c_origem.id = v_origem.cliente_id
LEFT JOIN estoque_consumos ec ON ec.estoque_id = e.id AND NOT ec.is_estornado
LEFT JOIN venda_itens vi_dest ON vi_dest.id = ec.venda_item_id
LEFT JOIN vendas v_dest ON v_dest.id = vi_dest.venda_id
LEFT JOIN clientes c_dest ON c_dest.id = v_dest.cliente_id
WHERE e.id = :estoque_id;
```
**JOINs: 8** | **Complexidade: Alta**

**2-Entidades:**
```sql
SELECT
    ci.id as compra_item_id,
    -- Origem (para quem compramos)
    ci.venda_item_origem_id,
    v_origem.id as venda_origem,
    c_origem.nome_razao as cliente_origem,
    -- Destino (para quem foi)
    a.venda_item_id as venda_destino_id,
    v_dest.id as venda_destino,
    c_dest.nome_razao as cliente_destino
FROM compra_itens ci
LEFT JOIN venda_itens vi_origem ON vi_origem.id = ci.venda_item_origem_id
LEFT JOIN vendas v_origem ON v_origem.id = vi_origem.venda_id
LEFT JOIN clientes c_origem ON c_origem.id = v_origem.cliente_id
LEFT JOIN alocacoes a ON a.compra_item_id = ci.id AND NOT a.is_estornado
LEFT JOIN venda_itens vi_dest ON vi_dest.id = a.venda_item_id
LEFT JOIN vendas v_dest ON v_dest.id = vi_dest.venda_id
LEFT JOIN clientes c_dest ON c_dest.id = v_dest.cliente_id
WHERE ci.id = :compra_item_id;
```
**JOINs: 7** | **Complexidade: Alta** (mas 1 JOIN a menos)

**Veredito: 2-Entidades levemente melhor**

---

### Comparação de Código Laravel

#### Models

**3-Entidades:**
```php
// 4 Models
class VendaItem extends Model { ... }
class CompraItem extends Model { ... }
class Estoque extends Model { ... }
class EstoqueConsumo extends Model { ... }
```

**2-Entidades:**
```php
// 3 Models
class VendaItem extends Model { ... }
class CompraItem extends Model { ... }  // mais métodos
class Alocacao extends Model { ... }
```

**Veredito: 2-Entidades tem menos models**, mas CompraItem é maior

---

#### Operação: Receber NFe e Criar Estoque

**3-Entidades:**
```php
// NfeService.php
public function importarNfe(Nfe $nfe): void
{
    foreach ($nfe->itens as $nfeItem) {
        // Encontrar compra_item correspondente
        $compraItem = $this->matchCompraItem($nfeItem);

        // Criar estoque separado
        $estoque = Estoque::create([
            'compra_item_id' => $compraItem->id,
            'nfe_item_id' => $nfeItem->id,
            'produto_id' => $nfeItem->produto_id,
            'quantidade_original' => $nfeItem->quantidade,
            'quantidade_disponivel' => $nfeItem->quantidade,
            'custo_unitario' => $nfeItem->valor_unitario,
            'lote' => $nfeItem->lote,
            'data_entrada' => now(),
            'status' => 'DISPONIVEL',
        ]);

        // Atualizar status do compra_item
        $compraItem->update(['status' => 'RECEBIDO']);
    }
}
```

**2-Entidades:**
```php
// NfeService.php
public function importarNfe(Nfe $nfe): void
{
    foreach ($nfe->itens as $nfeItem) {
        // Encontrar compra_item correspondente
        $compraItem = $this->matchCompraItem($nfeItem);

        // Atualizar compra_item com dados de inventário
        $compraItem->update([
            'nfe_item_id' => $nfeItem->id,
            'custo_unitario' => $nfeItem->valor_unitario,
            'lote' => $nfeItem->lote,
            'data_entrada' => now(),
            'status' => 'RECEBIDO',
        ]);
    }
}
```

**Veredito: 2-Entidades mais simples** (update vs create)

---

#### Operação: Alocar Estoque para Venda

**3-Entidades:**
```php
// AlocacaoService.php
public function alocar(VendaItem $vendaItem, Estoque $estoque): EstoqueConsumo
{
    // Validações...

    $consumo = EstoqueConsumo::create([
        'venda_item_id' => $vendaItem->id,
        'estoque_id' => $estoque->id,
        'quantidade' => $vendaItem->quantidade,
        'custo_unitario' => $estoque->custo_unitario,
    ]);

    // Trigger atualiza estoque.quantidade_disponivel
    // Trigger atualiza venda_item.status

    return $consumo;
}
```

**2-Entidades:**
```php
// AlocacaoService.php
public function alocar(VendaItem $vendaItem, CompraItem $compraItem): Alocacao
{
    // Validações...

    $alocacao = Alocacao::create([
        'venda_item_id' => $vendaItem->id,
        'compra_item_id' => $compraItem->id,
        'quantidade' => $vendaItem->quantidade,
        'custo_unitario' => $compraItem->custo_unitario,
    ]);

    // Trigger atualiza compra_item.quantidade_alocada
    // Trigger atualiza venda_item.status

    return $alocacao;
}
```

**Veredito: Empate** (código praticamente idêntico)

---

### Comparação de Splits

#### Cenário: NFe chega com quantidade parcial

**3-Entidades:**
```
1. Split venda_item (100 → 60 + 40)
2. Split compra_item (100 → 60 + 40)
3. Criar estoque para parte recebida (60)
4. Criar estoque_consumo ligando venda_item(60) ↔ estoque(60)

Operações: 4 INSERTs + 2 UPDATEs
Tabelas afetadas: 4
```

**2-Entidades:**
```
1. Split venda_item (100 → 60 + 40)
2. Split compra_item (100 → 60 + 40)
3. Atualizar compra_item(60) com dados de inventário
4. Criar alocacao ligando venda_item(60) ↔ compra_item(60)

Operações: 3 INSERTs + 2 UPDATEs
Tabelas afetadas: 3
```

**Veredito: 2-Entidades mais simples** (1 INSERT a menos)

---

### Comparação de Integridade de Dados

| Constraint | 3-Entidades | 2-Entidades |
|------------|-------------|-------------|
| FK venda → compra | ✓ compra_itens.venda_item_id | ✓ compra_itens.venda_item_origem_id |
| FK compra → estoque | ✓ estoques.compra_item_id | N/A (merged) |
| FK estoque → NFe | ✓ estoques.nfe_item_id | ✓ compra_itens.nfe_item_id |
| 1:1 venda ↔ consumo | ✓ UNIQUE INDEX | ✓ UNIQUE INDEX |
| 1:1 estoque ↔ consumo | ✓ UNIQUE INDEX | ✓ UNIQUE INDEX (compra_item) |
| Quantidade positiva | ✓ CHECK | ✓ CHECK |
| Status válido | ✓ ENUM | ✓ ENUM |
| Transições de status | ✓ TRIGGER | ✓ TRIGGER |

**Veredito: Empate** (mesma cobertura de integridade)

---

### Comparação de Performance

| Operação | 3-Entidades | 2-Entidades | Vantagem |
|----------|-------------|-------------|----------|
| **Listar inventário** | 1 tabela scan | 1 tabela scan + filtro | 3-Ent (índice mais simples) |
| **Buscar por lote** | estoques.lote | compra_itens.lote | Empate |
| **Relatório de custo** | SUM simples | SUM com subtração | 3-Ent (marginalmente) |
| **Alocar estoque** | INSERT + UPDATE | INSERT + UPDATE | Empate |
| **Receber NFe** | INSERT estoque | UPDATE compra | 2-Ent (menos I/O) |
| **Trilha de auditoria** | 8 JOINs | 7 JOINs | 2-Ent (marginalmente) |

**Veredito geral: Empate** com pequenas vantagens para cada lado

---

### Comparação de Manutenibilidade

| Aspecto | 3-Entidades | 2-Entidades |
|---------|-------------|-------------|
| **Entender o modelo** | Mais conceitos, mais claro cada um | Menos conceitos, cada um faz mais |
| **Adicionar campo de inventário** | Alterar estoques | Alterar compra_itens |
| **Adicionar campo de compra** | Alterar compra_itens | Alterar compra_itens |
| **Novo tipo de entrada** | Criar estoque sem compra | compra_item com tipo diferente |
| **Debug de alocação** | estoque_consumos + estoques | alocacoes + compra_itens |
| **Onboarding de dev** | Mais tabelas para aprender | Menos tabelas, mais estados |

---

### Comparação de Casos Especiais

#### Devolução de Cliente

**3-Entidades:**
```
1. Criar compra_item (tipo=DEVOLUCAO)? Ou não?
2. Criar estoque (origem=DEVOLUCAO)
3. estoque.compra_item_id = NULL ou criar compra fictícia?
```
**Problema:** estoque sem compra é caso especial

**2-Entidades:**
```
1. Criar compra_item (tipo=DEVOLUCAO)
2. Já é inventário quando status=RECEBIDO
```
**Problema:** "compra" que não é compra (naming)

---

#### Inventário Inicial (Migração)

**3-Entidades:**
```
1. Criar estoques diretamente
2. compra_item_id = NULL
3. nfe_item_id = NULL
```
**Problema:** estoque órfão (sem proveniência)

**2-Entidades:**
```
1. Criar compra_itens (tipo=INVENTARIO)
2. compra_id = NULL
3. status = RECEBIDO
```
**Problema:** "compra" sem compra (naming)

---

#### Transferência Entre Lojas

**3-Entidades:**
```
Loja A (saída):
1. Consumir estoque (motivo=TRANSFERENCIA)

Loja B (entrada):
2. Criar novo estoque (loja_id=B, transferencia_origem_id=...)
```

**2-Entidades:**
```
Loja A (saída):
1. Alocar compra_item (motivo=TRANSFERENCIA) - ou criar alocação especial?

Loja B (entrada):
2. Criar novo compra_item (tipo=TRANSFERENCIA, loja_id=B)
```

**Veredito: Ambos precisam de tratamento especial**

---

### Resumo da Comparação

| Critério | 3-Entidades | 2-Entidades | Vencedor |
|----------|-------------|-------------|----------|
| **Número de tabelas** | 4 | 3 | 2-Ent |
| **Número de models** | 4 | 3 | 2-Ent |
| **Clareza conceitual** | Alta | Média | 3-Ent |
| **Queries simples** | Empate | Empate | Empate |
| **Queries complexas** | 8 JOINs | 7 JOINs | 2-Ent |
| **Código de recebimento** | INSERT | UPDATE | 2-Ent |
| **Código de alocação** | Similar | Similar | Empate |
| **Splits** | 4 tabelas | 3 tabelas | 2-Ent |
| **Performance** | Empate | Empate | Empate |
| **Integridade** | Empate | Empate | Empate |
| **Casos especiais** | Nulls | Tipos | Empate |
| **Naming** | Natural | Forçado | 3-Ent |

**Conclusão preliminar:**
- **2-Entidades** ganha em simplicidade de estrutura
- **3-Entidades** ganha em clareza conceitual
- Em funcionalidade e integridade, são equivalentes

---

## Questões em Aberto

1. **Nomenclatura**: Renomear `compra_itens` para algo mais genérico?
   - `lotes`
   - `inventario`
   - `itens_estoque`
   - Manter `compra_itens`?

2. **Devoluções**: Como modelar devolução de cliente?
   - `compra_item` com `tipo='DEVOLUCAO'`?
   - Tabela separada?

3. **Transferências entre lojas**: Como tratar?
   - Baixa em loja A, entrada em loja B
   - Dois `compra_itens` ou um com referência?

4. **Migração**: Como migrar dados do modelo atual (3 entidades) para este?

---

## Próximos Passos

- [ ] Decidir nomenclatura final
- [ ] Detalhar fluxo de devoluções
- [ ] Detalhar fluxo de transferências
- [ ] Comparar queries de relatórios em ambos modelos
- [ ] Avaliar impacto na migração de dados
- [ ] Decisão final: ADR

---

## Documentos Relacionados

- [07-esquema-redesenhado.md](./07-esquema-redesenhado.md) - Schema atual (3 entidades)
- [02-decisoes.md](./02-decisoes.md) - ADRs do projeto
