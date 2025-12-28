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

## Comparação: Prós e Contras

### Prós do Modelo 2-Entidades

| Aspecto | Benefício |
|---------|-----------|
| **Simplicidade** | Menos tabelas, menos JOINs |
| **Modelo mental** | "compra_item é o que temos" |
| **Splits unificados** | Mesmo padrão parent_id para tudo |
| **Ciclo de vida natural** | PENDENTE → RECEBIDO é intuitivo |
| **Menos código** | Menos models, menos migrations |

### Contras do Modelo 2-Entidades

| Aspecto | Desvantagem |
|---------|-------------|
| **Tabela "gorda"** | compra_itens tem muitas colunas |
| **Conceito sobrecarregado** | Mesma entidade = pedido E inventário |
| **Casos especiais** | Devoluções não são realmente "compras" |
| **Naming** | `compra_item` nem sempre é uma compra |

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
