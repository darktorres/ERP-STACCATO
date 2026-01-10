# Schema Proposto: ERP Staccato v2 - Flowchart

## Overview: Architecture at a Glance

```mermaid
flowchart TB
    subgraph MasterData["📋 DADOS MESTRES"]
        direction LR
        Lojas["lojas"]
        Fornecedores["fornecedores"]
        Clientes["clientes"]
        Usuarios["usuarios"]
        Transportadoras["transportadoras"]

        Produtos["produtos"]
        Precos["produto_precos<br/>(versionado)"]
        Tributos["produto_tributos"]

        NCM["ncms"]
        Categorias["categorias"]

        Enderecos["enderecos<br/>(compartilhado)"]

        Lojas -.-> Enderecos
        Fornecedores -.-> Enderecos
        Clientes -.-> Enderecos
        Transportadoras -.-> Enderecos

        Produtos --> Precos
        Produtos --> Tributos
        Produtos --> NCM
        Produtos --> Categorias
    end

    MasterData --> CommercialFlow

    subgraph CommercialFlow["🛍️ FLUXO COMERCIAL"]
        direction TB

        subgraph Quotation["Orçamentos"]
            Orcamento["orcamentos"]
            OrcItem["orcamento_itens"]
            Orcamento --> OrcItem
        end

        subgraph Sales["Vendas"]
            Venda["vendas"]
            VendaItem["venda_itens<br/>(com parent_id/root_id)<br/>origem: COMPRA ou ESTOQUE"]
            Venda --> VendaItem
        end

        subgraph Purchase["Compras"]
            Compra["compras"]
            CompraItem["compra_itens<br/>(com parent_id/root_id)"]
            Compra --> CompraItem
        end

        Quotation -->|"Converter"| Sales
        Sales -->|"Se origem=COMPRA<br/>cria pedido"| Purchase
    end

    subgraph NFe["📄 NOTA FISCAL ENTRADA<br/>(Do Fornecedor)"]
        direction TB

        NFeTable["nfes<br/>tipo: ENTRADA<br/>compra_id"]
        NFeItens["nfe_itens (JSONB)<br/>compra_item_id"]
        NFeTable --> NFeItens
    end

    Purchase -->|"Fornecedor envia"| NFe

    NFe -->|"Cria Estoque"| Inventory

    subgraph Inventory["📦 INVENTÁRIO<br/>(Criado de NFe ENTRADA)"]
        direction TB

        subgraph StockModel["Estado Atual (stock.quant pattern)"]
            Lotes["estoque_lotes<br/>(quantidade_disponível,<br/>quantidade_reservada,<br/>data_entrada FIFO)<br/>← criado de nfe_item_id"]
        end

        subgraph StockLog["Log Histórico (stock.move pattern)"]
            Movimentacoes["estoque_movimentacoes<br/>(ENTRADA_COMPRA,<br/>SAIDA_VENDA, etc)"]
        end

        subgraph Location["Localização no Galpão"]
            Blocos["galpao_blocos"]
            Localizacoes["estoque_localizacoes<br/>(1 lote → múltiplos blocos)<br/>split de paletes"]
            Blocos -.-> Localizacoes
        end

        subgraph Allocation["Alocação 1:1<br/>(venda_item consome estoque)"]
            Alocacoes["alocacoes<br/>(venda_item ↔ lote)"]
        end

        StockModel --> Allocation
        StockLog --> Allocation
        Lotes --> Location
    end

    Sales -->|"origem=ESTOQUE:<br/>aloca estoque existente"| Allocation

    Inventory --> Logistics

    subgraph Logistics["🚚 LOGÍSTICA & SAÍDA"]
        direction TB

        subgraph LogisticCore["Entregas"]
            Entregas["entregas"]
            EntregaItem["entrega_itens"]
            Entregas --> EntregaItem
        end

        subgraph NFeSaidaBox["NFe SAÍDA (Para Cliente)"]
            NFeTableOut["nfes<br/>tipo: SAIDA<br/>venda_id"]
            NFeItensOut["nfe_itens<br/>venda_item_id"]
            NFeTableOut --> NFeItensOut
        end

        LogisticCore --> NFeSaidaBox
    end

    Sales -->|"Gera NFe para cliente"| NFeSaidaBox

    Logistics --> Financial

    subgraph Financial["💰 FINANCEIRO (Unified)"]
        direction TB

        subgraph UnifiedParcel["Unified Parcelas"]
            FP["financeiro_parcelas<br/>(tipo: RECEBER ou PAGAR)"]
        end

        subgraph Views["Clarity Views"]
            VRec["parcelas_receber"]
            VPag["parcelas_pagar"]
        end

        subgraph CNAB["CNAB/Remessa"]
            Remessas["remessas_cnab"]
            Retornos["retornos_cnab"]
        end

        FP --> Views
        FP -.-> Remessas
    end

    Financial --> Audit

    subgraph Audit["🔍 AUDITORIA"]
        direction LR
        AuditLog["audit_log<br/>(INSERT/UPDATE/DELETE<br/>com dados_antigos/novos)"]
    end
```

---

## Design Principles

