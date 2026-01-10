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

    subgraph CommercialFlow["🛍️ FLUXO COMERCIAL"]
        direction TB

        subgraph Quotation["Orçamentos"]
            Orcamento["orcamentos"]
            OrcItem["orcamento_itens"]
            Orcamento --> OrcItem
        end

        subgraph Sales["Vendas"]
            Venda["vendas"]
            VendaItem["venda_itens<br/>(origem: COMPRA ou ESTOQUE)"]
            Venda --> VendaItem
        end

        subgraph Purchase["Compras"]
            Compra["compras"]
            CompraItem["compra_itens"]
            Compra --> CompraItem
        end

        Quotation -->|"Converter"| Sales
        Sales -->|"Se origem=COMPRA<br/>cria pedido"| Purchase
    end

    subgraph NFe["📄 NOTA FISCAL ENTRADA<br/>(Do Fornecedor)"]
        direction TB

        NFeTable["nfes<br/>tipo: ENTRADA<br/>compra_id"]
        NFeItens["nfe_itens (JSONB)"]
        NFeTable --> NFeItens
    end

    subgraph Inventory["📦 INVENTÁRIO<br/>(Event Sourced)"]
        direction TB

        subgraph Events["Events (Append-Only)"]
            LotesEvents["estoque_lotes_events<br/>alocacoes_events"]
        end

        subgraph Views["Materialized Views<br/>(pg_ivm)"]
            Lotes["estoque_lotes"]
            Alocacoes["alocacoes<br/>(venda_item ↔ lote)"]
        end

        subgraph Location["Localização no Galpão"]
            Blocos["galpao_blocos"]
            Localizacoes["estoque_localizacoes"]
            Blocos -.-> Localizacoes
        end

        Events --> Views
        Views --> Location
    end

    subgraph Logistics["🚚 LOGÍSTICA & SAÍDA"]
        direction TB

        subgraph LogisticCore["Entregas"]
            Entregas["entregas"]
            EntregaItem["entrega_itens"]
            Entregas --> EntregaItem
        end

        subgraph NFeSaidaBox["NFe SAÍDA (Para Cliente)"]
            NFeTableOut["nfes<br/>tipo: SAIDA"]
            NFeItensOut["nfe_itens"]
            NFeTableOut --> NFeItensOut
        end

        LogisticCore --> NFeSaidaBox
    end

    subgraph Financial["💰 FINANCEIRO (Unified)"]
        direction TB

        subgraph UnifiedParcel["Unified Parcelas"]
            FP["financeiro_parcelas<br/>(tipo: RECEBER ou PAGAR)"]
        end

        subgraph Views2["Clarity Views"]
            VRec["parcelas_receber<br/>(de Vendas)"]
            VPag["parcelas_pagar<br/>(de Compras)"]
        end

        FP --> Views2
    end

    MasterData --> CommercialFlow

    Purchase -->|"Fornecedor envia"| NFe
    NFe -->|"Cria Estoque"| Inventory

    Sales -->|"origem=ESTOQUE:<br/>aloca existente"| Alocacoes

    Inventory --> Logistics

    Sales -->|"Gera<br/>parcelas_receber"| Financial
    Purchase -->|"Gera<br/>parcelas_pagar"| Financial
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

        P5["5️⃣ AUDITORIA EVENT-SOURCED<br/>Event Sourcing + pg_ivm<br/>➜ Histórico COMPLETO de tudo<br/>➜ Views em tempo real (sem cron)<br/>➜ Imutável (fn_prevent_mutation)"]
    end
