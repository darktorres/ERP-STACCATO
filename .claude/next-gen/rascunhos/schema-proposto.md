# Schema Proposto: ERP Staccato v2

> **Status**: Proposta
> **Data**: 2025-12-28
> **Base**: Pesquisa de ERPs (SAP, Odoo, ERPNext) + decisões arquiteturais

---

## 1. Princípios de Design

### 1.1 Lições da Indústria

| ERP | Modelo | Insight Chave |
|-----|--------|---------------|
| **SAP MM** | PO → Material Document → Stock | Documento de movimento entre pedido e estoque |
| **Odoo** | purchase.order → stock.picking → stock.move → stock.quant | `quant` = estado atual O(log n), não calcula de histórico |
| **ERPNext** | Stock Ledger Entry | Toda movimentação é um registro |

**Conclusão**: Nenhum ERP maduro funde "pedido" com "inventário". Sempre há camada de movimentação.

### 1.2 Decisões Adotadas

| Decisão | Escolha | Justificativa |
|---------|---------|---------------|
| Modelo de entidades | **3 entidades** | Validado pela indústria |
| Dados fiscais NFe | **JSONB** | Flexibilidade para reforma tributária |
| Status | **ENUMs PostgreSQL** | Type-safety, transições validadas |
| Auditoria | **CRUD + audit_log** | Event Sourcing rejeitado para v1 |
| Consumo estoque | **Seleção manual 1:1** | Variação de lote (tom/calibre) |

---

## 2. Visão Geral do Schema

```
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
```

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

-- Financeiro
CREATE TYPE financeiro_status AS ENUM (
    'PENDENTE',       -- Aguardando
    'AGENDADO',       -- CNAB gerado
    'PAGO',           -- Pago (contas a pagar)
    'RECEBIDO',       -- Recebido (contas a receber)
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
```

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
```

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

    -- Hierarquia de split (para entregas parciais, etc)
    parent_id INTEGER REFERENCES venda_itens(id),
    root_id INTEGER REFERENCES venda_itens(id),
    split_reason VARCHAR(50),  -- 'NFE_PARCIAL', 'ENTREGA_PARCIAL', 'DEVOLUCAO'

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

    -- Status
    status venda_item_status NOT NULL DEFAULT 'PENDENTE',

    -- Datas previstas/reais
    data_prev_entrega DATE,
    data_real_entrega TIMESTAMPTZ,

    -- Quem recebeu
    recebido_por VARCHAR(100),

    -- Observações
    observacoes TEXT,

    -- Links
    orcamento_item_id INTEGER REFERENCES orcamento_itens(id),
    compra_item_id INTEGER REFERENCES compra_itens(id),
    nfe_saida_item_id INTEGER,  -- FK para nfe_itens (saída)

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_venda_item_quantidade CHECK (quantidade > 0),
    CONSTRAINT chk_venda_item_valor CHECK (valor_unitario >= 0)
);

CREATE INDEX idx_venda_itens_venda ON venda_itens(venda_id);
CREATE INDEX idx_venda_itens_produto ON venda_itens(produto_id);
CREATE INDEX idx_venda_itens_status ON venda_itens(status);
CREATE INDEX idx_venda_itens_root ON venda_itens(root_id) WHERE root_id IS NOT NULL;

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

    -- Hierarquia de split
    parent_id INTEGER REFERENCES compra_itens(id),
    root_id INTEGER REFERENCES compra_itens(id),
    split_reason VARCHAR(50),

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

    -- Origem
    venda_item_id INTEGER REFERENCES venda_itens(id),

    -- NFe
    nfe_entrada_item_id INTEGER,  -- FK para nfe_itens

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),

    CONSTRAINT chk_compra_item_quantidade CHECK (quantidade > 0)
);