```mermaid
flowchart TB
    subgraph Principles["5 PRINCÍPIOS DE DESIGN"]
        direction TB

        P1["1️⃣ VALIDADO PELA INDÚSTRIA<br/>3 entidades: Pedido → Movimentação → Estoque<br/>(como SAP MM, Odoo, ERPNext)<br/>❌ Nunca mesclar pedido com inventário"]

        P2["2️⃣ DADOS FISCAIS FLEXÍVEIS<br/>JSONB em nfe_itens<br/>➜ Pronto para reforma tributária 2026-2033<br/>➜ Campos desconhecidos preservados"]

        P3["3️⃣ STATUS COM TYPE-SAFETY<br/>ENUMs PostgreSQL<br/>➜ Sem strings mágicas<br/>➜ Transições validadas no BD"]

        P4["4️⃣ CONSUMO MANUAL INTELIGENTE<br/>alocacoes (venda_item ↔ lote)<br/>➜ Usuário escolhe lote para qualidade<br/>➜ FIFO sugerido via view (não imposto)"]

        P5["5️⃣ AUDITORIA COMPLETA<br/>audit_log com triggers automáticos<br/>➜ Quem mudou o quê, quando e por quê<br/>➜ Reversão completa de dados"]
    end
```

---

## Master Data: Cascade Relationships

```mermaid
flowchart TB
    subgraph Master["DADOS MESTRES"]
        direction TB

        subgraph People["Pessoas & Organizações"]
            Lojas["lojas<br/>CNPJ, razão social"]
            Fornecedores["fornecedores<br/>CNPJ, prazo_pagamento"]
            Clientes["clientes<br/>CPF/CNPJ, limite_credito"]
            Usuarios["usuarios<br/>permissoes JSONB"]
            Transportadoras["transportadoras<br/>placa, RNTC"]
        end

        subgraph Shared["Compartilhado"]
            Enderecos["enderecos<br/>(polimórfico)<br/>tipo: principal, entrega, cobranca"]
        end

        subgraph Products["Produtos & Classificação"]
            Produtos["produtos<br/>código_comercial UNIQUE<br/>por fornecedor"]
            Precos["produto_precos<br/>(SCD-2: versioning)<br/>vigente_de/ate"]
            Tributos["produto_tributos<br/>ICMS, IPI, PIS,<br/>config_ibs_cbs JSONB"]
            NCM["ncms<br/>10 dígitos"]
            Categorias["categorias<br/>(árvore)"]
        end

        People --> Shared
        Products --> NCM
        Products --> Categorias
        Produtos --> Precos
        Produtos --> Tributos
    end
```

**Key Design Decision**: Produto tem FK obrigatório para fornecedor
- ✅ Garante origem
- ✅ Simplifica FIFO (já sabe fornecedor)
- ✅ Preços versionados (SCD-2) preservam histórico

---

## Commercial Flow: From Quote to Invoice

```mermaid
flowchart TB
    Start["🎯 INÍCIO DO PROCESSO"] --> Quotation

    subgraph Quotation["1️⃣ ORÇAMENTO"]
        Q1["orcamentos<br/>status: RASCUNHO → ENVIADO → APROVADO"]
        Q2["orcamento_itens<br/>posicao para reordenação<br/>snapshot de preço"]
        Q1 --> Q2
    end

    Quotation -->|"Cliente aprova"| Conversion

    subgraph Conversion["2️⃣ CONVERSÃO PARA VENDA"]
        C1["vendas<br/>orcamento_id = xxx<br/>status: ABERTA"]
        C2["venda_itens<br/>orcamento_item_id = xxx<br/>origem: COMPRA ou ESTOQUE"]
        C1 --> C2
    end

    Conversion --> Decision

    subgraph Decision["3️⃣ DECISÃO DE ORIGEM"]
        decision{"origem =<br/>COMPRA ou<br/>ESTOQUE?"}
    end

    Decision -->|"ESTOQUE"| FromStock["Verificar estoque<br/>disponível"]
    Decision -->|"COMPRA"| ToPurchase["Precisa comprar<br/>do fornecedor"]

    subgraph Purchase["4️⃣ PEDIDO DE COMPRA"]
        PO1["compras<br/>fornecedor_id, venda_id"]
        PO2["compra_itens<br/>venda_item_id (backreference)<br/>status: PENDENTE → CONFIRMADO"]
        PO1 --> PO2
    end

    ToPurchase --> Purchase

    Purchase -->|"Fornecedor envia NFe ENTRADA"| NFeReceival

    subgraph NFeReceival["5️⃣ IMPORTAÇÃO DE NFe ENTRADA<br/>(Cria Estoque)"]
        NFe["nfes<br/>tipo = ENTRADA<br/>compra_id = xxx<br/>XML raw + parsed"]
        NFeItem["nfe_itens (JSONB)<br/>compra_item_id = xxx<br/>dados: cfop, ncm, icms{}, etc"]
        NFe --> NFeItem
    end

    NFeReceival --> StockCreation

    subgraph StockCreation["6️⃣ CRIAÇÃO DE ESTOQUE<br/>(de NFe ENTRADA)"]
        Lote["estoque_lotes<br/>nfe_item_id = xxx<br/>quantidade_disponível<br/>data_entrada ← FIFO key"]
        Mov["estoque_movimentacoes<br/>tipo: ENTRADA_COMPRA<br/>log completo"]
        Lote --> Mov
    end

    FromStock --> Allocation
    StockCreation --> Allocation

    subgraph Allocation["7️⃣ ALOCAÇÃO (Manual 1:1)<br/>(venda_item ↔ estoque)"]
        Alocacao["alocacoes<br/>venda_item_id ↔ lote_id<br/>status venda_item = ESTOQUE"]
        Constraint["CONSTRAINT 1:1:<br/>cada venda_item<br/>= 1 lote ativo"]
        Alocacao --> Constraint
    end

    Allocation --> Logistics

    subgraph Logistics["8️⃣ LOGÍSTICA"]
        Entrega["entregas<br/>venda_id, status: AGENDADA"]
        EntregaItem["entrega_itens<br/>venda_item_id"]
        Entrega --> EntregaItem
    end

    Logistics --> NFeSaida

    subgraph NFeSaida["9️⃣ NFe SAÍDA"]
        NFeSai["nfes<br/>tipo = SAIDA<br/>venda_id = xxx"]
        NFeSaiItem["nfe_itens<br/>venda_item_id = xxx<br/>dados para cliente"]
        NFeSai --> NFeSaiItem
    end

    NFeSaida --> Financial

    subgraph Financial["🔟 FINANCEIRO (Unified)"]
        FP["financeiro_parcelas<br/>tipo: RECEBER<br/>venda_id = xxx<br/>numero_parcela, data_vencimento"]
    end

    Financial --> End["✅ VENDA COMPLETA"]
```

