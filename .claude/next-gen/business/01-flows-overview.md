# Business Process Flows - Complete Analysis

> Status: **Analyzed from codebase**
> Last updated: 2025-12-27
> Source: Deep analysis of C++ codebase

---

## Executive Summary

The ERP has **6 main interconnected flows**:

```mermaid
flowchart TB
    subgraph Cadastros["CADASTROS (Master Data)"]
        Fornecedor
        Produto
        Cliente
    end

    subgraph SalesFlow["SALES FLOW"]
        Orcamento[Orçamento] --> Venda
        Venda --> Compra["Compra (per supplier)"]
    end

    subgraph PurchaseFlow["PURCHASE FLOW"]
        Compra --> NFeEntrada["NFe Entrada"]
        NFeEntrada --> Estoque["Estoque (Receipt)"]
    end

    subgraph FulfillmentFlow["FULFILLMENT FLOW"]
        Venda --> Consumo["Consumo Estoque"]
        Estoque --> Consumo
        Consumo --> NFeSaida["NFe Saída"]
        NFeSaida --> Entrega["Entrega (Delivery)"]
    end

    subgraph Financeiro["FINANCEIRO"]
        ContasReceber["Contas a Receber"] <--> ContasPagar["Contas a Pagar"]
    end

    Entrega --> Financeiro
    Compra --> ContasPagar
    Venda --> ContasReceber
```

---

## Table of Contents

