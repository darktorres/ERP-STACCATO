# ERP Staccato - Documentação da Migração Web

> **Status**: Fase de Planejamento
> **Última atualização**: 2025-12-28
> **Stack Alvo**: Laravel 11 + PostgreSQL 16 + Inertia/Vue

---

## Links Rápidos

| Precisa...                      | Vá para                                                                                |
| ------------------------------- | -------------------------------------------------------------------------------------- |
| Entender o projeto              | [Visão Geral](#visão-geral-do-projeto)                                                 |
| **Comparar legado vs novo**     | [estrategia/00-comparativo-legado-novo.md](./estrategia/00-comparativo-legado-novo.md) |
| Ver arquitetura Laravel         | [tecnico/01-arquitetura.md](./tecnico/01-arquitetura.md)                               |
| Ver design do banco             | [tecnico/02-banco-dados.md](./tecnico/02-banco-dados.md)                               |
| Entender fluxos de negócio      | [negocios/](#fluxos-de-negócio)                                                        |
| Verificar fases da migração     | [estrategia/01-plano-migracao.md](./estrategia/01-plano-migracao.md)                   |
| Ver decisões em aberto          | [estrategia/02-decisoes.md](./estrategia/02-decisoes.md)                               |
| **Ver backlog de documentação** | [meta/backlog-documentacao.md](./meta/backlog-documentacao.md)                         |

---

## Visão Geral do Projeto

Reescrevendo a aplicação ERP desktop existente em C++ Qt como uma aplicação web moderna.

### Objetivos

1. Corrigir problemas arquiteturais no código legado
2. Melhorar manutenibilidade e testabilidade
3. Habilitar acesso multi-dispositivo (baseado em navegador)
4. Modernizar a stack tecnológica

### Escala do Sistema Atual

| Métrica              | Quantidade |
| -------------------- | ---------- |
| Arquivos Fonte C++   | 142        |
| Arquivos Header      | 141        |
| Formulários UI (.ui) | 87         |
| Linhas de Código     | ~50.000    |
| Tabelas no Banco     | 209        |
| Módulos Principais   | 7          |

---

## Estrutura da Documentação

```text
.claude/next-gen/
├── 00-indice.md                      # Este arquivo - navegação principal
│
├── estrategia/                       # Estratégia de migração
│   ├── 00-comparativo-legado-novo.md # Comparativo consolidado legado vs novo
│   ├── 01-plano-migracao.md          # Fases do padrão Strangler Fig
│   ├── 02-decisoes.md                # Registros de Decisão de Arquitetura (ADR)
│   ├── 03-melhorias.md               # Pontos problemáticos e opções de melhoria
│   ├── 04-simplificacao-l1l2.md      # Análise de achatamento de tabelas L1/L2
│   ├── 05-correcao-fifo.md           # Correção do consumo de estoque FIFO
│   ├── 06-normalizacao-fornecedor.md # FK para refs de fornecedor
│   ├── 07-esquema-redesenhado.md     # Schema completamente redesenhado
│   ├── 08-design-greenfield.md       # Design de fluxo greenfield
│   ├── 09-migracao-dados.md          # Estratégia de migração de dados
│   ├── 10-paridade-funcionalidades.md# Checklist de paridade funcional
│   └── 11-treinamento.md             # Plano de treinamento e rollout
│
├── negocios/                         # Documentação de lógica de negócio
│   ├── 01-visao-geral-fluxos.md      # Diagramas de fluxo de alto nível
│   ├── 02-fluxos-estoque.md          # Criação, consumo e devolução de estoque
│   ├── 03-fluxos-entrega-nfe.md      # Entrega, NFe, CNAB, Comissão
│   ├── 04-fluxos-cadastros.md        # Dados mestres, Orçamento, Galpão, Permissões
│   └── 05-regras-negocio.md          # Regras de negócio detalhadas
│
├── tecnico/                          # Arquitetura técnica
│   ├── 01-arquitetura.md             # Estrutura Laravel, padrões, serviços
│   ├── 02-banco-dados.md             # Redesign do schema PostgreSQL
│   ├── 03-frontend.md                # Avaliação de framework frontend
│   ├── 04-infraestrutura.md          # Auditoria, dados temporais, busca, performance
│   ├── 05-seguranca.md               # Autenticação, autorização, proteções
│   ├── 06-api.md                     # Design de API REST, versionamento
│   ├── 07-testes.md                  # Estratégia de testes (unit, integration, E2E)
│   ├── 08-erros-monitoramento.md     # Tratamento de erros, logging, Sentry
│   ├── 09-integracoes.md             # Integrações externas (ACBr, CNAB, etc)
│   ├── 10-design-system.md           # Design system, componentes, temas
│   ├── 11-concorrencia.md            # Locks, transações, race conditions
│   ├── 12-atalhos-teclado.md         # Atalhos de teclado e acessibilidade
│   ├── 13-impressao.md               # PDF, Excel, etiquetas, DANFE
│   ├── 14-devops.md                  # Docker, CI/CD, deploy, monitoramento
│   ├── 15-dicionario-dados.md        # Glossário de termos, enums, convenções
│   ├── 16-compatibilidade.md         # Matriz de suporte browser/dispositivo
│   ├── 17-validacao.md               # Estratégia de validação multicamada
│   ├── 18-dependencias.md            # Auditoria de dependências PHP/NPM
│   └── modulos/                      # Specs de implementação por módulo
│       ├── _indice.md                # Lista de prioridade dos módulos
│       ├── cadastros.md              # Módulo de Cadastros (CRUD base)
│       ├── compras.md                # Módulo de Compras
│       ├── estoque.md                # Módulo de Estoque
│       ├── financeiro.md             # Módulo Financeiro
│       ├── logistica.md              # Módulo de Logística
│       ├── nfe.md                    # Módulo NFe
│       ├── relatorios.md             # Módulo de Relatórios
│       └── vendas.md                 # Módulo de Vendas
│
└── meta/                             # Meta-documentação
    ├── backlog-documentacao.md       # Backlog de melhorias pendentes
    └── rastreador.md                 # Rastreador de progresso
```

---

## Documentação Técnica

### Arquitetura Base

| Doc | Título | Descrição |
|-----|--------|-----------|
| [01](./tecnico/01-arquitetura.md) | Arquitetura Laravel | Estrutura de diretórios, service layer, enums, eventos |
| [02](./tecnico/02-banco-dados.md) | Schema do Banco | PostgreSQL, normalização, ENUMs, auditoria, FTS |
| [03](./tecnico/03-frontend.md) | Framework Frontend | Livewire vs Inertia+Vue vs SPA |
| [04](./tecnico/04-infraestrutura.md) | Infraestrutura | Auditoria, temporal, busca, cache, performance |

### Segurança e API

| Doc | Título | Descrição |
|-----|--------|-----------|
| [05](./tecnico/05-seguranca.md) | Segurança | Autenticação, RBAC, proteções OWASP |
| [06](./tecnico/06-api.md) | Design de API | REST, versionamento, rate limiting |
| [17](./tecnico/17-validacao.md) | Validação | Validação multicamada (request, business, DB) |

### Qualidade e Operações

| Doc | Título | Descrição |
|-----|--------|-----------|
| [07](./tecnico/07-testes.md) | Testes | Unit, integration, E2E, coverage |
| [08](./tecnico/08-erros-monitoramento.md) | Erros e Monitoramento | Exception handling, logging, Sentry |
| [14](./tecnico/14-devops.md) | DevOps | Docker, CI/CD, deploy, observabilidade |

### Integrações e Funcionalidades

| Doc | Título | Descrição |
|-----|--------|-----------|
| [09](./tecnico/09-integracoes.md) | Integrações | ACBr, CNAB, CEP, SMTP, Google Maps |
| [11](./tecnico/11-concorrencia.md) | Concorrência | Locks otimistas/pessimistas, transações |
| [13](./tecnico/13-impressao.md) | Impressão | PDF, Excel, etiquetas térmicas, DANFE |

### UI/UX

| Doc | Título | Descrição |
|-----|--------|-----------|
| [10](./tecnico/10-design-system.md) | Design System | Componentes, cores, tipografia, temas |
| [12](./tecnico/12-atalhos-teclado.md) | Atalhos de Teclado | Keyboard shortcuts, command palette |
| [16](./tecnico/16-compatibilidade.md) | Compatibilidade | Browsers, dispositivos, breakpoints |

### Referência

| Doc | Título | Descrição |
|-----|--------|-----------|
| [15](./tecnico/15-dicionario-dados.md) | Dicionário de Dados | Glossário, enums, convenções de nomes |
| [18](./tecnico/18-dependencias.md) | Dependências | Auditoria Composer/NPM, licenças, riscos |

### [Módulos - Specs de Implementação](./tecnico/modulos/_indice.md)

| Módulo | Descrição | Complexidade |
|--------|-----------|--------------|
| [cadastros.md](./tecnico/modulos/cadastros.md) | Cliente, Fornecedor, Produto, Transportadora | Baixa |
| [compras.md](./tecnico/modulos/compras.md) | Pedidos de compra, recebimento | Média |
| [estoque.md](./tecnico/modulos/estoque.md) | Controle de estoque, FIFO, consumo | Média |
| [financeiro.md](./tecnico/modulos/financeiro.md) | Contas a pagar/receber, CNAB | Média |
| [vendas.md](./tecnico/modulos/vendas.md) | Orçamento, venda, faturamento | Alta |
| [nfe.md](./tecnico/modulos/nfe.md) | Emissão/recebimento de NFe | Alta |
| [logistica.md](./tecnico/modulos/logistica.md) | Entregas, agendamento | Média |
| [relatorios.md](./tecnico/modulos/relatorios.md) | Relatórios e dashboards | Média |

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

### [05 - Regras de Negócio Detalhadas](./negocios/05-regras-negocio.md)

- Precificação (sistema de 3 níveis de desconto)
- Cálculos de impostos
- Validações de CPF/CNPJ
- Regras de status e transições

---

## Documentação de Estratégia

### [00 - Comparativo Legado vs Novo](./estrategia/00-comparativo-legado-novo.md)

Visão consolidada das diferenças entre sistemas:

- Arquitetura de código (Widgets vs Service Layer)
- Schema de banco (L1/L2 vs tabela única, FIFO, ENUMs)
- Segurança (SQL injection vs Eloquent)
- Auditoria e rastreabilidade

### [01 - Plano de Migração](./estrategia/01-plano-migracao.md)

- Padrão Strangler Fig (recomendado)
- 8 fases
- Estratégias de mitigação de risco
- Requisitos de equipe

### [02 - Decisões de Arquitetura](./estrategia/02-decisoes.md)

Log de decisões no formato ADR:

- ADR-001: Backend Laravel (Aceito)
- ADR-002: Banco de dados PostgreSQL (Aceito)
- ADR-003: Framework frontend (Em aberto)
- ADR-004: Integração NFe (Aceito - ACBrMonitorConsole)
- ADR-005: Estratégia de migração (Em aberto)

### [03 - Melhorias de Fluxo e Schema](./estrategia/03-melhorias.md)

Pontos problemáticos e oportunidades de melhoria:

- Opções de simplificação de tabelas de dois níveis (L1/L2)
- Correção do consumo de estoque FIFO
- Normalização de referência de fornecedor
- Completar fluxo de devoluções
- Redesign do tratamento de status
- Divisão da tabela Produto

### Análises Profundas

| Doc | Título | Descrição |
|-----|--------|-----------|
| [04](./estrategia/04-simplificacao-l1l2.md) | Simplificação L1/L2 | Opções de achatamento de tabelas |
| [05](./estrategia/05-correcao-fifo.md) | Correção FIFO | Consumo correto First-In-First-Out |
| [06](./estrategia/06-normalizacao-fornecedor.md) | Normalização Fornecedor | FK em vez de VARCHAR |
| [07](./estrategia/07-esquema-redesenhado.md) | Schema Redesenhado | Schema completo com todas as correções |
| [08](./estrategia/08-design-greenfield.md) | Design Greenfield | Reimaginação completa do sistema |

### Execução da Migração

| Doc | Título | Descrição |
|-----|--------|-----------|
| [09](./estrategia/09-migracao-dados.md) | Migração de Dados | ETL, validação, rollback |
| [10](./estrategia/10-paridade-funcionalidades.md) | Paridade Funcional | Checklist de features |
| [11](./estrategia/11-treinamento.md) | Treinamento | Plano de capacitação e rollout |

---

## Meta-Documentação

| Doc | Descrição |
|-----|-----------|
| [backlog-documentacao.md](./meta/backlog-documentacao.md) | Backlog de melhorias pendentes |
| [rastreador.md](./meta/rastreador.md) | Rastreador de progresso |

---

## Legenda de Status

| Status        | Significado                       |
| ------------- | --------------------------------- |
| **Completo**  | Totalmente documentado, revisado  |
| **Rascunho**  | Conteúdo inicial, precisa revisão |
| **Em aberto** | Decisão pendente                  |

---

## Como Usar Esta Documentação

1. **Novo no projeto?** Comece com este índice, depois leia [negocios/01-visao-geral-fluxos.md](./negocios/01-visao-geral-fluxos.md)
2. **Planejando implementação?** Veja [estrategia/01-plano-migracao.md](./estrategia/01-plano-migracao.md)
3. **Trabalhando em um fluxo específico?** Veja o documento relevante em negocios/
4. **Tomando decisões técnicas?** Consulte tecnico/ e estrategia/02-decisoes.md
5. **Implementando um módulo?** Veja tecnico/modulos/ para specs detalhadas
