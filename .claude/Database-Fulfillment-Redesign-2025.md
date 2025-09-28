# ERP Database Redesign: Solving the Sale Fulfillment Problem

## Executive Summary

This document addresses a critical database design flaw in the current ERP system: the problematic `venda_has_produto` / `venda_has_produto2` structure used to track sale fulfillment. The current approach duplicates data, creates confusion, and violates fundamental database design principles while attempting to solve a legitimate business requirement.

**Current Problem:**
- Single sale line items need to be fulfilled from multiple sources (stock, purchase orders, etc.)
- Current solution creates duplicate records in `venda_has_produto2` table
- Results in data redundancy, integrity issues, and maintenance complexity

**Proposed Solution:**
- Separate "what was sold" from "how it's fulfilled" using proper domain modeling
- Implement clean fulfillment tracking with dedicated tables
- Maintain referential integrity and eliminate data duplication

## Current System Analysis

### The Business Requirement

The system needs to handle this common scenario:

```
Customer orders: 100 units of Product X
Available sources:
- 30 units in current stock
- 40 units from Purchase Order #123 (arriving next week)
- 30 units from Purchase Order #124 (arriving in 2 weeks)

Result: One sale line item fulfilled from three different sources
```

### Current Database Structure Problems

#### **Table: `venda_has_produto` (Original Sale Line Items)**
```sql
CREATE TABLE `venda_has_produto` (
  `idVendaProduto1` INT(11) NOT NULL AUTO_INCREMENT,
  `idVenda` VARCHAR(30) NOT NULL,
  `idProduto` INT(10) UNSIGNED NOT NULL,
  `produto` VARCHAR(250) NULL,
  `quant` DECIMAL(15,4) NOT NULL,        -- Original quantity ordered
  `prcUnitario` DECIMAL(15,4) NOT NULL,
  `total` DECIMAL(15,4) NOT NULL,
  `fornecedor` VARCHAR(100) NOT NULL,
  -- ... 40+ more columns including dates, status, etc.
);
```

#### **Table: `venda_has_produto2` (Split Fulfillment Records)**
```sql
CREATE TABLE `venda_has_produto2` (
  `idVendaProduto2` INT(11) NOT NULL AUTO_INCREMENT,
  `idVendaProdutoFK` INT(11) NOT NULL,   -- References venda_has_produto
  `idVenda` VARCHAR(30) NOT NULL,        -- DUPLICATE from parent
  `idProduto` INT(10) UNSIGNED NOT NULL, -- DUPLICATE from parent
  `produto` VARCHAR(250) NULL,           -- DUPLICATE from parent
  `quant` DECIMAL(15,4) NOT NULL,        -- Split quantity for this fulfillment
  `prcUnitario` DECIMAL(15,4) NOT NULL,  -- DUPLICATE from parent
  `total` DECIMAL(15,4) NOT NULL,        -- Calculated from split quantity
  `fornecedor` VARCHAR(100) NOT NULL,    -- DUPLICATE from parent
  -- ... 40+ more columns, mostly duplicated
);
```

#### **Critical Issues Identified:**

1. **Massive Data Duplication**
   - 90% of columns are identical between both tables
   - Product information duplicated in every split record
   - Sale information duplicated unnecessarily

2. **Data Integrity Risks**
   - No constraints ensuring split quantities sum to original quantity
   - Price changes in parent record don't propagate to splits
   - Status inconsistencies between parent and child records

3. **Maintenance Complexity**
   - Changes require updates to multiple records
   - Complex queries to get complete sale information
   - Business logic scattered across both tables

4. **Confusing Domain Model**
   - `produto` vs `produto2` naming is unintuitive
   - Relationship between tables unclear to developers
   - Mixed concerns in single table structure

5. **Query Performance Issues**
   - Joins required for basic sale information
   - Aggregations needed to reconstruct original line items
   - Index complexity due to denormalized structure

### Evidence from Current Codebase

The `copy_into_venda_has_produto2` stored procedure reveals the flawed approach:

```sql
CREATE PROCEDURE `copy_into_venda_has_produto2`(currentIdVendaProduto INT(11))
BEGIN
    INSERT INTO venda_has_produto2 (
        idVendaProdutoFK, idRelacionado, selecionado, entregou, recebeu,
        status, statusOriginal, idCompra, idNFeSaida, idNFeFutura,
        fornecedor, idVenda, idLoja, produto, obs, lote, prcUnitario,
        -- ... copying 40+ fields ...
    ) SELECT
        idVendaProduto1, idRelacionado, selecionado, entregou, recebeu,
        status, statusOriginal, idCompra, idNFeSaida, idNFeFutura,
        fornecedor, idVenda, idLoja, produto, obs, lote, prcUnitario,
        -- ... from venda_has_produto ...
    FROM venda_has_produto WHERE idVendaProduto1 = currentIdVendaProduto;
END
```

