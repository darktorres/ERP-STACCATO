# Architecture Decision Records (ADR)

> This file tracks key architectural decisions for the web migration project.
> Format: [ADR Template](https://adr.github.io/)

---

## Decision Log

| ID | Decision | Status | Date |
|----|----------|--------|------|
| ADR-001 | Use Laravel as backend framework | **Accepted** | 2025-12-27 |
| ADR-002 | Use PostgreSQL as database | **Accepted** | 2025-12-27 |
| ADR-003 | Frontend framework selection | **Open** | - |
| ADR-004 | NFe integration approach | **Open** | - |
| ADR-005 | Migration strategy | **Open** | - |
| ADR-006 | Multi-tenancy approach | **Open** | - |

---

## ADR-001: Use Laravel as Backend Framework

### Status
**Accepted** - 2025-12-27

### Context
Need to choose a backend framework for the web migration. Options considered:
- Laravel (PHP)
- Django (Python)
- .NET Core (C#)
- Node.js (Express/NestJS)

### Decision
Use **Laravel 11** as the backend framework.

### Rationale
1. **PHP ecosystem maturity** for business applications
2. **Eloquent ORM** excellent for complex relationships
3. **Built-in features**: auth, queues, events, scheduling
4. **Strong community** and package ecosystem
5. **Team familiarity** (assumed easier PHP learning curve)
6. **Good NFe libraries** available in PHP (sped-nfe)

### Consequences
- Need PHP 8.2+ hosting
- Team needs Laravel training
- Can leverage Composer packages

---

## ADR-002: Use PostgreSQL as Database

### Status
**Accepted** - 2025-12-27

### Context
Current system uses MySQL/MariaDB. Evaluating database options:
- Keep MySQL/MariaDB
- Migrate to PostgreSQL
- Use cloud-native (Aurora, Cloud SQL)

### Decision
Migrate to **PostgreSQL 16**.

### Rationale
1. **Native JSONB** - better for flexible tax data, product attributes
2. **Native ENUM types** - type-safe status fields
3. **CHECK constraints** - database-level business rules
4. **Full-text search** - built-in `tsvector` for product search
5. **Better concurrency** - MVCC handles concurrent users
6. **Schema support** - future multi-tenancy option

### Consequences
- Migration effort from MySQL
- Some query syntax differences
- Need PostgreSQL expertise
- Better long-term maintainability

---

## ADR-003: Frontend Framework Selection

### Status
**Open** - Decision needed

### Context
Need to choose frontend approach. Options:
1. Livewire (server-rendered)
2. Inertia + Vue
3. Inertia + React
4. Full SPA + API

### Options Analysis

See detailed analysis in [03-frontend.md](./03-frontend.md)

| Criteria | Livewire | Inertia+Vue | Inertia+React | Full SPA |
|----------|----------|-------------|---------------|----------|
| Learning curve | Low | Medium | Medium-High | High |
| Interactivity | Medium | High | High | Maximum |
| Complexity | Low | Medium | Medium | High |
| Team skills needed | PHP only | PHP + Vue | PHP + React | Separate teams |

### Recommendation
**Inertia + Vue** - Best balance of interactivity and simplicity.

### Decision
_Pending team input_

### Consequences
_To be filled after decision_

---

## ADR-004: NFe Integration Approach

### Status
**Open** - Decision needed

### Context
Need to integrate with Brazilian electronic invoice system (NFe).
Current implementation uses ACBrLib (Windows DLL).

### Options

| Option | Pros | Cons |
|--------|------|------|
| **Keep ACBr** (via API) | Works, free | Requires Windows, complex deployment |
| **SaaS Provider** (Focus, Enotas) | Simple, managed | Monthly cost, vendor lock-in |
| **Native PHP** (sped-nfe) | Full control, free | More dev work, maintenance burden |

See detailed analysis in [04-modules/nfe.md](./04-modules/nfe.md)

### Recommendation
**Start with SaaS** (Focus NFe or Enotas), abstract behind interface.
Consider native PHP later if volume justifies.

### Decision
_Pending cost analysis and team input_

### Consequences
_To be filled after decision_

---

## ADR-005: Migration Strategy

### Status
**Open** - Decision needed

### Context
Need to decide how to transition from C++ desktop to Laravel web.

### Options

| Strategy | Timeline | Risk | Cost |
|----------|----------|------|------|
| Big Bang | 6-12 months | High | Medium |
| Strangler Fig | 12-18 months | Medium | Medium |
| Parallel Run | 18-24 months | Low | High |

See detailed analysis in [05-migration-plan.md](./05-migration-plan.md)

### Recommendation
**Strangler Fig** - Incremental migration with shared database.

### Decision
_Pending stakeholder approval_

### Consequences
_To be filled after decision_

---

## ADR-006: Multi-tenancy Approach

### Status
**Open** - Decision needed

### Context
Current system uses `idLoja` column for tenant separation.
Need to decide multi-tenancy strategy for web version.

### Options

| Approach | Isolation | Complexity | Queries |
|----------|-----------|------------|---------|
| **Single DB + tenant_id** | Low | Low | Simple |
| **Schema per tenant** | Medium | Medium | Medium |
| **Database per tenant** | High | High | Complex cross-tenant |

### Recommendation
**Single DB with tenant_id** (current pattern) - simplest, proven.
Can evolve to schema-per-tenant later if needed.

### Decision
_Pending requirements clarification_

### Consequences
_To be filled after decision_

---

## Template for New Decisions

```markdown
## ADR-XXX: [Title]

### Status
**Proposed** / **Accepted** / **Deprecated** / **Superseded**

### Context
What is the issue that we're seeing that is motivating this decision?

### Decision
What is the change that we're proposing and/or doing?

### Rationale
Why is this decision being made? What alternatives were considered?

### Consequences
What becomes easier or more difficult because of this change?
```
