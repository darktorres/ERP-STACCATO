# Root Problem Analysis: 1:N:N Relationship Complexity

> **Essential Reading**: Read this FIRST to understand why schema redesign was necessary
> **Referenced from**: [03-decisoes/02-schema-redesenhado.md](../../03-decisoes/02-schema-redesenhado.md)
> **Type**: Problem visualization + root cause analysis

This document explains the core problem that the new schema solves:
- Old system structure (L1/L2 tables, FIFO broken, idRelacionado chains)
- Why M:N allocations are necessary
- How 3-entity model solves the problem

---

## The Problem: 1:N:N Relationship Chain

```mermaid
flowchart TB
    subgraph Problem["THE PROBLEM: Complex 1:N:N Relationships"]
        direction TB

        subgraph CustomerOrder["1. CUSTOMER ORDER (venda_has_produto2)"]
            VO["Item: Cliente pede 100 unidades<br/>produto X<br/>status: PENDENTE"]
        end

        subgraph PurchaseOrders["N. PURCHASE ORDERS (1:N)<br/>Uma venda pode ter MÚLTIPLOS pedidos"]
            PO1["Pedido Fornecedor #1<br/>Fornecedor A<br/>50 unidades"]
            PO2["Pedido Fornecedor #2<br/>Fornecedor B<br/>50 unidades"]
        end

        subgraph SupplierInvoices["N. SUPPLIER INVOICES (1:N)<br/>Cada pedido pode ter MÚLTIPLAS NFes"]
            NFe1["NFe #001<br/>30 unidades<br/>Fornecedor A"]
            NFe2["NFe #002<br/>20 unidades<br/>Fornecedor A"]
            NFe3["NFe #003<br/>50 unidades<br/>Fornecedor B"]
        end

        subgraph StockLots["N. STOCK ENTRIES (1:N)<br/>Cada NFe linha = 1 lote de estoque"]
            EST1["Estoque Lote 1<br/>30 un, T01<br/>status: DISPONÍVEL"]
            EST2["Estoque Lote 2<br/>20 un, T02<br/>status: DISPONÍVEL"]
            EST3["Estoque Lote 3<br/>50 un, T01<br/>status: DISPONÍVEL"]
        end

        VO -->|"N pedidos para atender"| PO1
        VO -->|"N pedidos para atender"| PO2
        PO1 -->|"N NFes para receber"| NFe1
        PO1 -->|"N NFes para receber"| NFe2
        PO2 -->|"N NFes para receber"| NFe3
        NFe1 -->|"1:1 cria"| EST1
        NFe2 -->|"1:1 cria"| EST2
        NFe3 -->|"1:1 cria"| EST3
    end

    Problem -->|"ATUAL: Complexo de acompanhar<br/>Sem visibilidade clara"| Current["CURRENT STATE<br/>Tabelas L1/L2<br/>Chains idRelacionado<br/>FIFO quebrado"]
```

---

## Current System Problems

```mermaid
flowchart LR
    subgraph Issues["PROBLEMAS NO SISTEMA ATUAL"]
        P1["1️⃣ Tabelas L1/L2<br/>venda_has_produto<br/>+ venda_has_produto2<br/>Sincronização difícil"]

        P2["2️⃣ FIFO Não Funciona<br/>Depende de<br/>produto.idEstoque<br/>pré-definido"]

        P3["3️⃣ Cadeias idRelacionado<br/>Splits formam árvore<br/>Difícil de rastrear<br/>Sem parent_id/root_id"]

        P4["4️⃣ Consumo Automático<br/>Pareamento automático<br/>Quebra com variação<br/>de lote (tons/calibres)"]

        P5["5️⃣ Devoluções Incompletas<br/>Sem reversão adequada<br/>Sem NFe devolução<br/>Status errado"]

        P6["6️⃣ Desnormalização<br/>fornecedor VARCHAR<br/>em 5+ tabelas<br/>Sem integridade"]
    end

    P1 --> Impact["❌ IMPACTO:<br/>Queries complexas<br/>Bugs sutis<br/>Difícil manter"]
    P2 --> Impact
    P3 --> Impact
    P4 --> Impact
    P5 --> Impact
    P6 --> Impact
```

---

## The Solution: Simplified Schema