This procedure literally copies almost all data from the parent record, proving the design flaw.

## Proposed Database Redesign

### Core Principle: Separate Concerns

The redesigned schema separates three distinct concepts:
1. **Sale Line Items** - What was sold to the customer
2. **Fulfillment Reservations** - How each line item will be fulfilled
3. **Fulfillment Completions** - How each line item was actually fulfilled

### New Schema Design

#### **1. Tabela Vendas (Registro Principal)**
```sql
CREATE TABLE vendas (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    numero_venda VARCHAR(20) UNIQUE NOT NULL,
    id_empresa UUID NOT NULL REFERENCES empresas(id),
    id_cliente UUID NOT NULL REFERENCES clientes(id),
    id_vendedor UUID NOT NULL REFERENCES usuarios(id),

    -- Totais financeiros
    subtotal DECIMAL(15,4) NOT NULL,
    desconto DECIMAL(15,4) DEFAULT 0,
    custo_frete DECIMAL(15,4) DEFAULT 0,
    valor_impostos DECIMAL(15,4) DEFAULT 0,
    total DECIMAL(15,4) NOT NULL,

    -- Gestão da venda
    status ENUM('rascunho', 'confirmado', 'processando', 'atendido', 'entregue', 'cancelado') DEFAULT 'rascunho',
    data_prevista_entrega DATE,
    observacoes TEXT,

    -- Campos de auditoria
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    criado_por UUID NOT NULL REFERENCES usuarios(id),

    INDEX idx_vendas_cliente (id_cliente),
    INDEX idx_vendas_status (status),
    INDEX idx_vendas_data (criado_em),
    INDEX idx_vendas_numero (numero_venda)
);
```

#### **2. Itens da Venda (O Que Foi Vendido)**
```sql
CREATE TABLE itens_venda (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_venda UUID NOT NULL REFERENCES vendas(id) ON DELETE CASCADE,
    numero_linha INT NOT NULL, -- Para ordem de exibição

    -- Informações do produto
    id_produto UUID NOT NULL REFERENCES produtos(id),
    codigo_produto VARCHAR(50) NOT NULL, -- Snapshot no momento da venda
    nome_produto VARCHAR(200) NOT NULL, -- Snapshot no momento da venda
    id_fornecedor UUID NOT NULL REFERENCES fornecedores(id),

    -- Quantidade e preços
    quantidade_pedida DECIMAL(15,4) NOT NULL,
    unidade_medida VARCHAR(10) NOT NULL,
    preco_unitario DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4), -- Para cálculos de margem

    -- Descontos e totais
    desconto_percentual DECIMAL(5,2) DEFAULT 0,
    desconto_valor DECIMAL(15,4) DEFAULT 0,
    subtotal_linha DECIMAL(15,4) NOT NULL, -- quantidade * preco_unitario
    total_linha DECIMAL(15,4) NOT NULL, -- subtotal - desconto

    -- Status do atendimento
    quantidade_reservada DECIMAL(15,4) DEFAULT 0,
    quantidade_alocada DECIMAL(15,4) DEFAULT 0,
    quantidade_enviada DECIMAL(15,4) DEFAULT 0,
    quantidade_entregue DECIMAL(15,4) DEFAULT 0,
    quantidade_cancelada DECIMAL(15,4) DEFAULT 0,

    -- Restrições de lógica de negócio
    CONSTRAINT chk_quantidades CHECK (
        quantidade_reservada <= quantidade_pedida AND
        quantidade_alocada <= quantidade_reservada AND
        quantidade_enviada <= quantidade_alocada AND
        quantidade_entregue <= quantidade_enviada AND
        quantidade_cancelada <= quantidade_pedida
    ),

    -- Metadados
    observacoes TEXT,
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    UNIQUE KEY unique_linha (id_venda, numero_linha),
    INDEX idx_itens_venda (id_venda),
    INDEX idx_itens_produto (id_produto),
    INDEX idx_itens_fornecedor (id_fornecedor)
);
```

