# ERP Staccato - Documentação da Migração Web

> **Status**: Fase de Planejamento
> **Última atualização**: 2025-12-27
> **Stack Alvo**: Laravel 11 + PostgreSQL 16 + Inertia/Vue (A definir)

---

## Links Rápidos

| Precisa... | Vá para |
|------------|---------|
| Entender o projeto | [Visão Geral](#visão-geral-do-projeto) |
| Ver arquitetura Laravel | [tecnico/01-arquitetura.md](./tecnico/01-arquitetura.md) |
| Ver design do banco | [tecnico/02-banco-dados.md](./tecnico/02-banco-dados.md) |
| Entender fluxos de negócio | [negocios/](#fluxos-de-negócio) |
| Verificar fases da migração | [estrategia/01-plano-migracao.md](./estrategia/01-plano-migracao.md) |
| Ver decisões em aberto | [estrategia/02-decisoes.md](./estrategia/02-decisoes.md) |

---

## Visão Geral do Projeto

Reescrevendo a aplicação ERP desktop existente em C++ Qt como uma aplicação web moderna.

### Objetivos
1. Corrigir problemas arquiteturais no código legado
2. Melhorar manutenibilidade e testabilidade
3. Habilitar acesso multi-dispositivo (baseado em navegador)
4. Modernizar a stack tecnológica

### Escala do Sistema Atual

| Métrica | Quantidade |
|---------|------------|
| Arquivos Fonte C++ | 142 |
| Arquivos Header | 141 |
| Formulários UI (.ui) | 87 |
| Linhas de Código | ~50.000 |
| Tabelas no Banco | 209 |
| Módulos Principais | 7 |

---

## Estrutura da Documentação

```
.claude/next-gen/
├── 00-indice.md                   # Este arquivo - navegação principal
│
├── tecnico/                       # Arquitetura técnica
│   ├── 01-arquitetura.md          # Estrutura Laravel, padrões, serviços
│   ├── 02-banco-dados.md          # Redesign do schema PostgreSQL
│   ├── 03-frontend.md             # Avaliação de framework frontend
│   ├── 04-infraestrutura.md       # Auditoria, dados temporais, busca
│   └── modulos/                   # Specs de implementação por módulo
│       ├── _indice.md             # Lista de prioridade dos módulos
│       ├── compras.md             # Implementação do módulo de Compras em Laravel
│       └── nfe.md                 # Opções de integração NFe
│
├── negocios/                      # Documentação de lógica de negócio
│   ├── 01-visao-geral-fluxos.md   # Diagramas de fluxo de alto nível
│   ├── 02-fluxos-estoque.md       # Criação, consumo e devolução de estoque
│   ├── 03-fluxos-entrega-nfe.md   # Entrega, NFe, CNAB, Comissão
│   └── 04-fluxos-cadastros.md     # Dados mestres, Orçamento, Galpão, Permissões
│
├── estrategia/                    # Estratégia de migração
│   ├── 01-plano-migracao.md       # Fases do padrão Strangler Fig
│   ├── 02-decisoes.md             # Registros de Decisão de Arquitetura
│   ├── 03-melhorias.md            # Pontos problemáticos e opções de melhoria
│   ├── 04-simplificacao-l1l2.md   # Análise profunda de achatamento de tabelas
│   ├── 05-correcao-fifo.md        # Correção do consumo de estoque FIFO
│   ├── 06-normalizacao-fornecedor.md # FK para refs de fornecedor
│   ├── 07-esquema-redesenhado.md  # Schema completamente redesenhado
│   └── 08-design-greenfield.md    # Design de fluxo greenfield
│
└── meta/
    └── rastreador.md              # Rastreador de progresso da documentação
```

---

## Documentação Técnica

### [01 - Arquitetura Laravel](./tecnico/01-arquitetura.md)
- Estrutura de diretórios proposta
- Padrão de camada de serviço
- Enums PHP 8.1+ para status
- Workflows orientados a eventos
- Validação com Form Request

### [02 - Schema do Banco de Dados](./tecnico/02-banco-dados.md)
- Justificativa da migração para PostgreSQL
- Normalização do schema (corrigindo nomes de fornecedor desnormalizados)
- Tipos ENUM para campos de status
- Trilha de auditoria com triggers
- Busca full-text com tsvector

### [03 - Framework Frontend](./tecnico/03-frontend.md)
- Livewire vs Inertia+Vue vs SPA Completo
- Recomendação: Inertia + Vue
- Componentes de exemplo

### [04 - Infraestrutura](./tecnico/04-infraestrutura.md)
- Arquitetura de trilha de auditoria
- Dados temporais (consultas point-in-time)
- Arquitetura de busca (PostgreSQL FTS vs Elasticsearch)
- Views materializadas para dashboards

### [modulos/ - Specs de Implementação](./tecnico/modulos/_indice.md)
Padrões de implementação Laravel módulo a módulo:
- [compras.md](./tecnico/modulos/compras.md) - Exemplos de serviço/controller do módulo de Compras
- [nfe.md](./tecnico/modulos/nfe.md) - Opções de integração NFe e interface de serviço

---

## Fluxos de Negócio

### [01 - Visão Geral dos Fluxos](./negocios/01-visao-geral-fluxos.md)
Visão de alto nível de todos os processos de negócio:
- Arquitetura de tabelas de dois níveis (L1/L2)
- Máquinas de estado de status
- Regras de integridade de dados
- Problemas conhecidos

### [02 - Fluxos de Estoque](./negocios/02-fluxos-estoque.md)
Análise profunda do gerenciamento de inventário:
- Cadeia de relacionamento 1:N:N
- Criação de estoque a partir de importação de NFe
- Algoritmo de Parear (matching)
- Lógica de consumo (problemas FIFO)
- Fluxo de devoluções e bugs

### [03 - Fluxos de Entrega, NFe e Financeiro](./negocios/03-fluxos-entrega-nfe.md)
- Agendamento e confirmação de entrega
- Emissão de NFe (integração ACBr)
- Geração de arquivo bancário CNAB 240
- Cálculo de comissão (RT)

### [04 - Cadastros e Outros Fluxos](./negocios/04-fluxos-cadastros.md)
- Fornecedor, Cliente, Produto, Transportadora
- Orçamento (sistema de desconto em três níveis)
- Galpão (blocos de armazém)
- Permissões de usuário (RBAC + PBAC)

---

## Documentação de Estratégia

### [01 - Plano de Migração](./estrategia/01-plano-migracao.md)
- Padrão Strangler Fig (recomendado)
- 8 fases em 18 meses
- Estratégias de mitigação de risco
- Requisitos de equipe

### [02 - Decisões de Arquitetura](./estrategia/02-decisoes.md)
Log de decisões no formato ADR:
- ADR-001: Backend Laravel (Aceito)
- ADR-002: Banco de dados PostgreSQL (Aceito)
- ADR-003: Framework frontend (Em aberto)
- ADR-004: Integração NFe (Em aberto)
- ADR-005: Estratégia de migração (Em aberto)

### [03 - Melhorias de Fluxo e Schema](./estrategia/03-melhorias.md)
Pontos problemáticos e oportunidades de melhoria:
- Opções de simplificação de tabelas de dois níveis (L1/L2)
- Correção do consumo de estoque FIFO
- Normalização de referência de fornecedor
- Completar fluxo de devoluções
- Redesign do tratamento de status
- Divisão da tabela Produto

### [04 - Análise Profunda da Simplificação L1/L2](./estrategia/04-simplificacao-l1l2.md)
Análise detalhada do achatamento da arquitetura de tabelas de dois níveis:
- Análise da arquitetura atual (padrão idRelacionado)
- Opção A: Tabela única com auto-referência (recomendada)
- Opção B: Manter apenas L2, derivar L1 via view materializada
- Opção C: Event sourcing (exagero para este caso)
- Estratégia de migração

### [05 - Correção do Consumo de Estoque FIFO](./estrategia/05-correcao-fifo.md)
Corrigir consumo de estoque para seguir corretamente First-In-First-Out:
- Causa raiz: `produto.idEstoque` aponta para UM estoque (sem FIFO)
- Solução: Seleção FIFO dinâmica com `ORDER BY data_entrada`
- Implementação de função PostgreSQL + serviço Laravel
- Casos extremos: consumo concorrente, FEFO, lote específico
- Estratégia de migração

### [06 - Normalização de Referência de Fornecedor](./estrategia/06-normalizacao-fornecedor.md)
Substituir nomes de fornecedor desnormalizados por referências FK adequadas:
- Problema: `fornecedor` VARCHAR armazenado em 9 tabelas (~85 refs no código)
- Solução: Usar `fornecedor_id` FK em todo lugar
- Migração: Popular FK a partir de nomes, tratar variações, remover colunas antigas
- Casos especiais: snapshots históricos, verificações de strings mágicas

### [07 - Schema Redesenhado](./estrategia/07-esquema-redesenhado.md)
Schema completamente redesenhado abordando todos os pontos problemáticos identificados:
- Princípios de design (tabela única de item, FIFO por padrão, FKs normalizadas)
- Schema PostgreSQL completo com ENUMs
- Modelo de entidade e diagrama de fluxo pedido-até-entrega
- Máquinas de estado de status com regras de transição
- Arquitetura orientada a eventos
- Caminho de migração em 5 fases

### [08 - Design Greenfield](./estrategia/08-design-greenfield.md)
Reimaginação completa dos fluxos de negócio como se construindo do zero:
- **Modelo de Fulfillment**: Itens não dividem - são atendidos em partes
- **Contextos Delimitados**: Vendas, Inventário, Compras, Entrega, Fiscal, Financeiro
- **Event Sourcing**: Estado derivado de eventos, trilha de auditoria completa
- **Reserva → Consumo**: Reivindicação de estoque em duas etapas com FIFO
- **Máquinas de Estado**: Transições explícitas com guards
- **Estratégia de Testes**: Testes unitários, de máquina de estado, integração, orientados a eventos
- **Recursos Avançados**: Entrega parcial, backorders, rastreamento de preço

---

## Legenda de Status

| Status | Significado |
|--------|-------------|
| **Completo** | Totalmente documentado, revisado |
| **Rascunho** | Conteúdo inicial, precisa revisão |
| **Em aberto** | Decisão pendente |

---

## Como Usar Esta Documentação

1. **Novo no projeto?** Comece com este índice, depois leia [negocios/01-visao-geral-fluxos.md](./negocios/01-visao-geral-fluxos.md)
2. **Planejando implementação?** Veja [estrategia/01-plano-migracao.md](./estrategia/01-plano-migracao.md)
3. **Trabalhando em um fluxo específico?** Veja o documento relevante em negocios/
4. **Tomando decisões técnicas?** Consulte tecnico/ e estrategia/02-decisoes.md