```

---

## Event Sourcing Architecture: Complete Audit Trail

```mermaid
flowchart TB
    subgraph Overview["🔄 EVENT SOURCING PATTERN"]
        direction TB

        AppInsert["🔵 APLICAÇÃO<br/>INSERT INTO vendas_events<br/>(tipo, dados_novo, usuario_id)"]

        Event["📝 EVENTO (Imutável)<br/>vendas_events<br/>event_id, entidade_id, tipo,<br/>dados_anterior, dados_novo,<br/>mudancas_totais,<br/>usuario_id, changed_at"]

        Trigger["⚡ PG_IVM DETECTS<br/>Nova linha em vendas_events"]

        View["🎯 VIEW UPDATED (Incremental)<br/>Materialized View: vendas<br/>SELECT DISTINCT ON (entidade_id)<br/>estado atual a partir eventos"]

        Query["✅ APP QUERIES<br/>SELECT * FROM vendas<br/>Sem saber de events"]

        AppInsert --> Event
        Event --> Trigger
        Trigger --> View
        View --> Query
    end

    Overview --> Levels

    subgraph Levels["📊 3 NÍVEIS DE ARTEFATOS"]
        direction TB

        L1["NÍVEL 1: Events (Append-Only)<br/>`*_events` tables<br/>• Imutáveis (fn_prevent_mutation)<br/>• Completo histórico<br/>• Rastreável (usuario_id, motivo)<br/>Exemplos: vendas_events, estoque_lotes_events,<br/>alocacoes_events, entrega_itens_events"]

        L2["NÍVEL 2: Materialized Views (Current State)<br/>`*` tables (original names)<br/>• Agregação dos eventos<br/>• Indexada, fast queries<br/>• pg_ivm atualiza incrementalmente<br/>Exemplos: vendas, estoque_lotes,<br/>alocacoes, entrega_itens"]

        L3["NÍVEL 3: Application (Queries Views)<br/>App not aware of event sourcing<br/>• Queries normal tables<br/>• No version management needed<br/>• Sees current state<br/>Backend code unchanged"]

        L1 --> L2 --> L3
    end

    Levels --> Benefits

    subgraph Benefits["✨ BENEFÍCIOS"]
        direction LR

        B1["📜 Auditoria Completa<br/>Cada mudança é evento<br/>imutável"]

        B2["⏰ Recuperação Temporal<br/>Restaurar BD em<br/>data específica"]

        B3["🔐 Compliance<br/>NFe, movimentações<br/>nunca mudam"]

        B4["⚙️ Sem Cron<br/>pg_ivm atualiza<br/>views automaticamente"]

        B5["🚀 Performance<br/>Append-only otimizado<br/>Sem UPDATE locks"]

        B1 --> B2 --> B3 --> B4 --> B5
    end
```

---

## Event Sourcing: Vendas Example (Status Transition)

```mermaid
flowchart TB
    Start["Usuário: Marcar venda<br/>como CONCLUIDA"] --> Insert["INSERT INTO vendas_events<br/>(entidade_id=5,<br/>tipo='STATUS_ALTERADO',<br/>dados_anterior={'status': 'ABERTA'},<br/>dados_novo={'status': 'CONCLUIDA'},<br/>usuario_id=1,<br/>motivo='Todos entregues')"]

    Insert --> Event["vendas_events table<br/>event_id: 1042<br/>entidade_id: 5<br/>tipo: STATUS_ALTERADO<br/>mudancas_totais: {status: {de: ABERTA, para: CONCLUIDA}}<br/>changed_at: 2025-01-10 14:30:00 BRT"]

    Event --> Immutable["🔒 IMUTÁVEL<br/>fn_prevent_mutation trigger<br/>UPDATE: ERROR<br/>DELETE: ERROR"]

    Event --> PgIVM["pg_ivm: Detecta novo evento"]

    PgIVM --> View["Materialized View: vendas<br/>SELECT DISTINCT ON (v.entidade_id)<br/>v.entidade_id as id,<br/>(v.dados_novo ->> 'numero') as numero,<br/>(v.dados_novo ->> 'status')::venda_status as status,<br/>v.changed_at,<br/>v.usuario_id<br/>FROM vendas_events v<br/>ORDER BY v.entidade_id, v.changed_at DESC"]

    View --> Updated["vendas view updated<br/>id: 5<br/>status: CONCLUIDA<br/>changed_at: 2025-01-10 14:30<br/>usuario_id: 1"]

    Updated --> Query["App: SELECT * FROM vendas WHERE id=5;<br/>Retorna status=CONCLUIDA ✅"]

    Query --> Audit["DBA: SELECT FROM vendas_events<br/>WHERE entidade_id=5<br/>Ver histórico COMPLETO"]

    style Immutable fill:#ffcdd2
    style Updated fill:#c8e6c9
    style Audit fill:#b3e5fc
