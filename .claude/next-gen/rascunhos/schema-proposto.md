# Schema Proposto: ERP Staccato v2

> **Status**: Proposta
> **Data**: 2025-12-28
> **Base**: Pesquisa de ERPs (SAP, Odoo, ERPNext) + decisões arquiteturais

---

## 1. Princípios de Design

### 1.1 Lições da Indústria

| ERP         | Modelo                                                    | Insight Chave                                             |
| ----------- | --------------------------------------------------------- | --------------------------------------------------------- |
| **SAP MM**  | PO → Material Document → Stock                            | Documento de movimento entre pedido e estoque             |
| **Odoo**    | purchase.order → stock.picking → stock.move → stock.quant | `quant` = estado atual O(log n), não calcula de histórico |
| **ERPNext** | Stock Ledger Entry                                        | Toda movimentação é um registro                           |

**Conclusão**: Nenhum ERP maduro funde "pedido" com "inventário". Sempre há camada de movimentação.

### 1.2 Decisões Adotadas

| Decisão             | Escolha                                 | Justificativa                         |
| ------------------- | --------------------------------------- | ------------------------------------- |
| Modelo de entidades | **3 entidades**                         | Validado pela indústria               |
| Dados fiscais NFe   | **JSONB**                               | Flexibilidade para reforma tributária |
| Status              | **ENUMs PostgreSQL**                    | Type-safety, transições validadas     |
| **Auditoria**       | **Event Sourcing + Materialized Views** | Histórico completo + zero overhead    |
| **View Manutenção** | **pg_ivm (Incremental)**                | Atualizações em tempo real, sem cron  |
| Consumo estoque     | **Seleção manual M:N**                  | Múltiplos lotes por item, qualidade   |

### 1.3 Arquitetura Event Sourcing + Materialized Views

**Problema Clássico:**
- Tabelas normalizadas UPDATE/DELETE reescrevem história
- Auditoria requer triggers complexos em cada tabela
- Concorrência causa race conditions
- Difícil recuperar "como era" em data passada

**Solução Adotada: Event Sourcing**

Para cada tabela operacional, há **dois artefatos**:

1. **`*_events` table** (Append-Only)
   - Imutável: apenas INSERT, nunca UPDATE/DELETE
   - Registra cada mudança de estado como evento
   - Não indexada para operações frequentes
   - Exemplo: `vendas_events`, `estoque_lotes_events`

2. **Materialized View** (Current State)
   - Agregação dos eventos mais recentes
   - Nome original (ex: `vendas`, `estoque_lotes`)
   - **Incrementally Maintained** via pg_ivm
   - Atualiza automaticamente quando evento é inserido
   - Indexada, otimizada para queries

**Fluxo:**

```
┌──────────────────────────────────────────────────┐
│        Aplicação insere evento                    │
│  INSERT INTO vendas_events (...)                 │
│      tipo='STATUS_ALTERADO', dados_novo={...}   │
└────────────────┬─────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────┐
│    pg_ivm detecta evento inserido                 │
└────────────────┬─────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────┐
│  Materialized View atualizada incrementalmente    │
│  SELECT * FROM vendas WHERE id=5                 │
│     ↓ Retorna estado atual (sem lag)             │
└──────────────────────────────────────────────────┘
```

**Benefícios:**

| Aspecto | Benefício |
|---------|-----------|
| **Auditoria** | Cada mudança é um evento imutável. Histórico completo preservado. |
| **Performance** | Views indexadas para queries rápidas. Append-only é otimizado para discos. |
| **Concorrência** | Sem UPDATE contention. INSERTs paralelos sem lock. |
| **Compliance** | Dados fiscais/movimentações jamais podem ser alterados. |
| **Recuperação** | Restaurar estado em data passada: rolar eventos até aquela data. |
| **Trigger Overhead** | Triggers apenas INSERT, não UPDATE. Mais rápido. |

**Estrutura de um Evento:**

```sql
CREATE TABLE vendas_events (
    event_id BIGSERIAL PRIMARY KEY,        -- Sequência global
    entidade_id INTEGER NOT NULL,          -- ID da venda (múltiplos eventos por ID)
    tipo VARCHAR(100) NOT NULL,            -- CRIADA, STATUS_ALTERADO, ITEM_ADICIONADO, etc
    dados_anterior JSONB,                  -- Estado antes da mudança
    dados_novo JSONB,                      -- Estado depois da mudança
    mudancas_totais JSONB,                 -- Apenas os campos que mudaram
    usuario_id INTEGER,                    -- Quem fez a mudança
    motivo TEXT,                           -- Por quê (obrigatório para algumas transições)
    changed_at TIMESTAMPTZ DEFAULT NOW(),  -- Quando (timestamp Brasileiro)

    CONSTRAINT chk_evento_valido CHECK (tipo IN (...all valid types...))
);

CREATE INDEX idx_vendas_events_entidade ON vendas_events(entidade_id, changed_at DESC);
CREATE INDEX idx_vendas_events_tipo ON vendas_events(tipo);
CREATE INDEX idx_vendas_events_data ON vendas_events(changed_at DESC);
```

**Materialized View (Agregação):**

```sql
-- View que reconstrói estado atual a partir dos eventos
CREATE MATERIALIZED VIEW vendas AS
SELECT DISTINCT ON (v.entidade_id)
    v.entidade_id as id,
    (v.dados_novo ->> 'numero')::VARCHAR as numero,
    (v.dados_novo ->> 'cliente_id')::INTEGER as cliente_id,
    (v.dados_novo ->> 'loja_id')::INTEGER as loja_id,
    (v.dados_novo ->> 'data_venda')::DATE as data_venda,
    (v.dados_novo ->> 'status')::venda_status as status,
    v.changed_at,
    v.usuario_id
FROM vendas_events v
ORDER BY v.entidade_id, v.changed_at DESC;

-- pg_ivm mantém esta view atualizada incrementalmente
-- Sem cron jobs, sem REFRESH MATERIALIZED VIEW completa
```

---

## 2. Visão Geral do Schema

````text
┌─────────────────────────────────────────────────────────────────────────┐
│                           DADOS MESTRES                                  │
├─────────────────────────────────────────────────────────────────────────┤
│  lojas    fornecedores    clientes    usuarios    transportadoras       │
│                 │                                                        │
│                 ▼                                                        │
│  produtos ─── produto_precos ─── produto_tributos                       │
│     │                                                                    │
│     │ ncm_id ──► ncms                                                   │
│     │ categoria_id ──► categorias                                       │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         FLUXO COMERCIAL                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  orcamentos ──► orcamento_itens                                         │
│       │              │                                                   │
│       ▼              ▼                                                   │
│  vendas ────────► venda_itens ◄───────────────────────┐                 │
│       │              │                                 │                 │
│       │              │ origem='COMPRA'                 │ origem='ESTOQUE'│
│       │              ▼                                 │                 │
│       │     compras ──► compra_itens                   │                 │
│       │                      │                         │                 │
└───────┼──────────────────────┼─────────────────────────┼─────────────────┘
        │                      │                         │
        │                      ▼                         │
┌───────┼─────────────────────────────────────────────────────────────────┐
│       │                      NFe                                         │
├───────┼─────────────────────────────────────────────────────────────────┤
│       │     nfes (header + XML raw)                                     │
│       │       │                                                          │
│       │       ▼                                                          │
│       │     nfe_itens (JSONB dados)                                     │
│       │       │                                                          │
└───────┼───────┼─────────────────────────────────────────────────────────┘
        │       │
        │       ▼
┌───────┼─────────────────────────────────────────────────────────────────┐
│       │                    INVENTÁRIO                                    │
├───────┼─────────────────────────────────────────────────────────────────┤
│       │     estoque_lotes (estado atual por lote)                       │
│       │       │                                                          │
│       │       │                         ┌──────────────────────┐        │
│       │       ▼                         ▼                      │        │
│       │     estoque_movimentacoes ◄── alocacoes ◄──────────────┘        │
│       │       (log de tudo)           (link venda_item ↔ lote)          │
│       │                                                                  │
└───────┼─────────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         LOGÍSTICA                                        │
├─────────────────────────────────────────────────────────────────────────┤
│  entregas ──► entrega_itens                                             │
│       │                                                                  │
│       ▼                                                                  │
│  nfes (tipo='SAIDA')                                                    │
└─────────────────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         FINANCEIRO                                       │
├─────────────────────────────────────────────────────────────────────────┤
│  contas_receber ──► parcelas_receber                                    │
│  contas_pagar ────► parcelas_pagar                                      │
│       │                                                                  │
│       ▼                                                                  │
│  remessas_cnab ──► retornos_cnab                                        │
└─────────────────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         AUDITORIA                                        │
├─────────────────────────────────────────────────────────────────────────┤
│  audit_log (trigger automático em todas as tabelas)                     │
└─────────────────────────────────────────────────────────────────────────┘
```text

---

## 3. Schema Completo

### 3.1 ENUMs

```sql
-- ============================================================================
-- ENUMS DE STATUS
-- ============================================================================

-- Pessoa
CREATE TYPE pessoa_tipo AS ENUM ('PF', 'PJ');

-- Orçamento
CREATE TYPE orcamento_status AS ENUM (
    'RASCUNHO',       -- Em edição
    'ENVIADO',        -- Enviado ao cliente
    'APROVADO',       -- Cliente aprovou
    'CONVERTIDO',     -- Virou venda
    'EXPIRADO',       -- Passou da validade
    'CANCELADO'       -- Cancelado
);

-- Venda (header)
CREATE TYPE venda_status AS ENUM (
    'ABERTA',         -- Em andamento
    'PARCIAL',        -- Alguns itens entregues
    'CONCLUIDA',      -- Todos entregues
    'CANCELADA'       -- Cancelada
);

-- Item de venda
CREATE TYPE venda_item_status AS ENUM (
    'PENDENTE',           -- Aguardando (compra ou estoque)
    'EM_COMPRA',          -- Pedido de compra criado
    'CONFIRMADO',         -- Fornecedor confirmou
    'FATURADO',           -- NFe de entrada recebida
    'EM_TRANSITO',        -- Mercadoria em trânsito
    'EM_RECEBIMENTO',     -- Sendo conferida
    'ESTOQUE',            -- Em estoque (alocado)
    'ENTREGA_AGENDADA',   -- Agendado para entrega
    'EM_ENTREGA',         -- Saiu para entrega
    'ENTREGUE',           -- Entregue ao cliente
    'DEVOLVIDO',          -- Devolvido
    'CANCELADO'           -- Cancelado
);

-- Origem do item
CREATE TYPE venda_item_origem AS ENUM (
    'COMPRA',         -- Precisa comprar
    'ESTOQUE'         -- Tem em estoque
);

-- Compra (header)
CREATE TYPE compra_status AS ENUM (
    'RASCUNHO',       -- Em edição
    'ENVIADA',        -- Enviada ao fornecedor
    'CONFIRMADA',     -- Fornecedor confirmou
    'PARCIAL',        -- Parcialmente recebida
    'RECEBIDA',       -- Totalmente recebida
    'CANCELADA'       -- Cancelada
);

-- Item de compra
CREATE TYPE compra_item_status AS ENUM (
    'PENDENTE',       -- Aguardando
    'CONFIRMADO',     -- Fornecedor confirmou
    'FATURADO',       -- NFe recebida
    'EM_TRANSITO',    -- Em trânsito
    'RECEBIDO',       -- Recebido
    'CANCELADO'       -- Cancelado
);

-- NFe
CREATE TYPE nfe_tipo AS ENUM (
    'ENTRADA',            -- Do fornecedor
    'SAIDA',              -- Para cliente
    'DEVOLUCAO_ENTRADA',  -- Devolução do cliente
    'DEVOLUCAO_SAIDA'     -- Devolução para fornecedor
);

CREATE TYPE nfe_status AS ENUM (
    'RASCUNHO',       -- Em edição
    'PENDENTE',       -- Aguardando envio
    'PROCESSANDO',    -- Enviada ao SEFAZ
    'AUTORIZADA',     -- Autorizada
    'REJEITADA',      -- Rejeitada
    'CANCELADA',      -- Cancelada
    'DENEGADA',       -- Denegada
    'INUTILIZADA'     -- Faixa inutilizada
);

-- Estoque
CREATE TYPE estoque_lote_status AS ENUM (
    'DISPONIVEL',     -- Pode ser alocado
    'RESERVADO',      -- Reservado (parcialmente alocado)
    'ESGOTADO',       -- Totalmente consumido
    'BLOQUEADO'       -- Bloqueado (avaria, etc)
);

