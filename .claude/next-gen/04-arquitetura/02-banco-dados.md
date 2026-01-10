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

**Para o schema PostgreSQL completo**, incluindo ENUMs, máquinas de estado e arquitetura de eventos com Event Sourcing, veja:
- [03-decisoes/02-schema-redesenhado.md](../03-decisoes/02-schema-redesenhado.md) - SQL completo
- [03-decisoes/02-schema-visual-overview.md](../03-decisoes/02-schema-visual-overview.md) - Visualizações e padrões

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

## Extensões PostgreSQL Recomendadas

### Extensões Essenciais

#### pg_trgm - Busca Fuzzy (Alta Prioridade)

Lida com erros de digitação na busca de produtos:

```sql
CREATE EXTENSION pg_trgm;

-- Índice para busca fuzzy
CREATE INDEX idx_produtos_nome_trgm
    ON produtos USING gin (descricao gin_trgm_ops);

-- Encontra produtos mesmo com typos
SELECT descricao, similarity(descricao, 'porcelanatu') as sim
FROM produtos
WHERE descricao % 'porcelanatu'  -- typo de "porcelanato"
ORDER BY sim DESC;

-- Combinado com full-text para melhores resultados
SELECT * FROM produtos
WHERE search_vector @@ to_tsquery('porcelanato')  -- exato
   OR descricao % 'porcelanatu';                   -- fuzzy
```

**Caso de uso**: Usuário digita "meza escritorio" → encontra "mesa escritório"

---

#### unaccent - Busca Sem Acentos (Alta Prioridade)

Crítico para português:

```sql
CREATE EXTENSION unaccent;

-- Wrapper imutável para índices
CREATE OR REPLACE FUNCTION imm_unaccent(text) RETURNS text AS $$
    SELECT unaccent($1)
$$ LANGUAGE sql IMMUTABLE;

-- Índice
CREATE INDEX idx_produtos_unaccent
    ON produtos (imm_unaccent(descricao));

-- Busca ignorando acentos
SELECT * FROM produtos
WHERE imm_unaccent(descricao) ILIKE imm_unaccent('%porcelanato%');
```

**Caso de uso**: Busca "ceramica" encontra "cerâmica"

---

#### pg_stat_statements - Monitoramento de Performance (Alta Prioridade)

Essencial para otimização:

```sql
CREATE EXTENSION pg_stat_statements;

-- Encontrar queries mais lentas
SELECT
    round(total_exec_time::numeric, 2) as total_ms,
    calls,
    round(mean_exec_time::numeric, 2) as avg_ms,
    query
FROM pg_stat_statements
ORDER BY total_exec_time DESC
LIMIT 20;

-- Encontrar queries mais chamadas
SELECT query, calls, mean_exec_time
FROM pg_stat_statements
ORDER BY calls DESC
LIMIT 20;
```

**Caso de uso**: Identificar queries que precisam de otimização

---

#### pgcrypto - Criptografia (Alta Prioridade)

Para dados sensíveis (conformidade LGPD):

```sql
CREATE EXTENSION pgcrypto;

-- Criptografar dados sensíveis
UPDATE clientes
SET cpf_encrypted = pgp_sym_encrypt(cpf, current_setting('app.encryption_key'));

-- Hash de senhas (se não usar Laravel)
UPDATE usuarios
SET password_hash = crypt('senha123', gen_salt('bf'));

-- Verificar senha
SELECT * FROM usuarios
WHERE password_hash = crypt('senha123', password_hash);
```

**Caso de uso**: Criptografar CPF/CNPJ em repouso para LGPD

---

### Extensões Recomendadas

#### uuid-ossp - UUIDs (Média Prioridade)

Para IDs públicos:

```sql
CREATE EXTENSION "uuid-ossp";

-- Usar UUIDs para recursos públicos (API, URLs)
CREATE TABLE vendas (
    id SERIAL PRIMARY KEY,              -- interno
    uuid UUID DEFAULT uuid_generate_v4(), -- público
    ...
);

CREATE UNIQUE INDEX idx_vendas_uuid ON vendas(uuid);

-- API usa UUID: /api/vendas/550e8400-e29b-41d4-a716-446655440000
-- Interno usa integer para JOINs (mais rápido)
```

**Caso de uso**: Esconder IDs sequenciais de clientes (segurança)

---