```

---

## Event Sourcing: Estoque Lotes Example (Allocation)

```mermaid
flowchart TB
    Start["Alocar estoque para venda_item #100"] --> Step1["1️⃣ INSERT INTO alocacoes<br/>(venda_item_id=100,<br/>lote_id=5,<br/>quantidade=50)"]

    Step1 --> Trigger["2️⃣ Trigger fn_alocacao_criada()"]

    Trigger --> EventInsert["3️⃣ INSERT INTO estoque_lotes_events<br/>entidade_id=5<br/>tipo='QUANTIDADE_ALTERADA'<br/>dados_anterior={<br/>  quantidade_disponivel: 100,<br/>  quantidade_reservada: 0<br/>}<br/>dados_novo={<br/>  quantidade_disponivel: 50,<br/>  quantidade_reservada: 50,<br/>  status: 'RESERVADO'<br/>}"]

    EventInsert --> Event["estoque_lotes_events<br/>event_id: 2099<br/>entidade_id: 5<br/>referencia_tipo: 'alocacao'<br/>referencia_id: 1"]

    Event --> IVM["pg_ivm: Updates materialized view"]

    IVM --> ViewUpdate["estoque_lotes view updated<br/>id: 5<br/>quantidade_disponivel: 50 (era 100)<br/>quantidade_reservada: 50 (era 0)<br/>status: RESERVADO (era DISPONIVEL)"]

    ViewUpdate --> Query["App: SELECT * FROM estoque_lotes<br/>WHERE id=5<br/>Vê estado ATUAL ✅"]

    Query --> Audit["DBA: Query estoque_lotes_events<br/>Ver CADEIA de alocações"]

    style EventInsert fill:#fff9c4
    style ViewUpdate fill:#c8e6c9
    style Audit fill:#b3e5fc
```

---

## Event Sourcing: Immutability Enforcement

```mermaid
flowchart TB
    Problem["❌ Tentativa: UPDATE evento"] --> UpdateAttempt["UPDATE estoque_lotes_events<br/>SET quantidade_disponivel = 100<br/>WHERE event_id = 2099"]

    UpdateAttempt --> Trigger["🔒 fn_prevent_mutation()<br/>BEFORE UPDATE trigger"]

    Trigger --> Error["ERROR:<br/>Tabela estoque_lotes_events é imutável<br/>(append-only)<br/>Não é permitido UPDATE no registro 2099"]

    Error --> Solution["✅ SOLUÇÃO:<br/>Se precisa corrigir quantidade<br/>INSERT novo evento com<br/>dados_novo corretos"]

    Solution --> Correct["INSERT INTO estoque_lotes_events<br/>entidade_id=5<br/>tipo='QUANTIDADE_ALTERADA'<br/>dados_anterior={...}<br/>dados_novo={quantidade_disponivel: 100}<br/>motivo='Correção: erro anterior'"]

    Correct --> Result["Resultado no BD:<br/>• Evento original preservado<br/>• Novo evento registrado<br/>• Cadeia completa visível<br/>• Auditoria = rastreável"]

    style Error fill:#ffcdd2
    style Correct fill:#c8e6c9
    style Result fill:#b3e5fc
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

## Allocation (Alocação): M:N Model