```mermaid
flowchart TB
    subgraph Solution["A SOLUÇÃO: Schema Redesenhado"]
        direction TB

        subgraph Step1["1️⃣ SIMPLIFICAR TABELAS"]
            direction LR
            Old["❌ venda_has_produto<br/>+ venda_has_produto2<br/>(2 tabelas)"]
            Arrow1["➜"]
            New["✅ venda_itens<br/>(1 tabela única)"]
            Old --> Arrow1 --> New
        end

        subgraph Step2["2️⃣ HIERARQUIA CLARA"]
            direction LR
            Old2["❌ idRelacionado<br/>cadeias"]
            Arrow2["➜"]
            New2["✅ parent_id + root_id<br/>estrutura clara"]
            Old2 --> Arrow2 --> New2
        end

        subgraph Step3["3️⃣ FIFO CORRETO"]
            direction LR
            Old3["❌ Automático<br/>(produto.idEstoque)"]
            Arrow3["➜"]
            New3["✅ MANUAL 1:1<br/>(ORDER BY data_entrada)"]
            Old3 --> Arrow3 --> New3
        end

        subgraph Step4["4️⃣ NORMALIZAR FK"]
            direction LR
            Old4["❌ fornecedor VARCHAR<br/>em múltiplas tabelas"]
            Arrow4["➜"]
            New4["✅ fornecedor_id FK<br/>em todo lugar"]
            Old4 --> Arrow4 --> New4
        end

        Step1 --> Solution2
        Step2 --> Solution2
        Step3 --> Solution2
        Step4 --> Solution2

        subgraph Solution2["RESULTADO:<br/>✅ Tabelas normalizadas<br/>✅ Queries simples<br/>✅ FIFO correto<br/>✅ Integridade garantida"]
        end
    end
```

---

## Processing Flow: How the Solution Handles 1:N:N

```mermaid
flowchart TB
    Start["INÍCIO: Cliente pede 100 unidades"] --> Step1

    subgraph Step1["PASSO 1: Criar Venda"]
        V1["venda_itens #100<br/>quantidade: 100<br/>status: PENDENTE<br/>parent_id: NULL<br/>root_id: NULL"]
    end

    Step1 --> Step2

    subgraph Step2["PASSO 2: Gerar Pedidos de Compra"]
        direction LR
        P1["Pedido A: 50 un<br/>Fornecedor X"]
        P2["Pedido B: 50 un<br/>Fornecedor Y"]
    end

    Step2 --> Step3

    subgraph Step3["PASSO 3: Receber NFes"]
        N1["NFe #001: 30 un<br/>de Fornecedor X<br/>➜ Estoque #1"]
        N2["NFe #002: 20 un<br/>de Fornecedor X<br/>➜ Estoque #2"]
        N3["NFe #003: 50 un<br/>de Fornecedor Y<br/>➜ Estoque #3"]
    end

    Step3 --> Step4

    subgraph Step4["PASSO 4A: NFe #001 (30 un)<br/>Atender Parcialmente"]
        SP1["venda_itens #100<br/>quantidade: 30<br/>status: ESTOQUE<br/>parent_id: NULL"]
        SP2["venda_itens #101 (SPLIT)<br/>quantidade: 70<br/>status: PENDENTE<br/>parent_id: 100<br/>root_id: 100"]
        EC1["estoque_consumos<br/>venda_item: #100<br/>estoque: #1<br/>quantidade: 30"]
    end

    Step3 --> Step5

    subgraph Step5["PASSO 4B: NFe #002 (20 un)<br/>Atender Mais um Pouco"]
        SP3["venda_itens #101 (SPLIT)<br/>quantidade: 20<br/>status: ESTOQUE<br/>parent_id: 100"]
        SP4["venda_itens #102 (SPLIT)<br/>quantidade: 50<br/>status: PENDENTE<br/>parent_id: 100<br/>root_id: 100"]
        EC2["estoque_consumos<br/>venda_item: #101<br/>estoque: #2<br/>quantidade: 20"]
    end

    Step3 --> Step6

    subgraph Step6["PASSO 4C: NFe #003 (50 un)<br/>Completar Atendimento"]
        SP5["venda_itens #102 (SPLIT)<br/>quantidade: 50<br/>status: ESTOQUE<br/>parent_id: 100"]
        EC3["estoque_consumos<br/>venda_item: #102<br/>estoque: #3<br/>quantidade: 50"]
    end

    Step4 --> Query
    Step5 --> Query
    Step6 --> Query

    subgraph Query["QUERY: Obter status<br/>do pedido original"]
        Q1["SELECT * FROM venda_itens<br/>WHERE id = 100<br/>   OR root_id = 100<br/><br/>Retorna 3 linhas:<br/>- 30 un (original)<br/>- 20 un (split 1)<br/>- 50 un (split 2)<br/>Total: 100 un ✅"]
    end

    Query --> End["FIM:<br/>Pedido completamente<br/>atendido por 3<br/>estoques diferentes"]
```