#### **3. Origens de Atendimento (Como Será Atendido)**
```sql
CREATE TABLE origens_atendimento (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_item_venda UUID NOT NULL REFERENCES itens_venda(id) ON DELETE CASCADE,

    -- Informações da origem
    tipo_origem ENUM('estoque', 'pedido_compra', 'transferencia', 'producao') NOT NULL,
    id_origem UUID NOT NULL, -- Referencia lotes_estoque, pedidos_compra, etc.
    referencia_origem VARCHAR(50), -- Referência legível (número PO, número lote, etc.)

    -- Alocação de quantidade
    quantidade_alocada DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4), -- Custo real desta origem

    -- Rastreamento de status
    status ENUM('planejado', 'reservado', 'alocado', 'atendido', 'cancelado') DEFAULT 'planejado',
    prioridade INT DEFAULT 1, -- Para prioridade de atendimento

    -- Cronograma
    data_prevista_disponibilidade DATE,
    alocado_em TIMESTAMP NULL,
    atendido_em TIMESTAMP NULL,

    -- Metadados
    observacoes TEXT,
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    criado_por UUID NOT NULL REFERENCES usuarios(id),

    INDEX idx_atendimento_item_venda (id_item_venda),
    INDEX idx_atendimento_origem (tipo_origem, id_origem),
    INDEX idx_atendimento_status (status)
);
```

#### **4. Conclusões de Atendimento (Como Foi Realmente Atendido)**
```sql
CREATE TABLE conclusoes_atendimento (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_origem_atendimento UUID NOT NULL REFERENCES origens_atendimento(id),
    id_item_venda UUID NOT NULL REFERENCES itens_venda(id),

    -- Detalhes da conclusão
    quantidade_atendida DECIMAL(15,4) NOT NULL,
    custo_unitario_real DECIMAL(15,4),
    numero_lote VARCHAR(50),
    numeros_serie JSON, -- Para produtos serializados

    -- Qualidade e condição
    codigo_condicao ENUM('novo', 'recondicionado', 'avariado', 'devolvido') DEFAULT 'novo',
    grau_qualidade ENUM('A', 'B', 'C') DEFAULT 'A',

    -- Cronograma e rastreamento
    atendido_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    enviado_em TIMESTAMP NULL,
    entregue_em TIMESTAMP NULL,

    -- Referências a documentos
    id_nfe UUID REFERENCES nfe(id),
    id_remessa UUID REFERENCES remessas(id),

    -- Metadados
    observacoes TEXT,
    atendido_por UUID NOT NULL REFERENCES usuarios(id),

    INDEX idx_conclusao_origem (id_origem_atendimento),
    INDEX idx_conclusao_item_venda (id_item_venda),
    INDEX idx_conclusao_data (atendido_em)
);
```

#### **5. Tabelas de Apoio para Solução Completa**

```sql
-- Lotes de estoque para atendimento via estoque
CREATE TABLE lotes_estoque (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_produto UUID NOT NULL REFERENCES produtos(id),
    id_deposito UUID NOT NULL REFERENCES depositos(id),
    numero_lote VARCHAR(50) NOT NULL,

    -- Quantidades
    quantidade_recebida DECIMAL(15,4) NOT NULL,
    quantidade_disponivel DECIMAL(15,4) NOT NULL,
    quantidade_reservada DECIMAL(15,4) DEFAULT 0,
    quantidade_alocada DECIMAL(15,4) DEFAULT 0,

    -- Custos
    custo_unitario DECIMAL(15,4) NOT NULL,
    custo_total DECIMAL(15,4) GENERATED ALWAYS AS (quantidade_recebida * custo_unitario),

    -- Qualidade e cronograma
    data_recebimento DATE NOT NULL,
    data_validade DATE,
    status_qualidade ENUM('bom', 'avariado', 'vencido', 'quarentena') DEFAULT 'bom',

    UNIQUE KEY unique_lote (id_produto, id_deposito, numero_lote),
    INDEX idx_lote_produto (id_produto),
    INDEX idx_lote_deposito (id_deposito),
    INDEX idx_lote_disponivel (quantidade_disponivel)
);

-- Pedidos de compra para atendimento via compras
CREATE TABLE pedidos_compra (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    numero_pedido VARCHAR(20) UNIQUE NOT NULL,
    id_fornecedor UUID NOT NULL REFERENCES fornecedores(id),

    -- Status e cronograma
    status ENUM('rascunho', 'enviado', 'confirmado', 'parcial', 'completo', 'cancelado') DEFAULT 'rascunho',
    data_pedido DATE NOT NULL,
    data_prevista_entrega DATE,

    -- Totais
    subtotal DECIMAL(15,4) NOT NULL,
    valor_impostos DECIMAL(15,4) DEFAULT 0,
    custo_frete DECIMAL(15,4) DEFAULT 0,
    total DECIMAL(15,4) NOT NULL,

    INDEX idx_pc_fornecedor (id_fornecedor),
    INDEX idx_pc_status (status),
    INDEX idx_pc_data (data_pedido)
);

CREATE TABLE itens_pedido_compra (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_pedido_compra UUID NOT NULL REFERENCES pedidos_compra(id) ON DELETE CASCADE,
    numero_linha INT NOT NULL,

    id_produto UUID NOT NULL REFERENCES produtos(id),
    quantidade_pedida DECIMAL(15,4) NOT NULL,
    quantidade_recebida DECIMAL(15,4) DEFAULT 0,
    custo_unitario DECIMAL(15,4) NOT NULL,
    total_linha DECIMAL(15,4) NOT NULL,

    data_prevista_entrega DATE,

    UNIQUE KEY unique_linha_pc (id_pedido_compra, numero_linha),
    INDEX idx_itens_pc_produto (id_produto)
);
```