CREATE INDEX idx_compra_itens_compra ON compra_itens(compra_id);
CREATE INDEX idx_compra_itens_venda_item ON compra_itens(venda_item_id) WHERE venda_item_id IS NOT NULL;
```

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
```

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

    -- Localização
    bloco_id INTEGER REFERENCES galpao_blocos(id),

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
-- ALOCAÇÕES (link 1:1 entre venda_item e lote)
-- ============================================================================
CREATE TABLE alocacoes (
    id SERIAL PRIMARY KEY,

    -- Links 1:1
    venda_item_id INTEGER NOT NULL REFERENCES venda_itens(id),
    lote_id INTEGER NOT NULL REFERENCES estoque_lotes(id),

    -- Quantidade e custo (snapshot)
    quantidade DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,
    custo_total DECIMAL(15,2) GENERATED ALWAYS AS (quantidade * custo_unitario) STORED,

    -- Estorno
    is_estornado BOOLEAN DEFAULT FALSE,
    estornado_em TIMESTAMPTZ,
    estorno_motivo VARCHAR(200),
    estornado_por INTEGER REFERENCES usuarios(id),

    -- Auditoria
    created_at TIMESTAMPTZ DEFAULT NOW(),
    created_by INTEGER REFERENCES usuarios(id)
);

-- Constraint 1:1: um venda_item só pode ter uma alocação ativa
CREATE UNIQUE INDEX idx_alocacoes_venda_item_ativo ON alocacoes(venda_item_id) WHERE NOT is_estornado;

-- Constraint 1:1: um lote só pode ser alocado uma vez (por completo)
CREATE UNIQUE INDEX idx_alocacoes_lote_ativo ON alocacoes(lote_id) WHERE NOT is_estornado;

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
```

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
```

### 3.7 Financeiro

```sql
-- ============================================================================
-- CONTAS A RECEBER
-- ============================================================================
CREATE TABLE contas_receber (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    cliente_id INTEGER NOT NULL REFERENCES clientes(id),
    venda_id INTEGER REFERENCES vendas(id),

    -- Identificação
    documento VARCHAR(50),  -- Número do documento

    -- Valor total
    valor_total DECIMAL(15,2) NOT NULL,

    -- Observações
    observacoes TEXT,

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE parcelas_receber (
    id SERIAL PRIMARY KEY,

    conta_id INTEGER NOT NULL REFERENCES contas_receber(id) ON DELETE CASCADE,

    -- Parcela
    numero_parcela SMALLINT NOT NULL DEFAULT 1,
    total_parcelas SMALLINT NOT NULL DEFAULT 1,

    -- Valores
    valor DECIMAL(15,2) NOT NULL,
    valor_recebido DECIMAL(15,2) DEFAULT 0,
    valor_juros DECIMAL(15,2) DEFAULT 0,
    valor_multa DECIMAL(15,2) DEFAULT 0,
    valor_desconto DECIMAL(15,2) DEFAULT 0,

    -- Datas
    data_vencimento DATE NOT NULL,
    data_recebimento DATE,

    -- Forma de pagamento
    forma_pagamento forma_pagamento,

    -- Status
    status financeiro_status NOT NULL DEFAULT 'PENDENTE',

    -- CNAB
    nosso_numero VARCHAR(50),
    linha_digitavel VARCHAR(100),
    codigo_barras VARCHAR(50),
    remessa_id INTEGER,  -- FK para remessas_cnab

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_parcelas_receber_conta ON parcelas_receber(conta_id);
CREATE INDEX idx_parcelas_receber_vencimento ON parcelas_receber(data_vencimento);
CREATE INDEX idx_parcelas_receber_status ON parcelas_receber(status);

-- ============================================================================
-- CONTAS A PAGAR
-- ============================================================================
CREATE TABLE contas_pagar (
    id SERIAL PRIMARY KEY,

    loja_id INTEGER NOT NULL REFERENCES lojas(id),
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),
    compra_id INTEGER REFERENCES compras(id),
    nfe_entrada_id INTEGER REFERENCES nfes(id),

    -- Identificação
    documento VARCHAR(50),

    -- Valor total
    valor_total DECIMAL(15,2) NOT NULL,

    -- Observações
    observacoes TEXT,

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE parcelas_pagar (
    id SERIAL PRIMARY KEY,

    conta_id INTEGER NOT NULL REFERENCES contas_pagar(id) ON DELETE CASCADE,

    -- Parcela
    numero_parcela SMALLINT NOT NULL DEFAULT 1,
    total_parcelas SMALLINT NOT NULL DEFAULT 1,

    -- Valores
    valor DECIMAL(15,2) NOT NULL,
    valor_pago DECIMAL(15,2) DEFAULT 0,
    valor_juros DECIMAL(15,2) DEFAULT 0,
    valor_multa DECIMAL(15,2) DEFAULT 0,
    valor_desconto DECIMAL(15,2) DEFAULT 0,

    -- Datas
    data_vencimento DATE NOT NULL,
    data_pagamento DATE,

    -- Forma de pagamento
    forma_pagamento forma_pagamento,

    -- Status
    status financeiro_status NOT NULL DEFAULT 'PENDENTE',

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_parcelas_pagar_conta ON parcelas_pagar(conta_id);
CREATE INDEX idx_parcelas_pagar_vencimento ON parcelas_pagar(data_vencimento);
CREATE INDEX idx_parcelas_pagar_status ON parcelas_pagar(status);

-- ============================================================================
-- CNAB
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
```

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
```

---

## 4. Triggers de Integridade

### 4.1 Validar Alocação

```sql
CREATE OR REPLACE FUNCTION fn_validar_alocacao()
RETURNS TRIGGER AS $$
DECLARE
    v_qtd_item DECIMAL(15,4);
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

    -- REGRA 1: Quantidade deve ser igual
    IF NEW.quantidade != v_qtd_item THEN
        RAISE EXCEPTION 'Quantidade da alocação (%) deve ser igual ao item (%)',
            NEW.quantidade, v_qtd_item;
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
```

### 4.2 Atualizar Estoque Após Alocação

```sql
CREATE OR REPLACE FUNCTION fn_apos_alocacao()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' AND NOT NEW.is_estornado THEN
        -- Alocação: diminuir disponível, aumentar reservado
        UPDATE estoque_lotes
        SET quantidade_disponivel = quantidade_disponivel - NEW.quantidade,
            quantidade_reservada = quantidade_reservada + NEW.quantidade,
            status = CASE
                WHEN quantidade_disponivel - NEW.quantidade = 0 THEN 'RESERVADO'::estoque_lote_status
                ELSE status
            END,
            updated_at = NOW()
        WHERE id = NEW.lote_id;

        -- Atualizar status do item
        UPDATE venda_itens
        SET status = 'ESTOQUE',
            updated_at = NOW()
        WHERE id = NEW.venda_item_id;

        -- Registrar movimentação
        INSERT INTO estoque_movimentacoes (lote_id, tipo, quantidade, custo_unitario, referencia_tipo, referencia_id, usuario_id)
        VALUES (NEW.lote_id, 'SAIDA_VENDA', -NEW.quantidade, NEW.custo_unitario, 'venda_item', NEW.venda_item_id, NEW.created_by);

    ELSIF TG_OP = 'UPDATE' AND NEW.is_estornado AND NOT OLD.is_estornado THEN
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
```

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
```