-- Tipo de movimentação
CREATE TYPE movimentacao_tipo AS ENUM (
    'ENTRADA_COMPRA',     -- Entrada via NFe de compra
    'ENTRADA_DEVOLUCAO',  -- Entrada via devolução de cliente
    'ENTRADA_AJUSTE',     -- Ajuste manual (inventário)
    'ENTRADA_TRANSFERENCIA', -- Transferência de outra loja
    'SAIDA_VENDA',        -- Saída para venda
    'SAIDA_AJUSTE',       -- Ajuste manual (quebra, etc)
    'SAIDA_TRANSFERENCIA' -- Transferência para outra loja
);

-- Entrega
CREATE TYPE entrega_status AS ENUM (
    'AGENDADA',       -- Agendada
    'EM_CARREGAMENTO',-- Sendo carregada
    'EM_TRANSITO',    -- A caminho
    'ENTREGUE',       -- Entregue
    'PARCIAL',        -- Entrega parcial
    'NAO_ENTREGUE',   -- Tentativa falhou
    'CANCELADA'       -- Cancelada
);

-- Financeiro - Tipo de Obrigação
CREATE TYPE financeiro_tipo AS ENUM (
    'RECEBER',        -- Contas a receber (cliente)
    'PAGAR'           -- Contas a pagar (fornecedor)
);

-- Financeiro - Status
CREATE TYPE financeiro_status AS ENUM (
    'PENDENTE',       -- Aguardando
    'AGENDADO',       -- CNAB gerado
    'PAGO',           -- Pago (PAGAR)
    'RECEBIDO',       -- Recebido (RECEBER)
    'ATRASADO',       -- Vencido
    'CANCELADO'       -- Cancelado
);

