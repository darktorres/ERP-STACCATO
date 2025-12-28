# Redesign do Schema de Banco de Dados

> Status: **Rascunho**
> Última atualização: 2025-12-28
> Alvo: PostgreSQL 16

---

## Escopo deste Documento

Este documento foca em **decisões técnicas de banco de dados**:

- Por que PostgreSQL (justificativa técnica)
- Análise dos problemas do schema atual
- Princípios e convenções de banco de dados
- Exemplos de correções propostas
- Busca full-text e indexação

**Para o schema completo redesenhado**, incluindo ENUMs, máquinas de estado e arquitetura de eventos, veja [estrategia/07-esquema-redesenhado.md](../estrategia/07-esquema-redesenhado.md).

---

## Por que PostgreSQL

| Recurso                 | Benefício para Este Projeto                                          |
| ----------------------- | -------------------------------------------------------------------- |
| **JSONB Nativo**        | Dados de impostos flexíveis, atributos de produto, logs de auditoria |
| **ENUM Nativo**         | Campos de status com type-safety                                     |
| **Restrições CHECK**    | Aplicação de regras de negócio no nível do BD                        |
| **Busca full-text**     | tsvector integrado para busca de produtos                            |
| **Melhor concorrência** | MVCC lida bem com usuários simultâneos                               |
| **Schemas**             | Opção de multi-tenancy (schema por loja)                             |
| **Particionamento**     | Particionamento de tabelas para tabelas de transação grandes         |

---

## Problemas Atuais do Schema

### 1. Nomes de Fornecedor Desnormalizados

Nomes de fornecedores armazenados como VARCHAR em múltiplas tabelas em vez de FK:

| Tabela                           | Coluna       |
| -------------------------------- | ------------ |
| `venda_has_produto2`             | `fornecedor` |
| `estoque`                        | `fornecedor` |
| `estoque_has_consumo`            | `fornecedor` |
| `compra_avulsa`                  | `fornecedor` |
| `pedido_fornecedor_has_produto2` | `fornecedor` |

**Impacto**: Se o nome do fornecedor mudar, requer atualização de 5+ tabelas.

**Correção**: Substituir por FK `fornecedor_id`.

---

### 2. Mega-Tabela: `produto`

A tabela `produto` tem **100+ colunas** incluindo:

- Dados principais do produto
- Múltiplas flags de rastreamento `*Upd` para cada campo
- Campos calculados (`estoqueRestante`)
- Valores históricos (`oldPrecoVenda`)
- Múltiplas flags booleanas espalhadas

**Correção**: Dividir em tabelas normalizadas:

```sql
-- Apenas dados principais do produto
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

-- Precificação versionada (histórico preservado)
CREATE TABLE produto_precos (
    id SERIAL PRIMARY KEY,
    produto_id INTEGER REFERENCES produtos(id) ON DELETE CASCADE,
    custo DECIMAL(15,2),
    preco_venda DECIMAL(15,2),
    margem DECIMAL(7,4),
    vigencia_inicio DATE NOT NULL DEFAULT CURRENT_DATE,
    vigencia_fim DATE,
    created_at TIMESTAMP DEFAULT NOW(),

    -- Apenas um preço ativo por produto de cada vez
    CONSTRAINT one_active_price EXCLUDE USING gist (
        produto_id WITH =,
        daterange(vigencia_inicio, vigencia_fim, '[]') WITH &&
    ) WHERE (vigencia_fim IS NOT NULL)
);

-- Atributos flexíveis (dimensões, cores, especificações)
CREATE TABLE produto_atributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id) ON DELETE CASCADE,
    atributos JSONB DEFAULT '{}'::jsonb,
    updated_at TIMESTAMP DEFAULT NOW()
);

-- View para obter preço atual
CREATE VIEW produto_preco_atual AS
SELECT DISTINCT ON (produto_id) *
FROM produto_precos
WHERE vigencia_inicio <= CURRENT_DATE
  AND (vigencia_fim IS NULL OR vigencia_fim >= CURRENT_DATE)
ORDER BY produto_id, vigencia_inicio DESC;
```

---

### 3. Tabelas de Detalhe em Dois Níveis

Padrão atual usa dois níveis:

- `venda_has_produto` (Nível 1 - agregado)
- `venda_has_produto2` (Nível 2 - detalhado)

**Problema**: Complexidade, problemas de sincronização.

**Correção**: Tabela única `venda_itens` com relacionamentos adequados:

```sql
CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,
    venda_id INTEGER REFERENCES vendas(id) ON DELETE CASCADE,
    produto_id INTEGER REFERENCES produtos(id),
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    quantidade DECIMAL(15,4) NOT NULL,
    preco_unitario DECIMAL(15,2) NOT NULL,
    desconto DECIMAL(7,4) DEFAULT 0,
    -- Desnormalizado para performance (capturado no momento da venda)
    descricao_produto VARCHAR(500),
    unidade VARCHAR(10),
    -- Rastreamento
    estoque_id INTEGER REFERENCES estoques(id), -- qual estoque foi consumido
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_venda_itens_venda ON venda_itens(venda_id);
CREATE INDEX idx_venda_itens_produto ON venda_itens(produto_id);
```

---

### 4. Explosão de Campos de Impostos

Atual: 35 campos inline para IBS/CBS/IS em `estoque` e `estoque_has_consumo`.

**Correção**: Tabela separada com JSONB:

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

-- Exemplo de estrutura JSONB para ICMS:
-- {
--   "cst": "00",
--   "orig": "0",
--   "vBC": 1000.00,
--   "pICMS": 18.00,
--   "vICMS": 180.00
-- }