---

## Stock Model: Three-Layer Pattern (Industry Standard)

```mermaid
flowchart TB
    subgraph Pattern["PADRÃO DA INDÚSTRIA<br/>Odoo: stock.quant, stock.move<br/>SAP MM: Material Ledger, Stock Ledger<br/>ERPNext: Stock Ledger Entry"]
        direction TB

        Layer1["LAYER 1: Pedido<br/>(compra_itens)<br/>O que foi pedido"]
        Layer2["LAYER 2: Movimento<br/>(estoque_movimentacoes)<br/>Como chegou"]
        Layer3["LAYER 3: Estado Atual<br/>(estoque_lotes)<br/>Quanto temos agora"]
    end

    Pattern --> Implementation

    subgraph Implementation["IMPLEMENTAÇÃO NO SCHEMA"]
        direction TB

        subgraph ActualState["Estado Atual (Fast Queries)"]
            Lotes["estoque_lotes<br/>➤ quantidade_disponível (rápido)<br/>➤ quantidade_reservada<br/>➤ status"]
        end

        subgraph HistoryLog["Histórico Completo (Auditoria)"]
            Mov["estoque_movimentacoes<br/>➤ tipo: ENTRADA_COMPRA, SAIDA_VENDA, ENTRADA_AJUSTE<br/>➤ quantidade (pos/neg)<br/>➤ quem, quando, por quê"]
        end

        subgraph Query["Query de Saldo"]
            View["VIEW estoque_saldos<br/>SELECT SUM(quantidade_disponível)<br/>FROM estoque_lotes<br/>WHERE status IN ('DISPONIVEL', 'RESERVADO')"]
        end

        Lotes --> Query
        Mov -.->|"auditoria"| Query
    end

    Implementation --> Advantage

    subgraph Advantage["VANTAGEM vs MONOLÍTICO<br/>❌ Legado: estoque.quant calculado de consumos"]
        direction LR

        Bad["Lento: recalcula sempre<br/>Complexo: múltiplos joins<br/>Frágil: inconsistente"]

        Arrow["➜"]

        Good["Rápido: O(1) lookup<br/>Simples: agregado<br/>Seguro: constraints garantem"]
    end
```

---

## Allocation (Alocação): The 1:1 Link