### Implementação da Lógica de Negócio

#### **1. Algoritmo de Planejamento de Atendimento**

```typescript
interface PlanoAtendimento {
  idItemVenda: string;
  quantidadeTotal: number;
  origens: OrigemAtendimento[];
}

interface OrigemAtendimento {
  tipoOrigem: 'estoque' | 'pedido_compra' | 'transferencia' | 'producao';
  idOrigem: string;
  quantidadeAlocada: number;
  custoUnitario: number;
  dataPrevista: Date;
  prioridade: number;
}

class PlanificadorAtendimento {
  async planejarAtendimento(idItemVenda: string): Promise<PlanoAtendimento> {
    const itemVenda = await this.obterItemVenda(idItemVenda);
    const origensDisponiveis = await this.obterOrigensDisponiveis(itemVenda.idProduto);

    let quantidadeRestante = itemVenda.quantidadePedida;
    const origens: OrigemAtendimento[] = [];

    // Prioridade 1: Estoque existente
    for (const lote of origensDisponiveis.estoque) {
      if (quantidadeRestante <= 0) break;

      const quantidadeAlocar = Math.min(quantidadeRestante, lote.quantidadeDisponivel);
      if (quantidadeAlocar > 0) {
        origens.push({
          tipoOrigem: 'estoque',
          idOrigem: lote.id,
          quantidadeAlocada: quantidadeAlocar,
          custoUnitario: lote.custoUnitario,
          dataPrevista: new Date(), // Disponível agora
          prioridade: 1
        });
        quantidadeRestante -= quantidadeAlocar;
      }
    }

    // Prioridade 2: Pedidos de compra confirmados
    for (const itemPC of origensDisponiveis.pedidosCompra) {
      if (quantidadeRestante <= 0) break;

      const quantidadeDisponivel = itemPC.quantidadePedida - itemPC.quantidadeAlocada;
      const quantidadeAlocar = Math.min(quantidadeRestante, quantidadeDisponivel);

      if (quantidadeAlocar > 0) {
        origens.push({
          tipoOrigem: 'pedido_compra',
          idOrigem: itemPC.id,
          quantidadeAlocada: quantidadeAlocar,
          custoUnitario: itemPC.custoUnitario,
          dataPrevista: itemPC.dataPrevistaEntrega,
          prioridade: 2
        });
        quantidadeRestante -= quantidadeAlocar;
      }
    }

    // Prioridade 3: Criar novo pedido de compra se necessário
    if (quantidadeRestante > 0) {
      const novoPC = await this.criarPedidoCompra(itemVenda.idProduto, quantidadeRestante);
      origens.push({
        tipoOrigem: 'pedido_compra',
        idOrigem: novoPC.id,
        quantidadeAlocada: quantidadeRestante,
        custoUnitario: novoPC.custoUnitario,
        dataPrevista: novoPC.dataPrevistaEntrega,
        prioridade: 3
      });
    }

    return {
      idItemVenda,
      quantidadeTotal: itemVenda.quantidadePedida,
      origens
    };
  }
}
```

#### **2. Fulfillment Execution**

```typescript
class ExecutorAtendimento {
  async executarAtendimento(idOrigemAtendimento: string, quantidadeReal: number): Promise<void> {
    const origem = await this.obterOrigemAtendimento(idOrigemAtendimento);
    const itemVenda = await this.obterItemVenda(origem.idItemVenda);

    // Criar registro de conclusão
    const conclusao = await this.criarConclusao({
      idOrigemAtendimento,
      idItemVenda: origem.idItemVenda,
      quantidadeAtendida: quantidadeReal,
      custoUnitarioReal: origem.custoUnitario,
      atendidoEm: new Date()
    });

    // Atualizar status da origem
    await this.atualizarOrigemAtendimento(idOrigemAtendimento, {
      status: 'atendido',
      atendidoEm: new Date()
    });

    // Atualizar quantidades do item de venda
    await this.atualizarQuantidadesItemVenda(origem.idItemVenda);

    // Atualizar estoque se origem for estoque
    if (origem.tipoOrigem === 'estoque') {
      await this.atualizarLoteEstoque(origem.idOrigem, -quantidadeReal);
    }

    // Verificar se item de venda foi completamente atendido
    const itemVendaAtualizado = await this.obterItemVenda(origem.idItemVenda);
    if (itemVendaAtualizado.quantidadeEntregue >= itemVendaAtualizado.quantidadePedida) {
      await this.atualizarStatusItemVenda(origem.idItemVenda, 'atendido');
    }

    // Verificar se toda a venda foi atendida
    await this.verificarStatusAtendimentoVenda(itemVenda.idVenda);
  }
}
```