```mermaid
flowchart TB
    subgraph Before["ANTES: 1:1 Rígido"]
        direction LR

        Problem["❌ Força splits complexos<br/>Múltiplos lotes<br/>= Múltiplos venda_itens<br/>= Hierarchy hell"]

        OldFlow["venda_item #100 (100un)<br/>↓<br/>Lote #1 (30un) → venda_item #100<br/>Lote #2 (50un) → venda_item #101<br/>Lote #3 (20un) → venda_item #102<br/>parent_id = 100, root_id = 100"]

        Problem --- OldFlow
    end

    Before --> Proposed

    subgraph Proposed["NOVO: M:N Flexível"]
        direction TB

        Step1["1️⃣ Usuário abre diálogo<br/>de pareamento para 1 venda_item"]
        Step2["2️⃣ Sistema sugere FIFO<br/>ORDER BY data_entrada<br/>usuário escolhe múltiplos lotes"]
        Step3["3️⃣ Sistema valida para CADA lote<br/>- Mesmo produto ✅<br/>- Mesmo fornecedor ✅<br/>- Quantidade disponível ✅<br/>- Soma <= item.quantidade ✅"]
        Step4["4️⃣ Criar múltiplas alocacoes<br/>INSERT INTO alocacoes<br/>venda_item_id ↔ lote_id (M:N)"]
        Step5["5️⃣ Atualizar estoque<br/>quantidade_disponível -= qtd<br/>quantidade_reservada += qtd"]
        Step6["6️⃣ Registrar movimentos<br/>INSERT movimentacoes<br/>tipo: SAIDA_VENDA"]

        Step1 --> Step2 --> Step3 --> Step4 --> Step5 --> Step6
    end

    Proposed --> Database

    subgraph Database["DATABASE VALIDATION<br/>Sem UNIQUE constraints!"]
        direction LR

        Trigger1["TRIGGER: Validar soma<br/>SUM(alocacoes.qtd)<br/><= venda_item.qtd"]

        Trigger2["TRIGGER: Atualizar status<br/>Se SUM = quantidade<br/>então status = ESTOQUE"]

        Trigger1 --- Trigger2
    end

    Database --> Examples

    subgraph Examples["CASOS SIMPLES"]
        direction TB

        Ex1["CASO 1: Dano Parcial<br/>alocacao #1: status=PARCIALMENTE_ESTORNADO<br/>= Sem split, sem hierarquia!"]

        Ex2["CASO 2: Múltiplos Lotes<br/>3 alocacoes para 1 venda_item<br/>= 1 registro, 3 linhas in alocacoes"]

        Ex3["CASO 3: Entrega Parcial<br/>1 alocacao, múltiplas entrega_itens<br/>= Não precisa split"]

        Ex1 --> Ex2 --> Ex3
    end
```

---

## Real-World Scenarios: M:N Allocation Model

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

    Lote1 --> Venda["venda_itens #100<br/>quantidade: 100un<br/>status: PENDENTE<br/>(1 item único!)"]
    Lote2 --> Venda
    Lote3 --> Venda

    Venda --> Allocation["Alocar 3 lotes<br/>para 1 venda_item"]

    Allocation --> A1["alocacoes #1<br/>venda_item #100 ↔ lote #2<br/>quantidade: 50un"]
    Allocation --> A2["alocacoes #2<br/>venda_item #100 ↔ lote #1<br/>quantidade: 30un"]
    Allocation --> A3["alocacoes #3<br/>venda_item #100 ↔ lote #3<br/>quantidade: 20un"]

    A1 --> Status["venda_item #100:<br/>quantidade: 100un<br/>status: ESTOQUE ✅<br/>(SUM(alocacoes.qtd) = 100)"]
    A2 --> Status
    A3 --> Status

    Status --> Query["Query: Onde vem este item?<br/>SELECT lote_id, quantidade<br/>FROM alocacoes<br/>WHERE venda_item_id = 100<br/><br/>Resultado: 3 linhas, 100un total ✅"]

    style Venda fill:#c8e6c9
    style Status fill:#c8e6c9
