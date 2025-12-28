# Registros de Decisão de Arquitetura (ADR)

> Este arquivo rastreia decisões arquiteturais chave para o projeto de migração web.
> Formato: [Template ADR](https://adr.github.io/)

---

## Log de Decisões

| ID | Decisão | Status | Data |
|----|---------|--------|------|
| ADR-001 | Usar Laravel como framework backend | **Aceito** | 2025-12-27 |
| ADR-002 | Usar PostgreSQL como banco de dados | **Aceito** | 2025-12-27 |
| ADR-003 | Seleção de framework frontend | **Em Aberto** | - |
| ADR-004 | Abordagem de integração NFe | **Em Aberto** | - |
| ADR-005 | Estratégia de migração | **Em Aberto** | - |
| ADR-006 | Abordagem de multi-tenancy | **Em Aberto** | - |

---

## ADR-001: Usar Laravel como Framework Backend

### Status
**Aceito** - 2025-12-27

### Contexto
Necessidade de escolher um framework backend para a migração web. Opções consideradas:
- Laravel (PHP)
- Django (Python)
- .NET Core (C#)
- Node.js (Express/NestJS)

### Decisão
Usar **Laravel 11** como framework backend.

### Justificativa
1. **Maturidade do ecossistema PHP** para aplicações empresariais
2. **Eloquent ORM** excelente para relacionamentos complexos
3. **Funcionalidades built-in**: auth, filas, eventos, agendamento
4. **Comunidade forte** e ecossistema de pacotes
5. **Familiaridade da equipe** (curva de aprendizado PHP mais fácil assumida)
6. **Boas bibliotecas NFe** disponíveis em PHP (sped-nfe)

### Consequências
- Necessário hosting PHP 8.2+
- Equipe precisa treinamento em Laravel
- Pode aproveitar pacotes Composer

---

## ADR-002: Usar PostgreSQL como Banco de Dados

### Status
**Aceito** - 2025-12-27

### Contexto
Sistema atual usa MySQL/MariaDB. Avaliando opções de banco de dados:
- Manter MySQL/MariaDB
- Migrar para PostgreSQL
- Usar cloud-native (Aurora, Cloud SQL)

### Decisão
Migrar para **PostgreSQL 16**.

### Justificativa
1. **JSONB nativo** - melhor para dados fiscais flexíveis, atributos de produtos
2. **Tipos ENUM nativos** - campos de status type-safe
3. **Constraints CHECK** - regras de negócio a nível de banco de dados
4. **Full-text search** - `tsvector` built-in para busca de produtos
5. **Melhor concorrência** - MVCC lida com usuários simultâneos
6. **Suporte a schemas** - opção futura de multi-tenancy

### Consequências
- Esforço de migração do MySQL
- Algumas diferenças de sintaxe de query
- Necessário expertise em PostgreSQL
- Melhor manutenibilidade a longo prazo

---

## ADR-003: Seleção de Framework Frontend

### Status
**Em Aberto** - Decisão necessária

### Contexto
Necessidade de escolher abordagem frontend. Opções:
1. Livewire (renderizado no servidor)
2. Inertia + Vue
3. Inertia + React
4. SPA Completo + API

### Análise de Opções

Ver análise detalhada em [03-frontend.md](./03-frontend.md)

| Critério | Livewire | Inertia+Vue | Inertia+React | SPA Completo |
|----------|----------|-------------|---------------|--------------|
| Curva de aprendizado | Baixa | Média | Média-Alta | Alta |
| Interatividade | Média | Alta | Alta | Máxima |
| Complexidade | Baixa | Média | Média | Alta |
| Habilidades necessárias | Apenas PHP | PHP + Vue | PHP + React | Equipes separadas |

### Recomendação
**Inertia + Vue** - Melhor equilíbrio entre interatividade e simplicidade.

### Decisão
_Pendente input da equipe_

### Consequências
_A ser preenchido após decisão_

---

## ADR-004: Abordagem de Integração NFe

### Status
**Em Aberto** - Decisão necessária

### Contexto
Necessidade de integrar com sistema de nota fiscal eletrônica brasileiro (NFe).
Implementação atual usa ACBrLib (DLL Windows).

### Opções

| Opção | Prós | Contras |
|-------|------|---------|
| **Manter ACBr** (via API) | Funciona, gratuito | Requer Windows, deploy complexo |
| **Provedor SaaS** (Focus, Enotas) | Simples, gerenciado | Custo mensal, vendor lock-in |
| **PHP Nativo** (sped-nfe) | Controle total, gratuito | Mais trabalho de dev, manutenção |

Ver análise detalhada em [04-modules/nfe.md](./04-modules/nfe.md)

### Recomendação
**Começar com SaaS** (Focus NFe ou Enotas), abstrair atrás de interface.
Considerar PHP nativo depois se o volume justificar.

### Decisão
_Pendente análise de custos e input da equipe_

### Consequências
_A ser preenchido após decisão_

---

## ADR-005: Estratégia de Migração

### Status
**Em Aberto** - Decisão necessária

### Contexto
Necessidade de decidir como fazer a transição do desktop C++ para web Laravel.

### Opções

| Estratégia | Prazo | Risco | Custo |
|------------|-------|-------|-------|
| Big Bang | 6-12 meses | Alto | Médio |
| Strangler Fig | 12-18 meses | Médio | Médio |
| Execução Paralela | 18-24 meses | Baixo | Alto |

Ver análise detalhada em [05-migration-plan.md](./05-migration-plan.md)

### Recomendação
**Strangler Fig** - Migração incremental com banco de dados compartilhado.

### Decisão
_Pendente aprovação dos stakeholders_

### Consequências
_A ser preenchido após decisão_

---

## ADR-006: Abordagem de Multi-tenancy

### Status
**Em Aberto** - Decisão necessária

### Contexto
Sistema atual usa coluna `idLoja` para separação de tenants.
Necessidade de decidir estratégia de multi-tenancy para versão web.

### Opções

| Abordagem | Isolamento | Complexidade | Queries |
|-----------|------------|--------------|---------|
| **BD Único + tenant_id** | Baixo | Baixo | Simples |
| **Schema por tenant** | Médio | Médio | Médio |
| **Banco por tenant** | Alto | Alto | Cross-tenant complexo |

### Recomendação
**BD Único com tenant_id** (padrão atual) - mais simples, comprovado.
Pode evoluir para schema-por-tenant depois se necessário.

### Decisão
_Pendente esclarecimento de requisitos_

### Consequências
_A ser preenchido após decisão_

---

## Template para Novas Decisões

```markdown
## ADR-XXX: [Título]

### Status
**Proposto** / **Aceito** / **Depreciado** / **Substituído**

### Contexto
Qual é o problema que estamos vendo que motiva esta decisão?

### Decisão
Qual é a mudança que estamos propondo e/ou fazendo?

### Justificativa
Por que esta decisão está sendo tomada? Quais alternativas foram consideradas?

### Consequências
O que fica mais fácil ou mais difícil por causa desta mudança?
```