### Query Examples

#### **1. Get Complete Sale Information**
```sql
-- Get sale with all line items and their fulfillment status
SELECT
    v.numero_venda,
    v.total AS total_venda,
    v.status AS status_venda,

    iv.numero_linha,
    iv.nome_produto,
    iv.quantidade_pedida,
    iv.quantidade_entregue,
    iv.total_linha,

    CASE
        WHEN iv.quantidade_entregue >= iv.quantidade_pedida THEN 'Atendido'
        WHEN iv.quantidade_reservada > 0 THEN 'Em Andamento'
        ELSE 'Pendente'
    END AS status_linha

FROM vendas v
JOIN itens_venda iv ON v.id = iv.id_venda
WHERE v.numero_venda = 'VEN-2025-001'
ORDER BY iv.numero_linha;
```

#### **2. Get Fulfillment Details for Line Item**
```sql
-- Get detailed fulfillment plan and progress for a line item
SELECT
    iv.nome_produto,
    iv.quantidade_pedida,

    oa.tipo_origem,
    oa.referencia_origem,
    oa.quantidade_alocada,
    oa.status AS status_origem,
    oa.data_prevista_disponibilidade,

    COALESCE(ca.quantidade_atendida, 0) AS quantidade_atendida,
    ca.atendido_em

FROM itens_venda iv
LEFT JOIN origens_atendimento oa ON iv.id = oa.id_item_venda
LEFT JOIN conclusoes_atendimento ca ON oa.id = ca.id_origem_atendimento
WHERE iv.id = ?
ORDER BY oa.prioridade, oa.data_prevista_disponibilidade;
```

#### **3. Inventory Availability Check**
```sql
-- Verificar estoque disponível para planejamento de atendimento
SELECT
    p.sku,
    p.nome,
    SUM(le.quantidade_disponivel) AS total_disponivel,
    COUNT(le.id) AS contagem_lotes,
    MIN(le.custo_unitario) AS custo_minimo,
    MAX(le.custo_unitario) AS custo_maximo,
    AVG(le.custo_unitario) AS custo_medio

FROM produtos p
LEFT JOIN lotes_estoque le ON p.id = le.id_produto
    AND le.status_qualidade = 'bom'
    AND (le.data_validade IS NULL OR le.data_validade > CURRENT_DATE)
WHERE p.id = ?
GROUP BY p.id, p.sku, p.nome;
```

### Data Integrity and Business Rules

#### **1. Quantity Validation Triggers**

```sql
-- Trigger para validar quantidades de itens de venda
DELIMITER $$
CREATE TRIGGER validar_quantidades_itens_venda
BEFORE UPDATE ON itens_venda
FOR EACH ROW
BEGIN
    -- Garantir que quantidades não excedam quantidade pedida
    IF NEW.quantidade_reservada > NEW.quantidade_pedida THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Quantidade reservada não pode exceder quantidade pedida';
    END IF;

    IF NEW.quantidade_alocada > NEW.quantidade_reservada THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Quantidade alocada não pode exceder quantidade reservada';
    END IF;

    IF NEW.quantidade_enviada > NEW.quantidade_alocada THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Quantidade enviada não pode exceder quantidade alocada';
    END IF;

    IF NEW.quantidade_entregue > NEW.quantidade_enviada THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Quantidade entregue não pode exceder quantidade enviada';
    END IF;
END$$
DELIMITER ;

-- Trigger para validar alocações de origem de atendimento
DELIMITER $$
CREATE TRIGGER validar_alocacao_origem_atendimento
BEFORE INSERT ON origens_atendimento
FOR EACH ROW
BEGIN
    DECLARE total_alocado DECIMAL(15,4);
    DECLARE quantidade_item_venda DECIMAL(15,4);

    -- Obter total atual alocado e quantidade do item de venda
    SELECT
        COALESCE(SUM(oa.quantidade_alocada), 0),
        iv.quantidade_pedida
    INTO total_alocado, quantidade_item_venda
    FROM itens_venda iv
    LEFT JOIN origens_atendimento oa ON iv.id = oa.id_item_venda
    WHERE iv.id = NEW.id_item_venda;

    -- Verificar se nova alocação excederia quantidade do item de venda
    IF (total_alocado + NEW.quantidade_alocada) > quantidade_item_venda THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Quantidade total alocada não pode exceder quantidade do item de venda';
    END IF;
END$$
DELIMITER ;
```