#### pg_cron - Jobs Agendados (Média Prioridade)

Executar jobs diretamente no PostgreSQL:

```sql
CREATE EXTENSION pg_cron;

-- Expirar orçamentos antigos diariamente à meia-noite
SELECT cron.schedule('expire-orcamentos', '0 0 * * *', $$
    UPDATE orcamentos
    SET status = 'EXPIRADO'
    WHERE status = 'ATIVO'
      AND data_emissao + (validade || ' days')::interval < CURRENT_DATE
$$);

-- Refresh de views materializadas a cada 15 minutos
SELECT cron.schedule('refresh-mv', '*/15 * * * *', $$
    REFRESH MATERIALIZED VIEW CONCURRENTLY mv_produto_estoque
$$);

-- Listar jobs agendados
SELECT * FROM cron.job;
```

**Caso de uso**: Backup ao Laravel scheduler - executar jobs críticos no BD

---

#### ltree - Dados Hierárquicos (Média Prioridade)

Para árvores de categorias ou organogramas:

```sql
CREATE EXTENSION ltree;

CREATE TABLE categorias (
    id SERIAL PRIMARY KEY,
    nome VARCHAR(100),
    path ltree  -- ex: 'revestimentos.pisos.porcelanato'
);

CREATE INDEX idx_categorias_path ON categorias USING gist(path);

-- Encontrar todas as subcategorias
SELECT * FROM categorias
WHERE path <@ 'revestimentos.pisos';

-- Encontrar ancestrais
SELECT * FROM categorias
WHERE path @> 'revestimentos.pisos.porcelanato';
```

**Caso de uso**: Categorias de produtos, hierarquia organizacional

---

### Extensões Opcionais

#### tablefunc - Tabelas Pivot (Baixa Prioridade)

Para relatórios com crosstab:

```sql
CREATE EXTENSION tablefunc;

-- Pivot de vendas por mês
SELECT * FROM crosstab(
    'SELECT produto_id,
            to_char(created_at, ''YYYY-MM'') as mes,
            SUM(quantidade)
     FROM venda_itens
     GROUP BY 1, 2
     ORDER BY 1, 2'
) AS ct(produto_id int, "2024-01" numeric, "2024-02" numeric);
```

**Caso de uso**: Relatórios mensais de vendas, análise comparativa

---

#### PostGIS - Geolocalização (Baixa Prioridade)

Para roteamento de entregas:

```sql
CREATE EXTENSION postgis;

ALTER TABLE clientes ADD COLUMN localizacao GEOGRAPHY(POINT);

-- Encontrar clientes em raio de 50km do depósito
SELECT nome, ST_Distance(localizacao, deposito_loc) as distancia
FROM clientes
WHERE ST_DWithin(localizacao, deposito_loc, 50000);

-- Otimizar rotas de entrega
SELECT * FROM clientes
ORDER BY localizacao <-> ST_MakePoint(-46.6339, -23.5507);
```

**Caso de uso**: Planejamento de zonas de entrega, otimização de rotas

---

#### pg_partman - Particionamento de Tabelas (Baixa Prioridade)

Para tabelas grandes (audit_log, eventos):

```sql
-- Particionar audit_log por mês
CREATE TABLE audit_log (
    id BIGSERIAL,
    created_at TIMESTAMPTZ NOT NULL,
    ...
) PARTITION BY RANGE (created_at);

-- Auto-criar partições
SELECT partman.create_parent(
    p_parent_table := 'public.audit_log',
    p_control := 'created_at',
    p_interval := '1 month',
    p_premake := 3  -- criar 3 meses à frente
);
```

**Caso de uso**: Manter audit_log performante conforme cresce

---

### Resumo de Extensões

| Extensão               | Caso de Uso                        | Prioridade | Complexidade |
| ---------------------- | ---------------------------------- | ---------- | ------------ |
| **pg_trgm**            | Busca fuzzy de produtos            | Alta       | Baixa        |
| **unaccent**           | Acentos em português               | Alta       | Baixa        |
| **pg_stat_statements** | Monitoramento de performance       | Alta       | Baixa        |
| **pgcrypto**           | Criptografar CPF/CNPJ (LGPD)       | Alta       | Média        |
| **uuid-ossp**          | IDs públicos                       | Média      | Baixa        |
| **pg_cron**            | Backup de jobs agendados           | Média      | Baixa        |
| **ltree**              | Hierarquia de categorias           | Média      | Média        |
| **tablefunc**          | Relatórios pivot                   | Baixa      | Média        |
| **PostGIS**            | Roteamento de entregas             | Baixa      | Alta         |
| **pg_partman**         | Particionamento de tabelas grandes | Baixa      | Média        |