-- Exemplo para IBS (Reforma Tributária):
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

### 5. Status como VARCHAR

Atual: Strings mágicas como `"PENDENTE"`, `"PEND. APROV."`, `"EM ENTREGA"`.

**Correção**: ENUMs do PostgreSQL:

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

-- Uso na tabela
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,
    status venda_status NOT NULL DEFAULT 'ORCAMENTO',
    -- ...
);
```

---

### 6. Sem Trilha de Auditoria

Atual: Algumas flags `*Upd` mas sem trilha de auditoria real.

**Correção**: Tabela de log de auditoria com triggers:

```sql
CREATE TABLE audit_log (
    id BIGSERIAL PRIMARY KEY,
    table_name VARCHAR(100) NOT NULL,
    record_id INTEGER NOT NULL,
    action VARCHAR(20) NOT NULL, -- INSERT, UPDATE, DELETE
    old_values JSONB,
    new_values JSONB,
    changed_fields TEXT[], -- lista de nomes de colunas alteradas
    user_id INTEGER REFERENCES usuarios(id),
    ip_address INET,
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_audit_log_table_record ON audit_log(table_name, record_id);
CREATE INDEX idx_audit_log_created ON audit_log(created_at);

-- Função genérica de trigger de auditoria
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

-- Aplicar às tabelas importantes
CREATE TRIGGER audit_vendas
    AFTER INSERT OR UPDATE OR DELETE ON vendas
    FOR EACH ROW EXECUTE FUNCTION audit_trigger_func();

CREATE TRIGGER audit_compras
    AFTER INSERT OR UPDATE OR DELETE ON compras
    FOR EACH ROW EXECUTE FUNCTION audit_trigger_func();
```

---

## Schema Principal Proposto

### Tabelas Mestras

```sql
-- Lojas (filiais)
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

-- Enderecos (polimórfico)
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

### Tabelas de Transação

```sql
-- Vendas
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id) NOT NULL,
    cliente_id INTEGER REFERENCES clientes(id),
    vendedor_id INTEGER REFERENCES usuarios(id),
    status venda_status NOT NULL DEFAULT 'ORCAMENTO',

    -- Totais (desnormalizados para performance)
    subtotal DECIMAL(15,2) DEFAULT 0,
    desconto DECIMAL(15,2) DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) DEFAULT 0,

    -- Datas
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
    venda_id INTEGER REFERENCES vendas(id), -- se vinculada a uma venda
    status compra_status NOT NULL DEFAULT 'PENDENTE',

    -- Totais
    subtotal DECIMAL(15,2) DEFAULT 0,
    frete DECIMAL(15,2) DEFAULT 0,
    total DECIMAL(15,2) DEFAULT 0,

    -- Datas previstas vs reais
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

### Tabelas de Estoque

```sql
-- Estoque (recebimentos/entradas de estoque)
CREATE TABLE estoques (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id) NOT NULL,
    compra_id INTEGER REFERENCES compras(id),
    produto_id INTEGER REFERENCES produtos(id) NOT NULL,
    fornecedor_id INTEGER REFERENCES fornecedores(id),

    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_disponivel DECIMAL(15,4) NOT NULL, -- disponível atual
    custo_unitario DECIMAL(15,2),

    -- Localização
    bloco_id INTEGER REFERENCES blocos(id), -- localização no armazém

    -- Datas
    data_entrada TIMESTAMP DEFAULT NOW(),
    validade DATE,

    observacoes TEXT,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_estoques_produto ON estoques(produto_id);
CREATE INDEX idx_estoques_disponivel ON estoques(produto_id, quantidade_disponivel)
    WHERE quantidade_disponivel > 0;

-- Consumos de estoque (rastreamento de consumo)
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

## Estratégia de Migração

### Fase 1: Novas Tabelas

Criar novas tabelas normalizadas junto às existentes.

### Fase 2: Escrita Dupla

Escrever em ambas tabelas antigas e novas durante a transição.

### Fase 3: Backfill

Migrar dados históricos das tabelas antigas para as novas.

### Fase 4: Mudar Leituras

Apontar leituras da aplicação para as novas tabelas.

### Fase 5: Limpeza

Remover tabelas antigas após validação.

---

## Busca Full-Text

```sql
-- Adicionar coluna de vetor de busca aos produtos
ALTER TABLE produtos ADD COLUMN search_vector tsvector;

CREATE INDEX idx_produtos_search ON produtos USING GIN(search_vector);

-- Trigger de atualização
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

-- Query de busca
SELECT * FROM produtos
WHERE search_vector @@ plainto_tsquery('portuguese', 'mesa escritorio')
ORDER BY ts_rank(search_vector, plainto_tsquery('portuguese', 'mesa escritorio')) DESC;
```

---

## Documentos Relacionados

- [../estrategia/07-esquema-redesenhado.md](../estrategia/07-esquema-redesenhado.md) - Schema completo redesenhado com todas as correções
- [../estrategia/04-simplificacao-l1l2.md](../estrategia/04-simplificacao-l1l2.md) - Detalhes da simplificação L1/L2
- [../estrategia/05-correcao-fifo.md](../estrategia/05-correcao-fifo.md) - Implementação FIFO
- [../estrategia/06-normalizacao-fornecedor.md](../estrategia/06-normalizacao-fornecedor.md) - Normalização de FK fornecedor
- [04-infraestrutura.md](./04-infraestrutura.md) - Auditoria, busca full-text, cache
