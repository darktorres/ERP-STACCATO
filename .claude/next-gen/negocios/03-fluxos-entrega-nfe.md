# Fluxos de Entrega, Emissão de NFe e Financeiro - Análise Profunda

> Status: **Completo**
> Última atualização: 2025-12-27
> Fonte: Análise profunda do código C++

---

## Índice

1. [Fluxo de Entrega](#1-fluxo-de-entrega)
2. [Fluxo de Emissão de NFe (Saída)](#2-fluxo-de-emissão-de-nfe-saída)
3. [Fluxo de Reconciliação Bancária CNAB](#3-fluxo-de-reconciliação-bancária-cnab)
4. [Fluxo de Cálculo de Comissão (RT)](#4-fluxo-de-cálculo-de-comissão-rt)

---

## 1. Fluxo de Entrega

### Transições de Status

```mermaid
stateDiagram-v2
    ESTOQUE --> ENTREGA_AGEND : Agendar entrega
    ENTREGA_AGEND --> EM_ENTREGA : NFe autorizada
    EM_ENTREGA --> ENTREGUE : Confirmar entrega
    state "ENTREGA AGEND." as ENTREGA_AGEND
```

### Diagrama de Fluxo Completo

```mermaid
flowchart TB
    Stock["ESTOQUE PRONTO<br/>status = 'ESTOQUE'"]

    Stock --> Agendar["WidgetLogisticaAgendarEntrega<br/>Selecionar produtos, veículo, data"]

    Agendar --> AgendarClick["on_pushButtonAgendarCarga_clicked()<br/>Gerar idEvento<br/>UPDATE status = 'ENTREGA AGEND.'"]

    AgendarClick --> Agendado["STATUS = 'ENTREGA AGEND.'<br/>Opcional: NFe Futura"]

    Agendado --> Entregas["WidgetLogisticaEntregas"]

    Entregas --> ConsultarNFe["processarConsultaNFe()<br/>UPDATE nfe status = 'AUTORIZADA'<br/>UPDATE status = 'EM ENTREGA'"]

    ConsultarNFe --> EmEntrega["STATUS = 'EM ENTREGA'"]

    EmEntrega --> Confirmar["InputDialogConfirmacao<br/>Inserir: dataRealEnt, entregou, recebeu<br/>Opcional: Upload de foto"]

    Confirmar --> Final["STATUS = 'ENTREGUE' (Final)"]
```

### Tabelas Principais

| Tabela | Propósito |
|--------|-----------|
| `venda_has_produto2` | Rastreamento principal de status do produto |
| `pedido_fornecedor_has_produto2` | Status paralelo do pedido de compra |
| `veiculo_has_produto` | Agrupamento de entrega por veículo/evento |
| `transportadora_has_veiculo` | Cadastro de veículos |

### Geração de Documento de Entrega

Dois arquivos Excel gerados a partir de modelos:
- `espelho_entrega.xlsx` → Comprovante de entrega
- `modelo_checklist.xlsx` → Checklist de verificação física

### Tratamento de Itens Quebrados

```
Usuário marca itens como quebrados → dividirEntrega()
    │
    ├── Linha original: ENTREGUE (qty reduzida)
    ├── Linha quebrada: QUEBRADO
    ├── Linha de reposição (opcional): REPO. ENTREGA
    └── Registro de crédito: Entrada negativa em conta_a_receber
```

---

## 2. Fluxo de Emissão de NFe (Saída)

### Pontos de Entrada

- **Tipos**: `Saida`, `Futura`, `SaidaAposFutura`, `Entrada`
- **Gatilho**: Usuário seleciona itens da venda → botão "Gerar NF-e"
- **Arquivo**: `cadastrarnfe.cpp`

### Fluxo Completo de Emissão

```mermaid
flowchart TB
    subgraph Step1["1. PREPARAÇÃO DE DADOS"]
        P1["validarDados()<br/>Verificar emitente, destinatário, produtos, impostos"]
        P2["criarChaveAcesso()<br/>Gerar chave de 44 caracteres"]
        P3["Carregar modelos de UI"]
        P1 --> P2 --> P3
    end

    subgraph Step2["2. GERAÇÃO DE XML"]
        X1["montarXML()"]
        X2["writeIdentificacao(), writeEmitente()<br/>writeDestinatario(), writeProduto()"]
        X3["Seções de impostos: ICMS, IPI, PIS<br/>COFINS, IBSCBS (2025), ISel"]
        X4["gerarNota(acbr)"]
        X1 --> X2 --> X3 --> X4
    end

    subgraph Step3["3. COMUNICAÇÃO ACBr"]
        A1["Socket TCP para ACBr Monitor"]
        A2["NFE.CriarNFe()"]
        A3["validarSchema()"]
        A1 --> A2 --> A3
    end

    subgraph Step4["4. PRÉ-CADASTRO"]
        R1["preCadastrarNota()<br/>INSERT nfe status='NOTA PENDENTE'"]
        R2["UPDATE venda_has_produto2<br/>idNFeSaida = :idNFe"]
        R1 --> R2
    end

    subgraph Step5["5. TRANSMISSÃO SEFAZ"]
        S1["enviarNFe()"]
        S2["processarResposta()"]
        S1 --> S2
        S2 -->|REJEIÇÃO| Retry["Deletar e tentar novamente"]
        S2 -->|AUTORIZADA| Auth["Atualizar status"]
    end

    subgraph Step6["6. PÓS-AUTORIZAÇÃO"]
        F1["atualizarNFe() status=AUTORIZADA"]
        F2["enviarEmail() para contabilidade"]
        F3["Geração do DANFE PDF"]
        F1 --> F2 --> F3
    end

    Step1 --> Step2 --> Step3 --> Step4 --> Step5 --> Step6
```

### Valores de Status da NFe

| Status | Significado |
|--------|-------------|
| `NOTA PENDENTE` | Pré-SEFAZ, aguardando autorização |
| `AUTORIZADA` | Aprovada pela SEFAZ |
| `DENEGADA` | Negada pela SEFAZ |
| `CANCELADA` | Cancelada após autorização |

### Fluxo de Cancelamento de NFe

```cpp
1. Usuário insere justificativa (15-200 caracteres)
2. Comando: NFE.CancelarNFe(<chaveAcesso>, <justificativa>)
3. Verificar: xEvento=Cancelamento registrado
4. UPDATE nfe SET status = 'CANCELADA'
5. UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL
```

### Reforma Tributária 2025 (IBS/CBS/IS)

Novas seções XML adicionadas:
- `[IBSCBS###]` - IBS (estadual+municipal) e CBS (federal)
- `[gIBSUF###]` - IBS Estadual
- `[gIBSMun###]` - IBS Municipal
- `[gCBS###]` - CBS Federal
- `[ISel###]` - Imposto Seletivo

---

## 3. Fluxo de Reconciliação Bancária CNAB

### Visão Geral

CNAB (Centro Nacional de Automação Bancária) é o padrão brasileiro de arquivos bancários.

- **CNAB 240**: Registros de largura fixa de 240 caracteres
- **Banco**: Itaú (código 341) - único banco implementado

### Tipos de Arquivo

| Tipo | Direção | Propósito |
|------|---------|-----------|
| **Remessa** | Saída | Instruções de pagamento para o banco |
| **Retorno** | Entrada | Confirmações de pagamento do banco |

### Fluxo de Status de Pagamento

```mermaid
stateDiagram-v2
    PENDENTE --> CONFERIDO
    CONFERIDO --> AGENDADO : CNAB remessa
    AGENDADO --> PAGO : CNAB retorno
```

### Fluxo de Status GARE (Imposto)

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

### Fluxo de Geração de Remessa

```mermaid
flowchart TB
    S1["1. Usuário seleciona pagamentos"]
    S2["2. Clica 'Remessa ITAU'"]
    S3["3. montarPagamento()<br/>Obter info bancária de usuario/fornecedor"]

    S1 --> S2 --> S3

    subgraph S4["4. remessaPagamentoItau240()"]
        File["Gerar arquivo CNAB 240:<br/>[Header Arquivo]<br/>[Header Lote] - Salário<br/>[Segmento A] - Detalhes<br/>[Trailer Lote]<br/>[Header Lote] - Fornecedor<br/>[Trailer Arquivo]"]
    end

    S3 --> S4

    S5["5. Salvar em /cnab/itau/cnab[seq].rem"]
    S6["6. INSERT INTO cnab"]
    S7["7. UPDATE status = 'AGENDADO'"]

    S4 --> S5 --> S6 --> S7
```

### Fluxo de Processamento de Retorno

```mermaid
flowchart TB
    R1["1. Usuário faz upload do arquivo .RET"]
    R2["2. retornoGareItau240(filePath)"]

    R1 --> R2

    subgraph R3["3. Analisar arquivo linha por linha"]
        Parse1["Header Lote (pos 7 = '1')"]
        Parse2["Segmento N (pos 13 = 'N')<br/>Extrair CNPJ, NFe, data<br/>Decodificar códigos de ocorrência"]
        Parse3["Trailer Lote (pos 7 = '5')"]
    end

    R2 --> R3

    R4["4. Para código 00 (PAGO):<br/>UPDATE status = 'PAGO GARE'"]
    R5["5. INSERT INTO cnab tipo='RETORNO'"]

    R3 --> R4 --> R5
```

### Principais Códigos de Ocorrência

| Código | Significado |
|--------|-------------|
| 00 | PAGAMENTO EFETUADO |
| BD | PAGAMENTO AGENDADO |
| CE | PAGAMENTO CANCELADO |
| SS | CANCELADO POR INSUFICIÊNCIA DE SALDO |

### Não Implementado

- Geração de boleto (TODO no código)
- Formato CNAB 400
- Bancos além do Itaú

---

## 4. Fluxo de Cálculo de Comissão (RT)

### Fórmula

```
Comissão = Valor da Venda × (Percentual de Comissão / 100)
```

- **Apenas para vendas de REPRESENTAÇÃO**: `venda.representacao = TRUE`
- **Percentual armazenado em**: `profissional.comissao` (DECIMAL 15,4)
- **Padrão**: 5% se não definido

### Quando a Comissão é Criada

**Na criação do pagamento** (não na venda ou entrega):

```cpp
// venda.cpp:1836-1877
const double taxaComissao = query.value("comissaoLoja").toDouble() / 100;
const bool calculaComissao = (taxaComissao > 0 && isRepresentacao && observacao != "FRETE");

if (calculaComissao) {
    const double valorBase = payment.valor;
    const double valorComissao = valorBase * taxaComissao;
    // Criar entrada em conta_a_receber_has_pagamento com comissao = TRUE
}
```

### Tabelas de Armazenamento

**Recebíveis** (`conta_a_receber_has_pagamento`):
- Flag `comissao = TRUE`
- `grupo = "Comissão Representação"`
- `dataPagamento = data_pagamento + 1 mês`

**Pagáveis para Estornos** (`conta_a_pagar_has_pagamento`):
- Valor negativo para estornos de devolução
- `grupo = "RT's"`

### Estorno de Comissão em Devoluções

```cpp
// devolucao.cpp:485-534
void criarComissaoProfissional() {
    const double rt = queryVenda.value("rt").toDouble();
    const double valor = (prcUn * quant) * (rt / 100) * -1;  // NEGATIVO

    INSERT INTO conta_a_pagar_has_pagamento (
        idVenda = idDevolucao,
        contraParte = profissional_name,
        valor = valor,  // Valor negativo
        grupo = "RT's",
        dataPagamento = quinzena_date  // Dia 15 ou 30
    )
}
```

### Diagrama de Fluxo Completo

```mermaid
flowchart TB
    Sale["CRIAÇÃO DA VENDA<br/>representacao=TRUE"]

    Sale --> PaymentSetup["CONFIGURAÇÃO DO CRONOGRAMA DE PAGAMENTO"]

    PaymentSetup --> ForEach{"PARA CADA PAGAMENTO<br/>representacao && taxaComissao > 0"}

    ForEach --> Normal["Entrada de Pagamento Normal<br/>INSERT conta_a_receber<br/>comissao=FALSE"]

    ForEach --> Commission["Entrada de Comissão<br/>INSERT conta_a_receber<br/>comissao=TRUE<br/>valor = pagamento × taxaComissao"]

    Normal --> Check{"DEVOLUÇÃO?"}
    Commission --> Check

    Check -->|Sim| Reversal["Estorno de Comissão<br/>INSERT conta_a_pagar<br/>valor = NEGATIVO<br/>grupo = 'RT's'"]

    Check -->|Não| Done["Concluído"]
```

### A Entidade "Profissional"

```sql
CREATE TABLE profissional (
    idProfissional INT AUTO_INCREMENT,
    idLoja INT,
    nome_razao VARCHAR(100),
    comissao DECIMAL(15,4),    -- Percentual de comissão
    banco, agencia, cc,        -- Info bancária para pagamento
    cpf / cnpj,                -- ID fiscal
    desativado TINYINT(1)
)
```

---

## Resumo

Todos os 4 fluxos críticos estão agora documentados:

| Fluxo | Arquivos Principais | Status |
|-------|---------------------|--------|
| Entrega | `widgetlogistica*.cpp`, `inputdialogconfirmacao.cpp` | Completo |
| Emissão de NFe | `cadastrarnfe.cpp`, `acbr.cpp` | Completo |
| CNAB/Banco | `cnab.cpp`, `widgetfinanceirocontas.cpp` | Completo |
| Comissão | `venda.cpp`, `devolucao.cpp`, `cadastroprofissional.cpp` | Completo |