---

### Script de Setup Inicial

```sql
-- Extensões essenciais para o ERP
CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE EXTENSION IF NOT EXISTS unaccent;
CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS pg_stat_statements;
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS pg_ivm;

-- Opcional baseado em necessidade
-- CREATE EXTENSION IF NOT EXISTS ltree;
-- CREATE EXTENSION IF NOT EXISTS pg_cron;
-- CREATE EXTENSION IF NOT EXISTS postgis;
```

---

## Recursos Nativos do PostgreSQL

### Tipos de Dados Avançados

#### Arrays

```sql
-- Armazenar múltiplos valores em uma coluna
CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,
    descricao VARCHAR(500),
    tags TEXT[],                    -- array de tags
    imagens VARCHAR(255)[],         -- array de URLs de imagem
    cores_disponiveis VARCHAR(50)[] -- cores disponíveis
);

-- Insert
INSERT INTO produtos (descricao, tags, cores_disponiveis)
VALUES ('Porcelanato 60x60', ARRAY['piso', 'interno', 'polido'], ARRAY['branco', 'bege', 'cinza']);

-- Query: encontrar produtos com tag específica
SELECT * FROM produtos WHERE 'polido' = ANY(tags);

-- Query: encontrar produtos com TODAS estas tags
SELECT * FROM produtos WHERE tags @> ARRAY['piso', 'interno'];

-- Índice para queries de array
CREATE INDEX idx_produtos_tags ON produtos USING GIN(tags);
```

**Caso de uso**: Tags de produto, cores disponíveis, telefones múltiplos

---

#### Range Types

```sql
-- Ranges de data para vigência de preços
CREATE TABLE produto_precos (
    id SERIAL PRIMARY KEY,
    produto_id INTEGER,
    valor_venda DECIMAL(15,2),
    vigencia DATERANGE NOT NULL,  -- [inicio, fim)

    -- Prevenir períodos de preço sobrepostos
    EXCLUDE USING gist (produto_id WITH =, vigencia WITH &&)
);

-- Insert preço válido de 1 Jan a 31 Mar
INSERT INTO produto_precos (produto_id, valor_venda, vigencia)
VALUES (1, 45.00, '[2025-01-01, 2025-04-01)');

-- Encontrar preço válido em data específica
SELECT * FROM produto_precos
WHERE produto_id = 1 AND vigencia @> '2025-02-15'::date;

-- Timestamp ranges para reservas
CREATE TABLE agendamentos (
    id SERIAL PRIMARY KEY,
    recurso_id INTEGER,
    periodo TSTZRANGE,

    -- Sem reserva dupla
    EXCLUDE USING gist (recurso_id WITH =, periodo WITH &&)
);
```

**Caso de uso**: Vigência de preços, agendamentos, reservas

---

#### INTERVAL

```sql
-- Cálculos de expiração
SELECT
    id,
    data_emissao,
    data_emissao + (validade || ' days')::INTERVAL as data_expiracao
FROM orcamentos;

-- Encontrar orçamentos expirando nos próximos 7 dias
SELECT * FROM orcamentos
WHERE status = 'ATIVO'
  AND data_emissao + (validade || ' days')::INTERVAL
      BETWEEN NOW() AND NOW() + INTERVAL '7 days';

-- Cálculo de idade
SELECT
    nome,
    data_nascimento,
    AGE(data_nascimento) as idade
FROM clientes;
```

---

### Colunas Geradas (Computed)

```sql
CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,
    quantidade DECIMAL(15,4) NOT NULL,
    valor_unitario DECIMAL(15,4) NOT NULL,
    desconto_percentual DECIMAL(5,2) DEFAULT 0,

    -- Colunas computadas (stored = persistido em disco)
    valor_desconto DECIMAL(15,2)
        GENERATED ALWAYS AS (quantidade * valor_unitario * desconto_percentual / 100) STORED,
    valor_total DECIMAL(15,2)
        GENERATED ALWAYS AS (quantidade * valor_unitario * (1 - desconto_percentual/100)) STORED
);

-- Não precisa calcular na aplicação - sempre correto
INSERT INTO venda_itens (quantidade, valor_unitario, desconto_percentual)
VALUES (10, 45.00, 5);

SELECT * FROM venda_itens;
-- valor_desconto = 22.50, valor_total = 427.50 (auto-calculado)
```

