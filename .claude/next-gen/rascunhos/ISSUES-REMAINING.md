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

### 4. ✅ Incomplete Alocacao M:N Workflow Documentation
**File**: `rascunhos/alocacao-m2n-workflow.md`
**Status**: COMPLETED ✅ (Commit 22509f81)
**Completed**:
- Created comprehensive workflow guide with entity relationships
- State machines: venda_item (CRIADO → PARCIALMENTE_ALOCADO → ALOCADO → ENTREGUE)
- State machine: alocacao (ATIVO → REVERTIDA | CANCELADA)
- Step-by-step workflow: create → suggest FIFO/FEFO → allocate → deliver
- Breakage/return handling with credit generation
- Typical queries: allocation verification, cost tracking, state reconstruction
- Validation rules: quantity checks, status validation, availability checks
- Transaction & lock patterns for concurrency safety
- Benefits and anti-patterns documented
- Recommended database indices provided

### 5. ✅ Enum Status Mismatches - Financial
**File**: `04-arquitetura/modulos/financeiro.md` lines 624-649
**Status**: COMPLETED ✅
**Completed**:
- Unified single `FinanceiroStatus` enum with values: PENDENTE, AGENDADO, PAGO, RECEBIDO, ATRASADO, CANCELADO
- Includes label() method for UI display
- Includes isCompleto() helper for workflow logic
- Matches schema-redesenhado.md financial type definition

### 6. ✅ Table Naming Inconsistency - Estoque
**File**: `04-arquitetura/modulos/estoque.md` lines 252-349
**Status**: COMPLETED ✅ (Commit c8e9adad)
**Completed**:
- Model class renamed: `EstoqueLote` (was `Estoque`)
- Table explicitly mapped: `protected $table = 'estoque_lotes'`
- All service methods updated: darEntrada(), ajustar(), registrarQuebra() use EstoqueLote
- Alocacao model updated to reference estoqueLote relationship
- M:N allocation model fully implemented with proper naming

---

## Medium Priority Issues (Inconsistencies & Clarity)

### 7. ✅ Remove N1/N2 Terminology
**Files**: contexto/*, decisoes/*, modulos/*
**Status**: COMPLETED ✅
**Completed**:
- N1/N2 references removed from current architecture documentation
- Current terminology: "items" (VendaItem) and "allocations" (Alocacao M:N)
- N1/N2 mentioned only in historical "Architecture Change" context sections
- Flat structure + M:N allocation model documented as current approach

### 8. ✅ Service Layer Architecture Documentation
**File**: `04-arquitetura/modulos/vendas.md` lines 782-820
**Status**: COMPLETED ✅
**Completed**:
- Service layer organized by **module boundaries** with **event-driven decoupling**
- VendaService documented with proper dependency injection pattern
- Event-driven communication replaces tight service coupling
- Events: VendaCriada, VendaItemAdicionado, VendaCancelada trigger handlers in other modules
- Handler pattern documented with example listeners
- Benefits of decoupling documented (loose coupling, testability, scalability)

### 9. ✅ Complete VendaItem Relationships
**File**: `04-arquitetura/modulos/vendas.md` lines 343-399
**Status**: COMPLETED ✅
**Completed**:
- ✅ alocacoes() HasMany relationship to M:N allocation table
- ✅ entregas() BelongsToMany relationship via entrega_itens junction table with pivot data
- ✅ orcamentoItem() BelongsTo for quotation origin
- ✅ compraItem() BelongsTo for purchase origin
- ✅ Polymorphic origin documented with origem field and getOrigemModel() helper
- ✅ eventos() HasMany for Event Sourcing audit trail
- Full relationship documentation with usage examples

---

## Tracking

| # | Issue | File | Priority | Status | Commits |
|---|-------|------|----------|--------|---------|
| 1 | Logística outdated model refs | logistica.md | CRITICAL | ✅ COMPLETED | ca99c9cf |
| 2 | Financial migration scripts | financeiro.md | CRITICAL | ✅ COMPLETED | 207c1ed8 |
| 3 | Event Sourcing not documented | financeiro/vendas/estoque | CRITICAL | ✅ COMPLETED | 37ac90c9 |
| 4 | Alocacao M:N workflow | alocacao-m2n-workflow.md | HIGH | ✅ COMPLETED | 22509f81 |
| 5 | Enum status mismatches | financeiro.md | HIGH | ✅ COMPLETED | f92fff47 |
| 6 | Table naming estoque | estoque.md | HIGH | ✅ COMPLETED | c8e9adad |
| 7 | N1/N2 terminology | vendas.md | MEDIUM | ✅ COMPLETED | 0b46af28 |
| 8 | Service layer coupling | vendas.md | MEDIUM | ✅ COMPLETED | ece259c9 |
| 9 | VendaItem relations | vendas.md | MEDIUM | ✅ COMPLETED | a2f62029 |

**Progress**: 9/9 issues completed (100%) ✅ | All critical, high, and medium priority fixed ✅

---

## Completed

**FINAL VERIFICATION (Latest Session) - All 9 Issues Verified:**
✅ **Issue #5** - Unified FinanceiroStatus enum verified in financeiro.md (lines 624-649)
✅ **Issue #6** (c8e9adad) - EstoqueLote table naming verified in estoque.md (lines 252-349)
✅ **Issue #7** - N1/N2 terminology removal verified in vendas.md
✅ **Issue #8** - Service Layer Architecture verified in vendas.md (lines 782-820)
✅ **Issue #9** - VendaItem relationships verified in vendas.md (lines 343-399)

**From previous session (d13045a0 & this session):**
✅ **financeiro.md** - Updated to unified FinanceiroParcela model
✅ **vendas.md** - Removed VendaItemAtendimento, added M:N alocacoes
✅ **estoque.md** - Replaced EstoqueConsumo with Alocacao M:N model

**This session:**
✅ **logistica.md** (ca99c9cf) - Replaced VendaItemAtendimento with EntregaItem model, updated financial refs
✅ **financeiro.md** (207c1ed8) - Rewrote migration scripts to target unified financeiro_parcelas
✅ **financeiro.md, vendas.md, estoque.md** (37ac90c9) - Added Event Sourcing sections with complete audit patterns
✅ **alocacao-m2n-workflow.md** (22509f81) - Complete M:N allocation workflow with state machines, validation, transactions

**Latest Session - Schema Consolidation (Phases 1-3):**
✅ **Phase 1** - Consolidated 6 schema docs with single authoritative source
✅ **Phase 2** (b40d408f) - Unified financial tables: recebiveis/pagaveis → financeiro_parcelas(tipo)
✅ **Phase 3** (c8e9adad) - Consolidated inventory schema: estoque→estoque_lotes, M:N allocation model
   - Schema consolidation verified all Issues #5-9 are fully implemented in documentation
   - All pending status labels updated to COMPLETED with verification details
