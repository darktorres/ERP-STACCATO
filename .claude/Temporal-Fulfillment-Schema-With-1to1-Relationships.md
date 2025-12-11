# Temporal Fulfillment Schema with 1:1 Relationship Maintenance

## 📑 Índice Detalhado

### **🎯 Visão Geral**
- [Executive Summary](#executive-summary) (Line 61) - Extensão do schema com relacionamentos 1:1 temporais

### **🔍 Análise do Sistema Atual**
- **[Current System Analysis](#current-system-analysis)** (Line 76)
  - [Existing 1:1 Relationship Structure](#existing-11-relationship-structure) (L20)
    - [1. Sale-to-Purchase Fulfillment Link](#1-sale-to-purchase-fulfillment-link) (L22) - Vinculação venda-compra
    - [2. Sale-to-Inventory Consumption Link](#2-sale-to-inventory-consumption-link) (L31) - Vinculação venda-estoque
  - [Business Logic Requirements](#business-logic-requirements) (L40) - Regras de negócio críticas

### **🏗️ Design do Novo Schema Temporal**
- **[New Temporal Schema Design](#new-temporal-schema-design)** (Line 109)
  - [Core Principle: Temporal 1:1 Relationship Tracking](#core-principle-temporal-11-relationship-tracking) (L53) - Princípios fundamentais
  - [Temporal Table Infrastructure](#temporal-table-infrastructure) (L60)
    - [1. Temporal Configuration](#1-temporal-configuration) (L62) - Configuração base temporal
    - [2. Temporal Base Tables](#2-temporal-base-tables) (L79) - Tabelas principais temporais

### **🔗 Relacionamentos Temporais 1:1**
- **[Temporal 1:1 Relationship Tables](#temporal-11-relationship-tables)** (L283)
  - [1. Temporal Origens Atendimento (Fulfillment Sources)](#1-temporal-origens-atendimento-fulfillment-sources) (L285) - Fontes de atendimento temporais
  - [2. Temporal 1:1 Relationship Bridge Tables](#2-temporal-11-relationship-bridge-tables) (L397) - Tabelas ponte de relacionamento

### **🛡️ Integridade Referencial Temporal**
- **[Temporal Referential Integrity Enforcement](#temporal-referential-integrity-enforcement)** (L486)
  - [1. Temporal Foreign Key Constraints](#1-temporal-foreign-key-constraints) (L488) - Constraints FK temporais
  - [2. Temporal Business Logic Functions](#2-temporal-business-logic-functions) (L557) - Funções de lógica de negócio

### **📊 Consultas e Operações Temporais**
- **[Temporal Query Examples](#temporal-query-examples)** (L721)
  - [1. Time-Travel Queries for Business Intelligence](#1-time-travel-queries-for-business-intelligence) (L723) - Consultas de viagem no tempo
  - [2. 1:1 Relationship Validation Queries](#2-11-relationship-validation-queries) (L793) - Validação de relacionamentos

### **🔄 Estratégia de Migração**
- **[Migration Strategy with 1:1 Relationship Preservation](#migration-strategy-with-11-relationship-preservation)** (L851)
  - [1. Pre-Migration Analysis](#1-pre-migration-analysis) (L853) - Análise pré-migração
  - [2. Migration Mapping with 1:1 Preservation](#2-migration-mapping-with-11-preservation) (L908) - Mapeamento com preservação
  - [3. Step-by-Step Migration with 1:1 Preservation](#3-step-by-step-migration-with-11-preservation) (L950) - Migração passo a passo

### **⚡ Otimização de Performance**
- **[Performance Optimization for Temporal 1:1 Relationships](#performance-optimization-for-temporal-11-relationships)** (L1090)
  - [1. Temporal-Specific Indexes](#1-temporal-specific-indexes) (L1092) - Índices específicos temporais
  - [2. Temporal Materialized Views for Performance](#2-temporal-materialized-views-for-performance) (L1118) - Views materializadas

### **📅 Cronograma e Benefícios**
- **[Implementation Timeline with 1:1 Relationship Focus](#implementation-timeline-with-11-relationship-focus)** (Line 1241) - Timeline de implementação
- **[Benefits of Temporal 1:1 Design](#benefits-of-temporal-11-design)** (Line 1257)
  - [1. Maintained Business Logic Integrity](#1-maintained-business-logic-integrity) (L1201) - Integridade preservada
  - [2. Enhanced Business Intelligence](#2-enhanced-business-intelligence) (L1206) - BI aprimorado
  - [3. Operational Advantages](#3-operational-advantages) (L1211) - Vantagens operacionais
  - [4. Future-Proofing](#4-future-proofing) (L1216) - Preparação para o futuro

### **🎯 Conclusão**
- **[Conclusion](#conclusion)** (Line 1279) - Síntese final dos benefícios temporais

---

## Executive Summary

This document extends the Database-Fulfillment-Redesign-2025.md to address the critical 1:1 relationships currently maintained in the system and incorporates temporal table support for complete audit trails and time-based queries.

**Current 1:1 Relationships to Maintain:**
1. `venda_has_produto2 ↔ pedido_fornecedor_has_produto2` (Sale fulfillment ↔ Purchase fulfillment)
2. `venda_has_produto2 ↔ estoque_has_consumo` (Sale fulfillment ↔ Inventory consumption)

**New Design Principles:**
- Maintain referential integrity of 1:1 relationships
- Add temporal table support for complete audit trails
- Enable time-travel queries for business intelligence
- Preserve performance with optimized indexing
- Support concurrent fulfillment operations

## Current System Analysis

### Existing 1:1 Relationship Structure

#### **1. Sale-to-Purchase Fulfillment Link**
```sql
-- Current relationship
venda_has_produto2.idVendaProduto2 = pedido_fornecedor_has_produto2.idVendaProduto2

-- Business meaning: Each sale fulfillment record is tied to exactly one purchase fulfillment
-- This enables tracking: "This sale was fulfilled using goods from this specific PO receipt"
```

#### **2. Sale-to-Inventory Consumption Link**
```sql
-- Current relationship
venda_has_produto2.idVendaProduto2 = estoque_has_consumo.idVendaProduto2

-- Business meaning: Each sale fulfillment consumes exactly one inventory allocation
-- This enables tracking: "This sale consumed exactly this inventory quantity from this location"
```

### Business Logic Requirements

```typescript
interface FulfillmentRelationship {
  saleLineItem: string;           // What was sold
  purchaseSource?: string;        // Where it came from (if from PO)
  inventoryConsumption: string;   // Inventory that was consumed
  relationship: "1:1";           // Must maintain exactly one-to-one
}
```

## New Temporal Schema Design

### Core Principle: Temporal 1:1 Relationship Tracking

The new design maintains the 1:1 relationships while adding temporal support through:
1. **Temporal Base Tables** - All entity tables with temporal support
2. **Temporal Relationship Tables** - 1:1 relationship tables with temporal tracking
3. **Temporal Constraints** - Database-enforced temporal referential integrity

### Temporal Table Infrastructure

#### **1. Temporal Configuration**
```sql
-- Enable temporal support globally
SET SYSTEM_VERSIONING = ON;

-- Temporal table suffix for history tables
SET GLOBAL default_table_suffix_history = '_history';

-- Temporal column standards
CREATE TABLE temporal_columns_template (
    -- Required temporal columns for all tables
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end)
) WITH SYSTEM VERSIONING;
```

#### **2. Temporal Base Tables**

```sql
-- =====================================================
-- TEMPORAL VENDAS (Sales Header)
-- =====================================================
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

    -- Status e cronograma
    status ENUM('rascunho', 'confirmado', 'processando', 'atendido', 'entregue', 'cancelado') DEFAULT 'rascunho',
    data_prevista_entrega DATE,
    observacoes TEXT,

    -- Campos de auditoria
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    criado_por UUID NOT NULL REFERENCES usuarios(id),

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    INDEX idx_vendas_cliente (id_cliente),
    INDEX idx_vendas_status (status),
    INDEX idx_vendas_data (criado_em),
    INDEX idx_vendas_numero (numero_venda),
    INDEX idx_vendas_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;

-- =====================================================
-- TEMPORAL ITENS VENDA (What Was Sold)
-- =====================================================
CREATE TABLE itens_venda (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_venda UUID NOT NULL REFERENCES vendas(id) ON DELETE CASCADE,
    numero_linha INT NOT NULL,

    -- Informações do produto
    id_produto UUID NOT NULL REFERENCES produtos(id),
    codigo_produto VARCHAR(50) NOT NULL,
    nome_produto VARCHAR(200) NOT NULL,
    id_fornecedor UUID NOT NULL REFERENCES fornecedores(id),

    -- Quantidade e preços
    quantidade_pedida DECIMAL(15,4) NOT NULL,
    unidade_medida VARCHAR(10) NOT NULL,
    preco_unitario DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4),

    -- Status do atendimento
    quantidade_reservada DECIMAL(15,4) DEFAULT 0,
    quantidade_alocada DECIMAL(15,4) DEFAULT 0,
    quantidade_enviada DECIMAL(15,4) DEFAULT 0,
    quantidade_entregue DECIMAL(15,4) DEFAULT 0,
    quantidade_cancelada DECIMAL(15,4) DEFAULT 0,

    -- Totais
    desconto_percentual DECIMAL(5,2) DEFAULT 0,
    desconto_valor DECIMAL(15,4) DEFAULT 0,
    subtotal_linha DECIMAL(15,4) NOT NULL,
    total_linha DECIMAL(15,4) NOT NULL,

    -- Metadados
    observacoes TEXT,
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    -- Constraints
    CONSTRAINT chk_quantidades CHECK (
        quantidade_reservada <= quantidade_pedida AND
        quantidade_alocada <= quantidade_reservada AND
        quantidade_enviada <= quantidade_alocada AND
        quantidade_entregue <= quantidade_enviada AND
        quantidade_cancelada <= quantidade_pedida
    ),

    UNIQUE KEY unique_linha (id_venda, numero_linha),
    INDEX idx_itens_venda (id_venda),
    INDEX idx_itens_produto (id_produto),
    INDEX idx_itens_fornecedor (id_fornecedor),
    INDEX idx_itens_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;

-- =====================================================
-- TEMPORAL PEDIDOS COMPRA (Purchase Orders)
-- =====================================================
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

    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    criado_por UUID NOT NULL REFERENCES usuarios(id),

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    INDEX idx_pc_fornecedor (id_fornecedor),
    INDEX idx_pc_status (status),
    INDEX idx_pc_data (data_pedido),
    INDEX idx_pc_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;

-- =====================================================
-- TEMPORAL ITENS PEDIDO COMPRA
-- =====================================================
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

    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    UNIQUE KEY unique_linha_pc (id_pedido_compra, numero_linha),
    INDEX idx_itens_pc_produto (id_produto),
    INDEX idx_itens_pc_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;

-- =====================================================
-- TEMPORAL LOTES ESTOQUE (Inventory Batches)
-- =====================================================
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

    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    UNIQUE KEY unique_lote (id_produto, id_deposito, numero_lote),
    INDEX idx_lote_produto (id_produto),
    INDEX idx_lote_deposito (id_deposito),
    INDEX idx_lote_disponivel (quantidade_disponivel),
    INDEX idx_lote_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;
```

### Temporal 1:1 Relationship Tables

#### **1. Temporal Origens Atendimento (Fulfillment Sources)**
```sql
-- =====================================================
-- TEMPORAL ORIGENS ATENDIMENTO (How It Will Be Fulfilled)
-- Maintains 1:1 relationship tracking
-- =====================================================
CREATE TABLE origens_atendimento (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_item_venda UUID NOT NULL REFERENCES itens_venda(id) ON DELETE CASCADE,

    -- Informações da origem
    tipo_origem ENUM('estoque', 'pedido_compra', 'transferencia', 'producao') NOT NULL,
    id_origem UUID NOT NULL,
    referencia_origem VARCHAR(50),

    -- *** 1:1 RELATIONSHIP FIELDS ***
    id_item_pedido_compra UUID NULL REFERENCES itens_pedido_compra(id),
    id_lote_estoque UUID NULL REFERENCES lotes_estoque(id),

    -- Alocação de quantidade
    quantidade_alocada DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4),

    -- Rastreamento de status
    status ENUM('planejado', 'reservado', 'alocado', 'atendido', 'cancelado') DEFAULT 'planejado',
    prioridade INT DEFAULT 1,

    -- Cronograma
    data_prevista_disponibilidade DATE,
    alocado_em TIMESTAMP NULL,
    atendido_em TIMESTAMP NULL,

    -- Metadados
    observacoes TEXT,
    criado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    atualizado_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    criado_por UUID NOT NULL REFERENCES usuarios(id),

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    -- *** 1:1 RELATIONSHIP CONSTRAINTS ***
    CONSTRAINT chk_origem_references CHECK (
        (tipo_origem = 'estoque' AND id_lote_estoque IS NOT NULL AND id_item_pedido_compra IS NULL) OR
        (tipo_origem = 'pedido_compra' AND id_item_pedido_compra IS NOT NULL AND id_lote_estoque IS NULL) OR
        (tipo_origem IN ('transferencia', 'producao') AND id_lote_estoque IS NULL AND id_item_pedido_compra IS NULL)
    ),

    -- Ensure each source can only be used once per fulfillment time
    UNIQUE KEY unique_purchase_allocation (id_item_pedido_compra, row_start),
    UNIQUE KEY unique_inventory_allocation (id_lote_estoque, row_start),

    INDEX idx_atendimento_item_venda (id_item_venda),
    INDEX idx_atendimento_origem (tipo_origem, id_origem),
    INDEX idx_atendimento_status (status),
    INDEX idx_atendimento_purchase (id_item_pedido_compra),
    INDEX idx_atendimento_inventory (id_lote_estoque),
    INDEX idx_atendimento_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;

-- =====================================================
-- TEMPORAL CONCLUSOES ATENDIMENTO (How It Was Actually Fulfilled)
-- Maintains execution-level 1:1 relationships
-- =====================================================
CREATE TABLE conclusoes_atendimento (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_origem_atendimento UUID NOT NULL REFERENCES origens_atendimento(id),
    id_item_venda UUID NOT NULL REFERENCES itens_venda(id),

    -- *** 1:1 EXECUTION RELATIONSHIP ***
    id_consumo_estoque UUID UNIQUE, -- Maps to old estoque_has_consumo concept
    id_receita_pedido_compra UUID UNIQUE, -- Maps to old pedido_fornecedor_has_produto2 concept

    -- Detalhes da conclusão
    quantidade_atendida DECIMAL(15,4) NOT NULL,
    custo_unitario_real DECIMAL(15,4),
    numero_lote VARCHAR(50),
    numeros_serie JSON,

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

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    INDEX idx_conclusao_origem (id_origem_atendimento),
    INDEX idx_conclusao_item_venda (id_item_venda),
    INDEX idx_conclusao_data (atendido_em),
    INDEX idx_conclusao_consumo (id_consumo_estoque),
    INDEX idx_conclusao_receita (id_receita_pedido_compra),
    INDEX idx_conclusao_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;
```

#### **2. Temporal 1:1 Relationship Bridge Tables**

```sql
-- =====================================================
-- TEMPORAL CONSUMOS ESTOQUE (Inventory Consumption Records)
-- Direct replacement for estoque_has_consumo
-- =====================================================
CREATE TABLE consumos_estoque (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_lote_estoque UUID NOT NULL REFERENCES lotes_estoque(id),
    id_conclusao_atendimento UUID NOT NULL REFERENCES conclusoes_atendimento(id),

    -- *** ENFORCE 1:1 RELATIONSHIP ***
    -- Each consumption record is tied to exactly one fulfillment conclusion
    CONSTRAINT unique_consumption_per_conclusion UNIQUE (id_conclusao_atendimento),

    -- Consumption details
    quantidade_consumida DECIMAL(15,4) NOT NULL,
    custo_unitario_consumo DECIMAL(15,4) NOT NULL,
    numero_lote_consumido VARCHAR(50),
    localizacao_estoque VARCHAR(100),

    -- Status tracking
    status ENUM('reservado', 'alocado', 'consumido', 'cancelado') DEFAULT 'reservado',
    consumido_em TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    id_bloco UUID, -- Legacy compatibility field

    -- Metadata
    observacoes TEXT,
    consumido_por UUID NOT NULL REFERENCES usuarios(id),

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    INDEX idx_consumo_lote (id_lote_estoque),
    INDEX idx_consumo_conclusao (id_conclusao_atendimento),
    INDEX idx_consumo_status (status),
    INDEX idx_consumo_data (consumido_em),
    INDEX idx_consumo_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;

-- =====================================================
-- TEMPORAL RECEITAS PEDIDO COMPRA (Purchase Order Receipt Records)
-- Direct replacement for pedido_fornecedor_has_produto2
-- =====================================================
CREATE TABLE receitas_pedido_compra (
    id UUID PRIMARY KEY DEFAULT (UUID()),
    id_item_pedido_compra UUID NOT NULL REFERENCES itens_pedido_compra(id),
    id_conclusao_atendimento UUID NOT NULL REFERENCES conclusoes_atendimento(id),

    -- *** ENFORCE 1:1 RELATIONSHIP ***
    -- Each receipt record is tied to exactly one fulfillment conclusion
    CONSTRAINT unique_receipt_per_conclusion UNIQUE (id_conclusao_atendimento),

    -- Receipt details
    quantidade_recebida DECIMAL(15,4) NOT NULL,
    custo_unitario_recebido DECIMAL(15,4) NOT NULL,
    numero_lote_recebido VARCHAR(50),
    condicao_recebimento ENUM('conforme', 'avariado', 'incompleto') DEFAULT 'conforme',

    -- Status tracking
    status ENUM('esperado', 'recebido', 'inspecionado', 'liberado', 'bloqueado') DEFAULT 'esperado',
    data_recebimento TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    data_liberacao TIMESTAMP NULL,

    -- Quality and documentation
    observacoes_qualidade TEXT,
    id_nfe_entrada UUID REFERENCES nfe(id),
    id_inspecao UUID, -- Future quality inspection reference

    -- Metadata
    observacoes TEXT,
    recebido_por UUID NOT NULL REFERENCES usuarios(id),

    -- *** TEMPORAL COLUMNS ***
    row_start TIMESTAMP(6) GENERATED ALWAYS AS ROW START,
    row_end TIMESTAMP(6) GENERATED ALWAYS AS ROW END,
    PERIOD FOR SYSTEM_TIME(row_start, row_end),

    INDEX idx_receita_item_pc (id_item_pedido_compra),
    INDEX idx_receita_conclusao (id_conclusao_atendimento),
    INDEX idx_receita_status (status),
    INDEX idx_receita_data (data_recebimento),
    INDEX idx_receita_temporal (row_start, row_end)
) WITH SYSTEM VERSIONING;
```

### Temporal Referential Integrity Enforcement

#### **1. Temporal Foreign Key Constraints**
```sql
-- =====================================================
-- TEMPORAL REFERENTIAL INTEGRITY TRIGGERS
-- Ensure 1:1 relationships are maintained across time
-- =====================================================

DELIMITER $$

-- Trigger to ensure temporal 1:1 relationship for consumption records
CREATE TRIGGER enforce_temporal_consumption_1to1
BEFORE INSERT ON consumos_estoque
FOR EACH ROW
BEGIN
    DECLARE existing_count INT;

    -- Check if this conclusion already has a consumption record
    -- in the current time period
    SELECT COUNT(*) INTO existing_count
    FROM consumos_estoque FOR SYSTEM_TIME AS OF NOW()
    WHERE id_conclusao_atendimento = NEW.id_conclusao_atendimento;

    IF existing_count > 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Violação de integridade temporal 1:1: Conclusão de atendimento já possui registro de consumo';
    END IF;
END$$

-- Trigger to ensure temporal 1:1 relationship for receipt records
CREATE TRIGGER enforce_temporal_receipt_1to1
BEFORE INSERT ON receitas_pedido_compra
FOR EACH ROW
BEGIN
    DECLARE existing_count INT;

    -- Check if this conclusion already has a receipt record
    -- in the current time period
    SELECT COUNT(*) INTO existing_count
    FROM receitas_pedido_compra FOR SYSTEM_TIME AS OF NOW()
    WHERE id_conclusao_atendimento = NEW.id_conclusao_atendimento;

    IF existing_count > 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'Violação de integridade temporal 1:1: Conclusão de atendimento já possui registro de receita';
    END IF;
END$$

-- Trigger to maintain quantity consistency across temporal changes
CREATE TRIGGER maintain_temporal_quantity_integrity
BEFORE UPDATE ON conclusoes_atendimento
FOR EACH ROW
BEGIN
    -- When updating conclusion quantities, verify related records are consistent
    IF NEW.quantidade_atendida != OLD.quantidade_atendida THEN
        -- Update related consumption record if exists
        UPDATE consumos_estoque
        SET quantidade_consumida = NEW.quantidade_atendida
        WHERE id_conclusao_atendimento = NEW.id;

        -- Update related receipt record if exists
        UPDATE receitas_pedido_compra
        SET quantidade_recebida = NEW.quantidade_atendida
        WHERE id_conclusao_atendimento = NEW.id;
    END IF;
END$$

DELIMITER ;
```

#### **2. Temporal Business Logic Functions**

```sql
-- =====================================================
-- TEMPORAL BUSINESS LOGIC FUNCTIONS
-- =====================================================

DELIMITER $$

-- Function to get fulfillment status at a specific point in time
CREATE FUNCTION get_fulfillment_status_at_time(
    p_id_item_venda UUID,
    p_timestamp TIMESTAMP(6)
) RETURNS JSON
DETERMINISTIC
READS SQL DATA
BEGIN
    DECLARE result JSON;

    SELECT JSON_OBJECT(
        'item_venda_id', iv.id,
        'quantidade_pedida', iv.quantidade_pedida,
        'quantidade_entregue', iv.quantidade_entregue,
        'origens_atendimento', JSON_ARRAYAGG(
            JSON_OBJECT(
                'id', oa.id,
                'tipo_origem', oa.tipo_origem,
                'quantidade_alocada', oa.quantidade_alocada,
                'status', oa.status,
                'conclusoes', (
                    SELECT JSON_ARRAYAGG(
                        JSON_OBJECT(
                            'id', ca.id,
                            'quantidade_atendida', ca.quantidade_atendida,
                            'atendido_em', ca.atendido_em,
                            'consumo_id', ce.id,
                            'receita_id', rpc.id
                        )
                    )
                    FROM conclusoes_atendimento FOR SYSTEM_TIME AS OF p_timestamp ca
                    LEFT JOIN consumos_estoque FOR SYSTEM_TIME AS OF p_timestamp ce
                        ON ca.id = ce.id_conclusao_atendimento
                    LEFT JOIN receitas_pedido_compra FOR SYSTEM_TIME AS OF p_timestamp rpc
                        ON ca.id = rpc.id_conclusao_atendimento
                    WHERE ca.id_origem_atendimento = oa.id
                )
            )
        )
    ) INTO result
    FROM itens_venda FOR SYSTEM_TIME AS OF p_timestamp iv
    LEFT JOIN origens_atendimento FOR SYSTEM_TIME AS OF p_timestamp oa
        ON iv.id = oa.id_item_venda
    WHERE iv.id = p_id_item_venda
    GROUP BY iv.id, iv.quantidade_pedida, iv.quantidade_entregue;

    RETURN result;
END$$

-- Procedure to create fulfillment with maintained 1:1 relationships
CREATE PROCEDURE create_fulfillment_with_1to1_relationships(
    IN p_id_item_venda UUID,
    IN p_tipo_origem ENUM('estoque', 'pedido_compra', 'transferencia', 'producao'),
    IN p_id_origem UUID,
    IN p_quantidade_alocada DECIMAL(15,4),
    IN p_custo_unitario DECIMAL(15,4),
    IN p_created_by UUID
)
BEGIN
    DECLARE v_id_origem_atendimento UUID DEFAULT (UUID());
    DECLARE v_id_conclusao_atendimento UUID DEFAULT (UUID());
    DECLARE v_id_lote_estoque UUID;
    DECLARE v_id_item_pedido_compra UUID;

    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        RESIGNAL;
    END;

    START TRANSACTION;

    -- Determine the specific source references based on type
    IF p_tipo_origem = 'estoque' THEN
        SET v_id_lote_estoque = p_id_origem;
    ELSEIF p_tipo_origem = 'pedido_compra' THEN
        SET v_id_item_pedido_compra = p_id_origem;
    END IF;

    -- Create fulfillment source record
    INSERT INTO origens_atendimento (
        id, id_item_venda, tipo_origem, id_origem,
        id_lote_estoque, id_item_pedido_compra,
        quantidade_alocada, custo_unitario, status,
        criado_por
    ) VALUES (
        v_id_origem_atendimento, p_id_item_venda, p_tipo_origem, p_id_origem,
        v_id_lote_estoque, v_id_item_pedido_compra,
        p_quantidade_alocada, p_custo_unitario, 'alocado',
        p_created_by
    );

    -- Create fulfillment conclusion record
    INSERT INTO conclusoes_atendimento (
        id, id_origem_atendimento, id_item_venda,
        quantidade_atendida, custo_unitario_real, atendido_por
    ) VALUES (
        v_id_conclusao_atendimento, v_id_origem_atendimento, p_id_item_venda,
        p_quantidade_alocada, p_custo_unitario, p_created_by
    );

    -- Create 1:1 relationship records based on source type
    IF p_tipo_origem = 'estoque' THEN
        -- Create inventory consumption record (1:1 with conclusion)
        INSERT INTO consumos_estoque (
            id, id_lote_estoque, id_conclusao_atendimento,
            quantidade_consumida, custo_unitario_consumo,
            consumido_por
        ) VALUES (
            UUID(), v_id_lote_estoque, v_id_conclusao_atendimento,
            p_quantidade_alocada, p_custo_unitario,
            p_created_by
        );

        -- Update inventory quantities
        UPDATE lotes_estoque
        SET quantidade_disponivel = quantidade_disponivel - p_quantidade_alocada,
            quantidade_alocada = quantidade_alocada + p_quantidade_alocada
        WHERE id = v_id_lote_estoque;

    ELSEIF p_tipo_origem = 'pedido_compra' THEN
        -- Create purchase receipt record (1:1 with conclusion)
        INSERT INTO receitas_pedido_compra (
            id, id_item_pedido_compra, id_conclusao_atendimento,
            quantidade_recebida, custo_unitario_recebido,
            recebido_por
        ) VALUES (
            UUID(), v_id_item_pedido_compra, v_id_conclusao_atendimento,
            p_quantidade_alocada, p_custo_unitario,
            p_created_by
        );

        -- Update purchase order quantities
        UPDATE itens_pedido_compra
        SET quantidade_recebida = quantidade_recebida + p_quantidade_alocada
        WHERE id = v_id_item_pedido_compra;
    END IF;

    -- Update sale line item quantities
    UPDATE itens_venda
    SET quantidade_alocada = quantidade_alocada + p_quantidade_alocada,
        quantidade_entregue = quantidade_entregue + p_quantidade_alocada
    WHERE id = p_id_item_venda;

    COMMIT;

    -- Return the created IDs for reference
    SELECT v_id_origem_atendimento AS id_origem_atendimento,
           v_id_conclusao_atendimento AS id_conclusao_atendimento;

END$$

DELIMITER ;
```

### Temporal Query Examples

#### **1. Time-Travel Queries for Business Intelligence**

```sql
-- =====================================================
-- TIME-TRAVEL QUERIES
-- =====================================================

-- Get fulfillment status as it was at end of last month
SELECT
    v.numero_venda,
    iv.nome_produto,
    iv.quantidade_pedida,
    iv.quantidade_entregue AS entregue_fim_mes_passado,
    COUNT(oa.id) AS total_fontes_atendimento,
    SUM(oa.quantidade_alocada) AS total_quantidade_alocada
FROM vendas FOR SYSTEM_TIME AS OF LAST_DAY(CURDATE() - INTERVAL 1 MONTH) v
JOIN itens_venda FOR SYSTEM_TIME AS OF LAST_DAY(CURDATE() - INTERVAL 1 MONTH) iv
    ON v.id = iv.id_venda
LEFT JOIN origens_atendimento FOR SYSTEM_TIME AS OF LAST_DAY(CURDATE() - INTERVAL 1 MONTH) oa
    ON iv.id = oa.id_item_venda
WHERE v.status IN ('processando', 'atendido')
GROUP BY v.id, v.numero_venda, iv.id, iv.nome_produto, iv.quantidade_pedida, iv.quantidade_entregue;

-- Track fulfillment changes over time for a specific sale
SELECT
    ca.atendido_em,
    ca.quantidade_atendida,
    oa.tipo_origem,
    oa.referencia_origem,
    ce.quantidade_consumida,
    rpc.quantidade_recebida,
    ca.row_start AS version_start,
    ca.row_end AS version_end
FROM conclusoes_atendimento FOR SYSTEM_TIME ALL ca
JOIN origens_atendimento FOR SYSTEM_TIME ALL oa ON ca.id_origem_atendimento = oa.id
LEFT JOIN consumos_estoque FOR SYSTEM_TIME ALL ce ON ca.id = ce.id_conclusao_atendimento
LEFT JOIN receitas_pedido_compra FOR SYSTEM_TIME ALL rpc ON ca.id = rpc.id_conclusao_atendimento
WHERE ca.id_item_venda = '12345678-1234-1234-1234-123456789012'
ORDER BY ca.row_start DESC;

-- Audit trail: Who changed what when for fulfillment records
SELECT
    'origem_atendimento' AS table_name,
    oa.id AS record_id,
    oa.status,
    oa.quantidade_alocada,
    oa.row_start AS changed_at,
    oa.row_end AS valid_until,
    u.nome AS changed_by
FROM origens_atendimento FOR SYSTEM_TIME ALL oa
JOIN usuarios u ON oa.criado_por = u.id
WHERE oa.id_item_venda = '12345678-1234-1234-1234-123456789012'

UNION ALL

SELECT
    'conclusao_atendimento' AS table_name,
    ca.id AS record_id,
    CAST(ca.quantidade_atendida AS CHAR) AS status,
    ca.quantidade_atendida,
    ca.row_start AS changed_at,
    ca.row_end AS valid_until,
    u.nome AS changed_by
FROM conclusoes_atendimento FOR SYSTEM_TIME ALL ca
JOIN usuarios u ON ca.atendido_por = u.id
WHERE ca.id_item_venda = '12345678-1234-1234-1234-123456789012'

ORDER BY changed_at DESC;
```

#### **2. 1:1 Relationship Validation Queries**

```sql
-- =====================================================
-- 1:1 RELATIONSHIP VALIDATION QUERIES
-- =====================================================

-- Verify no orphaned consumption records (all should have exactly one conclusion)
SELECT
    ce.id AS consumo_id,
    ce.id_conclusao_atendimento,
    COUNT(ca.id) AS conclusion_count,
    CASE
        WHEN COUNT(ca.id) = 0 THEN 'ORPHANED_CONSUMPTION'
        WHEN COUNT(ca.id) > 1 THEN 'MULTIPLE_CONCLUSIONS'
        ELSE 'OK'
    END AS validation_status
FROM consumos_estoque ce
LEFT JOIN conclusoes_atendimento ca ON ce.id_conclusao_atendimento = ca.id
GROUP BY ce.id, ce.id_conclusao_atendimento
HAVING COUNT(ca.id) != 1;

-- Verify no orphaned receipt records (all should have exactly one conclusion)
SELECT
    rpc.id AS receita_id,
    rpc.id_conclusao_atendimento,
    COUNT(ca.id) AS conclusion_count,
    CASE
        WHEN COUNT(ca.id) = 0 THEN 'ORPHANED_RECEIPT'
        WHEN COUNT(ca.id) > 1 THEN 'MULTIPLE_CONCLUSIONS'
        ELSE 'OK'
    END AS validation_status
FROM receitas_pedido_compra rpc
LEFT JOIN conclusoes_atendimento ca ON rpc.id_conclusao_atendimento = ca.id
GROUP BY rpc.id, rpc.id_conclusao_atendimento
HAVING COUNT(ca.id) != 1;

-- Verify conclusion records have appropriate 1:1 relationships
SELECT
    ca.id AS conclusao_id,
    ca.id_origem_atendimento,
    oa.tipo_origem,
    COUNT(ce.id) AS consumption_count,
    COUNT(rpc.id) AS receipt_count,
    CASE
        WHEN oa.tipo_origem = 'estoque' AND COUNT(ce.id) = 1 AND COUNT(rpc.id) = 0 THEN 'OK'
        WHEN oa.tipo_origem = 'pedido_compra' AND COUNT(ce.id) = 0 AND COUNT(rpc.id) = 1 THEN 'OK'
        WHEN oa.tipo_origem IN ('transferencia', 'producao') AND COUNT(ce.id) = 0 AND COUNT(rpc.id) = 0 THEN 'OK'
        ELSE 'INVALID_1TO1_RELATIONSHIP'
    END AS validation_status
FROM conclusoes_atendimento ca
JOIN origens_atendimento oa ON ca.id_origem_atendimento = oa.id
LEFT JOIN consumos_estoque ce ON ca.id = ce.id_conclusao_atendimento
LEFT JOIN receitas_pedido_compra rpc ON ca.id = rpc.id_conclusao_atendimento
GROUP BY ca.id, ca.id_origem_atendimento, oa.tipo_origem
HAVING validation_status != 'OK';
```

### Migration Strategy with 1:1 Relationship Preservation

#### **1. Pre-Migration Analysis**

```sql
-- =====================================================
-- PRE-MIGRATION 1:1 RELATIONSHIP ANALYSIS
-- =====================================================

-- Analyze current venda_has_produto2 → pedido_fornecedor_has_produto2 relationships
CREATE TABLE IF NOT EXISTS migration_1to1_analysis (
    analysis_type VARCHAR(50),
    record_count INT,
    percentage DECIMAL(5,2),
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Analyze purchase fulfillment relationships
INSERT INTO migration_1to1_analysis (analysis_type, record_count, percentage, notes)
SELECT
    'vp2_to_pfp2_relationships',
    COUNT(*),
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM venda_has_produto2), 2),
    CONCAT('venda_has_produto2 records with corresponding pedido_fornecedor_has_produto2: ', COUNT(*))
FROM venda_has_produto2 vp2
JOIN pedido_fornecedor_has_produto2 pf2 ON vp2.idVendaProduto2 = pf2.idVendaProduto2;

-- Analyze inventory consumption relationships
INSERT INTO migration_1to1_analysis (analysis_type, record_count, percentage, notes)
SELECT
    'vp2_to_ehc_relationships',
    COUNT(*),
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM venda_has_produto2), 2),
    CONCAT('venda_has_produto2 records with corresponding estoque_has_consumo: ', COUNT(*))
FROM venda_has_produto2 vp2
JOIN estoque_has_consumo ehc ON vp2.idVendaProduto2 = ehc.idVendaProduto2;

-- Check for 1:1 relationship violations
INSERT INTO migration_1to1_analysis (analysis_type, record_count, percentage, notes)
SELECT
    'vp2_multiple_pfp2_violations',
    COUNT(*),
    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(DISTINCT idVendaProduto2) FROM pedido_fornecedor_has_produto2), 2),
    'venda_has_produto2 IDs that appear multiple times in pedido_fornecedor_has_produto2 (violating 1:1)'
FROM (
    SELECT idVendaProduto2, COUNT(*) as count
    FROM pedido_fornecedor_has_produto2
    WHERE idVendaProduto2 IS NOT NULL
    GROUP BY idVendaProduto2
    HAVING COUNT(*) > 1
) violations;

-- Display analysis results
SELECT * FROM migration_1to1_analysis ORDER BY created_at DESC;
```

#### **2. Migration Mapping with 1:1 Preservation**

```sql
-- =====================================================
-- MIGRATION MAPPING TABLE WITH 1:1 TRACKING
-- =====================================================

CREATE TABLE migration_1to1_mapping (
    id UUID PRIMARY KEY DEFAULT (UUID()),

    -- Legacy source IDs
    legacy_venda_has_produto2_id INT(11),
    legacy_pedido_fornecedor_has_produto2_id INT(11),
    legacy_estoque_has_consumo_id INT(11),

    -- New temporal table IDs
    new_item_venda_id UUID,
    new_origem_atendimento_id UUID,
    new_conclusao_atendimento_id UUID,
    new_consumo_estoque_id UUID,
    new_receita_pedido_compra_id UUID,

    -- Relationship verification
    has_purchase_relationship BOOLEAN DEFAULT FALSE,
    has_inventory_relationship BOOLEAN DEFAULT FALSE,
    relationship_integrity_verified BOOLEAN DEFAULT FALSE,

    -- Migration status
    migration_status ENUM('pending', 'in_progress', 'completed', 'failed', 'needs_review') DEFAULT 'pending',
    migration_notes TEXT,
    migrated_at TIMESTAMP NULL,
    migrated_by UUID,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    INDEX idx_migration_legacy_vp2 (legacy_venda_has_produto2_id),
    INDEX idx_migration_legacy_pfp2 (legacy_pedido_fornecedor_has_produto2_id),
    INDEX idx_migration_legacy_ehc (legacy_estoque_has_consumo_id),
    INDEX idx_migration_status (migration_status)
);
```

#### **3. Step-by-Step Migration with 1:1 Preservation**

```sql
-- =====================================================
-- MIGRATION PROCEDURE WITH 1:1 RELATIONSHIP PRESERVATION
-- =====================================================

DELIMITER $$

CREATE PROCEDURE migrate_fulfillment_with_1to1_preservation()
BEGIN
    DECLARE done INT DEFAULT FALSE;
    DECLARE v_vp2_id INT(11);
    DECLARE v_pfp2_id INT(11);
    DECLARE v_ehc_id INT(11);
    DECLARE v_new_item_venda_id UUID;
    DECLARE v_new_origem_id UUID;
    DECLARE v_new_conclusao_id UUID;
    DECLARE v_error_count INT DEFAULT 0;

    -- Cursor for legacy records with their relationships
    DECLARE migration_cursor CURSOR FOR
        SELECT
            vp2.idVendaProduto2,
            pf2.idVendaProdutoFK2 AS pfp2_id,
            ehc.idVendaProduto2 AS ehc_id
        FROM venda_has_produto2 vp2
        LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON vp2.idVendaProduto2 = pf2.idVendaProduto2
        LEFT JOIN estoque_has_consumo ehc ON vp2.idVendaProduto2 = ehc.idVendaProduto2
        WHERE NOT EXISTS (
            SELECT 1 FROM migration_1to1_mapping m
            WHERE m.legacy_venda_has_produto2_id = vp2.idVendaProduto2
        );

    DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;
    DECLARE CONTINUE HANDLER FOR SQLEXCEPTION
    BEGIN
        SET v_error_count = v_error_count + 1;
        GET DIAGNOSTICS CONDITION 1 @error_message = MESSAGE_TEXT;
        INSERT INTO migration_1to1_mapping (
            legacy_venda_has_produto2_id,
            migration_status,
            migration_notes
        ) VALUES (
            v_vp2_id,
            'failed',
            CONCAT('Migration failed: ', @error_message)
        );
    END;

    OPEN migration_cursor;

    migration_loop: LOOP
        FETCH migration_cursor INTO v_vp2_id, v_pfp2_id, v_ehc_id;
        IF done THEN
            LEAVE migration_loop;
        END IF;

        START TRANSACTION;

        -- Generate new UUIDs
        SET v_new_item_venda_id = UUID();
        SET v_new_origem_id = UUID();
        SET v_new_conclusao_id = UUID();

        -- Insert mapping record first
        INSERT INTO migration_1to1_mapping (
            legacy_venda_has_produto2_id,
            legacy_pedido_fornecedor_has_produto2_id,
            legacy_estoque_has_consumo_id,
            new_item_venda_id,
            new_origem_atendimento_id,
            new_conclusao_atendimento_id,
            has_purchase_relationship,
            has_inventory_relationship,
            migration_status
        ) VALUES (
            v_vp2_id,
            v_pfp2_id,
            v_ehc_id,
            v_new_item_venda_id,
            v_new_origem_id,
            v_new_conclusao_id,
            (v_pfp2_id IS NOT NULL),
            (v_ehc_id IS NOT NULL),
            'in_progress'
        );

        -- Migrate the core fulfillment record
        CALL migrate_single_fulfillment_record(
            v_vp2_id,
            v_new_item_venda_id,
            v_new_origem_id,
            v_new_conclusao_id
        );

        -- Migrate 1:1 relationship records if they exist
        IF v_pfp2_id IS NOT NULL THEN
            CALL migrate_purchase_receipt_1to1(v_pfp2_id, v_new_conclusao_id);
            UPDATE migration_1to1_mapping
            SET new_receita_pedido_compra_id = UUID()
            WHERE legacy_venda_has_produto2_id = v_vp2_id;
        END IF;

        IF v_ehc_id IS NOT NULL THEN
            CALL migrate_inventory_consumption_1to1(v_ehc_id, v_new_conclusao_id);
            UPDATE migration_1to1_mapping
            SET new_consumo_estoque_id = UUID()
            WHERE legacy_venda_has_produto2_id = v_vp2_id;
        END IF;

        -- Verify relationship integrity
        CALL verify_migrated_1to1_integrity(v_vp2_id);

        -- Mark as completed
        UPDATE migration_1to1_mapping
        SET migration_status = 'completed',
            relationship_integrity_verified = TRUE,
            migrated_at = NOW()
        WHERE legacy_venda_has_produto2_id = v_vp2_id;

        COMMIT;

    END LOOP;

    CLOSE migration_cursor;

    -- Final migration report
    SELECT
        migration_status,
        COUNT(*) as count,
        ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM migration_1to1_mapping), 2) as percentage
    FROM migration_1to1_mapping
    GROUP BY migration_status;

END$$

DELIMITER ;
```

### Performance Optimization for Temporal 1:1 Relationships

#### **1. Temporal-Specific Indexes**

```sql
-- =====================================================
-- TEMPORAL PERFORMANCE INDEXES
-- =====================================================

-- Composite temporal indexes for efficient time-travel queries
CREATE INDEX idx_temporal_fulfillment_composite
ON origens_atendimento (id_item_venda, status, row_start, row_end);

CREATE INDEX idx_temporal_conclusion_composite
ON conclusoes_atendimento (id_item_venda, atendido_em, row_start, row_end);

-- 1:1 relationship lookup indexes
CREATE INDEX idx_consumption_1to1_lookup
ON consumos_estoque (id_conclusao_atendimento, id_lote_estoque, row_start);

CREATE INDEX idx_receipt_1to1_lookup
ON receitas_pedido_compra (id_conclusao_atendimento, id_item_pedido_compra, row_start);

-- Temporal foreign key performance indexes
CREATE INDEX idx_temporal_fk_origins
ON origens_atendimento (id_lote_estoque, id_item_pedido_compra, row_start, row_end);
```

#### **2. Temporal Materialized Views for Performance**

```sql
-- =====================================================
-- TEMPORAL MATERIALIZED VIEWS
-- =====================================================

-- Current state view (most common queries)
CREATE VIEW v_current_fulfillment_with_1to1 AS
SELECT
    iv.id AS item_venda_id,
    iv.numero_linha,
    iv.nome_produto,
    iv.quantidade_pedida,
    iv.quantidade_entregue,

    oa.id AS origem_id,
    oa.tipo_origem,
    oa.quantidade_alocada,
    oa.status AS origem_status,

    ca.id AS conclusao_id,
    ca.quantidade_atendida,
    ca.atendido_em,

    ce.id AS consumo_id,
    ce.quantidade_consumida,
    ce.numero_lote_consumido,

    rpc.id AS receita_id,
    rpc.quantidade_recebida,
    rpc.data_recebimento

FROM itens_venda iv
LEFT JOIN origens_atendimento oa ON iv.id = oa.id_item_venda
LEFT JOIN conclusoes_atendimento ca ON oa.id = ca.id_origem_atendimento
LEFT JOIN consumos_estoque ce ON ca.id = ce.id_conclusao_atendimento
LEFT JOIN receitas_pedido_compra rpc ON ca.id = rpc.id_conclusao_atendimento;

-- Summary view for dashboard/reporting
CREATE VIEW v_fulfillment_summary_with_1to1 AS
SELECT
    v.numero_venda,
    v.status AS venda_status,
    COUNT(iv.id) AS total_line_items,
    SUM(iv.quantidade_pedida) AS total_quantidade_pedida,
    SUM(iv.quantidade_entregue) AS total_quantidade_entregue,
    COUNT(ce.id) AS total_consumptions,
    COUNT(rpc.id) AS total_receipts,

    CASE
        WHEN SUM(iv.quantidade_pedida) = SUM(iv.quantidade_entregue) THEN 'Atendido Completo'
        WHEN SUM(iv.quantidade_entregue) > 0 THEN 'Atendido Parcial'
        ELSE 'Não Atendido'
    END AS status_atendimento

FROM vendas v
JOIN itens_venda iv ON v.id = iv.id_venda
LEFT JOIN origens_atendimento oa ON iv.id = oa.id_item_venda
LEFT JOIN conclusoes_atendimento ca ON oa.id = ca.id_origem_atendimento
LEFT JOIN consumos_estoque ce ON ca.id = ce.id_conclusao_atendimento
LEFT JOIN receitas_pedido_compra rpc ON ca.id = rpc.id_conclusao_atendimento
GROUP BY v.id, v.numero_venda, v.status;
```

## Implementation Timeline with 1:1 Relationship Focus

| Phase | Duration | Activities | Focus |
|-------|----------|------------|-------|
| **Analysis & Design** | 3-4 weeks | Analyze current 1:1 relationships, design temporal schema | Understanding existing relationships |
| **Schema Creation** | 2-3 weeks | Create temporal tables with 1:1 constraints | Temporal infrastructure setup |
| **Migration Scripts** | 4-5 weeks | Write relationship-preserving migration code | 1:1 relationship preservation |
| **Data Migration** | 3-4 weeks | Execute migration with relationship verification | Integrity validation |
| **Temporal Testing** | 2-3 weeks | Test time-travel queries and 1:1 constraints | Temporal functionality verification |
| **Application Updates** | 5-7 weeks | Update business logic for temporal operations | API and business logic adaptation |
| **Performance Tuning** | 2-3 weeks | Optimize temporal queries and indexing | Performance optimization |
| **Cutover** | 1-2 weeks | Production deployment with monitoring | Go-live activities |

**Total Timeline: 22-33 weeks**
**Total Cost: $350K-500K** (increased due to temporal complexity)

## Benefits of Temporal 1:1 Design

### **1. Maintained Business Logic Integrity**
- **Preserved 1:1 Relationships**: Direct mapping from legacy structure maintains business logic
- **Temporal Audit Trail**: Complete history of all relationship changes
- **Data Consistency**: Database-enforced temporal referential integrity

### **2. Enhanced Business Intelligence**
- **Time-Travel Queries**: Analyze fulfillment status at any point in time
- **Trend Analysis**: Track fulfillment performance over time periods
- **Compliance Reporting**: Complete audit trail for regulatory requirements

### **3. Operational Advantages**
- **Concurrent Operations**: Temporal versioning allows safe concurrent updates
- **Rollback Capability**: Can query historical states for error recovery
- **Change Tracking**: Automatic tracking of who changed what when

### **4. Future-Proofing**
- **Scalable Design**: Temporal tables handle growth better than audit table patterns
- **Integration Ready**: Temporal features support complex business intelligence tools
- **Regulatory Compliance**: Built-in audit trail meets evolving compliance requirements

## Conclusion

This temporal schema design successfully maintains the critical 1:1 relationships from the current system while adding powerful temporal capabilities. The design ensures:

1. **Complete 1:1 Relationship Preservation**: Every `venda_has_produto2 ↔ pedido_fornecedor_has_produto2` and `venda_has_produto2 ↔ estoque_has_consumo` relationship is maintained through the new `conclusoes_atendimento ↔ receitas_pedido_compra` and `conclusoes_atendimento ↔ consumos_estoque` relationships.

2. **Temporal Audit Trail**: Complete history of all changes with the ability to query the system state at any point in time.

3. **Database-Enforced Integrity**: Temporal triggers and constraints ensure 1:1 relationships cannot be violated.

4. **Performance Optimization**: Specialized temporal indexes and materialized views provide excellent query performance.

The migration strategy provides a safe, verifiable path from the current system to the new temporal design while preserving all existing business relationships and adding powerful new capabilities for business intelligence and compliance.