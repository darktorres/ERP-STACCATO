# ERP Staccato - Web Migration Documentation

> **Status**: Planning Phase
> **Last updated**: 2025-12-27
> **Target Stack**: Laravel 11 + PostgreSQL 16 + Inertia/Vue (TBD)

---

## Quick Links

| Need to... | Go to |
|------------|-------|
| Understand the project | [Overview](#project-overview) |
| See Laravel architecture | [technical/01-architecture.md](./technical/01-architecture.md) |
| See database design | [technical/02-database.md](./technical/02-database.md) |
| Understand business flows | [business/](#business-flows) |
| Check migration phases | [strategy/01-migration-plan.md](./strategy/01-migration-plan.md) |
| See open decisions | [strategy/02-decisions.md](./strategy/02-decisions.md) |

---

## Project Overview

Rewriting the existing C++ Qt desktop ERP application as a modern web application.

### Goals
1. Fix architectural problems in the legacy codebase
2. Improve maintainability and testability
3. Enable multi-device access (browser-based)
4. Modernize the technology stack

### Current System Scale

| Metric | Count |
|--------|-------|
| C++ Source Files | 142 |
| Header Files | 141 |
| UI Forms (.ui) | 87 |
| Lines of Code | ~50,000 |
| Database Tables | 209 |
| Main Modules | 7 |

---

## Documentation Structure

```
.claude/next-gen/
├── 00-index.md                 # This file - master navigation
│
├── technical/                  # Technical architecture
│   ├── 01-architecture.md      # Laravel structure, patterns, services
│   ├── 02-database.md          # PostgreSQL schema redesign
│   ├── 03-frontend.md          # Frontend framework evaluation
│   ├── 04-infrastructure.md    # Audit, temporal data, search
│   └── modules/                # Module implementation specs
│       ├── _index.md           # Module priority list
│       ├── compras.md          # Purchase module Laravel implementation
│       └── nfe.md              # NFe integration options
│
├── business/                   # Business logic documentation
│   ├── 01-flows-overview.md    # High-level flow diagrams
│   ├── 02-stock-flows.md       # Stock creation, consumption, returns
│   ├── 03-delivery-nfe-flows.md # Delivery, NFe, CNAB, Commission
│   └── 04-cadastros-flows.md   # Master data, Orçamento, Galpão, Permissions
│
├── strategy/                   # Migration strategy
│   ├── 01-migration-plan.md    # Strangler fig phases
│   ├── 02-decisions.md         # Architecture Decision Records
│   ├── 03-improvements.md      # Pain points & improvement options
│   └── 04-l1l2-simplification.md # Deep dive on table flattening
│
└── meta/
    └── tracker.md              # Documentation progress tracker
```

---

## Technical Documentation

### [01 - Laravel Architecture](./technical/01-architecture.md)
- Proposed directory structure
- Service layer pattern
- PHP 8.1+ Enums for status
- Event-driven workflows
- Form request validation

### [02 - Database Schema](./technical/02-database.md)
- PostgreSQL migration rationale
- Schema normalization (fixing denormalized supplier names)
- ENUM types for status fields
- Audit trail with triggers
- Full-text search with tsvector

### [03 - Frontend Framework](./technical/03-frontend.md)
- Livewire vs Inertia+Vue vs Full SPA
- Recommendation: Inertia + Vue
- Example components

### [04 - Infrastructure](./technical/04-infrastructure.md)
- Audit trail architecture
- Temporal data (point-in-time queries)
- Search architecture (PostgreSQL FTS vs Elasticsearch)
- Materialized views for dashboards

### [modules/ - Implementation Specs](./technical/modules/_index.md)
Module-by-module Laravel implementation patterns:
- [compras.md](./technical/modules/compras.md) - Purchase module service/controller examples
- [nfe.md](./technical/modules/nfe.md) - NFe integration options and service interface

---

## Business Flows

### [01 - Flows Overview](./business/01-flows-overview.md)
High-level view of all business processes:
- Two-level table architecture (L1/L2)
- Status state machines
- Data integrity rules
- Known problems

### [02 - Stock Flows](./business/02-stock-flows.md)
Deep analysis of inventory management:
- 1:N:N relationship chain
- Stock creation from NFe import
- Parear (matching) algorithm
- Consumption logic (FIFO issues)
- Returns flow and bugs

### [03 - Delivery, NFe & Financial Flows](./business/03-delivery-nfe-flows.md)
- Delivery scheduling and confirmation
- NFe emission (ACBr integration)
- CNAB 240 bank file generation
- Commission (RT) calculation

### [04 - Cadastros & Other Flows](./business/04-cadastros-flows.md)
- Fornecedor, Cliente, Produto, Transportadora
- Orçamento (three-level discount system)
- Galpão (warehouse blocks)
- User permissions (RBAC + PBAC)

---

## Strategy Documentation

### [01 - Migration Plan](./strategy/01-migration-plan.md)
- Strangler Fig pattern (recommended)
- 8 phases over 18 months
- Risk mitigation strategies
- Team requirements

### [02 - Architecture Decisions](./strategy/02-decisions.md)
ADR format decision log:
- ADR-001: Laravel backend (Accepted)
- ADR-002: PostgreSQL database (Accepted)
- ADR-003: Frontend framework (Open)
- ADR-004: NFe integration (Open)
- ADR-005: Migration strategy (Open)

### [03 - Flow & Schema Improvements](./strategy/03-improvements.md)
Pain points and improvement opportunities:
- Two-level tables (L1/L2) simplification options
- FIFO stock consumption fix
- Supplier reference normalization
- Returns flow completion
- Status handling redesign
- Produto table split

### [04 - L1/L2 Simplification Deep Dive](./strategy/04-l1l2-simplification.md)
Detailed analysis of flattening the two-level table architecture:
- Current architecture analysis (idRelacionado pattern)
- Option A: Single table with self-reference (recommended)
- Option B: Keep L2 only, derive L1 via materialized view
- Option C: Event sourcing (overkill for this case)
- Migration strategy

---

## Status Legend

| Status | Meaning |
|--------|---------|
| **Complete** | Fully documented, reviewed |
| **Draft** | Initial content, needs review |
| **Open** | Decision pending |

---

## How to Use This Documentation

1. **New to the project?** Start with this index, then read [business/01-flows-overview.md](./business/01-flows-overview.md)
2. **Planning implementation?** Check [strategy/01-migration-plan.md](./strategy/01-migration-plan.md)
3. **Working on a specific flow?** See the relevant business/ document
4. **Making technical decisions?** Reference technical/ and strategy/02-decisions.md
