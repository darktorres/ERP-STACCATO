# Documentation Tracker

> Status: **Complete**
> Last updated: 2025-12-27

---

## Purpose

Track progress of documenting all business flows for the web migration project.

---

## Documentation Structure

Reorganized on 2025-12-27 into categorical folders:

```
.claude/next-gen/
├── 00-index.md                    # Master navigation
├── technical/                     # Technical architecture
│   ├── 01-architecture.md         # Laravel design
│   ├── 02-database.md             # PostgreSQL schema
│   ├── 03-frontend.md             # UI framework
│   └── 04-infrastructure.md       # Audit, search, temporal
├── business/                      # Business flows
│   ├── 01-flows-overview.md       # High-level overview
│   ├── 02-stock-flows.md          # Stock deep dive
│   ├── 03-delivery-nfe-flows.md   # Delivery, NFe, Financial
│   └── 04-cadastros-flows.md      # Master data, Permissions
├── strategy/                      # Migration strategy
│   ├── 01-migration-plan.md       # Phases
│   └── 02-decisions.md            # ADRs
└── meta/
    └── tracker.md                 # This file
```

---

## Flow Coverage Matrix

**All 17 business flows documented (100% coverage)**

| # | Flow | Document | Status |
|---|------|----------|--------|
| 1 | Cadastros (Suppliers/Products/Clients) | business/04-cadastros-flows.md | ✅ Done |
| 2 | Orçamento (Budget creation/pricing) | business/04-cadastros-flows.md | ✅ Done |
| 3 | Orçamento → Venda (Budget to Order) | business/01-flows-overview.md | ✅ Done |
| 4 | Venda → Compra (Order to Purchase) | business/01-flows-overview.md | ✅ Done |
| 5 | Compra Confirmation (NFe Import) | business/02-stock-flows.md | ✅ Done |
| 6 | Stock Creation (from NFe) | business/02-stock-flows.md | ✅ Done |
| 7 | Stock Consumption | business/02-stock-flows.md | ✅ Done |
| 8 | Stock Splits (parear, dividir) | business/02-stock-flows.md | ✅ Done |
| 9 | NFe Emission (Saída - to customer) | business/03-delivery-nfe-flows.md | ✅ Done |
| 10 | Delivery (Entrega to customer) | business/03-delivery-nfe-flows.md | ✅ Done |
| 11 | Financial - Receivables | business/01-flows-overview.md | ✅ Done |
| 12 | Financial - Payables | business/03-delivery-nfe-flows.md | ✅ Done |
| 13 | Financial - CNAB/Bank | business/03-delivery-nfe-flows.md | ✅ Done |
| 14 | Commission Calculation | business/03-delivery-nfe-flows.md | ✅ Done |
| 15 | Returns (Devolução) | business/02-stock-flows.md | ✅ Done |
| 16 | Galpão (Warehouse blocks) | business/04-cadastros-flows.md | ✅ Done |
| 17 | User Permissions | business/04-cadastros-flows.md | ✅ Done |

---

## Technical Documentation

| Document | Purpose | Status |
|----------|---------|--------|
| technical/01-architecture.md | Laravel structure, services | Draft |
| technical/02-database.md | PostgreSQL schema | Draft |
| technical/03-frontend.md | Frontend framework evaluation | Draft |
| technical/04-infrastructure.md | Audit, temporal, search | Draft |

---

## Strategy Documentation

| Document | Purpose | Status |
|----------|---------|--------|
| strategy/01-migration-plan.md | Strangler fig phases | Draft |
| strategy/02-decisions.md | Architecture Decision Records | Draft |

---

## Progress Log

### 2025-12-27

- [x] Created initial documentation (00-12 series)
- [x] Completed coverage audit - 17/17 flows documented
- [x] Reorganized into categorical folder structure
- [x] Created master index (00-index.md)
- [x] Extracted infrastructure concepts into technical/04-infrastructure.md

---

## Documentation Complete

**Business Logic**: 100% documented
**Technical Design**: Draft complete, decisions pending
**Migration Strategy**: Draft complete, stakeholder review needed