```

**Key Points:**
- 1 venda_item = 1 registro (sem splits!)
- Múltiplas alocacoes (M:N) ligam ao mesmo item
- Sem parent_id/root_id complexity
- FIFO respeitado: SELECT... ORDER BY data_entrada

---

### Scenario 2: Partial Delivery (Cliente quer parte, resto depois)

```mermaid
flowchart TB
    Start["Cenário: Venda 100un<br/>Entrega 1: 40un em 2025-01-20<br/>Entrega 2: 60un em 2025-02-10"] --> VI["venda_itens #100<br/>quantidade: 100un<br/>status: ESTOQUE"]

    VI --> Alocacao["alocacoes #1<br/>venda_item #100 ↔ lote #5<br/>quantidade: 100un<br/>status: ATIVO"]

    Alocacao --> Entrega1["ENTREGA #1 criada<br/>data: 2025-01-20"]

    Entrega1 --> EI1["entrega_itens #1<br/>venda_item #100<br/>quantidade: 40un<br/>status: ENTREGUE"]

    EI1 --> Estoque1["lote #5 ainda tem:<br/>quantidade_disponível: 0<br/>quantidade_reservada: 100<br/>(60un ainda alocados)"]

    Estoque1 --> Entrega2["ENTREGA #2 criada<br/>data: 2025-02-10"]

    Entrega2 --> EI2["entrega_itens #2<br/>venda_item #100<br/>quantidade: 60un<br/>status: ENTREGUE"]

    EI2 --> Query["Query: Quanto foi entregue?<br/>SELECT SUM(quantidade)<br/>FROM entrega_itens<br/>WHERE venda_item_id = 100<br/>= 100un ✅"]

    Query --> NFeSaida["NFe SAIDA (tipo: SAIDA)<br/>referencia: venda_item #100<br/>(única referência!)"]

    NFeSaida --> End["✅ Venda completa<br/>100un em 2 entregas<br/>1 venda_item<br/>1 alocacao<br/>2 entrega_itens"]

    style VI fill:#c8e6c9
    style Alocacao fill:#c8e6c9
    style End fill:#c8e6c9
```

**Key Points:**
- 1 venda_item (sem splits!)
- 1 alocacao rastreia o estoque
- 2+ entrega_itens rastreiam entregas parciais
- Nenhuma hierarchia parent_id/root_id complexa
- Query simples: quanto foi entregue = SUM(entrega_itens.qtd)

---

### Scenario 3: Items Break After Delivery → Re-fulfillment

```mermaid
flowchart TB
    Start["Cenário: Entregue 100un<br/>Cliente reporta: 10un quebradas"] --> Delivered["venda_itens #100<br/>quantidade: 100un<br/>status: ENTREGUE<br/>alocacoes #1: ATIVO"]

    Delivered --> EI["entrega_itens #1<br/>quantidade: 100un<br/>status: ENTREGUE<br/>data: 2025-01-20"]

    EI --> Report["Cliente reporta quebra<br/>data: 2025-01-25<br/>quantidade: 10un"]

    Report --> Estorno["UPDATE alocacoes #1<br/>status = PARCIALMENTE_ESTORNADO<br/>estorno_motivo = 'Quebra após entrega'<br/>estornado_por = vendedor_id<br/>estornado_em = NOW()"]

    Estorno --> RestoreStock["UPDATE estoque_lotes #5<br/>quantidade_disponível += 10<br/>quantidade_reservada -= 10"]

    RestoreStock --> Mov["estoque_movimentacoes<br/>tipo: ENTRADA_AJUSTE<br/>quantidade: +10<br/>observacoes: 'Devolução por quebra'"]

    Mov --> Inventory["lote #5 reabastecido:<br/>quantidade_disponível: 10<br/>status: DISPONIVEL"]

    Inventory --> NewItem["Criar item de reposição<br/>venda_itens #101<br/>quantidade: 10un<br/>origem: ESTOQUE<br/>(novo pedido de cliente)"]

    NewItem --> NewAloc["alocacoes #2<br/>venda_item #101 ↔ lote #5<br/>quantidade: 10un<br/>status: ATIVO"]

    NewAloc --> NewDeliv["entrega_itens #2<br/>venda_item #101<br/>quantidade: 10un<br/>status: ENTREGUE<br/>data: 2025-02-05"]

    NewDeliv --> Audit["Auditoria completa:<br/>alocacoes #1: PARCIALMENTE_ESTORNADO<br/>alocacoes #2: ATIVO → ENTREGUE<br/>Fluxo rastreável"]

    style Estorno fill:#ffcdd2
    style Inventory fill:#c8e6c9
    style NewDeliv fill:#c8e6c9

    Delivered --> End["✅ Problema resolvido<br/>1 venda_item original<br/>Dano marcado em alocacoes.status<br/>Reposição = nova venda_item"]
