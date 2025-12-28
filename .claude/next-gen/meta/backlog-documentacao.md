# Backlog de Documentação

> Status: **Ativo**
> Criado: 2025-12-28
> Última atualização: 2025-12-28

---

## Visão Geral

| #   | Item                                                                          | Prioridade | Status   |
| --- | ----------------------------------------------------------------------------- | ---------- | -------- |
| 1   | [Specs de Módulos Faltantes](#1-specs-de-módulos-faltantes)                   | Alta       | Pendente |
| 2   | [ADRs em Aberto](#2-adrs-em-aberto)                                           | Alta       | Pendente |
| 3   | [Arquitetura de Segurança](#3-arquitetura-de-segurança)                       | Alta       | Pendente |
| 4   | [Design de API](#4-design-de-api)                                             | Média      | Pendente |
| 5   | [Estratégia de Testes](#5-estratégia-de-testes)                               | Média      | Pendente |
| 6   | [Scripts de Migração de Dados](#6-scripts-de-migração-de-dados)               | Média      | Pendente |
| 7   | [Tratamento de Erros e Monitoramento](#7-tratamento-de-erros-e-monitoramento) | Média      | Pendente |
| 8   | [DevOps/Deployment](#8-devopsdeployment)                                      | Baixa      | Pendente |
| 9   | [Benchmarks de Performance](#9-benchmarks-de-performance)                     | Baixa      | Pendente |
| 10  | [Substituição de Relatórios](#10-substituição-de-relatórios)                  | Baixa      | Pendente |

### Ganhos Rápidos

| #   | Item                               | Esforço | Status   |
| --- | ---------------------------------- | ------- | -------- |
| A   | Fechar ADR-004 (NFe já decidido)   | Baixo   | ✅ Feito |
| B   | Atualizar links quebrados nos docs | Baixo   | ✅ Feito |

---

## 1. Specs de Módulos Faltantes

**Problema**: Apenas `compras.md` e `nfe.md` existem em `tecnico/modulos/`.

| Módulo            | Arquivo                         | Complexidade | Status   |
| ----------------- | ------------------------------- | ------------ | -------- |
| Vendas            | `tecnico/modulos/vendas.md`     | Alta         | Pendente |
| Estoque           | `tecnico/modulos/estoque.md`    | Alta         | Pendente |
| Financeiro        | `tecnico/modulos/financeiro.md` | Alta         | Pendente |
| Logística/Entrega | `tecnico/modulos/logistica.md`  | Média        | Pendente |
| Relatórios        | `tecnico/modulos/relatorios.md` | Média        | Pendente |
| Cadastros         | `tecnico/modulos/cadastros.md`  | Baixa        | Pendente |

### Vendas (Alta Prioridade)

**Por que é importante**: Fluxo central do negócio, toca praticamente todos os outros módulos.

**Conteúdo esperado**:

- Classes/Controllers atuais no C++
- Fluxo de estados (Orçamento → Venda → Entrega)
- Regras de precificação (3 níveis de desconto)
- Integração com Estoque (consumo)
- Integração com Financeiro (contas a receber)
- Integração com NFe (emissão)
- Implementação Laravel proposta

### Estoque (Alta Prioridade)

**Por que é importante**: Correção do FIFO precisa de spec detalhada.

**Conteúdo esperado**:

- Modelo de dados atual vs proposto
- Algoritmo FIFO detalhado
- Reserva vs Consumo (two-phase)
- Integração com Galpão (blocos)
- Entrada de estoque (importação NFe)
- Devoluções e estornos

### Financeiro (Alta Prioridade)

**Por que é importante**: CNAB, boletos, conciliação bancária são críticos.

**Conteúdo esperado**:

- Contas a Pagar (fluxo completo)
- Contas a Receber (fluxo completo)
- Geração de CNAB 240
- Retorno de CNAB (baixa automática)
- Conciliação bancária
- Comissões (RT)

### Logística/Entrega (Média Prioridade)

**Conteúdo esperado**:

- Agendamento de entregas
- Atribuição de veículos
- Confirmação de entrega
- Integração com NFe (transporte)

### Relatórios (Média Prioridade)

**Conteúdo esperado**:

- Inventário de relatórios existentes (LimeReport)
- Estratégia de substituição
- Relatórios críticos vs nice-to-have

### Cadastros (Baixa Prioridade)

**Conteúdo esperado**:

- CRUD de Fornecedores, Clientes, Produtos, Transportadoras
- Validações (CPF, CNPJ, CEP)
- Busca e filtros

---

## 2. ADRs em Aberto

**Arquivo**: `estrategia/02-decisoes.md`

| ADR     | Decisão                | Status         | Ação                             |
| ------- | ---------------------- | -------------- | -------------------------------- |
| ADR-003 | Framework Frontend     | Em Aberto      | Decidir: Livewire vs Inertia+Vue |
| ADR-004 | Integração NFe         | ✅ **Fechado** | ACBrMonitorConsole ou sped-nfe   |
| ADR-005 | Estratégia de Migração | Em Aberto      | Confirmar Strangler Fig          |

### ~~ADR-004: Integração NFe~~ ✅ Fechado

Fechado em 2025-12-28. Ver [estrategia/02-decisoes.md](../estrategia/02-decisoes.md#adr-004-abordagem-de-integração-nfe)

---

## 3. Arquitetura de Segurança

**Problema**: Sistema atual tem vulnerabilidades de SQL injection.

**Arquivo sugerido**: `tecnico/05-seguranca.md`

**Tópicos a documentar**:

- Autenticação (Laravel Sanctum vs Passport)
- Autorização (Policies, Gates) - migração do RBAC+PBAC atual
- Validação de entrada (Form Requests)
- Proteção CSRF/XSS
- Auditoria de segurança
- Gerenciamento de sessão
- LGPD compliance

---

## 4. Design de API

**Problema**: Mencionado como "API-first" mas sem especificação.

**Arquivo sugerido**: `tecnico/06-api.md`

**Tópicos a documentar**:

- REST vs GraphQL (decisão)
- Versionamento de API (`/api/v1/...`)
- Rate limiting
- Catálogo de webhooks
- Documentação OpenAPI/Swagger
- Formato de resposta (JSON:API? Custom?)
- Paginação, filtros, ordenação

---

## 5. Estratégia de Testes

**Problema**: Testes existem em `tests/` mas novo sistema precisa de estratégia.

**Arquivo sugerido**: `tecnico/07-testes.md`

**Tópicos a documentar**:

- Testes unitários (Services)
- Testes de integração
- Testes E2E (Cypress/Playwright)
- Factories e seeders
- Cobertura mínima exigida
- CI/CD integration

---

## 6. Scripts de Migração de Dados

**Problema**: Plano de migração existe mas falta detalhamento de dados.

**Arquivo sugerido**: `estrategia/09-migracao-dados.md`

**Tópicos a documentar**:

- Mapeamento de tabelas (antigo → novo)
- Regras de transformação (L1/L2 → tabela única, etc.)
- Procedimentos de rollback
- Queries de validação (contagens, integridade)
- Ordem de migração (dependências entre tabelas)

**Exemplo de conteúdo**:

```text
| Tabela Antiga | Tabela Nova | Transformações |
|---------------|-------------|----------------|
| venda_has_produto + venda_has_produto2 | venda_itens | Merge L1/L2 |
| fornecedor (VARCHAR) | fornecedor_id (FK) | Lookup por nome |
```

---

## 7. Tratamento de Erros e Monitoramento

**Problema**: Sem estratégia padronizada de erros e monitoramento.

**Local sugerido**: Adicionar seção em `tecnico/01-arquitetura.md`

**Tópicos a documentar**:

- Hierarquia de exceções
- Formato de resposta de erro (API)
- Logging estruturado
- Monitoramento (Sentry? Laravel Telescope?)
- Alertas

---

## 8. DevOps/Deployment

**Problema**: Sem documentação de infraestrutura de deploy.

**Arquivo sugerido**: `tecnico/08-devops.md`

**Tópicos a documentar**:

- Configuração Docker
- Pipeline CI/CD (GitHub Actions?)
- Configuração de ambientes (dev, staging, prod)
- Backup/restore
- Scaling

---

## 9. Benchmarks de Performance

**Problema**: Sem baseline de performance nem metas.

**Local sugerido**: Adicionar seção em `tecnico/04-infraestrutura.md`

**Tópicos a documentar**:

- Baseline do sistema atual
- Tempos de resposta alvo
- Otimização de queries
- Estratégia de cache (Redis)
- Lazy loading vs eager loading

---

## 10. Substituição de Relatórios

**Problema**: Sistema usa LimeReport, precisa de estratégia de substituição.

**Arquivo sugerido**: `tecnico/modulos/relatorios.md`

**Tópicos a documentar**:

- Inventário de relatórios LimeReport existentes
- Classificação (crítico vs nice-to-have)
- Escolha de biblioteca (DomPDF, Laravel Excel, Browsershot)
- Templates de relatório
- Exportação (PDF, Excel, CSV)

---

## Progresso

### Concluídos

| Data       | Item   | Descrição                                               |
| ---------- | ------ | ------------------------------------------------------- |
| 2025-12-28 | Item A | Fechado ADR-004: NFe via ACBrMonitorConsole ou sped-nfe |
| 2025-12-28 | Item B | Corrigidos links quebrados em 8 arquivos                |

### Próximos

1. [ ] Item 3 - Arquitetura de segurança
2. [ ] Item 1 - Módulo Vendas
3. [ ] Item 1 - Módulo Estoque
4. [ ] Item 6 - Migração de dados

---

## Documentos Relacionados

- [00-indice.md](../00-indice.md) - Índice principal
- [rastreador.md](./rastreador.md) - Rastreador de documentação existente
- [estrategia/02-decisoes.md](../estrategia/02-decisoes.md) - ADRs
- [estrategia/03-melhorias.md](../estrategia/03-melhorias.md) - Pontos de dor identificados