---

## 5. Comparação: Legado vs Novo

| Aspecto | Legado | Novo |
|---------|--------|------|
| **Tabelas L1/L2** | venda_has_produto + venda_has_produto2 com idRelacionado | venda_itens única com parent_id/root_id |
| **Referência fornecedor** | VARCHAR em ~9 tabelas | fornecedor_id FK em todo lugar |
| **Dados fiscais NFe** | ~30 colunas em estoque + estoque_has_consumo | JSONB em nfe_itens |
| **Status** | Strings mágicas ("Em Estoque") | ENUMs PostgreSQL |
| **Consumo estoque** | FIFO automático (quebrado) | Seleção manual 1:1 + movimentações |
| **Auditoria** | Nenhuma | audit_log + triggers |
| **Tabela produto** | 100+ colunas | produtos + produto_precos + produto_tributos |
| **Devoluções** | Incompleto | Fluxo completo com NFe tipo DEVOLUCAO |

---

## 6. Estatísticas do Schema

| Métrica | Quantidade |
|---------|------------|
| ENUMs | 15 |
| Tabelas | 32 |
| Views | 2 |
| Triggers | 6 |
| Índices | ~40 |
| Constraints | ~30 |

---

## Documentos Relacionados

- [02-decisoes.md](./02-decisoes.md) - ADRs do projeto
- [03-melhorias.md](./03-melhorias.md) - Pontos de dor
- [07-esquema-redesenhado.md](./07-esquema-redesenhado.md) - Schema anterior (referência)
- [../brainstorming/schema-alternativo-2-entidades.md](../brainstorming/schema-alternativo-2-entidades.md) - Análise 2 vs 3 entidades