```

**Key Points:**
- Alocação original marcada como PARCIALMENTE_ESTORNADO
- Sem split de venda_item!
- Estoque restaurado automaticamente via trigger
- Reposição = novo venda_item + nova alocacao
- Auditoria clara no status de alocacoes

---

## Enforcement: 4-Layer Quantity Integrity

### How the System Prevents Over-Allocation

```mermaid
flowchart TB
    User["👤 USER TRIES TO ALLOCATE<br/>INSERT INTO alocacoes<br/>venda_item: 100<br/>lote: 5<br/>quantidade: 50"]

    User --> L1["🛡️ LAYER 1: CHECK Constraints<br/><br/>quantidade > 0 ?<br/>✅ Yes (50 > 0)<br/><br/>→ PASS"]

    L1 --> L2["🛡️ LAYER 2: GENERATED Columns<br/><br/>custo_total = 50 × 10.00<br/>= 500.00 (auto-calc)<br/><br/>→ PASS"]

    L2 --> L3["🛡️ LAYER 3: BEFORE INSERT Trigger<br/>fn_validar_alocacao()"]

    subgraph L3Check["Validation Checks"]
        Check1["1. SUM(alocacoes ATIVO)<br/>   WHERE venda_item=100<br/>   = 20 current"]
        Check2["2. New total?<br/>   20 + 50 = 70"]
        Check3["3. Item quantity?<br/>   100"]
        Check4["4. 70 ≤ 100? ✅ YES"]
        Check5["5. Stock available?<br/>   lote 5: 45 avail<br/>   45 ≥ 50? ❌ NO!"]

        Check1 --> Check2 --> Check3 --> Check4 --> Check5
    end

    L3 --> Reject["❌ TRANSACTION REJECTED<br/><br/>ERROR: Insufficient stock!<br/>Available 45, requested 50"]

    Reject --> Rollback["⏮️ ROLLBACK<br/>Nothing inserted<br/>Database unchanged"]

    Rollback --> App["📱 Application Error<br/>Show user message:<br/>'Not enough inventory'"]

    style User fill:#e3f2fd
    style L1 fill:#c8e6c9
    style L2 fill:#c8e6c9
    style L3 fill:#fff9c4
    style Reject fill:#ffcdd2
    style Rollback fill:#ffcdd2
    style App fill:#ffcdd2
```

**What Happened?** Trigger caught over-allocation BEFORE it could corrupt the database!

---

### Success Path: Correct Allocation

```mermaid
flowchart TB
    User["👤 USER ALLOCATES<br/>INSERT INTO alocacoes<br/>venda_item: 100<br/>lote: 5<br/>quantidade: 30"]

    User --> L1["🛡️ LAYER 1: CHECK Constraints<br/>30 > 0 ? ✅ YES"]

    L1 --> L2["🛡️ LAYER 2: GENERATED<br/>custo_total = 300.00 ✅"]

    L2 --> L3["🛡️ LAYER 3: BEFORE Trigger<br/>SUM = 20<br/>20 + 30 = 50 ≤ 100 ✅<br/>Stock 45 ≥ 30 ✅<br/>All checks PASS"]

    L3 --> INSERT["✅ INSERT SUCCEEDS<br/>Row added: venda_item=100,<br/>lote=5, qtd=30"]

    INSERT --> L4["🛡️ LAYER 4: AFTER Trigger<br/>fn_apos_alocacao()"]

    subgraph L4Auto["Automatic Updates"]
        Auto1["1. estoque_lotes #5:<br/>   disponível: 45 → 15<br/>   reservado: 0 → 30"]
        Auto2["2. estoque_movimentacoes:<br/>   INSERT: type=SAIDA_VENDA<br/>   quantidade=-30"]
        Auto3["3. venda_itens #100:<br/>   Check total alocado<br/>   20 + 30 = 50<br/>   Still < 100<br/>   Status stays PENDENTE"]

        Auto1 --> Auto2 --> Auto3
    end

    L4 --> Success["✅ TRANSACTION COMPLETE<br/>Database consistent:<br/>disponível + reservado = inicial"]

    Success --> Verify["📊 VERIFICATION<br/>SELECT FROM consistencia_estoque<br/>→ Status: OK"]

    style User fill:#e3f2fd
    style L1 fill:#c8e6c9
    style L2 fill:#c8e6c9
    style L3 fill:#fff9c4
    style L4 fill:#fff9c4
    style Success fill:#c8e6c9
    style Verify fill:#c8e6c9