-- Forma de pagamento
CREATE TYPE forma_pagamento AS ENUM (
    'DINHEIRO',
    'PIX',
    'CARTAO_DEBITO',
    'CARTAO_CREDITO',
    'BOLETO',
    'TRANSFERENCIA',
    'CHEQUE',
    'OUTROS'
);
```text

### 3.2 Dados Mestres

```sql
-- ============================================================================
-- ENDEREÇOS (compartilhado)
-- ============================================================================
CREATE TABLE enderecos (
    id SERIAL PRIMARY KEY,

    cep VARCHAR(9) NOT NULL,
    logradouro VARCHAR(200) NOT NULL,
    numero VARCHAR(20),
    complemento VARCHAR(100),
    bairro VARCHAR(100) NOT NULL,
    cidade VARCHAR(100) NOT NULL,
    uf CHAR(2) NOT NULL,

    -- IBGE para NFe
    codigo_ibge VARCHAR(7),
    codigo_pais VARCHAR(4) DEFAULT '1058', -- Brasil

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================================================
-- LOJAS
-- ============================================================================
CREATE TABLE lojas (
    id SERIAL PRIMARY KEY,

    codigo VARCHAR(10) UNIQUE NOT NULL,
    razao_social VARCHAR(200) NOT NULL,
    nome_fantasia VARCHAR(200),
    cnpj VARCHAR(18) UNIQUE NOT NULL,
    inscricao_estadual VARCHAR(20),
    inscricao_municipal VARCHAR(20),

    -- Contato
    email VARCHAR(200),
    telefone VARCHAR(20),

    -- Endereço
    endereco_id INTEGER REFERENCES enderecos(id),

    -- Configurações
    config JSONB DEFAULT '{}',
    -- Ex: {"serie_nfe": 1, "ambiente_nfe": "producao", "certificado_validade": "2025-12-31"}

    is_ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================================================
-- USUÁRIOS
-- ============================================================================
CREATE TABLE usuarios (
    id SERIAL PRIMARY KEY,

    nome VARCHAR(200) NOT NULL,
    email VARCHAR(200) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,

    loja_id INTEGER REFERENCES lojas(id),

    -- Perfil
    perfil VARCHAR(50) DEFAULT 'operador',  -- admin, gerente, vendedor, operador
    permissoes JSONB DEFAULT '{}',

    -- Vendedor
    is_vendedor BOOLEAN DEFAULT FALSE,
    comissao_percentual DECIMAL(5,2) DEFAULT 0,

    -- Controle
    ultimo_acesso TIMESTAMPTZ,
    tentativas_login INTEGER DEFAULT 0,
    bloqueado_ate TIMESTAMPTZ,

    is_ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================================================
-- FORNECEDORES
-- ============================================================================
CREATE TABLE fornecedores (
    id SERIAL PRIMARY KEY,

    -- Identificação
    razao_social VARCHAR(200) NOT NULL,
    nome_fantasia VARCHAR(200),
    cnpj VARCHAR(18) UNIQUE,
    inscricao_estadual VARCHAR(20),

    -- Contato
    email VARCHAR(200),
    telefone VARCHAR(20),
    contato_nome VARCHAR(100),

    -- Endereço
    endereco_id INTEGER REFERENCES enderecos(id),

    -- Dados bancários (para pagamento)
    banco_codigo VARCHAR(5),
    banco_nome VARCHAR(100),
    agencia VARCHAR(10),
    conta VARCHAR(20),
    conta_digito VARCHAR(2),
    pix_chave VARCHAR(100),

    -- Regras de negócio
    prazo_entrega_dias INTEGER DEFAULT 30,
    prazo_pagamento_dias INTEGER DEFAULT 30,
    is_frete_pago_loja BOOLEAN DEFAULT FALSE,
    is_representacao BOOLEAN DEFAULT FALSE,
    comissao_rt_percentual DECIMAL(5,2) DEFAULT 0,

    -- Observações
    observacoes TEXT,

    is_ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================================================
-- CLIENTES
-- ============================================================================
CREATE TABLE clientes (
    id SERIAL PRIMARY KEY,

    -- Identificação
    tipo pessoa_tipo NOT NULL DEFAULT 'PF',
    nome_razao VARCHAR(200) NOT NULL,
    nome_fantasia VARCHAR(200),
    cpf_cnpj VARCHAR(18) NOT NULL,
    rg_ie VARCHAR(20),

    -- Contato
    email VARCHAR(200),
    telefone VARCHAR(20),
    celular VARCHAR(20),

    -- Endereços
    endereco_principal_id INTEGER REFERENCES enderecos(id),
    endereco_entrega_id INTEGER REFERENCES enderecos(id),
    endereco_cobranca_id INTEGER REFERENCES enderecos(id),

    -- Crédito
    limite_credito DECIMAL(15,2) DEFAULT 0,
    saldo_devedor DECIMAL(15,2) DEFAULT 0,

    -- Vendedor responsável
    vendedor_id INTEGER REFERENCES usuarios(id),

    -- Flags
    is_incompleto BOOLEAN DEFAULT FALSE,  -- Cadastro rápido na venda
    is_contribuinte BOOLEAN DEFAULT FALSE,
    is_isento_ie BOOLEAN DEFAULT FALSE,

    -- Observações
    observacoes TEXT,

    is_ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT uk_cliente_cpf_cnpj UNIQUE (cpf_cnpj)
);

-- ============================================================================
-- TRANSPORTADORAS
-- ============================================================================
CREATE TABLE transportadoras (
    id SERIAL PRIMARY KEY,

    razao_social VARCHAR(200) NOT NULL,
    nome_fantasia VARCHAR(200),
    cnpj VARCHAR(18) UNIQUE,
    inscricao_estadual VARCHAR(20),

    -- Contato
    email VARCHAR(200),
    telefone VARCHAR(20),

    -- Endereço
    endereco_id INTEGER REFERENCES enderecos(id),

    -- Veículo padrão (para NFe)
    veiculo_placa VARCHAR(10),
    veiculo_uf CHAR(2),
    veiculo_rntc VARCHAR(20),

    is_ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- ============================================================================
-- NCM (Nomenclatura Comum do Mercosul)
-- ============================================================================
CREATE TABLE ncms (
    id SERIAL PRIMARY KEY,

    codigo VARCHAR(10) UNIQUE NOT NULL,
    descricao VARCHAR(500) NOT NULL,

    -- Alíquotas padrão (podem ser sobrescritas no produto)
    aliquota_ipi DECIMAL(5,2) DEFAULT 0,
    aliquota_ii DECIMAL(5,2) DEFAULT 0,  -- Imposto de importação

    is_ativo BOOLEAN DEFAULT TRUE
);

-- ============================================================================
-- CATEGORIAS
-- ============================================================================
CREATE TABLE categorias (
    id SERIAL PRIMARY KEY,

    nome VARCHAR(100) NOT NULL,
    categoria_pai_id INTEGER REFERENCES categorias(id),

    is_ativo BOOLEAN DEFAULT TRUE
);

-- ============================================================================
-- PRODUTOS
-- ============================================================================
CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,

    -- Vínculo com fornecedor (obrigatório)
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Identificação
    codigo_comercial VARCHAR(100) NOT NULL,  -- Código do fornecedor
    codigo_interno VARCHAR(50),               -- Código interno (opcional)
    codigo_barras VARCHAR(50),

    -- Descrição
    descricao VARCHAR(500) NOT NULL,
    descricao_curta VARCHAR(100),
    descricao_nfe VARCHAR(120),  -- Descrição para NFe (max 120 chars)

    -- Unidades
    unidade VARCHAR(10) DEFAULT 'UN',
    unidade_tributavel VARCHAR(10),  -- Se diferente da comercial
    fator_conversao DECIMAL(10,6) DEFAULT 1,

    -- Físico
    peso_bruto_kg DECIMAL(10,4),
    peso_liquido_kg DECIMAL(10,4),
    largura_cm DECIMAL(10,2),
    altura_cm DECIMAL(10,2),
    profundidade_cm DECIMAL(10,2),

    -- Caixaria
    unidades_por_caixa DECIMAL(10,4) DEFAULT 1,

    -- Classificação
    ncm_id INTEGER REFERENCES ncms(id),
    cest VARCHAR(10),  -- Código Especificador da Substituição Tributária
    categoria_id INTEGER REFERENCES categorias(id),

    -- Estoque mínimo
    estoque_minimo DECIMAL(15,4) DEFAULT 0,

    -- Flags
    tem_lote BOOLEAN DEFAULT FALSE,
    tem_validade BOOLEAN DEFAULT FALSE,
    controla_estoque BOOLEAN DEFAULT TRUE,

    is_ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT uk_produto_fornecedor_codigo UNIQUE (fornecedor_id, codigo_comercial)
);

CREATE INDEX idx_produtos_fornecedor ON produtos(fornecedor_id);
CREATE INDEX idx_produtos_codigo_barras ON produtos(codigo_barras) WHERE codigo_barras IS NOT NULL;

-- ============================================================================
-- PREÇOS DE PRODUTO (versionado - SCD-2)
-- ============================================================================
CREATE TABLE produto_precos (
    id SERIAL PRIMARY KEY,

    produto_id INTEGER NOT NULL REFERENCES produtos(id) ON DELETE CASCADE,

    -- Valores
    custo DECIMAL(15,4) NOT NULL,           -- Custo de aquisição
    valor_venda DECIMAL(15,4) NOT NULL,     -- Preço de venda

    -- Calculado
    margem DECIMAL(7,4) GENERATED ALWAYS AS (
        CASE WHEN custo > 0 THEN ((valor_venda / custo) - 1) * 100 ELSE 0 END
    ) STORED,

    -- Vigência
    vigente_de DATE NOT NULL DEFAULT CURRENT_DATE,
    vigente_ate DATE,  -- NULL = vigente

    -- Auditoria
    created_at TIMESTAMPTZ DEFAULT NOW(),
    created_by INTEGER REFERENCES usuarios(id)
);

-- View para preço atual
CREATE VIEW produto_preco_atual AS
SELECT DISTINCT ON (produto_id) *
FROM produto_precos
WHERE vigente_de <= CURRENT_DATE
  AND (vigente_ate IS NULL OR vigente_ate >= CURRENT_DATE)
ORDER BY produto_id, vigente_de DESC;

-- ============================================================================
-- TRIBUTOS DE PRODUTO
-- ============================================================================
CREATE TABLE produto_tributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id) ON DELETE CASCADE,

    -- Origem
    origem CHAR(1) DEFAULT '0',  -- 0=Nacional, 1=Estrangeira, etc

    -- ICMS
    cst_icms VARCHAR(3),
    aliquota_icms DECIMAL(5,2),
    reducao_bc_icms DECIMAL(5,2),

    -- ICMS-ST
    tem_st BOOLEAN DEFAULT FALSE,
    mva DECIMAL(7,4),
    aliquota_icms_st DECIMAL(5,2),

    -- IPI
    cst_ipi VARCHAR(2),
    aliquota_ipi DECIMAL(5,2),

    -- PIS/COFINS
    cst_pis VARCHAR(2),
    aliquota_pis DECIMAL(5,2),
    cst_cofins VARCHAR(2),
    aliquota_cofins DECIMAL(5,2),

    -- Configuração IBS/CBS (Reforma Tributária 2026-2033)
    config_ibs_cbs JSONB,

    updated_at TIMESTAMPTZ DEFAULT NOW()
);
```text

### 3.3 Fluxo Comercial

```sql
-- ============================================================================
-- ORÇAMENTOS
-- ============================================================================
CREATE TABLE orcamentos (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    cliente_id INTEGER REFERENCES clientes(id),  -- Pode ser nulo no início
    vendedor_id INTEGER NOT NULL REFERENCES usuarios(id),

    -- Número
    numero SERIAL,

    -- Validade
    data_emissao DATE NOT NULL DEFAULT CURRENT_DATE,
    data_validade DATE,

    -- Totais
    subtotal DECIMAL(15,2) NOT NULL DEFAULT 0,
    desconto_percentual DECIMAL(5,2) DEFAULT 0,
    desconto_valor DECIMAL(15,2) DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) NOT NULL DEFAULT 0,

    -- Observações
    observacoes TEXT,
    observacoes_internas TEXT,

    -- Status
    status orcamento_status NOT NULL DEFAULT 'RASCUNHO',

    -- Conversão
    venda_id INTEGER,  -- Preenchido quando convertido

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE orcamento_itens (
    id SERIAL PRIMARY KEY,

    orcamento_id INTEGER NOT NULL REFERENCES orcamentos(id) ON DELETE CASCADE,

    -- Ordenação
    posicao SMALLINT NOT NULL DEFAULT 1,

    -- Produto
    produto_id INTEGER NOT NULL REFERENCES produtos(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Snapshot para exibição
    descricao_produto VARCHAR(500) NOT NULL,
    codigo_produto VARCHAR(100) NOT NULL,

    -- Quantidades
    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_caixas DECIMAL(15,4),
    unidade VARCHAR(10) DEFAULT 'UN',

    -- Valores
    valor_unitario DECIMAL(15,4) NOT NULL,
    desconto_item_percentual DECIMAL(5,2) DEFAULT 0,
    valor_total DECIMAL(15,2) NOT NULL,

    -- Observações
    observacoes TEXT,

    created_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_orcamento_item_quantidade CHECK (quantidade > 0),
    CONSTRAINT chk_orcamento_item_valor CHECK (valor_unitario >= 0)
);

CREATE INDEX idx_orcamento_itens_orcamento ON orcamento_itens(orcamento_id);

-- ============================================================================
-- VENDAS
-- ============================================================================
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    cliente_id INTEGER NOT NULL REFERENCES clientes(id),
    vendedor_id INTEGER NOT NULL REFERENCES usuarios(id),

    -- Número
    numero SERIAL,

    -- Datas
    data_emissao DATE NOT NULL DEFAULT CURRENT_DATE,

    -- Totais
    subtotal DECIMAL(15,2) NOT NULL DEFAULT 0,
    desconto_percentual DECIMAL(5,2) DEFAULT 0,
    desconto_valor DECIMAL(15,2) DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) NOT NULL DEFAULT 0,

    -- Entrega
    endereco_entrega_id INTEGER REFERENCES enderecos(id),
    transportadora_id INTEGER REFERENCES transportadoras(id),

    -- Observações
    observacoes TEXT,
    observacoes_internas TEXT,

    -- Status
    status venda_status NOT NULL DEFAULT 'ABERTA',

    -- Origem
    orcamento_id INTEGER REFERENCES orcamentos(id),

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,

    venda_id INTEGER NOT NULL REFERENCES vendas(id) ON DELETE CASCADE,

    -- Ordenação
    posicao SMALLINT NOT NULL DEFAULT 1,

    -- Produto
    produto_id INTEGER NOT NULL REFERENCES produtos(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Snapshot para exibição
    descricao_produto VARCHAR(500) NOT NULL,
    codigo_produto VARCHAR(100) NOT NULL,

    -- Quantidades
    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_caixas DECIMAL(15,4),
    unidade VARCHAR(10) DEFAULT 'UN',

    -- Valores (snapshot no momento da venda)
    valor_unitario DECIMAL(15,4) NOT NULL,
    desconto_item_percentual DECIMAL(5,2) DEFAULT 0,
    valor_total DECIMAL(15,2) NOT NULL,

    -- Origem
    origem venda_item_origem NOT NULL DEFAULT 'COMPRA',

    -- Status (overall item status based on alocacoes)
    status venda_item_status NOT NULL DEFAULT 'PENDENTE',

    -- Observações
    observacoes TEXT,

    -- Links
    orcamento_item_id INTEGER REFERENCES orcamento_itens(id),
    compra_item_id INTEGER REFERENCES compra_itens(id),

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_venda_item_quantidade CHECK (quantidade > 0),
    CONSTRAINT chk_venda_item_valor CHECK (valor_unitario >= 0)
);

CREATE INDEX idx_venda_itens_venda ON venda_itens(venda_id);
CREATE INDEX idx_venda_itens_produto ON venda_itens(produto_id);
CREATE INDEX idx_venda_itens_status ON venda_itens(status);

-- ============================================================================
-- COMPRAS
-- ============================================================================
CREATE TABLE compras (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Número
    numero SERIAL,

    -- Datas
    data_emissao DATE NOT NULL DEFAULT CURRENT_DATE,
    data_prev_entrega DATE,
    data_real_entrega DATE,

    -- Totais
    subtotal DECIMAL(15,2) NOT NULL DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) NOT NULL DEFAULT 0,

    -- Observações
    observacoes TEXT,

    -- Status
    status compra_status NOT NULL DEFAULT 'RASCUNHO',

    -- Origem (se gerada de venda)
    venda_id INTEGER REFERENCES vendas(id),

    -- NFe de entrada
    nfe_entrada_id INTEGER,  -- FK para nfes

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE compra_itens (
    id SERIAL PRIMARY KEY,

    compra_id INTEGER NOT NULL REFERENCES compras(id) ON DELETE CASCADE,

    -- Produto
    produto_id INTEGER NOT NULL REFERENCES produtos(id),

    -- Quantidades
    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_caixas DECIMAL(15,4),
    unidade VARCHAR(10) DEFAULT 'UN',

    -- Valores
    valor_unitario DECIMAL(15,4),
    valor_total DECIMAL(15,2),

    -- Status
    status compra_item_status NOT NULL DEFAULT 'PENDENTE',

    -- Origem (pode estar vinculado a uma venda_item específica)
    venda_item_id INTEGER REFERENCES venda_itens(id),

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_compra_item_quantidade CHECK (quantidade > 0)
);

CREATE INDEX idx_compra_itens_compra ON compra_itens(compra_id);
CREATE INDEX idx_compra_itens_venda_item ON compra_itens(venda_item_id) WHERE venda_item_id IS NOT NULL;
```text

### 3.4 NFe

```sql
-- ============================================================================
-- NFe (Nota Fiscal Eletrônica)
-- ============================================================================
CREATE TABLE nfes (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),

    -- Tipo
    tipo nfe_tipo NOT NULL,
    modelo VARCHAR(2) DEFAULT '55',  -- 55=NFe, 65=NFCe

    -- Identificação
    numero INTEGER,
    serie INTEGER DEFAULT 1,
    chave VARCHAR(44) UNIQUE,

    -- Partes (tipo-dependente)
    -- ENTRADA: emitente=fornecedor, destinatario=loja
    -- SAIDA: emitente=loja, destinatario=cliente
    emitente_tipo VARCHAR(20),  -- 'fornecedor', 'loja'
    emitente_id INTEGER,
    destinatario_tipo VARCHAR(20),  -- 'cliente', 'loja', 'fornecedor'
    destinatario_id INTEGER,

    -- Totais (parseados para queries frequentes)
    valor_produtos DECIMAL(15,2),
    valor_frete DECIMAL(15,2),
    valor_seguro DECIMAL(15,2),
    valor_desconto DECIMAL(15,2),
    valor_ipi DECIMAL(15,2),
    valor_icms DECIMAL(15,2),
    valor_icms_st DECIMAL(15,2),
    valor_pis DECIMAL(15,2),
    valor_cofins DECIMAL(15,2),
    valor_total DECIMAL(15,2),

    -- Datas
    data_emissao TIMESTAMPTZ,
    data_saida_entrada TIMESTAMPTZ,
    data_autorizacao TIMESTAMPTZ,

    -- Status e protocolo
    status nfe_status NOT NULL DEFAULT 'RASCUNHO',
    protocolo VARCHAR(50),
    motivo_rejeicao TEXT,

    -- XML (fonte da verdade)
    xml_original TEXT,      -- XML completo
    xml_protocolo TEXT,     -- XML com protocolo (autorizada)

    -- Links
    venda_id INTEGER REFERENCES vendas(id),
    compra_id INTEGER REFERENCES compras(id),
    nfe_referenciada_id INTEGER REFERENCES nfes(id),  -- Para devoluções

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_nfes_tipo ON nfes(tipo);
CREATE INDEX idx_nfes_status ON nfes(status);
CREATE INDEX idx_nfes_chave ON nfes(chave) WHERE chave IS NOT NULL;
CREATE INDEX idx_nfes_venda ON nfes(venda_id) WHERE venda_id IS NOT NULL;
CREATE INDEX idx_nfes_compra ON nfes(compra_id) WHERE compra_id IS NOT NULL;

-- ============================================================================
-- NFe ITENS (JSONB para flexibilidade fiscal)
-- ============================================================================
CREATE TABLE nfe_itens (
    id SERIAL PRIMARY KEY,

    nfe_id INTEGER NOT NULL REFERENCES nfes(id) ON DELETE CASCADE,

    -- Identificação mínima para JOINs
    numero_item INTEGER NOT NULL,
    produto_id INTEGER REFERENCES produtos(id),  -- Após pareamento

    -- TODOS os dados fiscais em JSONB
    -- Estrutura flexível, preserva campos desconhecidos
    dados JSONB NOT NULL,

    -- Links para rastreabilidade
    venda_item_id INTEGER REFERENCES venda_itens(id),
    compra_item_id INTEGER REFERENCES compra_itens(id),

    created_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT uk_nfe_item UNIQUE (nfe_id, numero_item)
);

CREATE INDEX idx_nfe_itens_nfe ON nfe_itens(nfe_id);
CREATE INDEX idx_nfe_itens_dados ON nfe_itens USING GIN (dados);
-- Índices específicos (adicionar conforme profiling)
-- CREATE INDEX idx_nfe_itens_cfop ON nfe_itens ((dados->>'cfop'));

COMMENT ON TABLE nfe_itens IS 'Itens da NFe com dados fiscais em JSONB para flexibilidade (reforma tributária IBS/CBS)';
COMMENT ON COLUMN nfe_itens.dados IS 'Contém: cfop, ncm, cest, quantidade, valores, icms{}, ipi{}, pis{}, cofins{}, icms_st{}, etc';
```text

### 3.5 Inventário

```sql
-- ============================================================================
-- BLOCOS DO GALPÃO (localização física)
-- ============================================================================
CREATE TABLE galpao_blocos (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),

    codigo VARCHAR(20) NOT NULL,
    descricao VARCHAR(100),

    -- Posição física
    linha SMALLINT,
    coluna SMALLINT,

    is_ativo BOOLEAN DEFAULT TRUE,

    CONSTRAINT uk_bloco_codigo UNIQUE (loja_id, codigo)
);

-- ============================================================================
-- LOTES DE ESTOQUE (estado atual - como stock.quant do Odoo)
-- ============================================================================
CREATE TABLE estoque_lotes (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    produto_id INTEGER NOT NULL REFERENCES produtos(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),

    -- Origem
    nfe_item_id INTEGER REFERENCES nfe_itens(id),
    compra_item_id INTEGER REFERENCES compra_itens(id),

    -- Quantidades
    quantidade_inicial DECIMAL(15,4) NOT NULL,
    quantidade_disponivel DECIMAL(15,4) NOT NULL,
    quantidade_reservada DECIMAL(15,4) DEFAULT 0,  -- Alocada mas não entregue

    -- Custo
    custo_unitario DECIMAL(15,4) NOT NULL,
    custo_total DECIMAL(15,2) GENERATED ALWAYS AS (quantidade_inicial * custo_unitario) STORED,

    -- Rastreamento
    lote VARCHAR(50),
    data_validade DATE,

    -- Status
    status estoque_lote_status NOT NULL DEFAULT 'DISPONIVEL',

    -- FIFO key
    data_entrada TIMESTAMPTZ NOT NULL DEFAULT NOW(),

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_lote_quantidade_positiva CHECK (quantidade_inicial > 0),
    CONSTRAINT chk_lote_disponivel CHECK (quantidade_disponivel >= 0),
    CONSTRAINT chk_lote_reservada CHECK (quantidade_reservada >= 0),
    CONSTRAINT chk_lote_total CHECK (quantidade_disponivel + quantidade_reservada <= quantidade_inicial)
);

CREATE INDEX idx_estoque_lotes_produto ON estoque_lotes(produto_id);
CREATE INDEX idx_estoque_lotes_disponivel ON estoque_lotes(produto_id, loja_id, data_entrada)
    WHERE quantidade_disponivel > 0 AND status = 'DISPONIVEL';
CREATE INDEX idx_estoque_lotes_lote ON estoque_lotes(lote) WHERE lote IS NOT NULL;

-- ============================================================================
-- LOCALIZAÇÕES DE ESTOQUE (Múltiplas localizações por lote - split de paletes)
-- ============================================================================
CREATE TABLE estoque_localizacoes (
    id SERIAL PRIMARY KEY,

    lote_id INTEGER NOT NULL REFERENCES estoque_lotes(id) ON DELETE CASCADE,
    bloco_id INTEGER NOT NULL REFERENCES galpao_blocos(id),

    -- Quantidade nesta localização específica
    quantidade DECIMAL(15,4) NOT NULL,

    -- Quando foi movido para esta localização
    data_entrada TIMESTAMPTZ DEFAULT NOW(),

    created_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_localizacao_quantidade CHECK (quantidade > 0),
    CONSTRAINT uk_lote_bloco UNIQUE (lote_id, bloco_id)
);

CREATE INDEX idx_estoque_localizacoes_lote ON estoque_localizacoes(lote_id);
CREATE INDEX idx_estoque_localizacoes_bloco ON estoque_localizacoes(bloco_id);
CREATE INDEX idx_estoque_localizacoes_data ON estoque_localizacoes(data_entrada);

-- ============================================================================
-- MOVIMENTAÇÕES DE ESTOQUE (log de tudo - como stock.move do Odoo)
-- ============================================================================
CREATE TABLE estoque_movimentacoes (
    id BIGSERIAL PRIMARY KEY,

    lote_id INTEGER NOT NULL REFERENCES estoque_lotes(id),

    -- Tipo
    tipo movimentacao_tipo NOT NULL,

    -- Quantidade (positivo = entrada, negativo = saída)
    quantidade DECIMAL(15,4) NOT NULL,

    -- Custo no momento
    custo_unitario DECIMAL(15,4) NOT NULL,

    -- Referência (polimórfica)
    referencia_tipo VARCHAR(50),  -- 'nfe_item', 'venda_item', 'ajuste', 'transferencia'
    referencia_id INTEGER,

    -- Quem/quando
    usuario_id INTEGER REFERENCES usuarios(id),
    observacoes TEXT,

    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_movimentacoes_lote ON estoque_movimentacoes(lote_id);
CREATE INDEX idx_movimentacoes_tipo ON estoque_movimentacoes(tipo);
CREATE INDEX idx_movimentacoes_data ON estoque_movimentacoes(created_at);

-- ============================================================================
-- ALOCAÇÕES (M:N: múltiplos lotes podem abastecer 1 venda_item)
-- ============================================================================
CREATE TABLE alocacoes (
    id SERIAL PRIMARY KEY,

    -- M:N: venda_item pode ter múltiplas alocacoes (de diferentes lotes)
    venda_item_id INTEGER NOT NULL REFERENCES venda_itens(id) ON DELETE CASCADE,
    lote_id INTEGER NOT NULL REFERENCES estoque_lotes(id),

    -- Quantidade DESTA alocação (não precisa ser a quantidade total do item)
    quantidade DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,
    custo_total DECIMAL(15,2) GENERATED ALWAYS AS (quantidade * custo_unitario) STORED,

    -- Status da alocação (não do item)
    -- ATIVO: alocação ativa
    -- PARCIALMENTE_ESTORNADO: parte foi estornada
    -- TOTALMENTE_ESTORNADO: inteiramente estornada
    status VARCHAR(50) DEFAULT 'ATIVO',

    -- Estorno
    estornado_em TIMESTAMPTZ,
    estorno_motivo VARCHAR(200),
    estornado_por INTEGER REFERENCES usuarios(id),

    -- Auditoria
    created_at TIMESTAMPTZ DEFAULT NOW(),
    created_by INTEGER REFERENCES usuarios(id)
);

-- Índices (sem UNIQUE - permite M:N)
CREATE INDEX idx_alocacoes_venda_item ON alocacoes(venda_item_id);
CREATE INDEX idx_alocacoes_lote ON alocacoes(lote_id);
CREATE INDEX idx_alocacoes_status ON alocacoes(status);

-- TRIGGER: Validar que SUM(quantidade) de alocacoes ativas <= venda_item.quantidade
-- (Implementado em 4. Triggers de Integridade)

-- ============================================================================
-- VIEW: Saldo de Estoque por Produto
-- ============================================================================
CREATE VIEW estoque_saldos AS
SELECT
    loja_id,
    produto_id,
    fornecedor_id,
    SUM(quantidade_disponivel) as disponivel,
    SUM(quantidade_reservada) as reservado,
    SUM(quantidade_disponivel + quantidade_reservada) as total,
    SUM(quantidade_disponivel * custo_unitario) / NULLIF(SUM(quantidade_disponivel), 0) as custo_medio
FROM estoque_lotes
WHERE status IN ('DISPONIVEL', 'RESERVADO')
GROUP BY loja_id, produto_id, fornecedor_id;

-- ============================================================================
-- VIEW: Sugestão FIFO para Alocação
-- ============================================================================
CREATE VIEW estoque_fifo AS
SELECT
    el.*,
    p.descricao as produto_descricao,
    f.razao_social as fornecedor_nome,
    ROW_NUMBER() OVER (
        PARTITION BY el.produto_id, el.loja_id
        ORDER BY el.data_entrada
    ) as ordem_fifo
FROM estoque_lotes el
JOIN produtos p ON p.id = el.produto_id
JOIN fornecedores f ON f.id = el.fornecedor_id
WHERE el.quantidade_disponivel > 0
  AND el.status = 'DISPONIVEL';
```text

### 3.6 Logística

```sql
-- ============================================================================
-- ENTREGAS
-- ============================================================================
CREATE TABLE entregas (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    venda_id INTEGER NOT NULL REFERENCES vendas(id),

    -- Número
    numero SERIAL,

    -- Transporte
    transportadora_id INTEGER REFERENCES transportadoras(id),
    veiculo_placa VARCHAR(10),
    motorista_nome VARCHAR(100),

    -- Endereço
    endereco_id INTEGER NOT NULL REFERENCES enderecos(id),

    -- Datas
    data_agendada DATE NOT NULL,
    hora_inicio TIME,
    hora_fim TIME,
    data_realizada TIMESTAMPTZ,

    -- Status
    status entrega_status NOT NULL DEFAULT 'AGENDADA',

    -- Recebedor
    recebedor_nome VARCHAR(100),
    recebedor_documento VARCHAR(20),

    -- Observações
    observacoes TEXT,

    -- NFe de saída
    nfe_saida_id INTEGER REFERENCES nfes(id),

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_entregas_venda ON entregas(venda_id);
CREATE INDEX idx_entregas_data ON entregas(data_agendada);
CREATE INDEX idx_entregas_status ON entregas(status);

CREATE TABLE entrega_itens (
    id SERIAL PRIMARY KEY,

    entrega_id INTEGER NOT NULL REFERENCES entregas(id) ON DELETE CASCADE,
    venda_item_id INTEGER NOT NULL REFERENCES venda_itens(id),

    -- Quantidade (pode ser parcial)
    quantidade DECIMAL(15,4) NOT NULL,

    -- Status individual
    is_entregue BOOLEAN DEFAULT FALSE,

    observacoes TEXT
);

CREATE INDEX idx_entrega_itens_entrega ON entrega_itens(entrega_id);
```text

### 3.7 Financeiro

```sql
-- ============================================================================
-- FINANCEIRO (Unified: Contas a Receber & Pagar)
-- ============================================================================
CREATE TABLE financeiro_parcelas (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),

    -- Tipo de obrigação financeira
    tipo financeiro_status NOT NULL,  -- RECEBER ou PAGAR

    -- Polimórfico: cliente OU fornecedor (não ambos)
    cliente_id INTEGER REFERENCES clientes(id),
    fornecedor_id INTEGER REFERENCES fornecedores(id),

    -- Documentos de origem
    venda_id INTEGER REFERENCES vendas(id),
    compra_id INTEGER REFERENCES compras(id),
    nfe_entrada_id INTEGER REFERENCES nfes(id),  -- Para PAGAR

    -- Identificação da parcela
    numero_parcela SMALLINT NOT NULL DEFAULT 1,
    total_parcelas SMALLINT NOT NULL DEFAULT 1,

    -- Valores
    valor DECIMAL(15,2) NOT NULL,
    valor_recebido_pago DECIMAL(15,2) DEFAULT 0,  -- Unificado (recebido ou pago)
    valor_juros DECIMAL(15,2) DEFAULT 0,
    valor_multa DECIMAL(15,2) DEFAULT 0,
    valor_desconto DECIMAL(15,2) DEFAULT 0,

    -- Datas
    data_vencimento DATE NOT NULL,
    data_recebimento_pagamento DATE,  -- Unificado

    -- Forma de pagamento
    forma_pagamento forma_pagamento,

    -- Status
    status financeiro_status NOT NULL DEFAULT 'PENDENTE',

    -- CNAB (para ambos RECEBER e PAGAR)
    nosso_numero VARCHAR(50),
    linha_digitavel VARCHAR(100),
    codigo_barras VARCHAR(50),
    remessa_id INTEGER REFERENCES remessas_cnab(id),

    -- Documento de origem
    documento VARCHAR(50),
    observacoes TEXT,

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    -- Constraint: tipo RECEBER → cliente_id NOT NULL
    -- Constraint: tipo PAGAR → fornecedor_id NOT NULL
    CONSTRAINT chk_tipo_pessoa CHECK (
        (tipo = 'RECEBER' AND cliente_id IS NOT NULL AND fornecedor_id IS NULL)
        OR (tipo = 'PAGAR' AND fornecedor_id IS NOT NULL AND cliente_id IS NULL)
    )
);

CREATE INDEX idx_financeiro_loja_tipo ON financeiro_parcelas(loja_id, tipo);
CREATE INDEX idx_financeiro_cliente ON financeiro_parcelas(cliente_id) WHERE tipo = 'RECEBER';
CREATE INDEX idx_financeiro_fornecedor ON financeiro_parcelas(fornecedor_id) WHERE tipo = 'PAGAR';
CREATE INDEX idx_financeiro_vencimento ON financeiro_parcelas(data_vencimento);
CREATE INDEX idx_financeiro_status ON financeiro_parcelas(status);
CREATE INDEX idx_financeiro_remessa ON financeiro_parcelas(remessa_id);

-- ============================================================================
-- VIEWS: Para manter clarity (opcional mas recomendado)
-- ============================================================================
CREATE VIEW parcelas_receber AS
SELECT * FROM financeiro_parcelas WHERE tipo = 'RECEBER';

CREATE VIEW parcelas_pagar AS
SELECT * FROM financeiro_parcelas WHERE tipo = 'PAGAR';

COMMENT ON TABLE financeiro_parcelas IS
'Unified financial obligations (Contas a Receber e Contas a Pagar).
Use tipo column to distinguish between RECEBER (cliente) and PAGAR (fornecedor).
See views parcelas_receber/parcelas_pagar for convenience.';

COMMENT ON COLUMN financeiro_parcelas.tipo IS
'RECEBER = contas a receber (cliente), PAGAR = contas a pagar (fornecedor)';

COMMENT ON COLUMN financeiro_parcelas.valor_recebido_pago IS
'Unificado: valor recebido (RECEBER) or valor pago (PAGAR)';

COMMENT ON COLUMN financeiro_parcelas.data_recebimento_pagamento IS
'Unificado: data do recebimento (RECEBER) or data do pagamento (PAGAR)';

-- ============================================================================
-- CNAB (Remessas e Retornos)
-- ============================================================================
CREATE TABLE remessas_cnab (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),

    -- Arquivo
    nome_arquivo VARCHAR(200) NOT NULL,
    conteudo TEXT NOT NULL,

    -- Controle
    data_geracao TIMESTAMPTZ DEFAULT NOW(),
    data_envio TIMESTAMPTZ,

    -- Totais
    quantidade_titulos INTEGER NOT NULL,
    valor_total DECIMAL(15,2) NOT NULL,

    created_by INTEGER REFERENCES usuarios(id)
);

CREATE TABLE retornos_cnab (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),

    -- Arquivo
    nome_arquivo VARCHAR(200) NOT NULL,
    conteudo TEXT NOT NULL,

    -- Processamento
    data_processamento TIMESTAMPTZ DEFAULT NOW(),

    -- Totais
    quantidade_titulos INTEGER NOT NULL,
    quantidade_baixados INTEGER NOT NULL,
    valor_total DECIMAL(15,2) NOT NULL,

    processed_by INTEGER REFERENCES usuarios(id)
);
```text

### 3.8 Auditoria

```sql
-- ============================================================================
-- AUDIT LOG
-- ============================================================================
CREATE TABLE audit_log (
    id BIGSERIAL PRIMARY KEY,

    -- O que
    tabela VARCHAR(100) NOT NULL,
    registro_id INTEGER NOT NULL,
    acao VARCHAR(10) NOT NULL,  -- INSERT, UPDATE, DELETE

    -- Mudanças
    dados_antigos JSONB,
    dados_novos JSONB,
    campos_alterados TEXT[],

    -- Quem
    usuario_id INTEGER,
    usuario_nome VARCHAR(200),

    -- Onde
    ip_address INET,
    user_agent TEXT,

    -- Contexto
    transacao_id VARCHAR(50),
    modulo VARCHAR(50),

    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_audit_tabela_registro ON audit_log(tabela, registro_id);
CREATE INDEX idx_audit_usuario ON audit_log(usuario_id);
CREATE INDEX idx_audit_created ON audit_log(created_at);

-- ============================================================================
-- TRIGGER GENÉRICO DE AUDITORIA
-- ============================================================================
CREATE OR REPLACE FUNCTION fn_audit_trigger()
RETURNS TRIGGER AS $$
DECLARE
    v_old_data JSONB;
    v_new_data JSONB;
    v_changed_fields TEXT[];
BEGIN
    IF TG_OP = 'INSERT' THEN
        v_new_data := to_jsonb(NEW);
        INSERT INTO audit_log (tabela, registro_id, acao, dados_novos)
        VALUES (TG_TABLE_NAME, NEW.id, 'INSERT', v_new_data);
        RETURN NEW;

    ELSIF TG_OP = 'UPDATE' THEN
        v_old_data := to_jsonb(OLD);
        v_new_data := to_jsonb(NEW);

        -- Identificar campos alterados
        SELECT array_agg(key)
        INTO v_changed_fields
        FROM jsonb_each(v_old_data) AS o(key, value)
        WHERE v_new_data->>key IS DISTINCT FROM o.value::text;

        INSERT INTO audit_log (tabela, registro_id, acao, dados_antigos, dados_novos, campos_alterados)
        VALUES (TG_TABLE_NAME, NEW.id, 'UPDATE', v_old_data, v_new_data, v_changed_fields);
        RETURN NEW;

    ELSIF TG_OP = 'DELETE' THEN
        v_old_data := to_jsonb(OLD);
        INSERT INTO audit_log (tabela, registro_id, acao, dados_antigos)
        VALUES (TG_TABLE_NAME, OLD.id, 'DELETE', v_old_data);
        RETURN OLD;
    END IF;

    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

-- Aplicar em tabelas críticas
CREATE TRIGGER audit_vendas AFTER INSERT OR UPDATE OR DELETE ON vendas
    FOR EACH ROW EXECUTE FUNCTION fn_audit_trigger();

CREATE TRIGGER audit_venda_itens AFTER INSERT OR UPDATE OR DELETE ON venda_itens
    FOR EACH ROW EXECUTE FUNCTION fn_audit_trigger();

CREATE TRIGGER audit_compras AFTER INSERT OR UPDATE OR DELETE ON compras
    FOR EACH ROW EXECUTE FUNCTION fn_audit_trigger();

CREATE TRIGGER audit_estoque_lotes AFTER INSERT OR UPDATE OR DELETE ON estoque_lotes
    FOR EACH ROW EXECUTE FUNCTION fn_audit_trigger();

CREATE TRIGGER audit_alocacoes AFTER INSERT OR UPDATE OR DELETE ON alocacoes
    FOR EACH ROW EXECUTE FUNCTION fn_audit_trigger();

CREATE TRIGGER audit_nfes AFTER INSERT OR UPDATE OR DELETE ON nfes
    FOR EACH ROW EXECUTE FUNCTION fn_audit_trigger();
```text

---

## 4. Enforcement: Multi-Layer Quantity Integrity

### Overview: Garantindo Quantidade Correta

O schema implementa 4 camadas de proteção para garantir que **nunca há over-allocation ou quantidades negativas**:

```
CAMADA 1: CHECK Constraints (BD - sempre ativo)
    ↓
CAMADA 2: GENERATED Columns (BD - valores calculados)
    ↓
CAMADA 3: Triggers BEFORE INSERT/UPDATE (BD - validação)
    ↓
CAMADA 4: Verification Views + Periodic Jobs (Detecção)
```

#### **CAMADA 1: CHECK Constraints**

Regras declarativas que o PostgreSQL valida automaticamente:

```sql
-- Quantidades sempre positivas
ALTER TABLE estoque_lotes
ADD CONSTRAINT chk_lote_quantidade_positiva CHECK (quantidade_inicial > 0);

ALTER TABLE estoque_lotes
ADD CONSTRAINT chk_lote_disponivel CHECK (quantidade_disponivel >= 0);

ALTER TABLE estoque_lotes
ADD CONSTRAINT chk_lote_reservada CHECK (quantidade_reservada >= 0);

-- REGRA DE OURO: disponível + reservado ≤ inicial
ALTER TABLE estoque_lotes
ADD CONSTRAINT chk_lote_total CHECK (quantidade_disponivel + quantidade_reservada <= quantidade_inicial);

-- Alocações sempre positivas
ALTER TABLE alocacoes
ADD CONSTRAINT chk_alocacao_quantidade CHECK (quantidade > 0);

-- Entregas sempre positivas
ALTER TABLE entrega_itens
ADD CONSTRAINT chk_entrega_quantidade CHECK (quantidade > 0);

-- Itens sempre positivos
ALTER TABLE venda_itens
ADD CONSTRAINT chk_venda_quantidade CHECK (quantidade > 0);
```

**Efeito**: Qualquer INSERT/UPDATE que viole estas regras é **REJEITADO IMEDIATAMENTE** pelo BD.

#### **CAMADA 2: GENERATED Columns**

Valores calculados automaticamente - **não podem ser sobrescritos manualmente**:

```sql
-- Em alocacoes: custo_total = quantidade × custo_unitario
ALTER TABLE alocacoes
ADD COLUMN custo_total DECIMAL(15,2)
GENERATED ALWAYS AS (quantidade * custo_unitario) STORED;

-- Se alguém tentar: UPDATE alocacoes SET custo_total = 999
-- PostgreSQL retorna: ERROR: cannot update a generated column
```

**Efeito**: Impossível ter valores inconsistentes. Total é sempre correto.

#### **CAMADA 3: Triggers (Validação Inteligente)**

Triggers BEFORE INSERT validam regras complexas que CHECK não consegue:

```sql
-- Validar ANTES de inserir:
-- 1. SUM(alocacoes.qtd) não pode > venda_item.qtd
-- 2. Lote tem stock disponível
-- 3. Produto e fornecedor combinam
-- 4. Status permite alocação
```

Triggers AFTER INSERT/UPDATE atualizam automaticamente valores derivados:

```sql
-- Após alocar:
-- 1. Subtrair de estoque_lotes.quantidade_disponivel
-- 2. Adicionar a estoque_lotes.quantidade_reservada
-- 3. Registrar movimento em estoque_movimentacoes
-- 4. Atualizar status de venda_item se totalmente alocado
```

**Efeito**: Mesmo se aplicação tiver bug, triggers garantem integridade no BD.

#### **CAMADA 4: Verification Views + Cron Jobs**

Views que mostram inconsistências (se houvesse):
- `consistencia_estoque` - verifica REGRA DE OURO
- `verificacao_alocacoes` - detecta over-allocation
- `verificacao_entregas` - detecta delivery > allocation

Função `verificar_integridade_estoque()` executada nightly:
- Procura por erros nas 6 dimensões críticas
- Log em `integridade_log` para auditoria
- Alertas se encontrar problemas

**Efeito**: Detecção de bugs ou entrada direta de dados corruptora.

---

### 4.1 Validar Alocação

```sql
CREATE OR REPLACE FUNCTION fn_validar_alocacao()
RETURNS TRIGGER AS $$
DECLARE
    v_qtd_item DECIMAL(15,4);
    v_qtd_alocada_total DECIMAL(15,4);
    v_qtd_disponivel DECIMAL(15,4);
    v_status_item venda_item_status;
    v_produto_item INTEGER;
    v_produto_lote INTEGER;
    v_fornecedor_item INTEGER;
    v_fornecedor_lote INTEGER;
BEGIN
    -- Buscar dados do venda_item
    SELECT quantidade, status, produto_id, fornecedor_id
    INTO v_qtd_item, v_status_item, v_produto_item, v_fornecedor_item
    FROM venda_itens WHERE id = NEW.venda_item_id;

    -- Buscar dados do lote
    SELECT quantidade_disponivel, produto_id, fornecedor_id
    INTO v_qtd_disponivel, v_produto_lote, v_fornecedor_lote
    FROM estoque_lotes WHERE id = NEW.lote_id;

    -- REGRA 1: Quantidade alocada não pode exceder quantidade do item
    SELECT COALESCE(SUM(quantidade), 0)
    INTO v_qtd_alocada_total
    FROM alocacoes
    WHERE venda_item_id = NEW.venda_item_id AND status = 'ATIVO';

    IF (v_qtd_alocada_total + NEW.quantidade) > v_qtd_item THEN
        RAISE EXCEPTION 'Total alocado (% + %) excede quantidade do item (%)',
            v_qtd_alocada_total, NEW.quantidade, v_qtd_item;
    END IF;

    -- REGRA 2: Lote deve ter disponível
    IF v_qtd_disponivel < NEW.quantidade THEN
        RAISE EXCEPTION 'Lote sem quantidade disponível: disp=%, solicitado=%',
            v_qtd_disponivel, NEW.quantidade;
    END IF;

    -- REGRA 3: Mesmo produto
    IF v_produto_item != v_produto_lote THEN
        RAISE EXCEPTION 'Produto do item (%) diferente do lote (%)',
            v_produto_item, v_produto_lote;
    END IF;

    -- REGRA 4: Mesmo fornecedor
    IF v_fornecedor_item != v_fornecedor_lote THEN
        RAISE EXCEPTION 'Fornecedor do item (%) diferente do lote (%)',
            v_fornecedor_item, v_fornecedor_lote;
    END IF;

    -- REGRA 5: Status permite alocação
    IF v_status_item NOT IN ('PENDENTE', 'EM_COMPRA', 'CONFIRMADO', 'FATURADO',
                              'EM_TRANSITO', 'EM_RECEBIMENTO') THEN
        RAISE EXCEPTION 'Item com status "%" não pode ser alocado', v_status_item;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_validar_alocacao
    BEFORE INSERT ON alocacoes
    FOR EACH ROW EXECUTE FUNCTION fn_validar_alocacao();
```text

### 4.2 Atualizar Estoque Após Alocação

```sql
CREATE OR REPLACE FUNCTION fn_apos_alocacao()
RETURNS TRIGGER AS $$
DECLARE
    v_total_alocado DECIMAL(15,4);
    v_qtd_item DECIMAL(15,4);
BEGIN
    IF TG_OP = 'INSERT' AND NEW.status = 'ATIVO' THEN
        -- Alocação ativa: diminuir disponível, aumentar reservado
        UPDATE estoque_lotes
        SET quantidade_disponivel = quantidade_disponivel - NEW.quantidade,
            quantidade_reservada = quantidade_reservada + NEW.quantidade,
            status = CASE
                WHEN quantidade_disponivel - NEW.quantidade = 0 THEN 'RESERVADO'::estoque_lote_status
                ELSE status
            END,
            updated_at = NOW()
        WHERE id = NEW.lote_id;

        -- Calcular total alocado para este venda_item
        SELECT COALESCE(SUM(quantidade), 0)
        INTO v_total_alocado
        FROM alocacoes
        WHERE venda_item_id = NEW.venda_item_id AND status = 'ATIVO';

        SELECT quantidade
        INTO v_qtd_item
        FROM venda_itens
        WHERE id = NEW.venda_item_id;

        -- Atualizar status do item se totalmente alocado
        UPDATE venda_itens
        SET status = CASE
                WHEN v_total_alocado >= v_qtd_item THEN 'ESTOQUE'::venda_item_status
                ELSE status
            END,
            updated_at = NOW()
        WHERE id = NEW.venda_item_id;

        -- Registrar movimentação
        INSERT INTO estoque_movimentacoes (lote_id, tipo, quantidade, custo_unitario, referencia_tipo, referencia_id, usuario_id)
        VALUES (NEW.lote_id, 'SAIDA_VENDA', -NEW.quantidade, NEW.custo_unitario, 'venda_item', NEW.venda_item_id, NEW.created_by);

    ELSIF TG_OP = 'UPDATE' AND (NEW.status = 'TOTALMENTE_ESTORNADO' OR (NEW.status = 'PARCIALMENTE_ESTORNADO' AND OLD.status = 'ATIVO')) THEN
        -- Estorno: restaurar disponível, diminuir reservado
        UPDATE estoque_lotes
        SET quantidade_disponivel = quantidade_disponivel + OLD.quantidade,
            quantidade_reservada = quantidade_reservada - OLD.quantidade,
            status = 'DISPONIVEL'::estoque_lote_status,
            updated_at = NOW()
        WHERE id = OLD.lote_id;

        -- Registrar movimentação de estorno
        INSERT INTO estoque_movimentacoes (lote_id, tipo, quantidade, custo_unitario, referencia_tipo, referencia_id, usuario_id, observacoes)
        VALUES (OLD.lote_id, 'ENTRADA_AJUSTE', OLD.quantidade, OLD.custo_unitario, 'alocacao_estorno', OLD.id, NEW.estornado_por, NEW.estorno_motivo);
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_apos_alocacao
    AFTER INSERT OR UPDATE ON alocacoes
    FOR EACH ROW EXECUTE FUNCTION fn_apos_alocacao();
```text

### 4.3 Validar Transições de Status

```sql
CREATE OR REPLACE FUNCTION fn_validar_transicao_venda_item()
RETURNS TRIGGER AS $$
DECLARE
    v_permitidas venda_item_status[];
BEGIN
    IF NEW.status = OLD.status THEN
        RETURN NEW;
    END IF;

    v_permitidas := CASE OLD.status
        WHEN 'PENDENTE' THEN ARRAY['EM_COMPRA', 'ESTOQUE', 'CANCELADO']
        WHEN 'EM_COMPRA' THEN ARRAY['CONFIRMADO', 'CANCELADO']
        WHEN 'CONFIRMADO' THEN ARRAY['FATURADO', 'CANCELADO']
        WHEN 'FATURADO' THEN ARRAY['EM_TRANSITO']
        WHEN 'EM_TRANSITO' THEN ARRAY['EM_RECEBIMENTO']
        WHEN 'EM_RECEBIMENTO' THEN ARRAY['ESTOQUE']
        WHEN 'ESTOQUE' THEN ARRAY['ENTREGA_AGENDADA', 'CANCELADO', 'PENDENTE']
        WHEN 'ENTREGA_AGENDADA' THEN ARRAY['EM_ENTREGA', 'ESTOQUE']
        WHEN 'EM_ENTREGA' THEN ARRAY['ENTREGUE', 'ESTOQUE']
        WHEN 'ENTREGUE' THEN ARRAY['DEVOLVIDO']
        ELSE ARRAY[]::venda_item_status[]
    END;

    IF NOT (NEW.status = ANY(v_permitidas)) THEN
        RAISE EXCEPTION 'Transição inválida: % -> %', OLD.status, NEW.status;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_validar_transicao_venda_item
    BEFORE UPDATE ON venda_itens
    FOR EACH ROW
    WHEN (OLD.status IS DISTINCT FROM NEW.status)
    EXECUTE FUNCTION fn_validar_transicao_venda_item();
```text

### 4.4 Verificação de Integridade (Views)

```sql
-- ============================================================================
-- VERIFICAÇÃO: Inconsistências de Estoque
-- ============================================================================
-- Golden Rule: quantidade_disponivel + quantidade_reservada = quantidade_inicial
-- Esta view detecta violações

CREATE VIEW consistencia_estoque AS
SELECT
    el.id,
    el.produto_id,
    el.quantidade_inicial,
    el.quantidade_disponivel,
    el.quantidade_reservada,
    (el.quantidade_disponivel + el.quantidade_reservada) AS total_atual,
    CASE
        WHEN (el.quantidade_disponivel + el.quantidade_reservada) > el.quantidade_inicial
            THEN 'ERRO: Over-allocated'
        WHEN (el.quantidade_disponivel + el.quantidade_reservada) < el.quantidade_inicial
            THEN 'AVISO: Quantity lost (triggers falhou?)'
        WHEN el.quantidade_disponivel < 0
            THEN 'ERRO: Negative disponível'
        WHEN el.quantidade_reservada < 0
            THEN 'ERRO: Negative reservado'
        ELSE 'OK'
    END AS status,
    el.updated_at
FROM estoque_lotes el
ORDER BY
    CASE status WHEN 'OK' THEN 2 ELSE 1 END,
    el.updated_at DESC;

COMMENT ON VIEW consistencia_estoque IS
'Verifica integridade de estoque. Deve sempre mostrar status=OK para todos os lotes.
Se houver erros, há bug nos triggers ou entrada direta de dados.';

-- ============================================================================
-- VERIFICAÇÃO: Over-allocation de Itens de Venda
-- ============================================================================

CREATE VIEW verificacao_alocacoes AS
SELECT
    vi.id AS venda_item_id,
    vi.venda_id,
    vi.quantidade AS item_quantity,
    COALESCE(SUM(CASE WHEN a.status = 'ATIVO' THEN a.quantidade ELSE 0 END), 0) AS allocated_ativo,
    COALESCE(SUM(CASE WHEN a.status IN ('PARCIALMENTE_ESTORNADO', 'TOTALMENTE_ESTORNADO')
                      THEN a.quantidade ELSE 0 END), 0) AS allocated_estornado,
    vi.quantidade - COALESCE(SUM(CASE WHEN a.status = 'ATIVO' THEN a.quantidade ELSE 0 END), 0) AS pending,
    CASE
        WHEN COALESCE(SUM(CASE WHEN a.status = 'ATIVO' THEN a.quantidade ELSE 0 END), 0) > vi.quantidade
            THEN 'ERRO: Over-allocated!'
        WHEN COALESCE(SUM(CASE WHEN a.status = 'ATIVO' THEN a.quantidade ELSE 0 END), 0) = vi.quantidade
            THEN 'OK: Fully allocated'
        WHEN COALESCE(SUM(CASE WHEN a.status = 'ATIVO' THEN a.quantidade ELSE 0 END), 0) > 0
            THEN 'OK: Partially allocated'
        ELSE 'OK: Pending allocation'
    END AS status
FROM venda_itens vi
LEFT JOIN alocacoes a ON vi.id = a.venda_item_id
GROUP BY vi.id, vi.venda_id, vi.quantidade
ORDER BY
    CASE status WHEN 'OK: Fully allocated' THEN 2 WHEN 'OK: Partially allocated' THEN 3 ELSE 1 END,
    vi.id;

COMMENT ON VIEW verificacao_alocacoes IS
'Verifica se alocações não excedem quantidade do item.
Detecta over-allocation (deve estar sempre OK).';

-- ============================================================================
-- VERIFICAÇÃO: Integridade de Entregas
-- ============================================================================

CREATE VIEW verificacao_entregas AS
SELECT
    vi.id AS venda_item_id,
    vi.venda_id,
    vi.quantidade AS item_quantity,
    COALESCE(a.quantidade, 0) AS allocated_quantity,
    COALESCE(SUM(ei.quantidade), 0) AS delivered_quantity,
    vi.quantidade - COALESCE(SUM(ei.quantidade), 0) AS pending_delivery,
    CASE
        WHEN COALESCE(SUM(ei.quantidade), 0) > vi.quantidade
            THEN 'ERRO: Delivered > allocated'
        WHEN COALESCE(SUM(ei.quantidade), 0) = vi.quantidade
            AND a.status = 'ATIVO'
            THEN 'OK: Fully delivered'
        ELSE 'OK'
    END AS status
FROM venda_itens vi
LEFT JOIN alocacoes a ON vi.id = a.venda_item_id AND a.status = 'ATIVO'
LEFT JOIN entrega_itens ei ON vi.id = ei.venda_item_id
GROUP BY vi.id, vi.venda_id, vi.quantidade, a.quantidade, a.status
ORDER BY
    CASE status WHEN 'OK: Fully delivered' THEN 2 ELSE 1 END,
    vi.id;

COMMENT ON VIEW verificacao_entregas IS
'Verifica se entregas não excedem alocações.
Detecta inconsistências entre alocação e entrega física.';
```text

### 4.5 Função de Verificação Periódica

```sql
-- ============================================================================
-- VERIFICAÇÃO PERIÓDICA: Função de Integridade Completa
-- ============================================================================
-- Execute nightly via cron: SELECT verificar_integridade_estoque();

CREATE OR REPLACE FUNCTION verificar_integridade_estoque()
RETURNS TABLE(
    tipo VARCHAR,
    tabela VARCHAR,
    registro_id INTEGER,
    mensagem TEXT,
    severidade VARCHAR
) AS $$
BEGIN

    -- CHECK 1: Over-allocated lots (quantidade reservada > inicial)
    RETURN QUERY
    SELECT 'Lot Over-allocation'::VARCHAR, 'estoque_lotes'::VARCHAR,
           el.id,
           'Lot ' || el.id || ': total (' || (el.quantidade_disponivel + el.quantidade_reservada)::TEXT
           || ') > inicial (' || el.quantidade_inicial::TEXT || ')',
           'CRÍTICA'::VARCHAR
    FROM estoque_lotes el
    WHERE (el.quantidade_disponivel + el.quantidade_reservada) > el.quantidade_inicial;

    -- CHECK 2: Negative disponível
    RETURN QUERY
    SELECT 'Negative Stock'::VARCHAR, 'estoque_lotes'::VARCHAR,
           id,
           'Lot ' || id || ' has negative disponível: ' || quantidade_disponivel::TEXT,
           'CRÍTICA'::VARCHAR
    FROM estoque_lotes
    WHERE quantidade_disponivel < 0;

    -- CHECK 3: Negative reservado
    RETURN QUERY
    SELECT 'Negative Reserved'::VARCHAR, 'estoque_lotes'::VARCHAR,
           id,
           'Lot ' || id || ' has negative reservado: ' || quantidade_reservada::TEXT,
           'CRÍTICA'::VARCHAR
    FROM estoque_lotes
    WHERE quantidade_reservada < 0;

    -- CHECK 4: Over-allocated venda_items
    RETURN QUERY
    SELECT 'Item Over-allocation'::VARCHAR, 'venda_itens'::VARCHAR,
           vi.id,
           'Item ' || vi.id || ': allocated (' || SUM(a.quantidade)::TEXT
           || ') > requested (' || vi.quantidade::TEXT || ')',
           'CRÍTICA'::VARCHAR
    FROM venda_itens vi
    LEFT JOIN alocacoes a ON vi.id = a.venda_item_id AND a.status = 'ATIVO'
    GROUP BY vi.id, vi.quantidade
    HAVING SUM(a.quantidade) > vi.quantidade;

    -- CHECK 5: Delivery > allocation
    RETURN QUERY
    SELECT 'Delivery Mismatch'::VARCHAR, 'entrega_itens'::VARCHAR,
           vi.id,
           'Item ' || vi.id || ': delivered (' || SUM(ei.quantidade)::TEXT
           || ') > allocated (' || COALESCE(a.quantidade, 0)::TEXT || ')',
           'AVISO'::VARCHAR
    FROM venda_itens vi
    LEFT JOIN entrega_itens ei ON vi.id = ei.venda_item_id
    LEFT JOIN alocacoes a ON vi.id = a.venda_item_id AND a.status = 'ATIVO'
    GROUP BY vi.id, a.quantidade
    HAVING SUM(ei.quantidade) > COALESCE(a.quantidade, 0);

    -- CHECK 6: Allocated but marked PENDENTE (should be ESTOQUE)
    RETURN QUERY
    SELECT 'Status Mismatch'::VARCHAR, 'venda_itens'::VARCHAR,
           vi.id,
           'Item ' || vi.id || ' is allocated but status is ' || vi.status::TEXT || ' (should be ESTOQUE)',
           'AVISO'::VARCHAR
    FROM venda_itens vi
    WHERE vi.status = 'PENDENTE'
      AND EXISTS (SELECT 1 FROM alocacoes WHERE venda_item_id = vi.id AND status = 'ATIVO');

    -- If no errors found, return OK
    IF NOT FOUND THEN
        RETURN QUERY SELECT 'System Check'::VARCHAR, 'N/A'::VARCHAR, 0::INTEGER,
                           'All integrity checks passed ✅'::TEXT, 'INFO'::VARCHAR;
    END IF;

END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION verificar_integridade_estoque() IS
'Verifica integridade completa do sistema de estoque.
Deve ser executada periodicamente (cron nightly).
SELECT verificar_integridade_estoque();';

-- Create an audit table to log verification results
CREATE TABLE integridade_log (
    id BIGSERIAL PRIMARY KEY,
    tipo VARCHAR(100),
    tabela VARCHAR(100),
    registro_id INTEGER,
    mensagem TEXT,
    severidade VARCHAR(50),
    verificado_em TIMESTAMPTZ DEFAULT NOW(),
    CONSTRAINT chk_severidade CHECK (severidade IN ('INFO', 'AVISO', 'CRÍTICA'))
);

CREATE INDEX idx_integridade_log_severidade ON integridade_log(severidade);
CREATE INDEX idx_integridade_log_data ON integridade_log(verificado_em);

-- Function to log verification results
CREATE OR REPLACE FUNCTION log_verificacao_integridade()
RETURNS void AS $$
DECLARE
    r RECORD;
BEGIN
    FOR r IN SELECT * FROM verificar_integridade_estoque() LOOP
        INSERT INTO integridade_log (tipo, tabela, registro_id, mensagem, severidade)
        VALUES (r.tipo, r.tabela, r.registro_id, r.mensagem, r.severidade);
    END LOOP;
END;
$$ LANGUAGE plpgsql;

-- Schedule this to run nightly (requires pg_cron extension):
-- SELECT cron.schedule('verificacao_estoque_nightly', '0 2 * * *', 'SELECT log_verificacao_integridade();');
```

---

### 4.6 Immutability Enforcement (Append-Only Tables)

Certas tabelas são **append-only** (insert-only). Nenhuma UPDATE/DELETE permitida. Isto garante:
- **Auditoria Imutável**: Histórico nunca pode ser adulterado
- **Conformidade Fiscal**: Registros fiscais (NFe) preservados intactos
- **Segurança**: Impossível sobrescrever logs de investigação

**Tabelas Imutáveis:**
1. `audit_log` - Registro de todas alterações
2. `estoque_movimentacoes` - Transações de entrada/saída
3. `integridade_log` - Resultado de verificações
4. `nfe_itens` - Dados fiscais (nunca deve mudar após emissão)
5. `entrega_itens` - O que foi entregue é um fato histórico

**Padrão de Correção**: Quando precisa-se corrigir algo em tabelas imutáveis, não atualizar o registro original. Ao invés disso:
1. Inserir novo registro com valores corrigidos
2. Marcar o original como "revertido" (via nova linha em tabela de reversão)
3. Mostrar cadeia completa no relatório (original + reversão)

```sql
-- ============================================================================
-- IMMUTABILITY ENFORCEMENT: Trigger para Bloquear UPDATE/DELETE
-- ============================================================================

CREATE OR REPLACE FUNCTION fn_prevent_mutation()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'UPDATE' THEN
        RAISE EXCEPTION 'Tabela % é imutável (append-only). Não é permitido UPDATE no registro %',
            TG_TABLE_NAME, NEW.id;
    END IF;

    IF TG_OP = 'DELETE' THEN
        RAISE EXCEPTION 'Tabela % é imutável (append-only). Não é permitido DELETE no registro %',
            TG_TABLE_NAME, OLD.id;
    END IF;

    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION fn_prevent_mutation() IS
'Bloqueia UPDATE e DELETE em tabelas imutáveis.
Garante que auditoria, movimentações fiscais e histórico nunca podem ser adulterados.
Se precisar corrigir algo, inserir novo registro com valores corretos.';

-- ============================================================================
-- Apply immutability to audit_log
-- ============================================================================
CREATE TRIGGER trg_audit_log_immutable
    BEFORE UPDATE OR DELETE ON audit_log
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();

-- ============================================================================
-- Apply immutability to estoque_movimentacoes
-- ============================================================================
-- Toda movimentação de estoque é uma transação. Histórico jamais deve mudar.
CREATE TRIGGER trg_estoque_movimentacoes_immutable
    BEFORE UPDATE OR DELETE ON estoque_movimentacoes
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();

-- ============================================================================
-- Apply immutability to integridade_log
-- ============================================================================
-- Resultado de verificações de integridade. Auditoria DBA nunca deve ser apagada.
CREATE TRIGGER trg_integridade_log_immutable
    BEFORE UPDATE OR DELETE ON integridade_log
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();

-- ============================================================================
-- Apply immutability to nfe_itens
-- ============================================================================
-- Dados fiscais. Nunca mudam após emissão. Exigência de conformidade.
CREATE TRIGGER trg_nfe_itens_immutable
    BEFORE UPDATE OR DELETE ON nfe_itens
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();

-- ============================================================================
-- Apply immutability to entrega_itens
-- ============================================================================
-- O que foi entregue é um fato histórico. Não deve ser reescrito.
CREATE TRIGGER trg_entrega_itens_immutable
    BEFORE UPDATE OR DELETE ON entrega_itens
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();
```

**Exemplos de Uso:**

```sql
-- ❌ ERRO: Tentar atualizar movimento
UPDATE estoque_movimentacoes
SET quantidade = 100
WHERE id = 5;
-- Error: Tabela estoque_movimentacoes é imutável (append-only).
--        Não é permitido UPDATE no registro 5

-- ❌ ERRO: Tentar deletar log de auditoria
DELETE FROM audit_log WHERE id = 42;
-- Error: Tabela audit_log é imutável (append-only).
--        Não é permitido DELETE no registro 42

-- ✅ CORRETO: Corrigir via novo registro
-- Se entrega_itens registrou 50 unidades mas deveria ser 55:
-- 1. Manter registro original intacto
-- 2. Criar movimento de ajuste
INSERT INTO estoque_movimentacoes (
    lote_id, tipo, quantidade, custo_unitario,
    referencia_tipo, referencia_id, usuario_id, observacoes
) VALUES (
    123, 'ENTRADA_AJUSTE', 5, 10.00,
    'entrega_item_correcao', 99, 1, 'Correção: entrega_item #99 registrada como 50 mas era 55'
);

-- 3. Adicionar nota em auditoria explicando a correção
INSERT INTO audit_log (
    tabela, registro_id, operacao, usuario_id, dados_anterior, dados_novo, motivo
) VALUES (
    'entrega_itens', 99, 'CORRECAO', 1,
    '{"quantidade": 50}', '{"quantidade": 55}',
    'Correção de quantidade entregue - ajuste via estoque_movimentacoes #9999'
);

-- 4. Relatório agora mostra:
--    - entrega_item #99: 50 un (registro original)
--    - estoque_movimentacoes #9999: +5 un (ajuste)
--    - audit_log: registro da correção com rastreabilidade completa
--    Total efetivo: 55 un ✓
```

**Benefícios:**

| Aspecto | Benefício |
|---------|-----------|
| **Auditoria** | Cada alteração deixa rastro imutável. Impossível cobrir trilhas. |
| **Conformidade** | NFe e movimentações fiscais nunca mudam (exigência legal). |
| **Investigação** | Se há problema, histórico completo está preservado. |
| **Recuperação** | Revertendo histórico é reversível. Deletar é não. |
| **Performance** | Sem UPDATE/DELETE, sem lock contention em tabelas críticas. |

```

---

## 5. Event Sourcing: Implementação Prática

Esta seção mostra como tabelas operacionais são convertidas ao padrão event sourcing com pg_ivm.

### 5.1 Exemplo 1: Vendas (venda_status PENDENTE → CONCLUÍDA)

**Events Table (Append-Only):**

```sql
CREATE TABLE vendas_events (
    event_id BIGSERIAL PRIMARY KEY,
    entidade_id INTEGER NOT NULL,          -- ID da venda
    tipo VARCHAR(100) NOT NULL,
    dados_anterior JSONB,
    dados_novo JSONB,
    mudancas_totais JSONB,                 -- {"status": {"de": "PENDENTE", "para": "CONCLUÍDA"}}
    usuario_id INTEGER REFERENCES usuarios(id),
    motivo TEXT,
    changed_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_venda_eventos CHECK (tipo IN (
        'VENDA_CRIADA',
        'STATUS_ALTERADO',
        'VALOR_ATUALIZADO',
        'CLIENTE_ALTERADO',
        'VENDA_CANCELADA'
    ))
);

-- Índices para performance
CREATE INDEX idx_vendas_events_entidade ON vendas_events(entidade_id, changed_at DESC);
CREATE INDEX idx_vendas_events_tipo ON vendas_events(tipo);
CREATE INDEX idx_vendas_events_data ON vendas_events(changed_at DESC);

-- Tabela imutável: bloqueia UPDATE/DELETE
CREATE TRIGGER trg_vendas_events_immutable
    BEFORE UPDATE OR DELETE ON vendas_events
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();
```

**Materialized View (Current State):**

```sql
-- Reconstrói estado atual da venda a partir dos últimos eventos
CREATE MATERIALIZED VIEW vendas AS
SELECT DISTINCT ON (v.entidade_id)
    v.entidade_id as id,
    (v.dados_novo ->> 'numero')::VARCHAR as numero,
    (v.dados_novo ->> 'cliente_id')::INTEGER as cliente_id,
    (v.dados_novo ->> 'loja_id')::INTEGER as loja_id,
    (v.dados_novo ->> 'data_venda')::DATE as data_venda,
    (v.dados_novo ->> 'valor_total')::DECIMAL(15,2) as valor_total,
    (v.dados_novo ->> 'status')::venda_status as status,
    v.changed_at,
    v.usuario_id
FROM vendas_events v
ORDER BY v.entidade_id, v.changed_at DESC;

-- Índices na view para queries rápidas
CREATE UNIQUE INDEX idx_vendas_id ON vendas(id);
CREATE INDEX idx_vendas_cliente ON vendas(cliente_id);
CREATE INDEX idx_vendas_status ON vendas(status);

-- pg_ivm mantém automaticamente
-- (Requer: CREATE EXTENSION pg_ivm; DECLARE MATERIALIZED VIEW vendas WITH NO DATA AS ...)
```

**Como Usar (Aplicação):**

```sql
-- ❌ ANTES (UPDATE direto):
UPDATE vendas SET status = 'CONCLUIDA' WHERE id = 5;

-- ✅ DEPOIS (INSERT evento):
INSERT INTO vendas_events (entidade_id, tipo, dados_anterior, dados_novo, usuario_id, motivo)
SELECT 5, 'STATUS_ALTERADO',
       jsonb_build_object('status', status),
       jsonb_build_object('status', 'CONCLUIDA'),
       1, 'Todos os itens entregues'
FROM vendas
WHERE id = 5;
-- pg_ivm atualiza vendas view automaticamente
-- SELECT * FROM vendas WHERE id = 5; ← retorna status='CONCLUIDA'
```

**Auditoria Completa (DBA):**

```sql
-- Ver histórico completo de uma venda
SELECT event_id, tipo, mudancas_totais, usuario_id, changed_at
FROM vendas_events
WHERE entidade_id = 5
ORDER BY changed_at;

-- Resultado:
-- event_id | tipo                | mudancas_totais                              | usuario_id | changed_at
-- 1001     | VENDA_CRIADA        | {"numero": {"de": null, "para": "V-001"}}  | 1          | 2025-01-10 10:00
-- 1002     | CLIENTE_ALTERADO    | {"cliente_id": {"de": 5, "para": 10}}      | 1          | 2025-01-10 10:15
-- 1003     | STATUS_ALTERADO     | {"status": {"de": "PENDENTE", "para": ...} | 2          | 2025-01-10 14:30
```

---

### 5.2 Exemplo 2: Estoque Lotes (Alocações Modificam Estado)

**Events Table:**

```sql
CREATE TABLE estoque_lotes_events (
    event_id BIGSERIAL PRIMARY KEY,
    entidade_id INTEGER NOT NULL,          -- ID do lote
    tipo VARCHAR(100) NOT NULL,
    dados_anterior JSONB,
    dados_novo JSONB,
    mudancas_totais JSONB,
    usuario_id INTEGER REFERENCES usuarios(id),
    motivo TEXT,
    referencia_tipo VARCHAR(50),           -- 'alocacao', 'entrega', 'ajuste'
    referencia_id INTEGER,
    changed_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_lote_eventos CHECK (tipo IN (
        'LOTE_CRIADO',
        'QUANTIDADE_ALTERADA',
        'STATUS_ALTERADO',
        'LOCALIZACAO_ALTERADA',
        'LOTE_BLOQUEADO'
    ))
);

CREATE INDEX idx_estoque_lotes_events_entidade ON estoque_lotes_events(entidade_id, changed_at DESC);
CREATE INDEX idx_estoque_lotes_events_referencia ON estoque_lotes_events(referencia_tipo, referencia_id);

CREATE TRIGGER trg_estoque_lotes_events_immutable
    BEFORE UPDATE OR DELETE ON estoque_lotes_events
    FOR EACH ROW EXECUTE FUNCTION fn_prevent_mutation();
```

**Materialized View:**

```sql
CREATE MATERIALIZED VIEW estoque_lotes AS
SELECT DISTINCT ON (l.entidade_id)
    l.entidade_id as id,
    (l.dados_novo ->> 'produto_id')::INTEGER as produto_id,
    (l.dados_novo ->> 'fornecedor_id')::INTEGER as fornecedor_id,
    (l.dados_novo ->> 'numero_lote')::VARCHAR as numero_lote,
    (l.dados_novo ->> 'quantidade_inicial')::DECIMAL(15,4) as quantidade_inicial,
    (l.dados_novo ->> 'quantidade_disponivel')::DECIMAL(15,4) as quantidade_disponivel,
    (l.dados_novo ->> 'quantidade_reservada')::DECIMAL(15,4) as quantidade_reservada,
    (l.dados_novo ->> 'status')::estoque_lote_status as status,
    l.changed_at,
    l.usuario_id
FROM estoque_lotes_events l
ORDER BY l.entidade_id, l.changed_at DESC;

CREATE UNIQUE INDEX idx_estoque_lotes_id ON estoque_lotes(id);
CREATE INDEX idx_estoque_lotes_produto ON estoque_lotes(produto_id);
CREATE INDEX idx_estoque_lotes_status ON estoque_lotes(status);
```

**Trigger para Alocação (agora INSERT evento, não UPDATE):**

```sql
CREATE OR REPLACE FUNCTION fn_alocacao_criada()
RETURNS TRIGGER AS $$
BEGIN
    -- Inserir evento de redução de disponível
    INSERT INTO estoque_lotes_events (
        entidade_id, tipo, dados_anterior, dados_novo,
        usuario_id, motivo, referencia_tipo, referencia_id
    ) SELECT
        NEW.lote_id,
        'QUANTIDADE_ALTERADA',
        jsonb_build_object(
            'quantidade_disponivel', el.quantidade_disponivel,
            'quantidade_reservada', el.quantidade_reservada
        ),
        jsonb_build_object(
            'quantidade_disponivel', el.quantidade_disponivel - NEW.quantidade,
            'quantidade_reservada', el.quantidade_reservada + NEW.quantidade,
            'status', CASE WHEN (el.quantidade_disponivel - NEW.quantidade) = 0 THEN 'RESERVADO' ELSE 'DISPONIVEL' END
        ),
        NEW.created_by,
        'Alocação de ' || NEW.quantidade || ' unidades',
        'alocacao',
        NEW.id
    FROM estoque_lotes el
    WHERE el.id = NEW.lote_id;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_alocacao_cria_evento
    AFTER INSERT ON alocacoes
    FOR EACH ROW EXECUTE FUNCTION fn_alocacao_criada();
```

---

### 5.3 Exemplo 3: Alocações (Status ATIVO → ESTORNADO)

**Events Table:**

```sql
CREATE TABLE alocacoes_events (
    event_id BIGSERIAL PRIMARY KEY,
    entidade_id INTEGER NOT NULL,          -- ID da alocação
    tipo VARCHAR(100) NOT NULL,
    dados_anterior JSONB,
    dados_novo JSONB,
    mudancas_totais JSONB,
    usuario_id INTEGER REFERENCES usuarios(id),
    motivo TEXT,
    changed_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_alocacao_eventos CHECK (tipo IN (
        'ALOCACAO_CRIADA',
        'STATUS_ALTERADO',
        'QUANTIDADE_AJUSTADA',
        'ALOCACAO_ESTORNADA'
    ))
);

CREATE INDEX idx_alocacoes_events_entidade ON alocacoes_events(entidade_id, changed_at DESC);
```

**Materialized View:**

```sql
CREATE MATERIALIZED VIEW alocacoes AS
SELECT DISTINCT ON (a.entidade_id)
    a.entidade_id as id,
    (a.dados_novo ->> 'venda_item_id')::INTEGER as venda_item_id,
    (a.dados_novo ->> 'lote_id')::INTEGER as lote_id,
    (a.dados_novo ->> 'quantidade')::DECIMAL(15,4) as quantidade,
    (a.dados_novo ->> 'custo_unitario')::DECIMAL(15,4) as custo_unitario,
    (a.dados_novo ->> 'status')::VARCHAR as status,
    a.changed_at,
    a.usuario_id
FROM alocacoes_events a
ORDER BY a.entidade_id, a.changed_at DESC;
```

**Estorno (INSERT evento, não UPDATE):**

```sql
-- ❌ ANTES:
UPDATE alocacoes SET status = 'TOTALMENTE_ESTORNADO' WHERE id = 99;

-- ✅ DEPOIS:
INSERT INTO alocacoes_events (entidade_id, tipo, dados_anterior, dados_novo, usuario_id, motivo)
SELECT 99, 'ALOCACAO_ESTORNADA',
       jsonb_build_object('status', 'ATIVO'),
       jsonb_build_object('status', 'TOTALMENTE_ESTORNADO'),
       1, 'Cliente rejeitou mercadoria - qualidade'
FROM alocacoes
WHERE id = 99;
```

---

### 5.4 Setup pg_ivm para Views Incrementais

**Instalação (uma única vez):**

```sql
-- Instalar extensão
CREATE EXTENSION pg_ivm;

-- Converter materialized views para incremental

-- Para vendas:
DROP MATERIALIZED VIEW vendas;

CREATE INCREMENTAL MATERIALIZED VIEW vendas AS
SELECT DISTINCT ON (v.entidade_id)
    v.entidade_id as id,
    (v.dados_novo ->> 'numero')::VARCHAR as numero,
    (v.dados_novo ->> 'cliente_id')::INTEGER as cliente_id,
    ...
FROM vendas_events v
ORDER BY v.entidade_id, v.changed_at DESC;

-- Para estoque_lotes:
DROP MATERIALIZED VIEW estoque_lotes;

CREATE INCREMENTAL MATERIALIZED VIEW estoque_lotes AS
SELECT DISTINCT ON (l.entidade_id)
    ...
FROM estoque_lotes_events l
ORDER BY l.entidade_id, l.changed_at DESC;

-- Para alocacoes:
DROP MATERIALIZED VIEW alocacoes;

CREATE INCREMENTAL MATERIALIZED VIEW alocacoes AS
SELECT DISTINCT ON (a.entidade_id)
    ...
FROM alocacoes_events a
ORDER BY a.entidade_id, a.changed_at DESC;

-- Pronto! pg_ivm cuidará de manter todas as views atualizadas
-- Não precisa de cron jobs ou REFRESH MATERIALIZED VIEW
```

---

## 6. Comparação: Legado vs Novo

| Aspecto | Legado | Novo |
|---------|--------|------|
| **Tabelas L1/L2** | venda_has_produto + venda_has_produto2 com idRelacionado | venda_itens única (sem splits) |
| **Referência fornecedor** | VARCHAR em ~9 tabelas | fornecedor_id FK em todo lugar |
| **Dados fiscais NFe** | ~30 colunas em estoque + estoque_has_consumo | JSONB em nfe_itens (imutável) |
| **Status** | Strings mágicas ("Em Estoque") | ENUMs PostgreSQL |
| **Alocação de Estoque** | FIFO automático (quebrado) | M:N: múltiplos lotes por venda_item + entrega_itens para parciais |
| **Auditoria** | Nenhuma | **Event Sourcing completo** via `*_events` tables |
| **Histórico** | Triggers complexos em UPDATE | INSERTs imutáveis (sem UPDATE) |
| **Views Atualizadas** | Nenhuma | **pg_ivm**: views incrementais em tempo real |
| **Tabela produto** | 100+ colunas | produtos + produto_precos + produto_tributos |
| **Devoluções** | Incompleto | alocacoes.status (ATIVO/PARCIALMENTE_ESTORNADO/TOTALMENTE_ESTORNADO) |
| **Financeiro** | contas_receber + parcelas_receber + contas_pagar + parcelas_pagar (4 tabelas) | financeiro_parcelas unificado (1 tabela + 2 views) |
| **Localização Estoque** | Sem suporte a split (1 lote = 1 bloco) | estoque_localizacoes (1 lote → múltiplos blocos) |
| **Imutabilidade** | Nenhuma | fn_prevent_mutation() em todas as tabelas críticas |
| **Recuperação Temporal** | Impossível ("como era em Jan?") | Trivial: replay eventos até data desejada |

---

## 6. Estatísticas do Schema (Event Sourcing Edition)

| Métrica | Quantidade |
|---------|------------|
| ENUMs | 16 |
| **Tabelas Operacionais** | 31 (estado atual) |
| **Tabelas Events** | 15+ (histórico imutável) |
| **Materialized Views** | 15+ (via pg_ivm incremental) |
| **Views (Auditoria)** | 7 (+ 3 verificação) |
| Triggers | 20+ (INSERT eventos, validação) |
| Funções PL/pgSQL | 12+ (incluindo fn_prevent_mutation) |
| Índices | ~70+ (events tables + views + verificação) |
| CHECK Constraints | ~40 |
| GENERATED Columns | 2 |
| **Imutáveis (fn_prevent_mutation)** | 5 tabelas críticas mínimo |

**Enforcement Mechanisms (5 Camadas)**:
- **CAMADA 1**: CHECK constraints na quantidade (~7)
- **CAMADA 2**: GENERATED columns (custo_total auto-calculado)
- **CAMADA 3**: Triggers INSERT em `*_events` (validação + evento)
- **CAMADA 4**: fn_prevent_mutation() (imutabilidade)
- **CAMADA 5**: Verificação periódica + integridade_log

**Nota sobre Event Sourcing**:
- **`*_events` tables**: Append-only, nunca UPDATE/DELETE. Imutáveis por trigger.
- **Materialized Views**: Reconstruem estado atual a partir dos eventos mais recentes.
- **pg_ivm**: Extension que mantém views atualizadas incrementalmente em tempo real.
- **Zero cron jobs**: Não é preciso `REFRESH MATERIALIZED VIEW` manual. Automático.
- **Recuperação Temporal**: Restaurar estado em qualquer data? Replay eventos até aquela data.
- **Compliance**: Dados fiscais, movimentações e auditoria jamais podem ser alterados.
- **Performance**: Append-only é otimizado para disco. Sem UPDATE contention.
- ENUM adicional: financeiro_tipo (RECEBER/PAGAR)
- Nova tabela: estoque_localizacoes (para split de paletes em múltiplos blocos)
- Adicionado: integridade_log para auditoria de verificações periódicas

---

## Documentos Relacionados

- [02-decisoes.md](./02-decisoes.md) - ADRs do projeto
- [03-melhorias.md](./03-melhorias.md) - Pontos de dor
- [07-esquema-redesenhado.md](./07-esquema-redesenhado.md) - Schema anterior (referência)
- [../brainstorming/schema-alternativo-2-entidades.md](../brainstorming/schema-alternativo-2-entidades.md) - Análise 2 vs 3 entidades
````
