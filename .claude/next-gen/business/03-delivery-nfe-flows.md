# Delivery, NFe Emission & Financial Flows - Deep Analysis

> Status: **Complete**
> Last updated: 2025-12-27
> Source: Deep analysis of C++ codebase

---

## Table of Contents

1. [Delivery (Entrega) Flow](#1-delivery-entrega-flow)
2. [NFe Emission (Saída) Flow](#2-nfe-emission-saída-flow)
3. [CNAB/Bank Reconciliation Flow](#3-cnabbank-reconciliation-flow)
4. [Commission (RT) Calculation Flow](#4-commission-rt-calculation-flow)

---

## 1. Delivery (Entrega) Flow

### Status Transitions

```mermaid
stateDiagram-v2
    ESTOQUE --> ENTREGA_AGEND : Schedule delivery
    ENTREGA_AGEND --> EM_ENTREGA : NFe authorized
    EM_ENTREGA --> ENTREGUE : Confirm delivery
    state "ENTREGA AGEND." as ENTREGA_AGEND
```

### Complete Flow Diagram

```mermaid
flowchart TB
    Stock["STOCK READY<br/>status = 'ESTOQUE'"]

    Stock --> Agendar["WidgetLogisticaAgendarEntrega<br/>Select products, vehicle, date"]

    Agendar --> AgendarClick["on_pushButtonAgendarCarga_clicked()<br/>Generate idEvento<br/>UPDATE status = 'ENTREGA AGEND.'"]

    AgendarClick --> Agendado["STATUS = 'ENTREGA AGEND.'<br/>Optional: NFe Futura"]

    Agendado --> Entregas["WidgetLogisticaEntregas"]

    Entregas --> ConsultarNFe["processarConsultaNFe()<br/>UPDATE nfe status = 'AUTORIZADA'<br/>UPDATE status = 'EM ENTREGA'"]

    ConsultarNFe --> EmEntrega["STATUS = 'EM ENTREGA'"]

    EmEntrega --> Confirmar["InputDialogConfirmacao<br/>Enter: dataRealEnt, entregou, recebeu<br/>Optional: Upload photo"]

    Confirmar --> Final["STATUS = 'ENTREGUE' (Final)"]
```

### Key Tables

| Table | Purpose |
|-------|---------|
| `venda_has_produto2` | Main product status tracking |
| `pedido_fornecedor_has_produto2` | Purchase order parallel status |
| `veiculo_has_produto` | Delivery grouping by vehicle/event |
| `transportadora_has_veiculo` | Vehicle registry |

### Delivery Document Generation

Two Excel files generated from templates:
- `espelho_entrega.xlsx` → Delivery receipt
- `modelo_checklist.xlsx` → Physical verification checklist

### Broken Items Handling

```
User marks items as broken → dividirEntrega()
    │
    ├── Original line: ENTREGUE (reduced qty)
    ├── Broken line: QUEBRADO
    ├── Replacement line (optional): REPO. ENTREGA
    └── Credit record: Negative conta_a_receber entry
```

---

## 2. NFe Emission (Saída) Flow

### Entry Points

- **Types**: `Saida`, `Futura`, `SaidaAposFutura`, `Entrada`
- **Trigger**: User selects sale items → "Gerar NF-e" button
- **File**: `cadastrarnfe.cpp`

### Complete Emission Flow

```mermaid
flowchart TB
    subgraph Step1["1. DATA PREPARATION"]
        P1["validarDados()<br/>Check emitter, recipient, products, taxes"]
        P2["criarChaveAcesso()<br/>Generate 44-char key"]
        P3["Load UI models"]
        P1 --> P2 --> P3
    end

    subgraph Step2["2. XML GENERATION"]
        X1["montarXML()"]
        X2["writeIdentificacao(), writeEmitente()<br/>writeDestinatario(), writeProduto()"]
        X3["Tax sections: ICMS, IPI, PIS<br/>COFINS, IBSCBS (2025), ISel"]
        X4["gerarNota(acbr)"]
        X1 --> X2 --> X3 --> X4
    end

    subgraph Step3["3. ACBr COMMUNICATION"]
        A1["TCP Socket to ACBr Monitor"]
        A2["NFE.CriarNFe()"]
        A3["validarSchema()"]
        A1 --> A2 --> A3
    end

    subgraph Step4["4. PRE-REGISTRATION"]
        R1["preCadastrarNota()<br/>INSERT nfe status='NOTA PENDENTE'"]
        R2["UPDATE venda_has_produto2<br/>idNFeSaida = :idNFe"]
        R1 --> R2
    end

    subgraph Step5["5. SEFAZ TRANSMISSION"]
        S1["enviarNFe()"]
        S2["processarResposta()"]
        S1 --> S2
        S2 -->|REJECTION| Retry["Delete & retry"]
        S2 -->|AUTHORIZED| Auth["Update status"]
    end

    subgraph Step6["6. POST-AUTHORIZATION"]
        F1["atualizarNFe() status=AUTORIZADA"]
        F2["enviarEmail() to accounting"]
        F3["DANFE PDF generation"]
        F1 --> F2 --> F3
    end

    Step1 --> Step2 --> Step3 --> Step4 --> Step5 --> Step6
```

### NFe Status Values

| Status | Meaning |
|--------|---------|
| `NOTA PENDENTE` | Pre-SEFAZ, waiting authorization |
| `AUTORIZADA` | SEFAZ approved |
| `DENEGADA` | SEFAZ denied |
| `CANCELADA` | Cancelled after authorization |

### NFe Cancellation Flow

```cpp
1. User enters justification (15-200 chars)
2. Command: NFE.CancelarNFe(<chaveAcesso>, <justificativa>)
3. Verify: xEvento=Cancelamento registrado
4. UPDATE nfe SET status = 'CANCELADA'
5. UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL
```

### 2025 Tax Reform (IBS/CBS/IS)

New XML sections added:
- `[IBSCBS###]` - IBS (state+municipal) and CBS (federal)
- `[gIBSUF###]` - State IBS
- `[gIBSMun###]` - Municipal IBS
- `[gCBS###]` - Federal CBS
- `[ISel###]` - Selective tax

---

## 3. CNAB/Bank Reconciliation Flow

### Overview

CNAB (Centro Nacional de Automação Bancária) is the Brazilian bank file standard.

- **CNAB 240**: 240-character fixed-width records
- **Bank**: Itaú (code 341) - only bank implemented

### File Types

| Type | Direction | Purpose |
|------|-----------|---------|
| **Remessa** | Outgoing | Payment instructions to bank |
| **Retorno** | Incoming | Payment confirmations from bank |

### Payment Status Flow

```mermaid
stateDiagram-v2
    PENDENTE --> CONFERIDO
    CONFERIDO --> AGENDADO : CNAB remessa
    AGENDADO --> PAGO : CNAB retorno
```

### GARE (Tax) Status Flow

```mermaid
stateDiagram-v2
    PENDENTE_GARE --> LIBERADO_GARE
    LIBERADO_GARE --> GERADO_GARE : CNAB remessa
    GERADO_GARE --> PAGO_GARE : CNAB retorno
    state "PENDENTE GARE" as PENDENTE_GARE
    state "LIBERADO GARE" as LIBERADO_GARE
    state "GERADO GARE" as GERADO_GARE
    state "PAGO GARE" as PAGO_GARE
```

### Remessa Generation Flow

```mermaid
flowchart TB
    S1["1. User selects payments"]
    S2["2. Click 'Remessa ITAU'"]
    S3["3. montarPagamento()<br/>Get bank info from usuario/fornecedor"]

    S1 --> S2 --> S3

    subgraph S4["4. remessaPagamentoItau240()"]
        File["Generate CNAB 240 file:<br/>[Header Arquivo]<br/>[Header Lote] - Salary<br/>[Segmento A] - Details<br/>[Trailer Lote]<br/>[Header Lote] - Supplier<br/>[Trailer Arquivo]"]
    end

    S3 --> S4

    S5["5. Save to /cnab/itau/cnab[seq].rem"]
    S6["6. INSERT INTO cnab"]
    S7["7. UPDATE status = 'AGENDADO'"]

    S4 --> S5 --> S6 --> S7
```

### Retorno Processing Flow

```mermaid
flowchart TB
    R1["1. User uploads .RET file"]
    R2["2. retornoGareItau240(filePath)"]

    R1 --> R2

    subgraph R3["3. Parse file line-by-line"]
        Parse1["Header Lote (pos 7 = '1')"]
        Parse2["Segmento N (pos 13 = 'N')<br/>Extract CNPJ, NFe, date<br/>Decode occurrence codes"]
        Parse3["Trailer Lote (pos 7 = '5')"]
    end

    R2 --> R3

    R4["4. For code 00 (PAID):<br/>UPDATE status = 'PAGO GARE'"]
    R5["5. INSERT INTO cnab tipo='RETORNO'"]

    R3 --> R4 --> R5
```

### Key Occurrence Codes

| Code | Meaning |
|------|---------|
| 00 | PAGAMENTO EFETUADO (Payment Completed) |
| BD | PAGAMENTO AGENDADO (Payment Scheduled) |
| CE | PAGAMENTO CANCELADO (Payment Cancelled) |
| SS | CANCELADO POR INSUFICIÊNCIA DE SALDO (Insufficient Balance) |

### Not Implemented

- Boleto generation (TODO in code)
- CNAB 400 format
- Banks other than Itaú

---

## 4. Commission (RT) Calculation Flow

### Formula

```
Commission = Sale Value × (Commission Percentage / 100)
```

- **Only for REPRESENTAÇÃO sales**: `venda.representacao = TRUE`
- **Percentage stored in**: `profissional.comissao` (DECIMAL 15,4)
- **Default**: 5% if not set

### When Commission is Created

**At payment creation** (not at sale or delivery):

```cpp
// venda.cpp:1836-1877
const double taxaComissao = query.value("comissaoLoja").toDouble() / 100;
const bool calculaComissao = (taxaComissao > 0 && isRepresentacao && observacao != "FRETE");

if (calculaComissao) {
    const double valorBase = payment.valor;
    const double valorComissao = valorBase * taxaComissao;
    // Create entry in conta_a_receber_has_pagamento with comissao = TRUE
}
```

### Storage Tables

**Receivables** (`conta_a_receber_has_pagamento`):
- `comissao = TRUE` flag
- `grupo = "Comissão Representação"`
- `dataPagamento = payment_date + 1 month`

**Payables for Reversals** (`conta_a_pagar_has_pagamento`):
- Negative value for return clawbacks
- `grupo = "RT's"`

### Commission Reversal on Returns

```cpp
// devolucao.cpp:485-534
void criarComissaoProfissional() {
    const double rt = queryVenda.value("rt").toDouble();
    const double valor = (prcUn * quant) * (rt / 100) * -1;  // NEGATIVE

    INSERT INTO conta_a_pagar_has_pagamento (
        idVenda = idDevolucao,
        contraParte = profissional_name,
        valor = valor,  // Negative amount
        grupo = "RT's",
        dataPagamento = quinzena_date  // 15th or 30th
    )
}
```

### Complete Flow Diagram

```mermaid
flowchart TB
    Sale["SALE CREATION<br/>representacao=TRUE"]

    Sale --> PaymentSetup["PAYMENT SCHEDULE SETUP"]

    PaymentSetup --> ForEach{"FOR EACH PAYMENT<br/>representacao && taxaComissao > 0"}

    ForEach --> Normal["Normal Payment Entry<br/>INSERT conta_a_receber<br/>comissao=FALSE"]

    ForEach --> Commission["Commission Entry<br/>INSERT conta_a_receber<br/>comissao=TRUE<br/>valor = payment × taxaComissao"]

    Normal --> Check{"RETURN/DEVOLUÇÃO?"}
    Commission --> Check

    Check -->|Yes| Reversal["Commission Reversal<br/>INSERT conta_a_pagar<br/>valor = NEGATIVE<br/>grupo = 'RT's'"]

    Check -->|No| Done["Done"]
```

### The "Profissional" Entity

```sql
CREATE TABLE profissional (
    idProfissional INT AUTO_INCREMENT,
    idLoja INT,
    nome_razao VARCHAR(100),
    comissao DECIMAL(15,4),    -- Commission percentage
    banco, agencia, cc,        -- Bank info for payment
    cpf / cnpj,                -- Tax ID
    desativado TINYINT(1)
)
```

---

## Summary

All 4 critical flows are now documented:

| Flow | Key Files | Status |
|------|-----------|--------|
| Delivery | `widgetlogistica*.cpp`, `inputdialogconfirmacao.cpp` | Complete |
| NFe Emission | `cadastrarnfe.cpp`, `acbr.cpp` | Complete |
| CNAB/Bank | `cnab.cpp`, `widgetfinanceirocontas.cpp` | Complete |
| Commission | `venda.cpp`, `devolucao.cpp`, `cadastroprofissional.cpp` | Complete |