1. [Two-Level Table Architecture](#1-two-level-table-architecture)
2. [Orçamento → Venda Flow](#2-orçamento--venda-flow)
3. [Venda → Compra Flow](#3-venda--compra-flow)
4. [Compra → Estoque Flow](#4-compra--estoque-flow)
5. [Estoque → Consumo Flow](#5-estoque--consumo-flow)
6. [NFe Flow](#6-nfe-flow)
7. [Financial Flow](#7-financial-flow)
8. [Complete Status Reference](#8-complete-status-reference)
9. [Data Integrity Rules](#9-data-integrity-rules)
10. [Known Problems](#10-known-problems)

---

## 1. Two-Level Table Architecture

### Why Two Levels Exist

The system uses a **two-level hierarchy** for both sales and purchases:

```mermaid
flowchart LR
    subgraph L1["LEVEL 1 (Quote/Order)"]
        VHP[venda_has_produto]
        PFHP[pedido_fornecedor_has_produto]
    end

    subgraph L2["LEVEL 2 (Fulfillment/Delivery)"]
        VHP2[venda_has_produto2]
        PFHP2[pedido_fornecedor_has_produto2]
    end

    VHP --> VHP2
    PFHP --> PFHP2
```

### Purpose of Each Level

| Aspect | Level 1 | Level 2 |
|--------|---------|---------|
| **Purpose** | What was ordered | How it's being fulfilled |
| **Granularity** | One per product in quote | **Can have MULTIPLE per L1** (split deliveries) |
| **Status** | Order-level status | Item-level workflow status |
| **NFe Links** | None | 3 NFe references (Entrada, Saída, Futura) |
| **Estoque Link** | None | Links to estoque_has_consumo |
| **Dates** | Order dates | All workflow dates (6 date pairs) |

### Split Delivery Example

```
Customer orders: 100 units of Product A (venda_has_produto qty=100)

Split into 3 deliveries:
├── venda_has_produto2 [qty=40] → status=ENTREGUE (delivered)
├── venda_has_produto2 [qty=35] → status=EM ENTREGA (in transit)
└── venda_has_produto2 [qty=25] → status=EM COMPRA (being purchased)

Each L2 tracks independently:
- Own status progression
- Own purchase link (idCompra)
- Own stock consumption records
- Own NFe references
```

### Relationship Diagram

```mermaid
flowchart TB
    Venda["venda (header)"]

    Venda --> VHP["venda_has_produto (L1)"]
    Venda --> CAR["conta_a_receber_has_pagamento"]

    VHP --> VHP2_1["venda_has_produto2 (L2) #1"]
    VHP --> VHP2_2["venda_has_produto2 (L2) #2"]
    VHP --> PFHP["pedido_fornecedor_has_produto (L1)"]

    VHP2_1 --> EHC1["estoque_has_consumo"]
    VHP2_1 --> NFeSaida["idNFeSaida"]
    VHP2_1 --> NFeEntrada["idNFeEntrada"]
    VHP2_1 --> NFeFutura["idNFeFutura"]

    VHP2_2 --> EHC2["estoque_has_consumo"]

    PFHP --> PFHP2["pedido_fornecedor_has_produto2 (L2)"]
    PFHP2 --> EHCompra["estoque_has_compra"]
```

### Key Insight

**Level 2 is the "workhorse"** - it tracks the actual fulfillment:
- `venda_has_produto2.idVendaProduto2` is THE KEY that connects:
  - What inventory was consumed (`estoque_has_consumo`)
  - What was purchased (`pedido_fornecedor_has_produto2`)
  - What NFe was issued (`idNFeSaida`)

---

## 2. Orçamento → Venda Flow

### Status Values: Orçamento

| Status | Description | Transitions To |
|--------|-------------|----------------|
| `ATIVO` | Active quote, can be converted | FECHADO, EXPIRADO, PERDIDO |
| `EXPIRADO` | Past validity date | REPLICADO (if replicated) |
| `REPLICADO` | Source of a replica quote | - |
| `FECHADO` | Converted to Venda | - |
| `PERDIDO` | Manually marked as lost | - |

### Conversion Flow

```mermaid
flowchart TB
    Orc["ORÇAMENTO (ATIVO)"]

    Orc -->|"User clicks 'Gerar Venda'"| Val

    subgraph Val["VALIDATION"]
        V1["Quote not expired"]
        V2["Delivery address selected"]
        V3["Customer registration complete"]
    end

    Val --> Copy

    subgraph Copy["COPY PROCESS"]
        C1["1. Copy header: cliente, vendedor, endereços, valores"]
        C2["2. Copy items: orcamento_has_produto → venda_has_produto"]
        C3["3. Create L2: for each item → venda_has_produto2"]
        C4["4. Set initial status: ESTOQUE or PENDENTE"]
    end

    Copy --> Final

    subgraph Final["FINALIZATION"]
        F1["Orçamento.status = 'FECHADO'"]
        F2["Venda created with idOrcamento link"]
        F3["Venda dialog opens"]
    end
```

### Data Transformation

```
ORÇAMENTO                          VENDA
─────────                          ─────
idOrcamento          ───────────►  idOrcamento (FK)
idCliente            ───────────►  idCliente
idEnderecoEntrega    ───────────►  idEnderecoEntrega
idEnderecoFaturamento ──────────►  idEnderecoFaturamento
idProfissional       ───────────►  idProfissional
idUsuario            ───────────►  idUsuario
subTotalBru          ───────────►  subTotalBru
subTotalLiq          ───────────►  subTotalLiq
frete                ───────────►  frete
descontoPorc         ───────────►  descontoPorc
descontoReais        ───────────►  descontoReais
total                ───────────►  total
prazoEntrega         ───────────►  prazoEntrega
representacao        ───────────►  representacao
                     NEW ───────►  status = 'ATIVO'
                     NEW ───────►  data (sale date)
```

---

## 3. Venda → Compra Flow

### When Compras Are Generated

Compras (purchase orders) are generated when:
1. User opens "Gerar Compra" tab in Compras module
2. Selects pending items (status = INICIADO or PENDENTE)
3. Clicks "Gerar Compra" button

### Status Values: Venda Item (venda_has_produto2)

| Status | Description | Next Status |
|--------|-------------|-------------|
| `INICIADO` | Initial state after venda creation | EM COMPRA, ESTOQUE |
| `PENDENTE` | Waiting for action | EM COMPRA, ESTOQUE |
| `EM COMPRA` | Purchase order generated | EM FATURAMENTO |
| `EM FATURAMENTO` | Supplier confirmed/dispatched | EM ENTREGA |
| `EM ENTREGA` | Goods in transit | EM RECEBIMENTO, ESTOQUE |
| `EM RECEBIMENTO` | Being received at warehouse | ESTOQUE |
| `ESTOQUE` | In stock, ready for delivery | ENTREGA AGEND. |
| `ENTREGA AGEND.` | Delivery scheduled | EM ENTREGA (to customer) |
| `EM ENTREGA` | Out for delivery | ENTREGUE |
| `ENTREGUE` | Delivered to customer | (final) |
| `CANCELADO` | Cancelled | (final) |
| `DEVOLVIDO` | Returned | (final) |

### Flow Diagram

```mermaid
flowchart TB
    VHP2["venda_has_produto2<br/>status=INICIADO or PENDENTE"]

    VHP2 -->|"Gerar Compra clicked"| Process

    subgraph Process["FOR EACH SUPPLIER (grouped)"]
        P1["1. Generate new idCompra"]
        P2["2. Create pedido_fornecedor_has_produto (L1)"]
        P3["3. Create pedido_fornecedor_has_produto2 (L2)"]
        P4["4. Update venda_has_produto2: status='EM COMPRA'"]
        P5["5. Generate Excel purchase order"]
        P6["6. Send email to supplier"]
    end

    Process --> Result

    subgraph Result["LINKED RECORDS"]
        VHP2_Result["venda_has_produto2<br/>status=EM COMPRA<br/>idCompra=XXX"]
        PFHP2_Result["pedido_fornecedor_has_produto2<br/>status=PENDENTE<br/>idVendaProduto2=YYY"]
        VHP2_Result <-.->|"LINKED"| PFHP2_Result
    end
```

### Key: idCompra Links Everything

```mermaid
flowchart LR
    idCompra((idCompra))
    idCompra --> VHP2["venda_has_produto2.idCompra"]
    idCompra --> PFHP2["pedido_fornecedor_has_produto2.idCompra"]
    idCompra --> EHC["estoque_has_compra.idCompra"]
    idCompra --> CAP["conta_a_pagar_has_idcompra.idCompra"]
```

---

## 4. Compra → Estoque Flow

### Purchase Confirmation Steps

```mermaid
flowchart TB
    subgraph Step1["STEP 1: CONFIRMAR COMPRA"]
        S1A["Supplier confirms dispatch"]
        S1B["status = 'EM FATURAMENTO'"]
        S1C["dataRealConf = confirmation date"]
    end

    subgraph Step2["STEP 2: FATURAR"]
        S2A["Import supplier NFe XML"]
        S2B["Validate NFe against PO"]
        S2C["status = 'EM ENTREGA'"]
        S2D["idNFeEntrada = NFe ID"]
    end

    subgraph Step3["STEP 3: COLETA"]
        S3A["Pickup from supplier"]
        S3B["status = 'EM COLETA'"]
        S3C["dataRealColeta = pickup date"]
    end

    subgraph Step4["STEP 4: RECEBIMENTO"]
        S4A["Create estoque records"]
        S4B["Create estoque_has_compra links"]
        S4C["Assign warehouse location"]
        S4D["status = 'ESTOQUE'"]
    end

    Step1 --> Step2 --> Step3 --> Step4
```

### Estoque Record Creation

```sql
-- Created during NFe import / receiving
INSERT INTO estoque (
    idNFe,              -- Supplier's NFe
    idProduto,
    fornecedor,         -- Supplier name (denormalized)
    descricao,          -- Product description
    status,             -- 'ESTOQUE'
    quant,              -- Quantity received
    restante,           -- Available quantity (= quant initially)
    valorUnid,          -- Unit cost from NFe
    idBloco,            -- Warehouse location
    lote,               -- Batch/lot number
    -- All tax fields from NFe...
    vBC, pICMS, vICMS, vIPI, vPIS, vCOFINS...
)
```

---

## 5. Estoque → Consumo Flow

### Stock Consumption Logic

When a sale is ready for delivery, stock is "consumed" (allocated):

```mermaid
flowchart TB
    Trigger["TRIGGER: venda_has_produto2<br/>moves to ESTOQUE or shipping"]

    Trigger --> Function

    subgraph Function["Estoque::criarConsumo(idVendaProduto2, quantidade)"]
        F1["1. Find available stock (FIFO):<br/>ORDER BY data_entrada ASC"]
        F2["2. For each batch until fulfilled:<br/>• Create estoque_has_consumo<br/>• Update estoque.restante"]
        F3["3. Link to purchase order:<br/>• Set idVenda, idVendaProduto2<br/>• Split if partial"]
        F4["4. Copy lot number"]
        F1 --> F2 --> F3 --> F4
    end

    Function --> Result

    subgraph Result["RESULT: estoque_has_consumo"]
        R1["idEstoque = stock batch"]
        R2["idVendaProduto2 = sale line"]
        R3["quant = NEGATIVE value"]
        R4["status = 'CONSUMO'"]
    end
```

### Important: Negative Quantities

Stock consumption is stored as **NEGATIVE** values in `estoque_has_consumo.quant`:

```
estoque.quant = 100          (original received)
estoque.restante = 100       (available)

After consumption of 40:
estoque_has_consumo.quant = -40   (NEGATIVE!)
estoque.restante = 60             (updated)
```

### Consumption Reversal

When a sale is cancelled or item returned:

```cpp
// Estoque::desfazerConsumo()
1. DELETE FROM estoque_has_consumo WHERE idVendaProduto2 = ?
2. UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, idVendaProduto2 = NULL
3. UPDATE venda_has_produto2 SET status = 'PENDENTE', lote = NULL, idCompra = NULL
4. Recalculate estoque.restante
```

---

## 6. NFe Flow

### NFe Types

| Type | Direction | Purpose |
|------|-----------|---------|
| `SAIDA` | Outgoing | Invoice TO customer |
| `ENTRADA` | Incoming | Invoice FROM supplier |
| `FUTURA` | Scheduled | Future delivery invoice |
| `DEVOLUCAO` | Return | Return credit note |

### NFe Status Values

| Status | Description |
|--------|-------------|
| `NOTA PENDENTE` | Pre-registered, waiting SEFAZ |
| `AUTORIZADA` | Approved by SEFAZ |
| `DENEGADA` | Rejected by SEFAZ |
| `CANCELADA` | Cancelled after approval |
| `RESUMO` | Summary from manifest (not full XML) |
| `INUTILIZADA` | Voided/unused number |

### NFe Emission Flow (Saída)

```mermaid
flowchart TB
    S1["1. User opens CadastrarNFe dialog"]
    S2["2. validarDados()<br/>Check emitter, recipient, products, totals"]
    S3["3. criarChaveAcesso()<br/>Generate 44-char access key"]
    S4["4. preCadastrarNota()<br/>status='NOTA PENDENTE'"]
    S5["5. montarXML() + gerarNota()<br/>Build XML via ACBr"]
    S6["6. enviarNFe()<br/>Send to SEFAZ"]
    S7["7. processarResposta()"]

    S1 --> S2 --> S3 --> S4 --> S5 --> S6 --> S7

    S7 -->|AUTORIZADA| Auth["status='AUTORIZADA'"]
    S7 -->|DENEGADA| Den["status='DENEGADA'"]
    S7 -->|ERROR| Err["Show error, retry"]

    Auth --> S8["8. Update idNFeSaida"]
```

### NFe Import Flow (Entrada)

```mermaid
flowchart TB
    I1["1. User opens ImportarXML dialog<br/>Select from DB or browse XML"]
    I2["2. Parse XML<br/>Products, taxes, duplicatas"]
    I3["3. parear() - Match to PO items"]
    I4["4. User confirms matching"]
    I5["5. cadastrarNFe()<br/>Create stock records"]

    I1 --> I2 --> I3 --> I4 --> I5

    I3 --> Green["🟢 Exact match"]
    I3 --> Yellow["🟡 Partial match"]
    I3 --> Red["🔴 No match"]

    I5 --> R1["INSERT estoque"]
    I5 --> R2["INSERT estoque_has_compra"]
    I5 --> R3["UPDATE nfe.utilizada = TRUE"]
    I5 --> R4["UPDATE status = 'EM COLETA'"]
```

### Three NFe References per Sale Item

```mermaid
flowchart LR
    VHP2["venda_has_produto2"]
    VHP2 --> NFeSaida["idNFeSaida<br/>TO customer"]
    VHP2 --> NFeEntrada["idNFeEntrada<br/>FROM supplier"]
    VHP2 --> NFeFutura["idNFeFutura<br/>Future delivery"]
```

---

## 7. Financial Flow

### Contas a Receber (Receivables)

**Created**: Automatically when Venda is saved/confirmed

```mermaid
flowchart TB
    Trigger["TRIGGER: Venda.montarFluxoCaixa() on save"]

    Trigger --> ForEach["FOR EACH payment method"]

    subgraph ForEach["FOR EACH payment method"]
        GetConfig["1. Get forma_pagamento config<br/>idConta, parcelas, flags, taxa"]

        subgraph Parcelas["2. FOR EACH installment"]
            CalcDate["Calculate due date<br/>dMaisUm, pula1Mes, ajustaDiaUtil"]
            CalcVal["Calculate value<br/>total / parcelas"]
            Insert["INSERT conta_a_receber_has_pagamento<br/>status='PENDENTE'"]
            CalcDate --> CalcVal --> Insert
        end

        CardFee["3. IF card: INSERT Taxa Cartão<br/>valor = negative"]

        GetConfig --> Parcelas --> CardFee
    end
```

### Contas a Pagar (Payables)

**Created**: At purchase CONFIRMATION step (`widgetcompraconfirmar`)

```mermaid
flowchart TB
    Trigger["TRIGGER: User confirms purchase"]

    Trigger --> P1["1. Import supplier NFe (XML)"]
    P1 --> P2["2. Validate NFe against PO"]
    P2 --> P3["3. Extract duplicatas"]

    P3 --> ForEach

    subgraph ForEach["4. FOR EACH duplicata"]
        Insert["INSERT conta_a_pagar_has_pagamento<br/>contraParte = fornecedor<br/>status = 'PENDENTE'"]
    end

    ForEach --> Comm

    subgraph Comm["ALSO: Commissions"]
        RT["INSERT conta_a_pagar<br/>grupo = 'RT's'"]
    end
```

### Financial Status Values

| Status | Description |
|--------|-------------|
| `PENDENTE` | Not yet paid |
| `CONFERIDO` | Verified/confirmed |
| `AGENDADO` | Scheduled for payment |
| `RECEBIDO` | Payment received (receivables) |
| `PAGO` | Payment made (payables) |
| `CANCELADO` | Cancelled |
| `PAGO GARE` | Tax payment completed |

### Payment Recording

```
User edits dataRealizado column in Contas dialog
    │
    ▼
preencher() method auto-populates:
    • status = 'RECEBIDO' or 'PAGO'
    • valorReal = valor (actual amount)
    • tipoReal = tipo (actual method)
    • idConta = bank account
    • centroCusto = cost center
    │
    ▼
IF card payment:
    • Find matching "Taxa Cartão" record
    • Update that record identically
```

---

## 8. Complete Status Reference

### All Status Columns by Table

| Table | Column | Values |
|-------|--------|--------|
| `venda` | status | ATIVO, CANCELADO, ENTREGUE, DEVOLVIDO |
| `venda` | statusFinanceiro | PENDENTE, CONFERIDO, LIBERADO, PAGO, CANCELADO |
| `venda_has_produto2` | status | INICIADO, PENDENTE, EM COMPRA, EM FATURAMENTO, EM ENTREGA, EM RECEBIMENTO, EM COLETA, ESTOQUE, ENTREGA AGEND., ENTREGUE, CANCELADO, DEVOLVIDO, DEVOLVIDO ESTOQUE, QUEBRADO, REPO. ENTREGA, REPO. RECEB. |
| `pedido_fornecedor_has_produto` | status | PENDENTE, CONFIRMADO, FATURADO, CANCELADO |
| `pedido_fornecedor_has_produto2` | status | (same as venda_has_produto2) |
| `compra_avulsa` | status | PEND. APROV., CONFERIDO, COMPRADO, CANCELADO |
| `estoque` | status | TEMP, ESTOQUE, CANCELADO |
| `estoque_has_consumo` | status | TEMP, CONSUMO, AJUSTE, DEVOLVIDO |
| `nfe` | status | NOTA PENDENTE, AUTORIZADA, DENEGADA, CANCELADA, RESUMO, INUTILIZADA |
| `conta_a_receber` | status | PENDENTE, CONFERIDO, AGENDADO, RECEBIDO, CANCELADO |
| `conta_a_pagar` | status | PENDENTE, CONFERIDO, AGENDADO, PAGO, CANCELADO, PAGO GARE, PENDENTE GARE, LIBERADO GARE |
| `orcamento` | status | ATIVO, EXPIRADO, REPLICADO, FECHADO, PERDIDO |

### Complete Item Status Flow

```mermaid
stateDiagram-v2
    [*] --> INICIADO

    INICIADO --> EM_COMPRA : Generate PO
    INICIADO --> ESTOQUE : Stock exists
    INICIADO --> CANCELADO : Cancel

    EM_COMPRA --> EM_FATURAMENTO : Supplier confirms
    EM_COMPRA --> CANCELADO : Cancel

    EM_FATURAMENTO --> EM_ENTREGA_SUP : NFe received
    EM_FATURAMENTO --> CANCELADO : Cancel

    state "EM ENTREGA (supplier)" as EM_ENTREGA_SUP
    EM_ENTREGA_SUP --> EM_COLETA : Pickup
    EM_ENTREGA_SUP --> CANCELADO : Cancel

    EM_COLETA --> EM_RECEBIMENTO : Arrive warehouse

    EM_RECEBIMENTO --> ESTOQUE : Received

    ESTOQUE --> ENTREGA_AGEND : Schedule delivery

    state "ENTREGA AGEND." as ENTREGA_AGEND
    ENTREGA_AGEND --> EM_ENTREGA_CUST : Out for delivery

    state "EM ENTREGA (customer)" as EM_ENTREGA_CUST
    EM_ENTREGA_CUST --> ENTREGUE : Delivered

    ENTREGUE --> [*]
    CANCELADO --> [*]
```

---

## 9. Data Integrity Rules

### Invariants That Must ALWAYS Be True

```
FINANCIAL INTEGRITY:
──────────────────
1. SUM(conta_a_receber.valor) for venda == venda.total
2. SUM(conta_a_pagar.valor) for compra == compra.total
3. No orphan payments (always linked to transaction)

STOCK INTEGRITY:
───────────────
1. estoque.restante >= 0 (NEVER negative)
2. estoque.restante = estoque.quant + estoque.ajuste + SUM(estoque_has_consumo.quant)
3. SUM(estoque_has_consumo.quant) for item == venda_has_produto2.quant (as negative)

FLOW INTEGRITY:
──────────────
1. Venda cannot be ESTOQUE if compras not RECEBIDO (unless stock existed)
2. Compra RECEBIDO must create estoque records
3. Cancellation must reverse ALL downstream effects:
   • Delete consumos
   • Unlink purchases
   • Cancel financials
   • Reactivate orçamento (if applicable)

NFe INTEGRITY:
─────────────
1. NFe AUTORIZADA cannot be modified
2. NFe can only be CANCELADA within 24h (SEFAZ rule)
3. venda_has_produto2.idNFeSaida must point to valid NFe
```

### Status Cascade Rules

```
When purchase status changes → venda_has_produto2 status changes
    via stored procedure: update_venda_status()

When all venda_has_produto2 are ENTREGUE → venda.status = ENTREGUE
    via stored procedure: update_venda_status()

When venda cancelled →
    1. All venda_has_produto2.status = CANCELADO
    2. All conta_a_receber.status = CANCELADO
    3. All estoque_has_consumo deleted
    4. All pedido_fornecedor links cleared
    5. orcamento.status = ATIVO (reactivated)
```

---

## 10. Known Problems

### Issues Identified in Analysis

| Problem | Impact | Cause |
|---------|--------|-------|
| **Denormalized supplier names** | Updates require 5+ tables | fornecedor stored as VARCHAR, not FK |
| **No atomic transactions** | Inconsistent state possible | Separate SQL updates, not wrapped |
| **Status as strings** | No validation | Should be ENUM |
| **Two-level complexity** | Hard to query | Organic growth |
| **Payables at confirmation** | Timing issues? | Created at confirmation step, not order |
| **No audit trail** | Can't track changes | No history tables |
| **Negative quantities** | Confusing | estoque_has_consumo.quant is negative |

### Clarified Rules

1. **Can a venda item be split across multiple deliveries?**
   - **YES** - One `venda_has_produto` can have MULTIPLE `venda_has_produto2` records
   - This enables partial deliveries and split fulfillment
   - Each L2 record tracks a separate delivery/fulfillment path

2. **When are contas_a_pagar created for compras?**
   - **At confirmation step** (`widgetcompraconfirmar`)
   - This is when supplier invoice details are known

3. **What happens with partial receiving?**
   - **YES** - Can receive LESS than ordered
   - Venda item stays **partially fulfilled**
   - Need to track: quantity ordered vs quantity received

4. **Commission calculation timing?**
   - Still needs clarification
   - When is commission calculated?
   - What if sale cancelled after commission paid?

---

## Next Steps

1. [ ] Document each broken scenario specifically
2. [ ] Define atomic transaction boundaries
3. [ ] Map all stored procedures
4. [ ] Create test cases for each status transition
5. [ ] Design new normalized schema
