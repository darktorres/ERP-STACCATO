# Backlog de Documentação

> Status: **Ativo**
> Criado: 2025-12-28
> Última atualização: 2025-12-28

---

## Visão Geral

| #   | Item                                                                          | Prioridade | Status   |
| --- | ----------------------------------------------------------------------------- | ---------- | -------- |
| 1   | [Specs de Módulos Faltantes](#1-specs-de-módulos-faltantes)                   | Alta       | ✅ Feito |
| 2   | [ADRs em Aberto](#2-adrs-em-aberto)                                           | Alta       | ✅ Feito |
| 3   | [Arquitetura de Segurança](#3-arquitetura-de-segurança)                       | Alta       | ✅ Feito |
| 4   | [Design de API](#4-design-de-api)                                             | Média      | ✅ Feito |
| 5   | [Estratégia de Testes](#5-estratégia-de-testes)                               | Média      | ✅ Feito |
| 6   | [Scripts de Migração de Dados](#6-scripts-de-migração-de-dados)               | Média      | ✅ Feito |
| 7   | [Tratamento de Erros e Monitoramento](#7-tratamento-de-erros-e-monitoramento) | Média      | ✅ Feito |
| 8   | [DevOps/Deployment](#8-devopsdeployment)                                      | Baixa      | Pendente |
| 9   | [Benchmarks de Performance](#9-benchmarks-de-performance)                     | Baixa      | Pendente |
| 10  | [Substituição de Relatórios](#10-substituição-de-relatórios)                  | Baixa      | Pendente |
| 11  | [Checklist de Paridade de Funcionalidades](#11-checklist-de-paridade-de-funcionalidades) | Alta | ✅ Feito |
| 12  | [Regras de Negócio Detalhadas](#12-regras-de-negócio-detalhadas)              | Alta       | ✅ Feito |
| 13  | [Documentação de Integrações](#13-documentação-de-integrações)                | Alta       | ✅ Feito |
| 14  | [Plano de Treinamento de Usuários](#14-plano-de-treinamento-de-usuários)      | Média      | ✅ Feito |
| 15  | [Design System/Guia de UI](#15-design-systemguia-de-ui)                       | Média      | ✅ Feito |
| 16  | [Estratégia de Concorrência](#16-estratégia-de-concorrência)                  | Média      | ✅ Feito |
| 17  | [Atalhos de Teclado](#17-atalhos-de-teclado)                                  | Média      | Pendente |
| 18  | [Especificações de Impressão](#18-especificações-de-impressão)                | Média      | Pendente |
| 19  | [Dicionário de Dados](#19-dicionário-de-dados)                                | Baixa      | Pendente |
| 20  | [Matriz de Suporte Browser/Dispositivo](#20-matriz-de-suporte-browserdispositivo) | Baixa  | Pendente |
| 21  | [Auditoria de Dependências](#21-auditoria-de-dependências)                    | Baixa      | Pendente |
| 22  | [Estratégia de Validação](#22-estratégia-de-validação)                        | Alta       | ✅ Feito |

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
| Vendas            | `tecnico/modulos/vendas.md`     | Alta         | ✅ Feito |
| Estoque           | `tecnico/modulos/estoque.md`    | Alta         | ✅ Feito |
| Financeiro        | `tecnico/modulos/financeiro.md` | Alta         | ✅ Feito |
| Logística/Entrega | `tecnico/modulos/logistica.md`  | Média        | ✅ Feito |
| Relatórios        | `tecnico/modulos/relatorios.md` | Média        | ✅ Feito |
| Cadastros         | `tecnico/modulos/cadastros.md`  | Baixa        | ✅ Feito |

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

| ADR     | Decisão                | Status         | Decisão Final                    |
| ------- | ---------------------- | -------------- | -------------------------------- |
| ADR-003 | Framework Frontend     | ✅ **Fechado** | Inertia + Vue 3                  |
| ADR-004 | Integração NFe         | ✅ **Fechado** | ACBrMonitorConsole ou sped-nfe   |
| ADR-005 | Estratégia de Migração | ✅ **Fechado** | Strangler Fig                    |
| ADR-006 | Multi-tenancy          | ✅ **Fechado** | BD Único com loja_id             |

### ~~ADR-003: Framework Frontend~~ ✅ Fechado

Fechado em 2025-12-28. Decisão: **Inertia + Vue 3** com TypeScript, Tailwind CSS e PrimeVue.
Ver [estrategia/02-decisoes.md](../estrategia/02-decisoes.md#adr-003-seleção-de-framework-frontend)

### ~~ADR-004: Integração NFe~~ ✅ Fechado

Fechado em 2025-12-28. Ver [estrategia/02-decisoes.md](../estrategia/02-decisoes.md#adr-004-abordagem-de-integração-nfe)

### ~~ADR-005: Estratégia de Migração~~ ✅ Fechado

Fechado em 2025-12-28. Decisão: **Strangler Fig Pattern** com banco de dados compartilhado.
Ver [estrategia/02-decisoes.md](../estrategia/02-decisoes.md#adr-005-estratégia-de-migração)

### ~~ADR-006: Multi-tenancy~~ ✅ Fechado

Fechado em 2025-12-28. Decisão: **BD Único com coluna loja_id** (padrão atual).
Ver [estrategia/02-decisoes.md](../estrategia/02-decisoes.md#adr-006-abordagem-de-multi-tenancy)

---

## 3. Arquitetura de Segurança ✅

**Arquivo**: [`tecnico/05-seguranca.md`](../tecnico/05-seguranca.md)

**Tópicos documentados**:

- ✅ Autenticação (Laravel Sanctum + Fortify)
- ✅ Autorização (Spatie Permission + Policies + Gates)
- ✅ Validação de entrada (Form Requests, CPF/CNPJ)
- ✅ Proteção CSRF/XSS/SQL Injection
- ✅ Auditoria de segurança (spatie/activitylog)
- ✅ Gerenciamento de sessão
- ✅ LGPD compliance (exportação, anonimização)
- ✅ Migração de senhas SHA → bcrypt
- ✅ Rate limiting para login

---

## 4. Design de API ✅

**Arquivo**: [`tecnico/06-api.md`](../tecnico/06-api.md)

**Tópicos documentados**:

- ✅ Arquitetura REST com JSON
- ✅ Versionamento via URL (`/api/v1/...`)
- ✅ Catálogo completo de endpoints (Cadastros, Vendas, Compras, Estoque, Financeiro, NFe, Logística)
- ✅ Autenticação com Laravel Sanctum (SPA + API tokens)
- ✅ Autorização com Gates/Policies e multi-tenancy
- ✅ Rate limiting configurável por endpoint
- ✅ Formato de resposta padronizado (sucesso, erro, coleções)
- ✅ Paginação, filtros e ordenação com Spatie Query Builder
- ✅ Sistema de webhooks com retry e assinatura
- ✅ Documentação OpenAPI com L5-Swagger
- ✅ Integrações externas (Google Maps, QualP, CEP)
- ✅ Testes de API e contratos

---

## 5. Estratégia de Testes ✅

**Arquivo**: [`tecnico/07-testes.md`](../tecnico/07-testes.md)

**Tópicos documentados**:

- ✅ Pirâmide de testes (75% unit, 20% integration, 5% E2E)
- ✅ Estrutura de diretórios (Unit, Feature, Integration, E2E)
- ✅ Ferramentas (PHPUnit, Pest, Cypress, Mockery)
- ✅ Testes unitários (Services, ValueObjects, Models, Rules)
- ✅ Testes de Feature (API, Workflows)
- ✅ Testes de integração (ACBr, CNAB)
- ✅ Testes E2E com Cypress
- ✅ Factories com states
- ✅ Mocking de serviços externos
- ✅ CI/CD com GitHub Actions
- ✅ Métricas de cobertura por módulo

---

## 6. Scripts de Migração de Dados ✅

**Arquivo**: [`estrategia/09-migracao-dados.md`](./09-migracao-dados.md)

**Tópicos documentados**:

- ✅ Mapeamento completo de 40+ tabelas (antigo → novo)
- ✅ Script SQL de merge L1/L2 com hierarquia parent_id/root_id
- ✅ Normalização de fornecedor VARCHAR → FK
- ✅ Split de tabela `produto` (100 cols → 4 tabelas)
- ✅ Migração de senhas SHA → bcrypt (lazy)
- ✅ Migração de permissões → Spatie
- ✅ Scripts de validação (contagens, integridade, somas)
- ✅ Ordem de migração em 8 fases
- ✅ Procedimento de rollback
- ✅ Comando Artisan `migracao:executar`

---

## 7. Tratamento de Erros e Monitoramento ✅

**Arquivo**: [`tecnico/08-erros-monitoramento.md`](../tecnico/08-erros-monitoramento.md)

**Tópicos documentados**:

- ✅ Hierarquia de exceções (Business, Integration, Infrastructure, Security)
- ✅ Exception Handler com JSON responses
- ✅ Logging estruturado (business, integrations, security, performance)
- ✅ Monitoramento com Sentry, Telescope (dev), Pulse (prod)
- ✅ Health checks (database, redis, cache, queue, storage, acbr)
- ✅ Sistema de alertas (Slack, Email)
- ✅ Métricas customizadas
- ✅ Dashboard de erros

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

## 11. Checklist de Paridade de Funcionalidades ✅

**Arquivo**: [`estrategia/10-paridade-funcionalidades.md`](../estrategia/10-paridade-funcionalidades.md)

**Tópicos documentados**:

- ✅ Inventário completo de 152 funcionalidades do C++ (por módulo)
- ✅ Mapeamento C++ → Laravel para cada funcionalidade
- ✅ Status de implementação (⬜/🔄/✅/❌)
- ✅ Funcionalidades novas (8 itens que não existem no C++)
- ✅ Critérios de aceitação por módulo
- ✅ Processo de validação

---

## 12. Regras de Negócio Detalhadas ✅

**Arquivo**: [`negocios/05-regras-negocio.md`](../negocios/05-regras-negocio.md)

**Tópicos documentados**:

- ✅ Precificação (3 níveis de desconto com fórmulas)
- ✅ Frete (percentual, peso, caminhões, autorização)
- ✅ Comissões (profissional e RT com calendário)
- ✅ Crédito de cliente (uso, adição, restauração)
- ✅ Impostos (ICMS, ST, PIS, COFINS - alocação proporcional)
- ✅ Devoluções (janela de tempo, status, crédito)
- ✅ Aprovações (permissões por tipo, fluxo autorização)
- ✅ Consumo de estoque FIFO (divisão de compra)
- ✅ Representação (flag, identificador com sufixo R)
- ✅ Constantes e thresholds configuráveis

---

## 13. Documentação de Integrações ✅

**Arquivo**: [`tecnico/09-integracoes.md`](../tecnico/09-integracoes.md)

**Tópicos documentados**:

- ✅ **ACBr**: Socket TCP 3434, comandos NFe, parsing de respostas
- ✅ **CNAB 240**: Itaú, GARE, fornecedor, salário, retorno
- ✅ **CEP**: Banco local + fallback ViaCEP/BrasilAPI
- ✅ **SMTP**: Email com anexos (NFe XML/DANFE)
- ✅ **Google Maps**: Geocodificação de endereços
- ✅ **QualP**: API de cálculo de frete
- ✅ **Certificados**: A1/A3, verificação de validade, alertas
- ✅ **Resiliência**: Retry com backoff, circuit breaker
- ✅ **Monitoramento**: Health checks, logging estruturado

---

## 14. Plano de Treinamento de Usuários ✅

**Arquivo**: [`estrategia/11-treinamento.md`](../estrategia/11-treinamento.md)

**Tópicos documentados**:

- ✅ 8 perfis de usuário detalhados (Admin, Gerentes, Vendedor, Operacional, etc.)
- ✅ Mapeamento completo desktop → web (menus, abas, ações)
- ✅ 8 módulos de treinamento (1-8 horas cada)
- ✅ Cronograma de rollout gradual por módulo
- ✅ Lista de vídeos necessários (10 vídeos)
- ✅ FAQ de transição (15+ perguntas)
- ✅ Estrutura de suporte (N1/N2/N3)
- ✅ Métricas de sucesso e indicadores
- ✅ Programa de usuários-chave (Champions)

---

## 15. Design System/Guia de UI ✅

**Arquivo**: [`tecnico/10-design-system.md`](../tecnico/10-design-system.md)

**Tópicos documentados**:

- ✅ Paleta de cores (primária, secundária, status, contexto de negócio)
- ✅ Tipografia (Inter, escala, hierarquia)
- ✅ Espaçamento (sistema 4px, grid)
- ✅ Componentes base (Button, Input, Select, Checkbox)
- ✅ Componentes complexos (Card, Table, Modal, Toast, Badge)
- ✅ Estados de interação (hover, focus, disabled, loading, error)
- ✅ Iconografia (Heroicons, tamanhos, cores)
- ✅ Responsividade (breakpoints, padrões de layout)
- ✅ Acessibilidade (WCAG 2.1 AA, ARIA)
- ✅ Estrutura de componentes Vue

---

## 16. Estratégia de Concorrência ✅

**Arquivo**: [`tecnico/11-concorrencia.md`](../tecnico/11-concorrencia.md)

**Tópicos documentados**:

- ✅ Optimistic locking com `updated_at` (edição de registros)
- ✅ Pessimistic locking com `SELECT FOR UPDATE` (estoque)
- ✅ Atomic updates para crédito (increment/decrement)
- ✅ Queue-based processing com `ShouldBeUnique`
- ✅ Database locks com expiração para edições longas
- ✅ Cenários de race condition documentados
- ✅ Tratamento de deadlocks com retry
- ✅ Testes de concorrência e stress

---

## 17. Atalhos de Teclado

**Problema**: App C++ tem navegação por teclado extensiva que precisa ser preservada.

**Arquivo sugerido**: `tecnico/12-atalhos-teclado.md`

**Tópicos a documentar**:

- Inventário de atalhos do C++ atual
- Mapeamento para atalhos web (considerando conflitos com browser)
- Navegação por Tab entre campos
- Enter para confirmar/avançar
- Esc para cancelar/fechar
- Atalhos globais (Ctrl+N novo, Ctrl+S salvar, etc.)
- Atalhos por módulo
- Implementação Vue (VueUse `useMagicKeys` ou similar)
- Acessibilidade (focus visible, skip links)

---

## 18. Especificações de Impressão

**Problema**: Múltiplos documentos precisam ser impressos com layouts específicos.

**Arquivo sugerido**: `tecnico/13-impressao.md`

**Tópicos a documentar**:

- **DANFE**: Layout oficial, validações
- **Boletos**: Layout bancário, código de barras
- **Romaneio de entrega**: Itens, endereço, assinatura
- **Etiquetas**: Produtos, volumes
- **Relatórios**: Cabeçalho padrão, paginação, totalizadores
- Biblioteca escolhida (DomPDF, Browsershot, wkhtmltopdf)
- Impressão direta vs download PDF
- Configuração de impressora (térmica, laser)
- Tamanhos de papel (A4, carta, etiqueta)

---

## 19. Dicionário de Dados

**Problema**: Sem documentação centralizada de termos e campos do banco.

**Arquivo sugerido**: `tecnico/14-dicionario-dados.md`

**Tópicos a documentar**:

- Glossário de termos de negócio (RT, L1, L2, galpão, bloco, etc.)
- Significado de cada coluna das tabelas principais
- Valores possíveis de enums (status, tipos)
- Unidades de medida
- Convenções de nomenclatura
- Campos legados vs novos
- Campos calculados vs armazenados

---

## 20. Matriz de Suporte Browser/Dispositivo

**Problema**: Sem definição de quais browsers/dispositivos serão suportados.

**Arquivo sugerido**: `tecnico/15-compatibilidade.md`

**Tópicos a documentar**:

- Browsers suportados (Chrome, Firefox, Edge, Safari - versões mínimas)
- Dispositivos (Desktop, Tablet, Mobile)
- Resoluções mínimas
- Funcionalidades que requerem desktop (impressão, certificado digital)
- Testes de compatibilidade (BrowserStack, manual)
- Polyfills necessários
- Progressive enhancement vs graceful degradation

---

## 21. Auditoria de Dependências

**Problema**: Sem inventário de pacotes e suas licenças/riscos.

**Arquivo sugerido**: `tecnico/16-dependencias.md`

**Tópicos a documentar**:

- **Composer packages**: Lista completa com versões e licenças
- **NPM packages**: Lista completa com versões e licenças
- Pacotes críticos (sem alternativa fácil)
- Pacotes com vulnerabilidades conhecidas
- Estratégia de atualização (Dependabot, manual)
- Licenças problemáticas (GPL, copyleft)
- Alternativas para pacotes abandonados

---

## 22. Estratégia de Validação ✅

**Arquivo**: [`tecnico/17-validacao.md`](../tecnico/17-validacao.md)

**Tópicos documentados**:

- ✅ **Validação de Request**: Form Requests, validadores brasileiros, sanitização, arquivos
- ✅ **Validação de Regras de Negócio**: Service layer, Value Objects, regras configuráveis
- ✅ **Validação de Banco**: CHECK constraints, triggers, validação em Model
- ✅ **Validação de Response**: API Resources, schema JSON, contract tests
- ✅ **Validação de Jobs**: Pré-condições, pós-validação, circuit breaker
- ✅ **Monitoramento**: Logging estruturado, métricas, dashboard

---

## Progresso

### Concluídos

| Data       | Item   | Descrição                                               |
| ---------- | ------ | ------------------------------------------------------- |
| 2025-12-28 | Item A | Fechado ADR-004: NFe via ACBrMonitorConsole ou sped-nfe |
| 2025-12-28 | Item B | Corrigidos links quebrados em 8 arquivos                |
| 2025-12-28 | Item 1 | Criadas specs de todos os 6 módulos faltantes           |
| 2025-12-28 | Item 2 | Fechados todos os ADRs (003, 005, 006)                  |
| 2025-12-28 | Item 3 | Arquitetura de segurança completa                       |
| 2025-12-28 | Item 6 | Scripts de migração de dados                            |
| 2025-12-28 | Item 11 | Checklist de paridade - 152 funcionalidades mapeadas   |
| 2025-12-28 | Item 12 | Regras de negócio - 10 domínios documentados           |
| 2025-12-28 | Item 13 | Integrações - 7 sistemas externos documentados         |
| 2025-12-28 | Item 22 | Estratégia de validação - 5 camadas documentadas       |
| 2025-12-28 | Item 4  | Design de API - REST + OpenAPI + webhooks              |
| 2025-12-28 | Item 5  | Estratégia de testes - Unit/Feature/E2E + CI/CD        |
| 2025-12-28 | Item 7  | Erros e monitoramento - Sentry + logs + alertas        |
| 2025-12-28 | Item 14 | Treinamento - 8 perfis, 8 módulos, rollout gradual     |
| 2025-12-28 | Item 15 | Design System - cores, tipografia, componentes Vue     |
| 2025-12-28 | Item 16 | Concorrência - locks, queues, race conditions          |

### Próximos

**Média Prioridade:**
1. [ ] Item 17 - Atalhos de teclado
2. [ ] Item 18 - Especificações de impressão

**Baixa Prioridade:**
3. [ ] Item 8 - DevOps/Deployment
4. [ ] Item 9 - Benchmarks de performance
5. [ ] Item 10 - Substituição de relatórios
6. [ ] Item 19 - Dicionário de dados
7. [ ] Item 20 - Matriz de suporte browser/dispositivo
8. [ ] Item 21 - Auditoria de dependências

---

## Documentos Relacionados

- [00-indice.md](../00-indice.md) - Índice principal
- [rastreador.md](./rastreador.md) - Rastreador de documentação existente
- [estrategia/02-decisoes.md](../estrategia/02-decisoes.md) - ADRs
- [estrategia/03-melhorias.md](../estrategia/03-melhorias.md) - Pontos de dor identificados
