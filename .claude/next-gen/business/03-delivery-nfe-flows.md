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

```
ESTOQUE → ENTREGA AGEND. → EM ENTREGA → ENTREGUE
```

### Complete Flow Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         DELIVERY FLOW                                    │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  STOCK READY (status = 'ESTOQUE')                                       │
│       │                                                                  │
│       ▼                                                                  │
│  WidgetLogisticaAgendarEntrega                                          │
│       │                                                                  │
│       ├── User selects products from view_agendar_entrega               │
│       ├── User selects vehicle (transportadora_has_veiculo)             │
│       ├── User selects delivery date                                    │
│       │                                                                  │
│       ▼                                                                  │
│  on_pushButtonAgendarCarga_clicked()                                    │
│       │                                                                  │
│       ├── Generate idEvento (groups products for this delivery)         │
│       │                                                                  │
│       ├── UPDATE venda_has_produto2                                     │
│       │   SET status = 'ENTREGA AGEND.', dataPrevEnt = :date           │
│       │                                                                  │
│       ├── UPDATE pedido_fornecedor_has_produto2                         │
│       │   SET status = 'ENTREGA AGEND.', dataPrevEnt = :date           │
│       │                                                                  │
│       └── INSERT INTO veiculo_has_produto                               │
│           (idEvento, idVeiculo, data, status='ENTREGA AGEND.')          │
│                                                                          │
│       ▼                                                                  │
│  STATUS = 'ENTREGA AGEND.'                                              │
│       │                                                                  │
│       │  [Optional: Generate NFe Futura or link NFe]                    │
│       │                                                                  │
│       ▼                                                                  │
│  WidgetLogisticaEntregas                                                │
│       │                                                                  │
│       ├── on_pushButtonConsultarNFe_clicked()                           │
│       │   Queries SEFAZ for NFe authorization                           │
│       │                                                                  │
│       ├── processarConsultaNFe()                                        │
│       │   ├── UPDATE nfe SET status = 'AUTORIZADA'                     │
│       │   ├── UPDATE venda_has_produto2                                │
│       │   │   SET status = 'EM ENTREGA', idNFeSaida = :idNFe           │
│       │   ├── UPDATE pedido_fornecedor_has_produto2                    │
│       │   │   SET status = 'EM ENTREGA'                                │
│       │   └── UPDATE veiculo_has_produto                               │
│       │       SET status = 'EM ENTREGA', idNFeSaida = :idNFe           │
│       │                                                                  │
│       ▼                                                                  │
│  STATUS = 'EM ENTREGA'                                                  │
│       │                                                                  │
│       ▼                                                                  │
│  on_pushButtonConfirmarEntrega_clicked()                                │
│       │                                                                  │
│       └── Opens InputDialogConfirmacao (Tipo::Entrega)                  │
│           │                                                              │
│           ├── User enters: dataRealEnt, entregou, recebeu               │
│           ├── Optional: Upload delivery photo (WebDAV)                  │
│           │                                                              │
│           └── confirmarEntrega()                                        │
│               ├── UPDATE veiculo_has_produto                            │
│               │   SET status = 'ENTREGUE'                               │
│               ├── UPDATE venda_has_produto2                             │
│               │   SET status = 'ENTREGUE', entregou, recebeu,          │
│               │       dataRealEnt                                       │
│               └── UPDATE pedido_fornecedor_has_produto2                 │
│                   SET status = 'ENTREGUE', dataRealEnt                 │
│                                                                          │
│       ▼                                                                  │
│  STATUS = 'ENTREGUE' (Final)                                            │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
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

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         NFe EMISSION FLOW                                │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. DATA PREPARATION                                                     │
│     │                                                                    │
│     ├── validarDados() - Check all required fields                      │
│     │   • Emitter: CNPJ, IE, address                                    │
│     │   • Recipient: Name, CPF/CNPJ, address                            │
│     │   • Products: NCM, CFOP, quantity, prices                         │
│     │   • Taxes: ICMS, IPI, PIS, COFINS, IBS, CBS (2025+)              │
│     │                                                                    │
│     ├── criarChaveAcesso() - Generate 44-char access key                │
│     │   UF(2) + AAMM(4) + CNPJ(8) + Model(2) + Series(3) +             │
│     │   Number(9) + Type(1) + Code(8) + DV(1) = 44 chars               │
│     │                                                                    │
│     └── Load UI models: modelVenda, modelLoja, modelProduto             │
│                                                                          │
│  2. XML GENERATION                                                       │
│     │                                                                    │
│     ├── montarXML() - Build ACBr command string                         │
│     │   ├── writeIdentificacao() - NFe metadata                         │
│     │   ├── writeEmitente() - Issuer data                               │
│     │   ├── writeDestinatario() - Recipient data                        │
│     │   ├── writeProduto() - For each product:                          │
│     │   │   ├── [Produto###] - Product details                          │
│     │   │   ├── [ICMS###] - CST, Base, Rate, Value                     │
│     │   │   ├── [IPI###] - Classification, CST                          │
│     │   │   ├── [PIS###] - CST, Base, Rate, Value                      │
│     │   │   ├── [COFINS###] - CST, Base, Rate, Value                   │
│     │   │   ├── [IBSCBS###] - IBS/CBS (2025 reform)                    │
│     │   │   └── [ISel###] - Selective tax                               │
│     │   ├── writeTotal() - Aggregated totals                            │
│     │   ├── writeTransportadora() - Transport info                      │
│     │   ├── writePagamento() - Payment method                           │
│     │   └── writeComplemento() - Additional info                        │
│     │                                                                    │
│     └── gerarNota(acbr) - Send to ACBr                                  │
│                                                                          │
│  3. ACBr COMMUNICATION                                                   │
│     │                                                                    │
│     ├── TCP Socket to ACBr Monitor (localhost:port)                     │
│     ├── Command: NFE.CriarNFe("...fields...")                           │
│     ├── validarSchema() - Validate XML structure                        │
│     └── Response: OK: <filePath> or ERROR: <message>                    │
│                                                                          │
│  4. PRE-REGISTRATION                                                     │
│     │                                                                    │
│     ├── preCadastrarNota()                                              │
│     │   INSERT INTO nfe (status='NOTA PENDENTE', ...)                   │
│     │                                                                    │
│     └── Link products:                                                   │
│         UPDATE venda_has_produto2                                        │
│         SET status = 'EM ENTREGA', idNFeSaida = :idNFe                  │
│                                                                          │
│  5. SEFAZ TRANSMISSION                                                   │
│     │                                                                    │
│     ├── enviarNFe(acbr, filePath, idNFe)                                │
│     │   Command: NFE.EnviarNFe(<file>, lote, assina, ...)              │
│     │                                                                    │
│     └── processarResposta()                                             │
│         ├── REJECTION → Delete NFe, show error, retry                   │
│         ├── INTERNAL ERROR → Delete NFe, prompt retry                   │
│         └── AUTHORIZED/DENIED → Update NFe status                       │
│                                                                          │
│  6. POST-AUTHORIZATION                                                   │
│     │                                                                    │
│     ├── atualizarNFe() - Update status to AUTORIZADA/DENEGADA          │
│     ├── enviarEmail() - Send to accounting                              │
│     └── DANFE generation (Windows DLL: ACBrNFe32.dll)                   │
│         Output: ./pdf/<chaveAcesso>-nfe.pdf                             │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
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

```
PENDENTE → CONFERIDO → AGENDADO → PAGO
                         │
                    (CNAB remessa)
                         │
                    (Bank processes)
                         │
                    (CNAB retorno)
                         ▼
                       PAGO
```

### GARE (Tax) Status Flow

```
PENDENTE GARE → LIBERADO GARE → GERADO GARE → PAGO GARE
                                     │
                               (CNAB remessa)
                                     │
                               (CNAB retorno)
```

### Remessa Generation Flow

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    CNAB REMESSA FLOW                                     │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. User selects payments in WidgetFinanceiroContas                     │
│     │                                                                    │
│  2. Click "Remessa ITAU"                                                │
│     │                                                                    │
│  3. montarPagamento() for each payment:                                 │
│     ├── Salary (RH-SALÁRIOS): Get bank info from usuario table         │
│     └── Supplier (PRODUTOS-VENDA): Get bank info from fornecedor       │
│     │                                                                    │
│  4. remessaPagamentoItau240() / remessaGareItau240()                    │
│     │                                                                    │
│     ├── Generate CNAB 240 file:                                         │
│     │   [Header Arquivo]                                                │
│     │   [Header Lote] - Salary batch (tipo=30)                         │
│     │   [Segmento A] - Payment details                                  │
│     │   [Trailer Lote]                                                  │
│     │   [Header Lote] - Supplier batch (tipo=20)                       │
│     │   [Segmento A] - Payment details                                  │
│     │   [Trailer Lote]                                                  │
│     │   [Trailer Arquivo]                                               │
│     │                                                                    │
│  5. Save to /cnab/itau/cnab[seq].rem                                    │
│     │                                                                    │
│  6. INSERT INTO cnab (tipo='REMESSA', banco='ITAU', ...)               │
│     │                                                                    │
│  7. UPDATE conta_a_pagar_has_pagamento                                  │
│     SET status = 'AGENDADO', idCnab = [id]                             │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

### Retorno Processing Flow

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    CNAB RETORNO FLOW                                     │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. User uploads .RET file                                              │
│     │                                                                    │
│  2. retornoGareItau240(filePath)                                        │
│     │                                                                    │
│  3. Parse file line-by-line:                                            │
│     ├── Header Lote (position 7 = '1')                                  │
│     ├── Segmento N (position 13 = 'N')                                  │
│     │   ├── Extract CNPJ, NF-e number, payment date                    │
│     │   └── Decode occurrence codes (00=PAID, BD=SCHEDULED, etc.)      │
│     └── Trailer Lote (position 7 = '5')                                 │
│     │                                                                    │
│  4. For successful payments (code 00):                                  │
│     UPDATE conta_a_pagar_has_pagamento                                   │
│     SET status = 'PAGO GARE',                                           │
│         valorReal = valor,                                               │
│         dataRealizado = [payment_date]                                  │
│     WHERE idNFe = [nfe_id]                                              │
│     │                                                                    │
│  5. INSERT INTO cnab (tipo='RETORNO', ...)                             │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
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

```
┌──────────────────────────────────────────────────────────────────────────┐
│                    COMMISSION FLOW                                       │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  SALE CREATION                                                           │
│  └── idVenda, idProfissional, representacao=TRUE                        │
│       │                                                                  │
│       ▼                                                                  │
│  PAYMENT SCHEDULE SETUP                                                  │
│       │                                                                  │
│       ▼                                                                  │
│  FOR EACH PAYMENT (if representacao=TRUE && taxaComissao > 0):          │
│       │                                                                  │
│       ├── Normal Payment Entry                                          │
│       │   INSERT conta_a_receber (comissao=FALSE)                       │
│       │                                                                  │
│       └── Commission Entry                                               │
│           INSERT conta_a_receber (                                       │
│               comissao=TRUE,                                             │
│               valor = payment × taxaComissao,                           │
│               dataPagamento = date + 1 month,                           │
│               grupo = "Comissão Representação"                          │
│           )                                                              │
│                                                                          │
│  IF RETURN/DEVOLUÇÃO:                                                   │
│       │                                                                  │
│       └── Commission Reversal                                            │
│           INSERT conta_a_pagar (                                         │
│               valor = (price × qty × rt%) × -1,  // NEGATIVE            │
│               grupo = "RT's",                                            │
│               dataPagamento = quinzena (15th or 30th)                   │
│           )                                                              │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
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