```

---

### The Golden Rule Enforcement

```mermaid
flowchart TB
    Rule["🏆 GOLDEN RULE<br/><br/>quantidade_disponível + quantidade_reservada<br/>=<br/>quantidade_inicial<br/><br/>ALWAYS. EVERYWHERE."]

    Rule --> Protection["Protected By:"]

    Protection --> P1["✅ CHECK constraint<br/>chk_lote_total"]
    Protection --> P2["✅ BEFORE INSERT trigger<br/>SUM validation"]
    Protection --> P3["✅ AFTER INSERT trigger<br/>Auto-update both sides"]
    Protection --> P4["✅ Nightly verification<br/>verificar_integridade_estoque()"]

    P1 --> Example["EXAMPLE: Lot #5"]

    Example --> Initial["Initial State:<br/>inicial: 100<br/>disponível: 100<br/>reservado: 0<br/>Total: 100 ✅"]

    Initial --> Event1["EVENT: Allocate 30"]
    Event1 --> After1["After Allocation:<br/>inicial: 100<br/>disponível: 70<br/>reservado: 30<br/>Total: 100 ✅"]

    After1 --> Event2["EVENT: Allocate 20"]
    Event2 --> After2["After 2nd Allocation:<br/>inicial: 100<br/>disponível: 50<br/>reservado: 50<br/>Total: 100 ✅"]

    After2 --> Event3["EVENT: Estorno 30"]
    Event3 --> After3["After Reversal:<br/>inicial: 100<br/>disponível: 80<br/>reservado: 20<br/>Total: 100 ✅"]

    After3 --> Statement["RULE MAINTAINED<br/>At every step!<br/>No exceptions!"]

    style Rule fill:#fff176
    style Statement fill:#fff176
    style Initial fill:#c8e6c9
    style After1 fill:#c8e6c9
    style After2 fill:#c8e6c9
    style After3 fill:#c8e6c9
```

---

### Verification Layers: Catching Bugs

```mermaid
flowchart TB
    Admin["🔍 DBA MONITORING"]

    Admin --> Query1["Query View: consistencia_estoque"]

    Query1 --> Check1["Check 1:<br/>disponível + reservado = inicial?<br/>All lots: ✅ OK"]

    Check1 --> Query2["Query View: verificacao_alocacoes"]

    Query2 --> Check2["Check 2:<br/>SUM(alocacoes.qtd) ≤ item.qtd?<br/>All items: ✅ OK"]

    Check2 --> Query3["Query View: verificacao_entregas"]

    Query3 --> Check3["Check 3:<br/>SUM(entrega_itens) ≤ alocacao.qtd?<br/>All items: ✅ OK"]

    Check3 --> Nightly["⏰ NIGHTLY JOB<br/>SELECT verificar_integridade_estoque()"]

    Nightly --> NightlyCheck["6 Critical Checks:<br/>1. Over-allocated lots<br/>2. Negative disponível<br/>3. Negative reservado<br/>4. Over-allocated items<br/>5. Delivery > allocation<br/>6. Status mismatches"]

    NightlyCheck --> Result{"Any<br/>Errors?"}

    Result -->|No| AllGood["✅ All Clear<br/>Log: 'All integrity checks passed'"]

    Result -->|Yes| Error["⚠️ ALERT!<br/>Log error details<br/>integridade_log table<br/>Email admin"]

    AllGood --> End["System Healthy"]
    Error --> End

    style Admin fill:#e3f2fd
    style AllGood fill:#c8e6c9
    style Error fill:#ffcdd2
    style End fill:#c8e6c9