#### **2. Automated Status Updates**

```sql
-- Procedimento para atualizar status do item de venda baseado em conclusões de atendimento
DELIMITER $$
CREATE PROCEDURE atualizar_status_item_venda(IN id_item_venda_param UUID)
BEGIN
    DECLARE total_atendido DECIMAL(15,4);
    DECLARE total_pedido DECIMAL(15,4);

    -- Calcular quantidade total atendida
    SELECT
        COALESCE(SUM(ca.quantidade_atendida), 0),
        iv.quantidade_pedida
    INTO total_atendido, total_pedido
    FROM itens_venda iv
    LEFT JOIN origens_atendimento oa ON iv.id = oa.id_item_venda
    LEFT JOIN conclusoes_atendimento ca ON oa.id = ca.id_origem_atendimento
    WHERE iv.id = id_item_venda_param;

    -- Atualizar quantidades do item de venda
    UPDATE itens_venda
    SET
        quantidade_entregue = total_atendido,
        atualizado_em = CURRENT_TIMESTAMP
    WHERE id = id_item_venda_param;

END$$
DELIMITER ;
```

## Migration Strategy from Current System

### Phase 1: Analysis and Mapping (2-3 weeks)

#### **1. Data Analysis**
```sql
-- Analyze current data patterns
SELECT
    COUNT(*) AS total_produto1_records,
    COUNT(DISTINCT idVenda) AS unique_sales,
    AVG(records_per_sale) AS avg_items_per_sale
FROM (
    SELECT idVenda, COUNT(*) AS records_per_sale
    FROM venda_has_produto
    GROUP BY idVenda
) stats;

-- Analyze split patterns
SELECT
    vp1.idVendaProduto1,
    vp1.quant AS original_quantity,
    COUNT(vp2.idVendaProduto2) AS split_count,
    SUM(vp2.quant) AS total_split_quantity,
    CASE
        WHEN ABS(vp1.quant - SUM(vp2.quant)) < 0.0001 THEN 'Balanced'
        WHEN vp1.quant > SUM(vp2.quant) THEN 'Under-allocated'
        ELSE 'Over-allocated'
    END AS allocation_status
FROM venda_has_produto vp1
LEFT JOIN venda_has_produto2 vp2 ON vp1.idVendaProduto1 = vp2.idVendaProdutoFK
GROUP BY vp1.idVendaProduto1, vp1.quant
HAVING COUNT(vp2.idVendaProduto2) > 0;
```

#### **2. Mapping Rules**
```sql
-- Create mapping table for migration
CREATE TABLE migration_mapping (
    old_venda_id VARCHAR(30),
    old_produto1_id INT(11),
    old_produto2_ids JSON, -- Array of venda_has_produto2 IDs
    new_sale_id UUID,
    new_line_item_id UUID,
    new_fulfillment_source_ids JSON, -- Array of new fulfillment source IDs
    migration_status ENUM('pending', 'in_progress', 'completed', 'failed') DEFAULT 'pending',
    migration_notes TEXT,
    migrated_at TIMESTAMP NULL
);
```

### Phase 2: New Schema Creation (1-2 weeks)

#### **1. Create New Tables**
```sql
-- Create all new tables with proper constraints and indexes
-- (Use the schema definitions provided above)

-- Create migration-specific tables
CREATE TABLE auditoria_migracao (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    nome_tabela VARCHAR(50) NOT NULL,
    id_registro_antigo VARCHAR(50) NOT NULL,
    id_registro_novo UUID,
    tipo_migracao ENUM('venda', 'item_venda', 'origem_atendimento', 'conclusao'),
    status_migracao ENUM('sucesso', 'falhou', 'ignorado'),
    mensagem_erro TEXT,
    migrado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Phase 3: Data Migration (3-4 weeks)

#### **1. Migration Scripts**

```sql
-- Passo 1: Migrar vendas
INSERT INTO vendas (
    id, numero_venda, id_cliente, id_vendedor,
    subtotal, total, status, criado_em, criado_por
)
SELECT
    UUID() AS id,
    v.idVenda AS numero_venda,
    obter_uuid_cliente(v.idCliente) AS id_cliente,
    obter_uuid_usuario(v.idUsuario) AS id_vendedor,
    v.subTotalBru AS subtotal,
    v.total AS total,
    mapear_status_venda(v.status) AS status,
    v.created AS criado_em,
    obter_uuid_usuario(v.idUsuario) AS criado_por