---

## Key Data Structure Changes

```mermaid
flowchart TB
    subgraph Before["ANTES: Tabelas L1/L2 + idRelacionado"]
        direction TB

        L1["venda_has_produto (L1)<br/>idVendaProduto = 100<br/>idVenda = 1<br/>quantidade = 100"]
        L2["venda_has_produto2 (L2)<br/>idVendaProduto2 = 999<br/>idVendaProdutoFK = 100<br/>quantidade = 100<br/>idRelacionado = NULL"]
        L2a["venda_has_produto2 (L2 Split)<br/>idVendaProduto2 = 1000<br/>idVendaProdutoFK = 100<br/>quantidade = 30<br/>idRelacionado = 999"]
        L2b["venda_has_produto2 (L2 Split)<br/>idVendaProduto2 = 1001<br/>idVendaProdutoFK = 100<br/>quantidade = 70<br/>idRelacionado = 999"]

        L1 --> L2
        L2 --> L2a
        L2 --> L2b

        Note1["❌ Complexo:<br/>2 tabelas para 1 conceito<br/>idRelacionado pouco claro"]
    end

    Before -->|"MIGRAÇÃO"| After

    subgraph After["DEPOIS: Tabela Única venda_itens"]
        direction TB

        New1["venda_itens<br/>id = 100<br/>venda_id = 1<br/>quantidade = 100<br/>parent_id = NULL<br/>root_id = NULL"]
        New2["venda_itens (Split)<br/>id = 101<br/>venda_id = 1<br/>quantidade = 30<br/>parent_id = 100<br/>root_id = 100"]
        New3["venda_itens (Split)<br/>id = 102<br/>venda_id = 1<br/>quantidade = 70<br/>parent_id = 100<br/>root_id = 100"]

        New1 --> New2
        New1 --> New3

        Note2["✅ Simples:<br/>1 tabela, 1 conceito<br/>Hierarquia clara"]
    end
```

---

## Manual Stock Matching (vs Automatic)

```mermaid
flowchart TB
    subgraph Why["POR QUE MANUAL?"]
        Problem["Cerâmica tem variação:<br/>- Tom (T01, T02, T03)<br/>- Calibre (A, B)<br/>- Acabamento (polido, etc)<br/><br/>Cliente pediu: Tom T01<br/>NFe chegou com T02<br/><br/>❌ Automático usaria T02<br/>✅ Manual: usuário escolhe"]
    end

    Why --> Manual

    subgraph Manual["PROCESSO MANUAL: venda_item → estoque_consumos"]
        direction TB

        Start["Usuário abre diálogo<br/>de pareamento"]
        Start --> List["Sistema lista estoques<br/>disponíveis para produto"]
        List --> Select["Usuário seleciona<br/>estoque apropriado<br/>(ex: T01, lote correto)"]
        Select --> Create["INSERT estoque_consumos<br/>venda_item_id: 100<br/>estoque_id: 1<br/>quantidade: 30"]
        Create --> Update["UPDATE estoque<br/>quantidade_disponivel -= 30<br/>status = CONSUMIDO (se 0)"]
        Update --> Status["UPDATE venda_item<br/>status = ESTOQUE"]
        Status --> Done["✅ Pareamento completo"]
    end

    Manual --> Benefits["BENEFÍCIOS:<br/>✅ Controle do usuário<br/>✅ Garantir qualidade<br/>✅ Rastreabilidade<br/>✅ Sem FIFO quebrado"]
```

---

## 1:1 Consumption Constraint

```mermaid
flowchart TB
    subgraph Constraint["REGRA: Relacionamento 1:1"]
        direction TB

        Rule1["1️⃣ Cada venda_item<br/>pode ter APENAS 1<br/>estoque_consumos ATIVO<br/><br/>Índice único:<br/>UNIQUE(venda_item_id)<br/>WHERE NOT is_estornado"]

        Rule2["2️⃣ Cada estoque<br/>pode ser consumido por<br/>APENAS 1 venda_item<br/><br/>Índice único:<br/>UNIQUE(estoque_id)<br/>WHERE NOT is_estornado"]

        Rule1 --> Enforce
        Rule2 --> Enforce

        subgraph Enforce["IMPOSIÇÃO NO BD<br/>PostgreSQL valida automaticamente"]
            direction LR
            Success["✅ Insert sucesso:<br/>Regras respeitadas"]
            Fail["❌ Insert falha:<br/>UNIQUE constraint violation<br/>Mensagem clara"]
        end
    end

    Constraint --> Example

    subgraph Example["EXEMPLO"]
        direction TB

        E1["Tenta parear venda_item #100<br/>pela SEGUNDA VEZ"]
        E1 -->|"Banco rejeita"| Error["Erro:<br/>duplicate key value<br/>violates unique constraint"]
        Error --> Benefit["✅ Protege contra<br/>double-booking"]
    end
```