```

---

### What Enforcement Prevents

```mermaid
flowchart TB
    Risks["⚠️ RISKS WITHOUT ENFORCEMENT"]

    Risks --> R1["App Bug:<br/>INSERT without validation<br/>❌ Over-allocate"]

    Risks --> R2["Direct DB Query:<br/>Manual INSERT estoque<br/>❌ Negative quantity"]

    Risks --> R3["Calculation Error:<br/>Manual custo_total<br/>❌ Wrong totals"]

    Risks --> R4["Trigger Failure:<br/>Cascade not updated<br/>❌ Inconsistent state"]

    R1 --> Protection["🛡️ ENFORCEMENT CATCHES IT"]
    R2 --> Protection
    R3 --> Protection
    R4 --> Protection

    Protection --> How["HOW?"]

    How --> L1["Layer 1: CHECK<br/>Rejects negative"]

    How --> L2["Layer 2: GENERATED<br/>Can't override total"]

    How --> L3["Layer 3: Trigger<br/>Validates before insert"]

    How --> L4["Layer 4: Verify<br/>Detects corruption"]

    L1 --> Result["✅ DATABASE ALWAYS<br/>CONSISTENT"]
    L2 --> Result
    L3 --> Result
    L4 --> Result

    style Risks fill:#ffcdd2
    style R1 fill:#ffcdd2
    style R2 fill:#ffcdd2
    style R3 fill:#ffcdd2
    style R4 fill:#ffcdd2
    style Protection fill:#fff9c4
    style Result fill:#c8e6c9
```

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

## pg_ivm Setup: Installation & Configuration

```mermaid
flowchart TB
    subgraph Step1["PASSO 1: Instalar Extensão"]
        direction TB
        Install["# Uma vez (no servidor PostgreSQL)<br/>CREATE EXTENSION pg_ivm;<br/><br/>✅ pg_ivm agora está disponível"]
    end

    Step1 --> Step2

    subgraph Step2["PASSO 2: Converter Views para Incremental"]
        direction TB
        Drop["1️⃣ DROP antigas views<br/>DROP MATERIALIZED VIEW vendas;<br/>DROP MATERIALIZED VIEW estoque_lotes;<br/>DROP MATERIALIZED VIEW alocacoes;"]
        Create["2️⃣ Recriar como INCREMENTAL<br/>CREATE INCREMENTAL MATERIALIZED VIEW vendas AS<br/>SELECT DISTINCT ON (v.entidade_id)<br/>  v.entidade_id as id,<br/>  (v.dados_novo ->> 'status') as status<br/>FROM vendas_events v<br/>ORDER BY v.entidade_id, v.changed_at DESC;"]
        Drop --> Create
    end

    Step2 --> Step3

    subgraph Step3["PASSO 3: Repeat for All Views"]
        direction LR
        V1["estoque_lotes"]
        V2["alocacoes"]
        V3["entrega_itens"]
        V4["financeiro_parcelas"]
        V5["..."]
        V1 --> V2 --> V3 --> V4 --> V5
    end

    Step3 --> Complete

    subgraph Complete["✅ PRONTO!"]
        direction TB
        Now["pg_ivm cuida de TUDO:<br/>• INSERT em *_events table<br/>  ↓<br/>• pg_ivm detecta mudança<br/>  ↓<br/>• Materialized view atualizada incrementalmente<br/>  ↓<br/>• App queries view (sempre atual)<br/><br/>📌 Sem cron jobs<br/>📌 Sem REFRESH MATERIALIZED VIEW manual<br/>📌 Zero overhead (incremental, não full rebuild)"]
    end

    style Install fill:#c8e6c9
    style Complete fill:#81c784
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