FROM venda v
WHERE v.created > '2024-01-01'; -- Migrar dados recentes primeiro

-- Passo 2: Migrar itens de venda (de venda_has_produto)
INSERT INTO itens_venda (
    id, id_venda, numero_linha, id_produto, sku_produto, nome_produto,
    quantidade_pedida, preco_unitario, total_linha, id_fornecedor, criado_em
)
SELECT
    UUID() AS id,
    s.id AS id_venda,
    ROW_NUMBER() OVER (PARTITION BY vp.idVenda ORDER BY vp.idVendaProduto1) AS numero_linha,
    obter_uuid_produto(vp.idProduto) AS id_produto,
    p.codComercial AS sku_produto,
    vp.produto AS nome_produto,
    vp.quant AS quantidade_pedida,
    vp.prcUnitario AS preco_unitario,
    vp.total AS total_linha,
    obter_uuid_fornecedor(vp.fornecedor) AS id_fornecedor,
    vp.created AS criado_em
FROM venda_has_produto vp
JOIN vendas s ON s.numero_venda = vp.idVenda
JOIN produto p ON p.idProduto = vp.idProduto;

-- Passo 3: Migrar origens de atendimento (de venda_has_produto2)
INSERT INTO origens_atendimento (
    id, id_item_venda, tipo_origem, id_origem, referencia_origem,
    quantidade_alocada, custo_unitario, status, criado_em
)
SELECT
    UUID() AS id,
    iv.id AS id_item_venda,
    determinar_tipo_origem(vp2.idCompra, vp2.estoque) AS tipo_origem,
    determinar_id_origem(vp2.idCompra, vp2.estoque) AS id_origem,
    COALESCE(vp2.idCompra, vp2.lote, 'ESTOQUE') AS referencia_origem,
    vp2.quant AS quantidade_alocada,
    vp2.prcUnitario AS custo_unitario,
    mapear_status_atendimento(vp2.status) AS status,
    vp2.created AS criado_em
FROM venda_has_produto2 vp2
JOIN venda_has_produto vp1 ON vp2.idVendaProdutoFK = vp1.idVendaProduto1
JOIN itens_venda iv ON iv.id_venda IN (
    SELECT id FROM vendas WHERE numero_venda = vp2.idVenda
) AND iv.id_produto = obter_uuid_produto(vp2.idProduto);
```

#### **2. Migration Functions**

```sql
-- Função para determinar tipo de origem a partir de dados legados
DELIMITER $$
CREATE FUNCTION determinar_tipo_origem(compra_id INT, estoque_flag TINYINT)
RETURNS ENUM('estoque', 'pedido_compra', 'transferencia', 'producao')
DETERMINISTIC
BEGIN
    IF estoque_flag = 1 THEN
        RETURN 'estoque';
    ELSEIF compra_id IS NOT NULL THEN
        RETURN 'pedido_compra';
    ELSE
        RETURN 'estoque'; -- Assumptido padrão
    END IF;
END$$
DELIMITER ;

-- Função para mapear status legado para novo status
DELIMITER $$
CREATE FUNCTION mapear_status_atendimento(legacy_status VARCHAR(45))
RETURNS ENUM('planejado', 'reservado', 'alocado', 'atendido', 'cancelado')
DETERMINISTIC
BEGIN
    CASE legacy_status
        WHEN 'ENTREGUE' THEN RETURN 'atendido';
        WHEN 'CANCELADO' THEN RETURN 'cancelado';
        WHEN 'PENDENTE' THEN RETURN 'planejado';
        WHEN 'CONFIRMADO' THEN RETURN 'alocado';
        ELSE RETURN 'planejado';
    END CASE;
END$$
DELIMITER ;
```

### Phase 4: Validation and Testing (2-3 weeks)

#### **1. Data Validation Queries**

```sql
-- Validar completude da migração
SELECT
    'Vendas' AS tipo_entidade,
    COUNT(*) AS contagem_legado,
    (SELECT COUNT(*) FROM vendas) AS contagem_nova,
    (SELECT COUNT(*) FROM vendas) - COUNT(*) AS diferenca
FROM venda
WHERE created > '2024-01-01'

UNION ALL

SELECT
    'Itens Venda' AS tipo_entidade,
    COUNT(*) AS contagem_legado,
    (SELECT COUNT(*) FROM itens_venda) AS contagem_nova,
    (SELECT COUNT(*) FROM itens_venda) - COUNT(*) AS diferenca
