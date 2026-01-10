# ERP Staccato - Documentacao da Migracao Web

> **Status**: Fase de Planejamento
> **Ultima atualizacao**: 2025-12-29
> **Stack Alvo**: Laravel 11 + PostgreSQL 16 + Inertia/Vue

---

## Ordem de Leitura Recomendada

```text
01-contexto  ->  02-analise  ->  03-decisoes  ->  04-arquitetura  ->  05-execucao
 (o que e)      (problemas)     (escolhas)      (como construir)    (como migrar)
```

---

## Links Rapidos

| Precisa...                     | Va para                                                                                |
| ------------------------------ | -------------------------------------------------------------------------------------- |
| Entender o projeto             | [Visao Geral](#visao-geral-do-projeto)                                                 |
| Entender fluxos de negocio     | [01-contexto/01-visao-geral-fluxos.md](./01-contexto/01-visao-geral-fluxos.md)         |
| Comparar legado vs novo        | [02-analise/01-comparativo-legado-novo.md](./02-analise/01-comparativo-legado-novo.md) |
| Ver schema PostgreSQL completo | [03-decisoes/02-schema-redesenhado.md](./03-decisoes/02-schema-redesenhado.md)         |
| Ver arquitetura visual schema  | [03-decisoes/02-schema-visual-overview.md](./03-decisoes/02-schema-visual-overview.md) |
| Ver decisoes de arquitetura    | [03-decisoes/01-adrs.md](./03-decisoes/01-adrs.md)                                     |
| Ver arquitetura Laravel        | [04-arquitetura/01-arquitetura.md](./04-arquitetura/01-arquitetura.md)                 |
| Ver design do banco            | [04-arquitetura/02-banco-dados.md](./04-arquitetura/02-banco-dados.md)                 |
| Verificar fases da migracao    | [05-execucao/01-plano-migracao.md](./05-execucao/01-plano-migracao.md)                 |

---

## Visao Geral do Projeto

Reescrevendo a aplicacao ERP desktop existente em C++ Qt como uma aplicacao web moderna.

### Objetivos

1. Corrigir problemas arquiteturais no codigo legado
2. Melhorar manutenibilidade e testabilidade
3. Habilitar acesso multi-dispositivo (baseado em navegador)
4. Modernizar a stack tecnologica

### Escala do Sistema Atual

| Metrica              | Quantidade |
| -------------------- | ---------- |
| Arquivos Fonte C++   | 142        |
| Arquivos Header      | 141        |
| Formularios UI (.ui) | 87         |
| Linhas de Codigo     | ~50.000    |
| Tabelas no Banco     | 209        |
| Modulos Principais   | 7          |

---

## Estrutura da Documentacao

```text
.claude/next-gen/
├── 00-indice.md                          # Este arquivo - navegacao principal
│
├── 01-contexto/                          # FASE 1: Entender o sistema
│   ├── 01-visao-geral-fluxos.md          # Diagramas de fluxo de alto nivel
│   ├── 02-fluxos-estoque.md              # Criacao, consumo e devolucao de estoque
│   ├── 03-fluxos-entrega-nfe.md          # Entrega, NFe, CNAB, Comissao
│   ├── 04-fluxos-cadastros.md            # Dados mestres, Orcamento, Galpao, Permissoes
│   └── 05-regras-negocio.md              # Regras de negocio detalhadas
│
├── 02-analise/                           # FASE 2: Problemas e oportunidades
│   ├── 01-comparativo-legado-novo.md     # Comparativo consolidado legado vs novo
│   ├── 02-melhorias.md                   # Pontos problematicos e opcoes de melhoria
│   ├── 03-simplificacao-l1l2.md          # Analise de achatamento de tabelas L1/L2
│   ├── 04-correcao-fifo.md               # Correcao do consumo de estoque FIFO
│   └── 05-normalizacao-fornecedor.md     # FK para refs de fornecedor
│
├── 03-decisoes/                          # FASE 3: O que escolhemos
│   ├── 01-adrs.md                        # Registros de Decisao de Arquitetura
│   ├── 02-schema-redesenhado.md          # 🟢 AUTHORITATIVE - Schema PostgreSQL completo
│   └── 02-schema-visual-overview.md      # 📊 Visual companion (flowcharts + patterns)
│
├── 04-arquitetura/                       # FASE 4: Como construir
│   ├── 01-arquitetura.md                 # Estrutura Laravel, padroes, servicos
│   ├── 02-banco-dados.md                 # Redesign do schema PostgreSQL
│   ├── 03-frontend.md                    # Avaliacao de framework frontend
│   ├── 04-infraestrutura.md              # Auditoria, dados temporais, busca, performance
│   ├── 05-seguranca.md                   # Autenticacao, autorizacao, protecoes
│   ├── 06-api.md                         # Design de API REST, versionamento
│   ├── 07-testes.md                      # Estrategia de testes (unit, integration, E2E)
│   ├── 08-erros-monitoramento.md         # Tratamento de erros, logging, Sentry
│   ├── 09-integracoes.md                 # Integracoes externas (ACBr, CNAB, etc)
│   ├── 10-design-system.md               # Design system, componentes, temas
│   ├── 11-concorrencia.md                # Locks, transacoes, race conditions
│   ├── 12-atalhos-teclado.md             # Atalhos de teclado e acessibilidade
│   ├── 13-impressao.md                   # PDF, Excel, etiquetas, DANFE
│   ├── 14-devops.md                      # Docker, CI/CD, deploy, monitoramento
│   ├── 15-dicionario-dados.md            # Glossario de termos, enums, convencoes
│   ├── 16-compatibilidade.md             # Matriz de suporte browser/dispositivo
│   ├── 17-validacao.md                   # Estrategia de validacao multicamada
│   ├── 18-dependencias.md                # Auditoria de dependencias PHP/NPM
│   └── modulos/                          # Specs de implementacao por modulo
│       ├── _indice.md                    # Lista de prioridade dos modulos
│       ├── cadastros.md                  # Modulo de Cadastros (CRUD base)
│       ├── compras.md                    # Modulo de Compras
│       ├── estoque.md                    # Modulo de Estoque
│       ├── financeiro.md                 # Modulo Financeiro
│       ├── logistica.md                  # Modulo de Logistica
│       ├── nfe.md                        # Modulo NFe
│       ├── relatorios.md                 # Modulo de Relatorios
│       └── vendas.md                     # Modulo de Vendas
│
├── 05-execucao/                          # FASE 5: Como migrar
│   ├── 01-plano-migracao.md              # Fases do padrao Strangler Fig
│   ├── 02-migracao-dados.md              # Estrategia de migracao de dados
│   ├── 03-paridade-funcionalidades.md    # Checklist de paridade funcional
│   └── 04-treinamento.md                 # Plano de treinamento e rollout
│
└── rascunhos/                            # Exploracoes Ativas e Referencias
    ├── v1-v2-evolution-roadmap.md        # 🛣️ V1→V2+ incremental evolution path
    ├── 03-design-greenfield.md           # 📘 Complete system design overview (v2.0 vision)
    ├── alocacao-m2n-workflow.md          # 📘 M:N allocation workflow guide
    └── event-sourcing-analise.md         # 🔮 Event Sourcing v2+ roadmap (hybrid in v1)
```

---

## 01 - Contexto (Entender o Sistema)

| Doc                                          | Titulo                   | Descricao                                                      |
| -------------------------------------------- | ------------------------ | -------------------------------------------------------------- |
| [01](./01-contexto/01-visao-geral-fluxos.md) | Visao Geral dos Fluxos   | Diagramas de alto nivel, arquitetura L1/L2, maquinas de estado |
| [02](./01-contexto/02-fluxos-estoque.md)     | Fluxos de Estoque        | Criacao, consumo FIFO, devolucoes, algoritmo Parear            |
| [03](./01-contexto/03-fluxos-entrega-nfe.md) | Entrega, NFe, Financeiro | Agendamento, emissao NFe, CNAB 240, comissao                   |
| [04](./01-contexto/04-fluxos-cadastros.md)   | Cadastros e Outros       | Dados mestres, orcamento, galpao, permissoes                   |
| [05](./01-contexto/05-regras-negocio.md)     | Regras de Negocio        | Precificacao, impostos, validacoes, transicoes de status       |

---

## 02 - Analise (Problemas e Oportunidades)

| Doc                                              | Titulo                     | Descricao                                 |
| ------------------------------------------------ | -------------------------- | ----------------------------------------- |
| [01](./02-analise/01-comparativo-legado-novo.md) | Comparativo Legado vs Novo | Arquitetura, schema, seguranca, auditoria |
| [02](./02-analise/02-melhorias.md)               | Melhorias Propostas        | Pontos problematicos e opcoes de correcao |
| [03](./02-analise/03-simplificacao-l1l2.md)      | Simplificacao L1/L2        | Opcoes de achatamento de tabelas          |
| [04](./02-analise/04-correcao-fifo.md)           | Correcao FIFO              | Consumo correto First-In-First-Out        |
| [05](./02-analise/05-normalizacao-fornecedor.md) | Normalizacao Fornecedor    | FK em vez de VARCHAR                      |

---

## 03 - Decisoes (O Que Escolhemos)

| Doc                                                    | Titulo                   | Descricao                                          |
| ------------------------------------------------------ | ------------------------ | -------------------------------------------------- |
| [01](./03-decisoes/01-adrs.md)                         | ADRs                     | Laravel, PostgreSQL, frontend, NFe, migracao      |
| [02](./03-decisoes/02-schema-redesenhado.md)           | Schema Redesenhado       | 🟢 **AUTHORITATIVE** - Schema PostgreSQL completo |
| [02b](./03-decisoes/02-schema-visual-overview.md)      | Visual Overview          | 📊 Flowcharts, Event Sourcing, allocation models  |

---

## 04 - Arquitetura (Como Construir)

### Base

| Doc                                         | Titulo              | Descricao                                              |
| ------------------------------------------- | ------------------- | ------------------------------------------------------ |
| [01](./04-arquitetura/01-arquitetura.md)    | Arquitetura Laravel | Estrutura de diretorios, service layer, enums, eventos |
| [02](./04-arquitetura/02-banco-dados.md)    | Schema do Banco     | PostgreSQL, normalizacao, ENUMs, auditoria, FTS        |
| [03](./04-arquitetura/03-frontend.md)       | Framework Frontend  | Livewire vs Inertia+Vue vs SPA                         |
| [04](./04-arquitetura/04-infraestrutura.md) | Infraestrutura      | Auditoria, temporal, busca, cache, performance         |

### Seguranca e API

| Doc                                    | Titulo        | Descricao                                     |
| -------------------------------------- | ------------- | --------------------------------------------- |
| [05](./04-arquitetura/05-seguranca.md) | Seguranca     | Autenticacao, RBAC, protecoes OWASP           |
| [06](./04-arquitetura/06-api.md)       | Design de API | REST, versionamento, rate limiting            |
| [17](./04-arquitetura/17-validacao.md) | Validacao     | Validacao multicamada (request, business, DB) |

### Qualidade e Operacoes

| Doc                                              | Titulo                | Descricao                              |
| ------------------------------------------------ | --------------------- | -------------------------------------- |
| [07](./04-arquitetura/07-testes.md)              | Testes                | Unit, integration, E2E, coverage       |
| [08](./04-arquitetura/08-erros-monitoramento.md) | Erros e Monitoramento | Exception handling, logging, Sentry    |
| [14](./04-arquitetura/14-devops.md)              | DevOps                | Docker, CI/CD, deploy, observabilidade |

### Integracoes e Funcionalidades

| Doc                                       | Titulo       | Descricao                               |
| ----------------------------------------- | ------------ | --------------------------------------- |
| [09](./04-arquitetura/09-integracoes.md)  | Integracoes  | ACBr, CNAB, CEP, SMTP, Google Maps      |
| [11](./04-arquitetura/11-concorrencia.md) | Concorrencia | Locks otimistas/pessimistas, transacoes |
| [13](./04-arquitetura/13-impressao.md)    | Impressao    | PDF, Excel, etiquetas termicas, DANFE   |

### UI/UX

| Doc                                          | Titulo             | Descricao                             |
| -------------------------------------------- | ------------------ | ------------------------------------- |
| [10](./04-arquitetura/10-design-system.md)   | Design System      | Componentes, cores, tipografia, temas |
| [12](./04-arquitetura/12-atalhos-teclado.md) | Atalhos de Teclado | Keyboard shortcuts, command palette   |
| [16](./04-arquitetura/16-compatibilidade.md) | Compatibilidade    | Browsers, dispositivos, breakpoints   |

### Referencia

| Doc                                           | Titulo              | Descricao                                |
| --------------------------------------------- | ------------------- | ---------------------------------------- |
| [15](./04-arquitetura/15-dicionario-dados.md) | Dicionario de Dados | Glossario, enums, convencoes de nomes    |
| [18](./04-arquitetura/18-dependencias.md)     | Dependencias        | Auditoria Composer/NPM, licencas, riscos |

### [Modulos - Specs de Implementacao](./04-arquitetura/modulos/_indice.md)

| Modulo                                                  | Descricao                                    | Complexidade |
| ------------------------------------------------------- | -------------------------------------------- | ------------ |
| [cadastros.md](./04-arquitetura/modulos/cadastros.md)   | Cliente, Fornecedor, Produto, Transportadora | Baixa        |
| [compras.md](./04-arquitetura/modulos/compras.md)       | Pedidos de compra, recebimento               | Media        |
| [estoque.md](./04-arquitetura/modulos/estoque.md)       | Controle de estoque, FIFO, consumo           | Media        |
| [financeiro.md](./04-arquitetura/modulos/financeiro.md) | Contas a pagar/receber, CNAB                 | Media        |
| [vendas.md](./04-arquitetura/modulos/vendas.md)         | Orcamento, venda, faturamento                | Alta         |
| [nfe.md](./04-arquitetura/modulos/nfe.md)               | Emissao/recebimento de NFe                   | Alta         |
| [logistica.md](./04-arquitetura/modulos/logistica.md)   | Entregas, agendamento                        | Media        |
| [relatorios.md](./04-arquitetura/modulos/relatorios.md) | Relatorios e dashboards                      | Media        |

---

## 05 - Execucao (Como Migrar)

| Doc                                                | Titulo             | Descricao                             |
| -------------------------------------------------- | ------------------ | ------------------------------------- |
| [01](./05-execucao/01-plano-migracao.md)           | Plano de Migracao  | Padrao Strangler Fig, 8 fases, riscos |
| [02](./05-execucao/02-migracao-dados.md)           | Migracao de Dados  | ETL, validacao, rollback              |
| [03](./05-execucao/03-paridade-funcionalidades.md) | Paridade Funcional | Checklist de features                 |
| [04](./05-execucao/04-treinamento.md)              | Treinamento        | Plano de capacitacao e rollout        |

---

## Rascunhos

Documentos de referencia ativa (exploracoes, roadmaps, design completo):

| Doc                                                           | Titulo               | Descricao                                                  |
| ----------------------------------------------------------- | -------------------- | ---------------------------------------------------------- |
| [v1-v2-evolution](./rascunhos/v1-v2-evolution-roadmap.md)    | V1→V2+ Evolution Map | Roadmap for evolving from pragmatic v1 to enterprise v2+   |
| [design-greenfield](./rascunhos/03-design-greenfield.md)    | Complete System Design | Design overview v1.0 (antes module-specific deep-dives)   |
| [alocacao-m2n](./rascunhos/alocacao-m2n-workflow.md)        | M:N Allocation Flow  | Detailed workflow guide para modelo de alocacoes          |
| [event-sourcing](./rascunhos/event-sourcing-analise.md)     | Event Sourcing Road  | Roadmap para v2+ (ES + CQRS), hybrid approach em v1       |

---

## Legenda de Status

| Status        | Significado                       |
| ------------- | --------------------------------- |
| **Completo**  | Totalmente documentado, revisado  |
| **Rascunho**  | Conteudo inicial, precisa revisao |
| **Em aberto** | Decisao pendente                  |

---

## Como Usar Esta Documentacao

1. **Novo no projeto?** Comece com [01-contexto/01-visao-geral-fluxos.md](./01-contexto/01-visao-geral-fluxos.md)
2. **Entendendo problemas?** Veja [02-analise/01-comparativo-legado-novo.md](./02-analise/01-comparativo-legado-novo.md)
3. **Tomando decisoes?** Consulte [03-decisoes/01-adrs.md](./03-decisoes/01-adrs.md)
4. **Implementando?** Veja [04-arquitetura/](./04-arquitetura/) e [04-arquitetura/modulos/](./04-arquitetura/modulos/)
5. **Planejando migracao?** Veja [05-execucao/01-plano-migracao.md](./05-execucao/01-plano-migracao.md)