```mermaid
flowchart TB
    subgraph Before["ANTES: Consumo Automático"]
        direction LR

        Problem["❌ Automático quebra com variação<br/>Cliente quer tom T01<br/>Sistema aloca T02"]

        OldFlow["venda_item (quant=100)<br/>↓<br/>estoque_has_consumo (automático)<br/>↓<br/>Qual estoque? Aleatório!"]

        Problem --- OldFlow
    end

    Before --> Proposed

    subgraph Proposed["NOVO: Alocação Manual Inteligente"]
        direction TB

        Step1["1️⃣ Usuário abre diálogo<br/>de pareamento"]
        Step2["2️⃣ Sistema sugere FIFO<br/>ORDER BY data_entrada<br/>mas usuário escolhe lote"]
        Step3["3️⃣ Sistema valida<br/>- Mesmo produto ✅<br/>- Mesmo fornecedor ✅<br/>- Quantidade disponível ✅"]
        Step4["4️⃣ Criar alocacao<br/>INSERT INTO alocacoes<br/>venda_item_id ↔ lote_id"]
        Step5["5️⃣ Atualizar estoque<br/>quantidade_disponível -= qtd<br/>quantidade_reservada += qtd"]
        Step6["6️⃣ Registrar movimento<br/>INSERT movimentacao<br/>tipo: SAIDA_VENDA"]

        Step1 --> Step2 --> Step3 --> Step4 --> Step5 --> Step6
    end

    Proposed --> Database

    subgraph Database["CONSTRAINTS NO BD<br/>Proteção automática"]
        direction LR

        Constraint1["UNIQUE INDEX<br/>alocacoes(venda_item_id)<br/>WHERE NOT is_estornado<br/>⚠️ 1 aloc. por item"]

        Constraint2["UNIQUE INDEX<br/>alocacoes(lote_id)<br/>WHERE NOT is_estornado<br/>⚠️ 1 item por lote"]

        Constraint1 --- Constraint2
    end

    Database --> Examples

    subgraph Examples["EXEMPLOS PRÁTICOS"]
        direction TB

        Ex1["CASO 1: Devolução Parcial<br/>Alocação original: 100 un<br/>Cliente devolve 30<br/>➜ Dividir venda_item<br/>   - 30 un (DEVOLVIDO)<br/>   - 70 un (ESTOQUE)"]

        Ex2["CASO 2: Múltiplos Lotes<br/>Venda 100 un, tem 3 lotes:<br/>- Lote 1 (T01): 40 un → aloca<br/>- Lote 2 (T01): 35 un → aloca<br/>- Lote 3 (T02): 25 un → aloca<br/>= 3 alocações para 1 venda_item"]

        Ex1 --> Ex2
    end
```

---

## Real-World Scenarios: Complex Allocations

### Scenario 1: Multiple NFes from One Purchase Order

```mermaid
flowchart TB
    Start["Cenário: Compra 100un<br/>mas fornecedor envia em 3 NFes"] --> PO["compras<br/>quantidade: 100"]

    PO --> NFe1["NFe #001<br/>30 unidades<br/>data: 2025-01-05"]
    PO --> NFe2["NFe #002<br/>50 unidades<br/>data: 2025-01-08"]
    PO --> NFe3["NFe #003<br/>20 unidades<br/>data: 2025-01-10"]

    NFe1 --> Lote1["estoque_lotes #1<br/>30 un, FIFO=2025-01-05"]
    NFe2 --> Lote2["estoque_lotes #2<br/>50 un, FIFO=2025-01-08"]
    NFe3 --> Lote3["estoque_lotes #3<br/>20 un, FIFO=2025-01-10"]

    Lote1 --> Venda["venda_itens #100<br/>quantidade: 100un<br/>status: PENDENTE"]
    Lote2 --> Venda
    Lote3 --> Venda

    Venda --> Decision{"Alocar de 1 ou<br/>múltiplos lotes?"}

    Decision -->|"Opção 1: Uma alocação<br/>(maior quantidade primeiro)"| Split["DIVIDIR venda_item:<br/>venda_itens #100 (50un)<br/>+ venda_itens #101 (30un)<br/>+ venda_itens #102 (20un)"]

    Decision -->|"Opção 2: Consolidar<br/>lotes"| Consolidate["Mover estoque entre<br/>blocos físicos<br/>(ENTRADA_AJUSTE)"]

    Split --> A1["alocacoes #1<br/>venda_item #100 ↔ lote #2<br/>(50un, FIFO recente)"]
    Split --> A2["alocacoes #2<br/>venda_item #101 ↔ lote #1<br/>(30un, FIFO antigo)"]
    Split --> A3["alocacoes #3<br/>venda_item #102 ↔ lote #3<br/>(20un, FIFO recente)"]

    Consolidate --> A1

    A1 --> End["✅ 100un alocadas<br/>em 3 alocacoes<br/>(respeitando FIFO)"]
    A2 --> End
    A3 --> End

    style Split fill:#e1f5ff
    style Consolidate fill:#fff3e0
```

**Key Points:**
- Cada NFe = 1 `estoque_lotes` (respeita FIFO)
- 1 venda_item pode necessitar de múltiplas alocações
- **Solução**: dividir venda_item em splits (parent_id) + múltiplas alocações
- Alternativa: consolidar lotes (1 entrada_ajuste em bloco único)

---

### Scenario 2: Partial Delivery (Cliente quer parte, resto depois)

