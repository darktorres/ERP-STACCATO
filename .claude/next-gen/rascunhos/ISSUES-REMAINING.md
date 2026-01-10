# Remaining Consistency Issues - Progress Tracking

## Critical Issues (Block Implementation)

### 1. ✅ Logística Module - Outdated Data Model References
**File**: `04-arquitetura/modulos/logistica.md`
**Status**: COMPLETED ✅ (Commit ca99c9cf)
**Completed**:
- Replaced `VendaItemAtendimento` with new `EntregaItem` model
- Updated all database references: `venda_has_produto2` → `entrega_itens`
- Updated financial references: `conta_a_receber_has_pagamento` → `financeiro_parcelas`
- Updated `EntregaService` to work with `VendaItem` and `EntregaItem`
- Added `EntregaItemStatus` enum with proper states
- Updated state machine: ESTOQUE → ALOCADO for delivery eligibility

### 2. ✅ Financial Migration Scripts - Wrong Target Tables
**File**: `04-arquitetura/modulos/financeiro.md`
**Status**: COMPLETED ✅ (Commit 207c1ed8)
**Completed**:
- Rewrote migration scripts to target unified `financeiro_parcelas` table
- Maps old `contas_receber` → new table with `tipo='RECEBER'`
- Maps old `contas_pagar` → new table with `tipo='PAGAR'`
- Handles status transformation (RECEBIDO/PAGO → unified FinanceiroStatus)
- Includes overdue date calculation (ATRASADO status)
- Adds performance indices for payment scheduling

### 3. ✅ Event Sourcing Pattern - Documented in Core Modules
**Files**: financeiro.md, vendas.md, estoque.md
**Status**: COMPLETED ✅ (Commit 37ac90c9)
**Completed**:
- Added Event Sourcing section to financeiro.md with `financeiro_parcelas_events` and `FinanceiroEventType`
- Added Event Sourcing section to vendas.md with `venda_itens_events`, `alocacoes_events`, and event types
- Added Event Sourcing section to estoque.md with `estoque_movimentacoes`, `alocacoes_eventos` tables
- Each module now documents: append-only tables, immutability triggers (fn_prevent_mutation), event types
- Added event recording patterns in services with complete code examples
- Added audit trail query examples and materialized view setup
- Documented pg_ivm (incremental view maintenance) for real-time views

---

## High Priority Issues (Architectural Misalignments)

### 4. ⚠️ Incomplete Alocacao M:N Workflow Documentation
**Files**: vendas.md, estoque.md, logistica.md
**Status**: PENDING
**Details**:
- While models exist, full workflow not clearly documented
- Missing: How alocacoes affect venda_item status
- Missing: Quantity validation rules
- Missing: FIFO suggestion algorithm
- Missing: Allocation reversal triggers
- Fix: Document complete M:N allocation workflow with state machine diagram

### 5. ⚠️ Enum Status Mismatches - Financial
**File**: `04-arquitetura/modulos/financeiro.md` lines 413-453
**Status**: PENDING
**Details**:
- Old enums: `ContaReceberStatus`, `ContaPagarStatus` (separate)
- New schema: Single `FinanceiroStatus` with unified values
- Values differ: Old has `CONFERIDO`, new has `ATRASADO`
- Fix: Standardize to single `FinanceiroStatus` enum matching schema-proposto

### 6. ⚠️ Table Naming Inconsistency - Estoque
**File**: `04-arquitetura/modulos/estoque.md`
**Status**: PENDING
**Details**:
- Code uses `Estoque` class
- Schema-proposto names table `estoque_lotes`
- Migration scripts reference old name
- Fix: Align class naming and table references throughout

---

## Medium Priority Issues (Inconsistencies & Clarity)

### 7. 📝 Remove N1/N2 Terminology
**Files**: contexto/*, decisoes/*, modulos/*
**Status**: PENDING
**Details**:
- References to "N1", "N2" levels throughout documentation
- Suggests old two-level item structure still exists
- Should use: "item" and "allocation" terminology
- Fix: Search and replace N1/N2 with appropriate new terminology

### 8. 📝 Service Layer Architecture Documentation
**File**: `04-arquitetura/modulos/vendas.md`
**Status**: PENDING
**Details**:
- VendaService injects EstoqueService and ContaReceberService
- Tight coupling between modules
- Need to document proper separation of concerns
- Fix: Clarify service layer boundaries and event-driven architecture

### 9. 📝 Complete VendaItem Relationships
**File**: `04-arquitetura/modulos/vendas.md` lines 311-384
**Status**: PENDING
**Details**:
- Missing `entregas` relationship (should link via `entrega_itens`)
- Incomplete polymorphic relationship to origin (COMPRA vs ESTOQUE)
- Fix: Add missing relationships and explain polymorphic linking

---

## Tracking

| # | Issue | File | Priority | Status | Commits |
|---|-------|------|----------|--------|---------|
| 1 | Logística outdated model refs | logistica.md | CRITICAL | ✅ COMPLETED | ca99c9cf |
| 2 | Financial migration scripts | financeiro.md | CRITICAL | ✅ COMPLETED | 207c1ed8 |
| 3 | Event Sourcing not documented | financeiro/vendas/estoque | CRITICAL | ✅ COMPLETED | 37ac90c9 |
| 4 | Alocacao M:N workflow | vendas/estoque/logistica | HIGH | PENDING | - |
| 5 | Enum status mismatches | financeiro.md | HIGH | PENDING | - |
| 6 | Table naming estoque | estoque.md | HIGH | PENDING | - |
| 7 | N1/N2 terminology | Multiple | MEDIUM | PENDING | - |
| 8 | Service layer coupling | vendas.md | MEDIUM | PENDING | - |
| 9 | VendaItem relations | vendas.md | MEDIUM | PENDING | - |

**Progress**: 3/9 issues completed (33%) | All critical issues fixed ✅ | 6 remaining issues

---

## Completed

**From previous session (d13045a0):**
✅ **financeiro.md** - Updated to unified FinanceiroParcela model
✅ **vendas.md** - Removed VendaItemAtendimento, added M:N alocacoes
✅ **estoque.md** - Replaced EstoqueConsumo with Alocacao M:N model

**This session:**
✅ **logistica.md** (ca99c9cf) - Replaced VendaItemAtendimento with EntregaItem model, updated financial refs
✅ **financeiro.md** (207c1ed8) - Rewrote migration scripts to target unified financeiro_parcelas
✅ **financeiro.md, vendas.md, estoque.md** (37ac90c9) - Added Event Sourcing sections with complete audit patterns