FROM venda_has_produto vp
JOIN venda v ON v.idVenda = vp.idVenda
WHERE v.created > '2024-01-01'

UNION ALL

SELECT
    'Origens Atendimento' AS tipo_entidade,
    COUNT(*) AS contagem_legado,
    (SELECT COUNT(*) FROM origens_atendimento) AS contagem_nova,
    (SELECT COUNT(*) FROM origens_atendimento) - COUNT(*) AS diferenca
FROM venda_has_produto2 vp2
JOIN venda v ON v.idVenda = vp2.idVenda
WHERE v.created > '2024-01-01';

-- Validar consistência de quantidades
SELECT
    iv.id AS id_item_venda,
    iv.quantidade_pedida,
    COALESCE(SUM(oa.quantidade_alocada), 0) AS total_alocado,
    iv.quantidade_pedida - COALESCE(SUM(oa.quantidade_alocada), 0) AS diferenca
FROM itens_venda iv
LEFT JOIN origens_atendimento oa ON iv.id = oa.id_item_venda
GROUP BY iv.id, iv.quantidade_pedida
HAVING ABS(diferenca) > 0.001;
```

### Phase 5: Cutover and Decommissioning (1-2 weeks)

#### **1. Parallel Running Period**
- Run both old and new systems simultaneously
- Compare results for critical business operations
- Gradually shift read operations to new system

#### **2. Application Updates**
- Update all queries to use new schema
- Implement new business logic for fulfillment tracking
- Add validation and error handling

#### **3. Training and Documentation**
- Train users on new fulfillment concepts
- Update operational procedures
- Create troubleshooting guides

## Benefits of New Design

### **1. Data Integrity**
- **Eliminates Duplication**: Single source of truth for each data element
- **Enforces Constraints**: Database-level validation of business rules
- **Audit Trail**: Complete history of fulfillment decisions and changes

### **2. Query Performance**
- **Optimized Indexes**: Proper indexing strategy for common queries
- **Reduced Joins**: Fewer tables needed for basic operations
- **Materialized Views**: Can create optimized views for reporting without embedding business logic

### **3. Maintainability**
- **Clear Separation**: Distinct tables for distinct concepts
- **Extensibilidade**: Fácil adição de novos tipos de origem de atendimento
- **Debugging**: Easier to trace issues through logical data flow

### **4. Inteligência de Negócios**
- **Análise de Custos**: Rastreamento preciso de custos por origem de atendimento
- **Métricas de Performance**: Relatórios de velocidade e precisão de atendimento
- **Otimização de Estoque**: Melhor visibilidade da utilização do estoque

### **5. Scalability**
- **Horizontal Scaling**: Tables can be partitioned independently
- **Estratégia de Cache**: Limites claros para cache de diferentes tipos de dados
- **API Design**: Clean domain model enables better API design

## Implementation Timeline

| Phase | Duration | Activities | Team Size |
|-------|----------|------------|-----------|
| Analysis & Design | 2-3 weeks | Data analysis, schema design, validation | 2 developers, 1 DBA |
| Schema Creation | 1-2 weeks | Create tables, constraints, procedures | 1 developer, 1 DBA |
| Migration Scripts | 3-4 weeks | Write and test migration code | 2 developers |
| Data Migration | 2-3 weeks | Execute migration, validate results | 2 developers, 1 DBA |
| Application Updates | 4-6 weeks | Update business logic, testing | 3 developers, 1 QA |
| Cutover | 1-2 weeks | Production deployment, monitoring | Full team |

**Total Timeline: 13-20 weeks**
**Total Cost: $200K-300K**

## Conclusion

A estrutura atual `venda_has_produto` / `venda_has_produto2` representa um anti-padrão clássico em design de banco de dados: usar duplicação de tabelas para resolver um problema de modelagem de domínio. Embora atenda ao requisito de negócio legítimo de rastrear atendimento dividido, cria débito técnico significativo e carga de manutenção.

The proposed redesign:

1. **Separates Concerns**: Clear distinction between what was sold vs. how it's fulfilled
2. **Eliminates Duplication**: Single source of truth for each data element
3. **Habilita Flexibilidade**: Pode facilmente acomodar novos tipos e cenários de atendimento
4. **Improves Performance**: Optimized schema with proper indexing
5. **Enhances Reliability**: Database-enforced business rules and data integrity

This investment in proper database design will pay dividends through:
- Reduced development time for new features
- Fewer bugs and data integrity issues
- Better performance and scalability
- Easier integration with external systems
- Improved business intelligence and reporting capabilities

The migration can be executed with minimal business disruption using the phased approach outlined above, providing a solid foundation for the ERP system's future growth and evolution.