**Caso de uso**: Totais de linha, cálculos fiscais, margens

---

### Window Functions

```sql
-- Totais acumulados
SELECT
    id,
    data,
    valor,
    SUM(valor) OVER (ORDER BY data) as saldo_acumulado
FROM movimentacoes
WHERE conta_id = 1;

-- Ranking de produtos por vendas
SELECT
    produto_id,
    SUM(quantidade) as total_vendido,
    RANK() OVER (ORDER BY SUM(quantidade) DESC) as ranking
FROM venda_itens
GROUP BY produto_id;

-- Valores anteriores/próximos (para comparações)
SELECT
    mes,
    vendas,
    LAG(vendas) OVER (ORDER BY mes) as vendas_mes_anterior,
    vendas - LAG(vendas) OVER (ORDER BY mes) as variacao
FROM vendas_mensais;

-- Partição por categoria
SELECT
    categoria,
    produto,
    vendas,
    vendas * 100.0 / SUM(vendas) OVER (PARTITION BY categoria) as percentual_categoria
FROM produtos_vendas;
```

**Caso de uso**: Relatórios comparativos, rankings, saldos acumulados

---

### UPSERT (ON CONFLICT)

```sql
-- Insert ou update em um statement
INSERT INTO produto_precos (produto_id, custo, valor_venda)
VALUES (123, 30.00, 45.00)
ON CONFLICT (produto_id)
DO UPDATE SET
    custo = EXCLUDED.custo,
    valor_venda = EXCLUDED.valor_venda,
    updated_at = NOW();

-- Insert ou ignorar
INSERT INTO importacao_log (chave_nfe, status)
VALUES ('12345678901234567890123456789012345678901234', 'PROCESSADO')
ON CONFLICT (chave_nfe) DO NOTHING;

-- Update condicional
INSERT INTO estoque_diario (produto_id, data, quantidade)
VALUES (1, CURRENT_DATE, 100)
ON CONFLICT (produto_id, data)
DO UPDATE SET quantidade = estoque_diario.quantidade + EXCLUDED.quantidade;
```

**Caso de uso**: Sync de dados, idempotência, contadores

---

### RETURNING Clause

```sql
-- Obter ID inserido sem query separada
INSERT INTO vendas (cliente_id, vendedor_id, status)
VALUES (1, 2, 'ORCAMENTO')
RETURNING id, created_at;

-- Obter todas as linhas afetadas após update
UPDATE venda_itens
SET status = 'CANCELADO'
WHERE venda_id = 123
RETURNING id, produto_id, quantidade;

-- Encadear com CTE
WITH inserted AS (
    INSERT INTO vendas (cliente_id, total)
    VALUES (1, 1000.00)
    RETURNING id
)
INSERT INTO venda_parcelas (venda_id, valor, vencimento)
SELECT id, 500.00, CURRENT_DATE + n * 30
FROM inserted, generate_series(1, 2) as n;
```

**Caso de uso**: Evitar round-trips, operações atômicas

---

### CTEs (Common Table Expressions)

```sql
-- CTE recursivo para árvore de categorias
WITH RECURSIVE categoria_tree AS (
    -- Base: categorias de topo
    SELECT id, nome, parent_id, 0 as nivel, nome::text as path
    FROM categorias
    WHERE parent_id IS NULL

    UNION ALL

    -- Recursivo: filhos
    SELECT c.id, c.nome, c.parent_id, ct.nivel + 1, ct.path || ' > ' || c.nome
    FROM categorias c
    JOIN categoria_tree ct ON c.parent_id = ct.id
)
SELECT * FROM categoria_tree ORDER BY path;

-- Múltiplos CTEs para relatórios complexos
WITH
vendas_mes AS (
    SELECT produto_id, SUM(quantidade) as qtd
    FROM venda_itens
    WHERE created_at >= DATE_TRUNC('month', CURRENT_DATE)
    GROUP BY produto_id
),
estoque_atual AS (
    SELECT produto_id, SUM(quantidade_disponivel) as qtd
    FROM estoques
    GROUP BY produto_id
)
SELECT
    p.descricao,
    COALESCE(v.qtd, 0) as vendas_mes,
    COALESCE(e.qtd, 0) as estoque,
    COALESCE(e.qtd, 0) / NULLIF(COALESCE(v.qtd, 1), 0) as meses_estoque
FROM produtos p
LEFT JOIN vendas_mes v ON p.id = v.produto_id
LEFT JOIN estoque_atual e ON p.id = e.produto_id;
```