---

## Reversing/Canceling (Estorno)

```mermaid
flowchart TB
    Start["Usuário cancela devolução<br/>de 30 un (parcial)"] --> GetConsumption

    subgraph GetConsumption["1️⃣ Localizar consumo"]
        Q["SELECT * FROM estoque_consumos<br/>WHERE venda_item_id = :id<br/>  AND NOT is_estornado"]
        Q --> Found["Encontra:<br/>estoque_id: 1<br/>quantidade: 30"]
    end

    GetConsumption --> MarkStornado

    subgraph MarkStornado["2️⃣ Marcar como estornado<br/>(soft delete)"]
        Update["UPDATE estoque_consumos<br/>SET is_estornado = TRUE,<br/>    estornado_em = NOW(),<br/>    estorno_motivo = 'cliente devolveu'<br/>WHERE venda_item_id = 100"]
        Update --> Keep["✅ Registro mantido<br/>para auditoria"]
    end

    MarkStornado --> RestoreStock

    subgraph RestoreStock["3️⃣ Restaurar estoque"]
        RestoreQty["UPDATE estoques<br/>SET quantidade_disponivel += 30<br/>WHERE id = 1"]
        RestoreQty --> RestoreStatus["UPDATE estoques<br/>SET status = 'DISPONIVEL'<br/>WHERE id = 1"]
    end

    RestoreStock --> UpdateVenda

    subgraph UpdateVenda["4️⃣ Atualizar venda_item"]
        Decision{"Devolução<br/>parcial ou<br/>total?"}
        Decision -->|"Total"| SetDevolvido["status = 'DEVOLVIDO'"]
        Decision -->|"Parcial"| Split["Dividir venda_item<br/>Original (30): DEVOLVIDO<br/>Restante (70): ESTOQUE"]
    end

    UpdateVenda --> CreateNFe

    subgraph CreateNFe["5️⃣ NFe de Devolução"]
        NFe["Gerar NFe de devolução<br/>referenciando NFe original<br/>tipo = 'DEVOLUCAO_ENTRADA'"]
        NFe --> Submit["Submeter ao SEFAZ"]
    end

    CreateNFe --> End["✅ Reversão completa<br/>com trilha de auditoria"]
```

---

## Summary: Problem vs Solution

| Aspecto | **PROBLEMA ATUAL** | **SOLUÇÃO PROPOSTA** |
|---------|-------------------|---------------------|
| **Estrutura L1/L2** | 2 tabelas (venda_has_produto + venda_has_produto2) | 1 tabela (venda_itens) |
| **Splits** | idRelacionado cadeias | parent_id + root_id |
| **Consumo** | Automático (quebrado FIFO) | Manual 1:1 com FIFO correto |
| **Refs Fornecedor** | VARCHAR em 9 tabelas | fornecedor_id FK |
| **Pareamento** | Sem validação | Constraints de integridade |
| **Devoluções** | Incompleto | Fluxo completo + NFe |
| **Auditoria** | Nenhuma | audit_log completo |
| **Query para pedido total** | Complexa (L1+L2+idRelacionado) | Simples (parent_id/root_id) |

---

## Implementation Roadmap

```mermaid
flowchart LR
    Phase1["📋 Fase 1<br/>Criar novo schema<br/>em paralelo"] --> Phase2
    Phase2["🔄 Fase 2<br/>Migração de dados<br/>históricos"] --> Phase3
    Phase3["✏️ Fase 3<br/>Escrita dupla<br/>em ambas tabelas"] --> Phase4
    Phase4["📖 Fase 4<br/>Trocar leituras<br/>para nova schema"] --> Phase5
    Phase5["🗑️ Fase 5<br/>Depreciar<br/>tabelas antigas"]

    Phase1 --> Success["✅ Sistema modernizado<br/>sem downtime"]
```

---

## References

- Complete schema: `./.claude/next-gen/03-decisoes/02-schema-redesenhado.md`
- Current flow analysis: `./.claude/next-gen/01-contexto/02-fluxos-estoque.md`
- Improvement options: `./.claude/next-gen/02-analise/02-melhorias.md`
- Database architecture: `./.claude/next-gen/04-arquitetura/02-banco-dados.md`