```mermaid
flowchart TB
    Start["Cenário: Venda 100un<br/>Entrega 1: 40un<br/>Entrega 2: 60un (depois)"] --> VI["venda_itens #100<br/>quantidade: 100un<br/>status: ESTOQUE"]

    VI --> Alocacao["alocacoes #1<br/>venda_item #100 ↔ lote #5<br/>quantidade: 100un"]

    Alocacao --> Status1["status = ESTOQUE<br/>(quantidade_reservada = 100)"]

    Status1 --> Entrega1["ENTREGA #1<br/>data: 2025-01-20<br/>agendada: 40un"]

    Entrega1 --> Split["SPLIT: venda_item #100"]

    Split --> VI_Entregue["venda_itens #100<br/>quantidade: 40un<br/>status: ENTREGUE<br/>parent_id: NULL<br/>root_id: NULL<br/>(original reduzido)"]

    Split --> VI_Pendente["venda_itens #103<br/>quantidade: 60un<br/>status: ENTREGA_AGENDADA<br/>parent_id: 100<br/>root_id: 100<br/>(novo split)"]

    VI_Entregue --> Mov1["estoque_consumos #1<br/>venda_item #100 → lote #5<br/>quantidade: 40un"]

    VI_Pendente --> Reserva["lote #5 ainda tem:<br/>quantidade_disponível: 0<br/>quantidade_reservada: 100<br/>(60un pendentes)"]

    Reserva --> Entrega2["ENTREGA #2<br/>data: 2025-02-10<br/>agendada: 60un restantes"]

    Entrega2 --> VI_EntregueResto["venda_itens #103<br/>status: ENTREGUE"]

    VI_EntregueResto --> Mov2["estoque_consumos #2<br/>venda_item #103 → lote #5<br/>quantidade: 60un"]

    Mov2 --> NFeSaida["NFe SAIDA (tipo: SAIDA)<br/>referencia: venda_items #100, #103<br/>(ambas entregues)"]

    NFeSaida --> End["✅ Venda completa<br/>100un em 2 entregas<br/>1 alocação"]

    style VI_Entregue fill:#c8e6c9
    style VI_Pendente fill:#ffe0b2
```

**Key Points:**
- Alocação permanece 1:1 (lote ↔ venda_item original)
- Split usa parent_id para rastrear entregas parciais
- UNIQUE (venda_item_id) na alocação continua válida
- NFe SAIDA referencia ambas as partes da split

---

### Scenario 3: Items Break After Delivery → Re-fulfillment

```mermaid
flowchart TB
    Start["Cenário: Entregue 100un<br/>Cliente reporta: 10un quebradas"] --> Delivered["venda_itens #100<br/>status: ENTREGUE<br/>alocacoes #1 ativo"]

    Delivered --> Report["Cliente reporta quebra<br/>data: 2025-01-25<br/>quantidade: 10un"]

    Report --> Split1["SPLIT: venda_itens #100"]

    Split1 --> VI_Bom["venda_itens #100<br/>quantidade: 90un<br/>status: ENTREGUE<br/>(reduzido)"]

    Split1 --> VI_Quebrado["venda_itens #104<br/>quantidade: 10un<br/>status: DEVOLVIDO<br/>parent_id: 100<br/>root_id: 100<br/>split_reason: QUEBRA"]

    VI_Quebrado --> Estorno["UPDATE alocacoes #1<br/>is_estornado = TRUE<br/>estornado_por = vendedor_id<br/>estorno_motivo = 'Quebra após entrega'"]

    Estorno --> RestoreStock["UPDATE estoque_lotes #5<br/>quantidade_disponível += 10<br/>quantidade_reservada -= 10"]

    RestoreStock --> NFeDevolucao["NFe DEVOLUCAO_ENTRADA<br/>tipo: DEVOLUCAO_ENTRADA<br/>nfe_referenciada_id: (NFe original)<br/>cfop: 1411 (devolução)<br/>quantidade: 10un"]

    NFeDevolucao --> Mov["estoque_movimentacoes<br/>tipo: ENTRADA_AJUSTE<br/>quantidade: +10<br/>observacoes: 'Devolução por quebra'"]

    Mov --> Inventory["lote #5 reabastecido:<br/>quantidade_disponível: 10<br/>status: DISPONIVEL"]

    Inventory --> NewSale["Cliente pede reposição<br/>venda_itens #105<br/>quantidade: 10un<br/>origem: ESTOQUE"]

    NewSale --> NewAloc["alocacoes #2<br/>venda_item #105 ↔ lote #5<br/>quantidade: 10un<br/>(mesmo lote, dados originais)"]

    NewAloc --> Delivered2["✅ Reposição entregue<br/>Data: 2025-02-05"]

    style VI_Quebrado fill:#ffcdd2
    style Inventory fill:#c8e6c9
    style Delivered2 fill:#c8e6c9

    Delivered --> End["Auditoria completa:<br/>- Estorno rastreado<br/>- NFe registrada<br/>- Reposição documentada"]
```

**Key Points:**
- Quebra = split do venda_item original
- Estorno (soft delete) preserva auditoria
- NFe DEVOLUCAO_ENTRADA registra oficialmente
- Estoque reabastecido e pronto para nova alocação
- Mesmo lote pode ser alocado novamente

---

## Status State Machines: Venda vs Compra vs Entrega