**Caso de uso**: Hierarquias, relatórios complexos, legibilidade

---

### Row-Level Security (RLS)

```sql
-- Habilitar RLS na tabela
ALTER TABLE vendas ENABLE ROW LEVEL SECURITY;

-- Policy: usuários só veem vendas da sua loja
CREATE POLICY vendas_loja_policy ON vendas
    FOR ALL
    USING (loja_id = current_setting('app.current_loja_id')::integer);

-- Policy: admins veem tudo
CREATE POLICY vendas_admin_policy ON vendas
    FOR ALL
    TO admin_role
    USING (true);

-- Setar contexto na aplicação
SET app.current_loja_id = '1';
SELECT * FROM vendas;  -- Só vê loja_id = 1
```

**Caso de uso**: Multi-tenancy, segurança por loja

---

### LISTEN/NOTIFY (Real-time)

```sql
-- Na aplicação: escutar eventos
LISTEN new_order;

-- Trigger para notificar em novos pedidos
CREATE OR REPLACE FUNCTION notify_new_order() RETURNS trigger AS $$
BEGIN
    PERFORM pg_notify('new_order', json_build_object(
        'id', NEW.id,
        'cliente_id', NEW.cliente_id,
        'total', NEW.total
    )::text);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER vendas_notify
    AFTER INSERT ON vendas
    FOR EACH ROW EXECUTE FUNCTION notify_new_order();
```

**Integração Laravel:**

```php
// Usando Laravel com pg_notify
DB::listen('new_order', function ($notification) {
    $data = json_decode($notification->payload);
    broadcast(new NewOrderEvent($data));
});
```

**Caso de uso**: Updates real-time, trigger de websockets

---

### Partial Indexes

```sql
-- Indexar apenas registros ativos (menor, mais rápido)
CREATE INDEX idx_orcamentos_ativos
    ON orcamentos(cliente_id, created_at)
    WHERE status = 'ATIVO';

-- Indexar apenas estoque disponível
CREATE INDEX idx_estoque_disponivel
    ON estoques(produto_id, loja_id)
    WHERE quantidade_disponivel > 0;

-- Indexar apenas NFe pendentes
CREATE INDEX idx_nfe_pendentes
    ON nfes(created_at)
    WHERE status IN ('PENDENTE', 'PROCESSANDO');

-- 90% das queries usam esses índices parciais
-- Muito menores que índices de tabela inteira
```

**Caso de uso**: Performance em queries frequentes

---

### DISTINCT ON

```sql
-- Obter último preço por produto (específico do PostgreSQL)
SELECT DISTINCT ON (produto_id)
    produto_id,
    valor_venda,
    created_at
FROM produto_precos
ORDER BY produto_id, created_at DESC;

-- Obter primeiro pedido por cliente
SELECT DISTINCT ON (cliente_id)
    cliente_id,
    id as primeira_venda_id,
    total,
    created_at
FROM vendas
ORDER BY cliente_id, created_at ASC;
```

**Caso de uso**: Primeiro/último por grupo sem subquery

---

### FILTER Clause

```sql
-- Agregados condicionais em uma query
SELECT
    COUNT(*) as total,
    COUNT(*) FILTER (WHERE status = 'ATIVO') as ativos,
    COUNT(*) FILTER (WHERE status = 'CANCELADO') as cancelados,
    SUM(total) FILTER (WHERE status = 'CONCLUIDO') as valor_concluido,
    AVG(total) FILTER (WHERE created_at >= CURRENT_DATE - 30) as media_30_dias
FROM vendas;

-- Pivot-like sem crosstab
SELECT
    produto_id,
    SUM(quantidade) FILTER (WHERE EXTRACT(MONTH FROM created_at) = 1) as jan,
    SUM(quantidade) FILTER (WHERE EXTRACT(MONTH FROM created_at) = 2) as fev,
    SUM(quantidade) FILTER (WHERE EXTRACT(MONTH FROM created_at) = 3) as mar
FROM venda_itens
WHERE EXTRACT(YEAR FROM created_at) = 2025
GROUP BY produto_id;
```