```mermaid
stateDiagram-v2
    [*] --> PENDENTE_VENDA: Venda criada

    PENDENTE_VENDA --> EM_COMPRA: origem=COMPRA
    PENDENTE_VENDA --> ESTOQUE: origem=ESTOQUE
    PENDENTE_VENDA --> CANCELADO: Cancelar

    EM_COMPRA --> CONFIRMADO: Fornecedor confirma
    EM_COMPRA --> CANCELADO: Cancelar

    CONFIRMADO --> FATURADO: NFe recebida
    CONFIRMADO --> CANCELADO: Cancelar

    FATURADO --> EM_TRANSITO: Saiu fornecedor
    EM_TRANSITO --> EM_RECEBIMENTO: Chegou
    EM_RECEBIMENTO --> ESTOQUE: Conferido
    FATURADO --> CANCELADO: Cancelar

    ESTOQUE --> ENTREGA_AGENDADA: Agendar entrega
    ESTOQUE --> CANCELADO: Cancelar

    ENTREGA_AGENDADA --> EM_ENTREGA: Saiu para entrega
    ENTREGA_AGENDADA --> ESTOQUE: Desagendar

    EM_ENTREGA --> ENTREGUE: Confirmar entrega
    EM_ENTREGA --> ESTOQUE: Entrega falhou

    ENTREGUE --> DEVOLVIDO: Cliente devolveu

    CANCELADO --> [*]
    DEVOLVIDO --> [*]
```

---

## ENUMs: Complete Type Safety

```mermaid
flowchart TB
    subgraph ENUMs["15 ENUMs POSTGRESQL"]
        direction TB

        subgraph Pessoa["Pessoa"]
            PF["pessoa_tipo:<br/>PF, PJ"]
        end

        subgraph Orcamento["Orçamento"]
            OS["orcamento_status:<br/>RASCUNHO → ENVIADO →<br/>APROVADO → CONVERTIDO →<br/>EXPIRADO / CANCELADO"]
        end

        subgraph Venda["Venda"]
            VS["venda_status:<br/>ABERTA → PARCIAL →<br/>CONCLUIDA / CANCELADA"]
            VIS["venda_item_status:<br/>12 estados"]
            VIO["venda_item_origem:<br/>COMPRA, ESTOQUE"]
        end

        subgraph Compra["Compra"]
            CS["compra_status:<br/>RASCUNHO → ENVIADA →<br/>CONFIRMADA → PARCIAL →<br/>RECEBIDA / CANCELADA"]
            CIS["compra_item_status:<br/>PENDENTE → CONFIRMADO →<br/>FATURADO → EM_TRANSITO →<br/>RECEBIDO / CANCELADO"]
        end

        subgraph NFe["NFe"]
            NT["nfe_tipo:<br/>ENTRADA, SAIDA,<br/>DEVOLUCAO_ENTRADA,<br/>DEVOLUCAO_SAIDA"]
            NS["nfe_status:<br/>RASCUNHO → PENDENTE →<br/>PROCESSANDO → AUTORIZADA<br/>/ REJEITADA / CANCELADA"]
        end

        subgraph Estoque["Estoque"]
            ELS["estoque_lote_status:<br/>DISPONIVEL → RESERVADO →<br/>ESGOTADO / BLOQUEADO"]
            MT["movimentacao_tipo:<br/>ENTRADA_*, SAIDA_*"]
        end

        subgraph Entrega["Entrega"]
            ES["entrega_status:<br/>AGENDADA → EM_CARREGAMENTO →<br/>EM_TRANSITO → ENTREGUE<br/>/ PARCIAL / NAO_ENTREGUE"]
        end

        subgraph Financeiro["Financeiro"]
            FS["financeiro_status:<br/>PENDENTE → AGENDADO →<br/>PAGO / RECEBIDO / ATRASADO"]
            FP["forma_pagamento:<br/>DINHEIRO, PIX,<br/>CARTAO_*, BOLETO, etc"]
        end

        PF --> Pessoa
        OS --> Orcamento
        VS --> Venda
        VIS --> Venda
        VIO --> Venda
        CS --> Compra
        CIS --> Compra
        NT --> NFe
        NS --> NFe
        ELS --> Estoque
        MT --> Estoque
        ES --> Entrega
        FS --> Financeiro
        FP --> Financeiro
    end

    ENUMs --> Benefit

    subgraph Benefit["BENEFÍCIOS"]
        direction LR

        TypeSafe["✅ Type-safety<br/>Sem strings<br/>mágicas"]

        Validate["✅ Transições<br/>Validadas em<br/>trigger"]

        Query["✅ Queries<br/>Tipadas<br/>Mais rápido"]
    end
```

---

## NFe Strategy: Flexible JSONB for Tax Reforms

```mermaid
flowchart TB
    subgraph Problem["PROBLEMA ATUAL<br/>35+ colunas em estoque/estoque_has_consumo<br/>Para ICMS, IPI, PIS, COFINS<br/>❌ Reforma Tributária IBS/CBS: +35 colunas 2026-2033"]
    end

    Problem --> Solution

    subgraph Solution["SOLUÇÃO: JSONB em nfe_itens"]
        direction TB

        NFeHeader["nfes (header)<br/>id, numero, serie, chave<br/>tipo, status<br/>xml_original (raw)<br/>xml_protocolo (com protocolo)"]

        NFeItems["nfe_itens (itens)<br/>numero_item<br/>dados JSONB<br/>(preserva campos desconhecidos)"]

        NFeHeader --> NFeItems
    end

    Solution --> Structure

    subgraph Structure["ESTRUTURA JSONB dados"]
        direction TB

        BaseFields["Campos Mínimos:<br/>cfop, ncm, cest<br/>descricao, quantidade<br/>valor_unitario, valor_total"]

        ICMS["icms JSON:<br/>cst, origem, modalidade_bc<br/>valor_bc, aliquota, valor"]

        ICMSST["icms_st JSON:<br/>modalidade_bc, mva<br/>valor_bc, aliquota, valor"]

        IPI["ipi JSON:<br/>cst, valor_bc, aliquota, valor"]

        PIS["pis JSON:<br/>cst, valor_bc, aliquota, valor"]

        COFINS["cofins JSON:<br/>cst, valor_bc, aliquota, valor"]

        Future["Reforma 2026-2033:<br/>config_ibs_cbs JSON<br/>(adiciona campos novos<br/>sem migration)"]

        BaseFields --> ICMS
        ICMS --> ICMSST
        ICMSST --> IPI
        IPI --> PIS
        PIS --> COFINS
        COFINS --> Future
    end

    Structure --> Query

    subgraph Query["QUERIES FLEXÍVEIS"]
        direction LR

        Q1["Buscar por CFOP<br/>WHERE dados->>'cfop' = '5102'"]
        Q2["ICMS > 0<br/>WHERE (dados->'icms'->>'valor')::numeric > 0"]
        Q3["Soma de ICMS-ST<br/>SUM((dados->'icms_st'->>'valor')::numeric)"]

        Q1 --- Q2
        Q2 --- Q3
    end

    Query --> Index

    subgraph Index["ÍNDICES"]
        direction LR

        GIN["CREATE INDEX<br/>ON nfe_itens<br/>USING GIN(dados)<br/>✅ Full-text fiscal"]

        Specific["CREATE INDEX<br/>ON nfe_itens<br/>((dados->>'cfop'))<br/>✅ Específico (conforme profiling)"]

        GIN --- Specific
    end
```

---

## Product Versionization: SCD-2 Pattern

```mermaid
flowchart TB
    subgraph Current["PROBLEMA: Preço Estático"]
        direction LR

        Problem["❌ Atualizar produto.preco_venda<br/>↓<br/>Perde histórico<br/>❌ Não sabe preço em 2024-06"]
    end

    Current --> SCD2

    subgraph SCD2["SOLUÇÃO: Slowly Changing Dimension Type 2"]
        direction TB

        Main["produtos<br/>id, fornecedor_id, codigo_comercial<br/>(dados imutáveis)"]

        PriceTable["produto_precos<br/>produto_id, custo, valor_venda<br/>vigente_de, vigente_ate<br/>margem (GENERATED)"]

        View["VIEW produto_preco_atual<br/>SELECT * FROM produto_precos<br/>WHERE vigente_de <= TODAY<br/>  AND (vigente_ate IS NULL OR vigente_ate >= TODAY)<br/>ORDER BY produto_id, vigente_de DESC"]

        Main --> PriceTable
        PriceTable --> View
    end

    SCD2 --> Example

    subgraph Example["EXEMPLO"]
        direction TB

        Ex["Histórico de preços do Produto #1:<br/><br/>id | custo | venda | vigente_de | vigente_ate<br/>1  | 30.00 | 45.00 | 2024-01-01 | 2024-05-31<br/>2  | 30.00 | 50.00 | 2024-06-01 | 2024-12-31<br/>3  | 32.00 | 55.00 | 2025-01-01 | NULL (vigente)"]
    end

    Example --> Audit

    subgraph Audit["AUDITORIA DE PREÇO"]
        direction LR

        Audit1["Saber preço em data X:<br/>SELECT * FROM produto_precos<br/>WHERE produto_id = 1<br/>  AND vigente_de <= '2024-07-15'<br/>  AND vigente_ate >= '2024-07-15'"]

        Audit2["Quem atualizou:<br/>SELECT * FROM audit_log<br/>WHERE tabela = 'produto_precos'<br/>  AND registro_id = 2"]

        Audit1 --- Audit2
    end
```

---

## Inventory Views: Helper Queries

```mermaid
flowchart TB
    subgraph Views["2 VIEWS IMPORTANTES"]
        direction TB

        Saldos["estoque_saldos<br/>SELECT loja_id, produto_id, fornecedor_id<br/>SUM(quantidade_disponível) as disponivel<br/>SUM(quantidade_reservada) as reservado<br/>SUM(...) as total<br/>custo_medio<br/>GROUP BY loja_id, produto_id<br/><br/>👁️ Dashboard de estoque"]

        FIFO["estoque_fifo<br/>SELECT el.*, ROW_NUMBER() OVER<br/>(PARTITION BY produto_id, loja_id<br/> ORDER BY data_entrada)<br/><br/>👁️ Sugestão FIFO para alocação"]
    end

    Views --> Queries

    subgraph Queries["QUERY EXAMPLES"]
        direction TB

        Q1["Estoque baixo<br/>SELECT * FROM estoque_saldos<br/>WHERE disponivel < produto.estoque_minimo"]

        Q2["Próximo FIFO a alocar<br/>SELECT * FROM estoque_fifo<br/>WHERE produto_id = 123<br/>  AND ordem_fifo = 1<br/>  AND quantidade_disponível > 0"]

        Q3["Valor em estoque<br/>SELECT SUM(quantidade_disponível * custo_unitario)<br/>FROM estoque_lotes<br/>WHERE produto_id = 123"]

        Q1 --> Q2
        Q2 --> Q3
    end
```