**Caso de uso**: Dashboards, relatórios pivot simples

---

### Particionamento de Tabelas (Nativo)

```sql
-- Particionar por range (data)
CREATE TABLE audit_log (
    id BIGSERIAL,
    created_at TIMESTAMPTZ NOT NULL,
    table_name VARCHAR(100),
    action VARCHAR(20),
    old_data JSONB,
    new_data JSONB
) PARTITION BY RANGE (created_at);

-- Criar partições
CREATE TABLE audit_log_2025_01 PARTITION OF audit_log
    FOR VALUES FROM ('2025-01-01') TO ('2025-02-01');
CREATE TABLE audit_log_2025_02 PARTITION OF audit_log
    FOR VALUES FROM ('2025-02-01') TO ('2025-03-01');

-- Particionar por lista (loja)
CREATE TABLE vendas (
    id SERIAL,
    loja_id INTEGER NOT NULL,
    ...
) PARTITION BY LIST (loja_id);

CREATE TABLE vendas_loja_1 PARTITION OF vendas FOR VALUES IN (1);
CREATE TABLE vendas_loja_2 PARTITION OF vendas FOR VALUES IN (2);
```

**Caso de uso**: Tabelas grandes, arquivamento, multi-tenancy

---

### Advisory Locks

```sql
-- Prevenir processamento concorrente do mesmo pedido
SELECT pg_advisory_lock(hashtext('process_order_' || order_id::text));

-- Fazer trabalho...

SELECT pg_advisory_unlock(hashtext('process_order_' || order_id::text));

-- Try lock (não bloqueante)
SELECT pg_try_advisory_lock(12345);

-- Session-level vs transaction-level
SELECT pg_advisory_xact_lock(12345);  -- Liberado em commit/rollback
```

**Integração Laravel:**

```php
DB::select("SELECT pg_advisory_lock(?)", [crc32("nfe_emit_{$nfeId}")]);
try {
    // Processar NFe
} finally {
    DB::select("SELECT pg_advisory_unlock(?)", [crc32("nfe_emit_{$nfeId}")]);
}
```

**Caso de uso**: Prevenir processamento duplicado, rate limiting

---

### Resumo de Recursos Nativos

| Recurso                | Caso de Uso                       | Complexidade |
| ---------------------- | --------------------------------- | ------------ |
| **Arrays**             | Tags, múltiplos valores           | Baixa        |
| **Range Types**        | Vigência de preços, agendamentos  | Média        |
| **Generated Columns**  | Totais calculados, margens        | Baixa        |
| **Window Functions**   | Rankings, saldos, comparativos    | Média        |
| **UPSERT**             | Sync, idempotência                | Baixa        |
| **RETURNING**          | Evitar round-trips                | Baixa        |
| **CTEs**               | Hierarquias, queries complexas    | Média        |
| **Row-Level Security** | Multi-tenancy por loja            | Média        |
| **LISTEN/NOTIFY**      | Real-time updates                 | Média        |
| **Partial Indexes**    | Performance em queries frequentes | Baixa        |
| **DISTINCT ON**        | Primeiro/último por grupo         | Baixa        |
| **FILTER**             | Agregados condicionais            | Baixa        |
| **Partitioning**       | Tabelas grandes                   | Alta         |
| **Advisory Locks**     | Concorrência, duplicatas          | Média        |

---

## Documentos Relacionados

- [../estrategia/07-esquema-redesenhado.md](../estrategia/07-esquema-redesenhado.md) - Schema completo redesenhado com todas as correções
- [../estrategia/04-simplificacao-l1l2.md](../estrategia/04-simplificacao-l1l2.md) - Detalhes da simplificação L1/L2
- [../estrategia/05-correcao-fifo.md](../estrategia/05-correcao-fifo.md) - Implementação FIFO
- [../estrategia/06-normalizacao-fornecedor.md](../estrategia/06-normalizacao-fornecedor.md) - Normalização de FK fornecedor
- [04-infraestrutura.md](./04-infraestrutura.md) - Auditoria, busca full-text, cache