---

## Triggers: Automatic Data Protection

```mermaid
flowchart TB
    subgraph Triggers["6 TRIGGERS AUTOMÁTICOS"]
        direction TB

        T1["fn_validar_alocacao()<br/>➜ Valida regras antes INSERT<br/>- Quantidade igual<br/>- Mesmo produto<br/>- Mesmo fornecedor<br/>- Status permite<br/>- Estoque suficiente"]

        T2["fn_apos_alocacao()<br/>➜ Atualiza estoque após alocação<br/>- quantidade_disponível -=<br/>- quantidade_reservada +=<br/>- Status DISPONIVEL→RESERVADO<br/>- Registra movimentação"]

        T3["fn_audit_trigger()<br/>➜ Log automático em INSERT/UPDATE/DELETE<br/>- dados_antigos JSONB<br/>- dados_novos JSONB<br/>- campos_alterados TEXT[]"]

        T4["fn_validar_transicao_venda_item()<br/>➜ Valida transições de status<br/>- Garante fluxo válido<br/>- Rejeita transições inválidas"]

        T5["Triggers financeiros<br/>➜ Atualizar saldo_devedor cliente<br/>➜ Atualizar saldo fornecedor"]

        T6["Triggers de NFe<br/>➜ Criar movimentações de estoque<br/>➜ Validar chave NFe"]
    end
```

---

## Comparison: Legacy vs Proposed

```mermaid
flowchart LR
    subgraph Legacy["❌ LEGADO"]
        direction TB

        L1["Tabelas L1/L2<br/>venda_has_produto +<br/>venda_has_produto2"]
        L2["idRelacionado cadeias<br/>(confuso)"]
        L3["fornecedor VARCHAR<br/>em 9 tabelas"]
        L4["Status strings<br/>'Em Estoque'"]
        L5["FIFO automático<br/>(quebrado)"]
        L6["~30 colunas<br/>fiscais em estoque"]
        L7["Sem auditoria"]
        L8["Devoluções incompletas"]
        L9["Mega-tabela produto<br/>100+ colunas"]
    end

    Legacy -->|"MIGRAÇÃO"| Proposed

    subgraph Proposed["✅ PROPOSTO"]
        direction TB

        P1["Tabela única venda_itens<br/>parent_id + root_id"]
        P2["Hierarquia clara<br/>(auto-referência)"]
        P3["fornecedor_id FK<br/>em todo lugar"]
        P4["ENUMs PostgreSQL<br/>com transições"]
        P5["Seleção manual<br/>+ sugestão FIFO"]
        P6["JSONB em nfe_itens<br/>(flexível)"]
        P7["audit_log completo<br/>(triggers automáticos)"]
        P8["Fluxo completo<br/>+ NFe DEVOLUCAO_*"]
        P9["produtos + produto_precos +<br/>produto_tributos"]
    end
```

---

## Statistics

```
SCHEMA PROPOSTO STATISTICS:
├── ENUMs........................... 15
├── Tabelas......................... 32
├── Views........................... 2
├── Triggers........................ 6
├── Índices......................... ~40
├── Constraints..................... ~30
├── Foreign Keys.................... ~25
└── CHECK Constraints............... ~15

MAIOR MUDANÇA:
• Dados fiscais: ~30 colunas → JSONB em nfe_itens
• Tabelas: L1+L2 (2) → Única venda_itens (1)
• Referências: VARCHAR → FK
• Status: Strings → ENUMs
• Auditoria: 0 → triggers automáticos
```

---

## Key Implementation Notes

```mermaid
flowchart TB
    subgraph Notes["⚠️ NOTAS IMPORTANTES"]
        direction TB

        N1["1️⃣ Produto obrigatório tem fornecedor_id<br/>➜ Garante origem<br/>➜ Simplifica relacionamentos"]

        N2["2️⃣ Preços são versionados (SCD-2)<br/>➜ Preserva histórico<br/>➜ Queries em date específica possível"]

        N3["3️⃣ Alocações são soft-delete<br/>➜ is_estornado = TRUE (nunca DELETE)<br/>➜ Auditoria completa"]

        N4["4️⃣ Movimentações são imutáveis<br/>➜ INSERT apenas<br/>➜ Histórico garantido"]

        N5["5️⃣ NFe itens em JSONB<br/>➜ Pronto para reforma tributária<br/>➜ Sem novas migrations"]

        N6["6️⃣ Split automático por parent_id/root_id<br/>➜ Entregas parciais<br/>➜ Devoluções parciais"]
    end
```

---

## References

- Industry Patterns: SAP MM, Odoo (stock.quant/stock.move), ERPNext
- Decision Rationale: `./.claude/next-gen/03-decisoes/02-schema-redesenhado.md`
- Problems Addressed: `./.claude/next-gen/02-analise/02-melhorias.md`
- Full Implementation: `./.claude/next-gen/rascunhos/schema-proposto.md`
