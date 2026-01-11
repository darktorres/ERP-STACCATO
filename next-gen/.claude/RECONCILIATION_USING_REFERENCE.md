# Reconciliation of Documentation Inconsistencies
## Using `03-decisoes/02-schema-visual-overview.md` as Authoritative Reference

> Reference Document: `03-decisoes/02-schema-visual-overview.md`
> Status: COMPLETE REFERENCE AUTHORITY
> Generated: 2026-01-10

---

## #1: CRITICAL - Table Naming: `estoques` vs `estoque_lotes`

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

| Line | Reference | Usage |
|------|-----------|-------|
| 393 | `estoque_lotes` | Flow chart: "6️⃣ CRIAÇÃO DE ESTOQUE (de NFe ENTRADA)" |
| 452 | `estoque_lotes` | "LAYER 3: Estado Atual (estoque_lotes)" |
| 552-554 | `estoque_lotes #1`, `#2`, `#3` | Scenario example: "Multiple NFes from One Purchase Order" |
| 638 | `estoque_lotes #5` | Scenario example: "Items Break After Delivery" |

**VERDICT**: The authoritative reference **CONSISTENTLY uses `estoque_lotes`** across all scenarios and architecture diagrams.

### CONFLICTING SOURCES

| Source | Table Name | Line | Status |
|--------|-----------|------|--------|
| ✅ **modulos/estoque.md** | `estoque_lotes` | 94, 252 | **CORRECT** |
| ❌ **02-banco-dados.md** | `estoques` | 443, 473 | **WRONG** - Out of sync with reference |
| ✅ **Reference (02-schema-visual-overview.md)** | `estoque_lotes` | 393, 452, 552+ | **AUTHORITATIVE** |

### DECISION
- **FIX**: Update `02-banco-dados.md` to use `estoque_lotes` instead of `estoques`
- **Keep**: `modulos/estoque.md` is correct
- **Keep**: All code using `estoque_lotes`

---

## #2: CRITICAL - EstoqueStatus Enum Values

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

| Line | Status Value | Context | Note |
|------|--------------|---------|------|
| 267 | DISPONIVEL | Initial state | "status: DISPONIVEL (era)" |
| 267 | RESERVADO | After allocation | "status: 'RESERVADO' (allocated)" |
| 460 | DISPONIVEL | In WHERE clause | `status IN ('DISPONIVEL', 'RESERVADO')` |
| 460 | RESERVADO | In WHERE clause | Available for querying |
| 638 | DISPONIVEL | After restoration | "status: DISPONIVEL" |
| 938 | DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO | State machine | `estoque_lote_status: DISPONIVEL → RESERVADO → ESGOTADO / BLOQUEADO` |

**VERDICT**: The authoritative reference defines **4 statuses for estoque_lotes**:
1. **DISPONIVEL** - Available for allocation
2. **RESERVADO** - Allocated to a sale (via alocacoes)
3. **ESGOTADO** - Completely consumed
4. **BLOQUEADO** - Blocked (damaged, etc.)

### CONFLICTING SOURCES

| Source | Values | Status |
|--------|--------|--------|
| ✅ **02-banco-dados.md (L206)** | DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO | **CORRECT** - Matches reference |
| ❌ **modulos/estoque.md Enum (L660)** | TEMP, ESTOQUE, CANCELADO | **WRONG** - Different values, wrong purpose |
| ⚠️ **modulos/estoque.md State Machine (L135)** | TEMP, ESTOQUE, CONSUMIDO, CANCELADO | **WRONG** - Confuses estoque_lotes status with venda_item status |

### CLARIFICATION: Three Different Status Fields

The reference distinguishes between three different status fields:

1. **estoque_lotes.status** (Line 267, 460, 938)
   - Values: DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO
   - Purpose: Track physical inventory state
   - Source: PostgreSQL ENUM in 02-banco-dados.md (L206) ✓

2. **venda_items.status** (Line 518, 566, 588)
   - Values: ESTOQUE, PENDENTE, ENTREGUE (implied from flow)
   - Purpose: Track position in sales process
   - Source: NOT yet formally defined, but shown in scenarios

3. **alocacoes.status** (Line 632)
   - Values: ATIVO, PARCIALMENTE_ESTORNADO, CANCELADA (implied)
   - Purpose: Track allocation validity
   - Source: Partially shown in reference

### DECISION
- **modulos/estoque.md PHP Enum (EstoqueStatus)** is **WRONG**
- This enum tries to represent `estoque_lotes.status` but uses wrong values (TEMP/ESTOQUE/CANCELADO)
- **FIX**: Update EstoqueStatus enum to use: DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO
- **Create separately**: Enums for venda_item status and alocacao status

---

## #3: HIGH - Status Semantics Confusion

### ROOT CAUSE IDENTIFIED

The confusion in `modulos/estoque.md` line 101 vs line 660 comes from mixing two different concepts:

| Concept | Values | Purpose | Source |
|---------|--------|---------|--------|
| **estoque_lotes.status** | DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO | Physical inventory | Reference, 02-banco-dados.md |
| **venda_item.status** | ESTOQUE, PENDENTE, ENTREGUE | Sales process | Reference (implicit) |

The file incorrectly shows "RECEBIDO, RESERVADO, CONSUMIDO, QUEBRA, DEVOLUCAO" in the comment - these don't match either enum!

### DECISION
- **FIX**: Update modulos/estoque.md line 101 comment to match authoritative reference
- Use: DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO for estoque_lotes

---

## #4: CRITICAL - Field Naming: `lote_id` (FK Column)

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

| Line | Context | Field Name |
|------|---------|-----------|
| 257 | INSERT INTO alocacoes | `lote_id=5` |
| 402 | alocacoes definition | `venda_item_id ↔ lote_id` |
| 504 | Insertion step | `venda_item_id ↔ lote_id (M:N)` |
| 570 | Query example | `SELECT lote_id, quantidade FROM alocacoes WHERE venda_item_id = 100` |

**VERDICT**: The reference uses **`lote_id`** consistently.

### CLARIFICATION
- **FK column name in alocacoes**: Should be `estoque_lote_id` or `lote_id` (need to decide)
- **What it references**: The `id` column of `estoque_lotes` table
- The reference shows `lote_id` which is a shorthand
- Database convention would be `estoque_lote_id` to be explicit

### DECISION
- Use **`estoque_lote_id`** in alocacoes table (explicit, follows naming convention)
- In queries and code, this references `estoque_lotes.id`
- Consistent with: `venda_item_id`, `compra_item_id`, etc.

---

## #5: MEDIUM - `tipo` Field Overload in financeiro

### NOT ADDRESSED IN REFERENCE

The reference document doesn't detail the financeiro module deeply. However, from modulos/financeiro.md:

| Table | Column | Values | Purpose |
|-------|--------|--------|---------|
| `financeiro_parcelas` | `tipo` | RECEBER, PAGAR | **Discriminator** - Which table is referenced |
| `centros_custo` | `tipo` | OPERACIONAL, ADMINISTRATIVO, COMERCIAL | **Classification** - Type of cost center |

**VERDICT**: These are legitimately different fields with different purposes, both named `tipo`. Not necessarily a bug, but should be clearly documented to avoid confusion.

### DECISION
- Keep both fields as-is
- **FIX**: Add documentation clarifying the two different `tipo` fields
- Add database constraints to prevent confusion

---

## #6: MEDIUM - Event Sourcing Table Naming

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

| Line | Table Name | Pattern |
|------|-----------|---------|
| 81 | `estoque_lotes_events` | `*_events` |
| 81 | `alocacoes_events` | `*_events` |
| 196 | `estoque_lotes_events` | Consistently `*_events` |
| 196 | `alocacoes_events` | Consistently `*_events` |

**VERDICT**: The reference uses **`*_events` pattern** (English).

### CONFLICTING SOURCES

| Source | Pattern | Status |
|--------|---------|--------|
| ✅ **Reference (02-schema-visual-overview.md)** | `*_events` | **CORRECT** |
| ⚠️ **modulos/estoque.md** | `estoque_movimentacoes` | **WRONG** - Uses Portuguese |
| ❌ **modulos/financeiro.md** | `financeiro_parcelas_events` | **CORRECT** - Matches reference |

### DECISION
- **FIX**: Rename `estoque_movimentacoes` → `estoque_lotes_events`
- Update all references in modulos/estoque.md
- Keep `alocacoes_events` (already correct)
- Keep `financeiro_parcelas_events` (already correct)

---

## #7: HIGH - Allocation Status vs Event Type

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

| Line | Status | Context |
|------|--------|---------|
| 632 | PARCIALMENTE_ESTORNADO | alocacoes status when partially reversed |
| 590 | ATIVO | alocacoes status when active |

**VERDICT**: The reference shows `alocacoes` has status field with at least:
- ATIVO (active)
- PARCIALMENTE_ESTORNADO (partially reversed)
- (CANCELADA implied)

### DECISION
- **FIX**: In modulos/estoque.md, use single AlocacaoStatus enum
- Values: ATIVO, PARCIALMENTE_ESTORNADO, CANCELADA
- Remove AllocationEventType (it's redundant - use event types in alocacoes_events table instead)

---

## #8: CRITICAL - Phase 1/2 Scope Dependency

### NOT ADDRESSED IN REFERENCE

The visual overview doesn't address the phase planning. However, logical dependency is clear:

- Cadastros marks banking fields as Phase 2
- Financeiro Phase 2 (Bank Integration) depends on those fields
- **CIRCULAR DEPENDENCY**

### DECISION
- Financeiro Phase 2 cannot start until Cadastros Phase 2 completes
- Either:
  - A) Move banking fields to Phase 1 in Cadastros, OR
  - B) Create Financeiro Phase 1 without bank integration, add integration in Phase 2 (after Cadastros Phase 2)
- Currently unclear which approach is intended

---

## #9: MEDIUM - Venda_item `origem` Field

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

| Line | Reference | Values |
|------|-----------|--------|
| 361 | venda_itens | `origem: COMPRA ou ESTOQUE` |

**VERDICT**: The reference confirms **two values**:
1. **COMPRA** - Item needs to be purchased from supplier
2. **ESTOQUE** - Item comes from existing inventory

### DECISION
- **FIX**: Create formal enum in modulos/vendas.md
- Values: COMPRA, ESTOQUE
- This discriminates how item is fulfilled (from stock vs from purchase order)

---

## #10: HIGH - FK Reference Consistency

### REFERENCE AUTHORITY (02-schema-visual-overview.md)

All references to alocacoes show M:N relationship to estoque_lotes:
- Line 257: `lote_id=5`
- Line 504: `venda_item_id ↔ lote_id (M:N)`
- Line 570: `SELECT lote_id, quantidade FROM alocacoes WHERE venda_item_id = 100`

**VERDICT**: FK should reference `estoque_lotes.id`

### DECISION
- **FIX**: Update `02-banco-dados.md` to define `estoque_lotes` (not `estoques`)
- All FK columns named `estoque_lote_id` reference this table

---

## Summary: Using Reference as Authority

| Issue | Reference Authority | Decision | Action |
|-------|--------|-----------|-----------|
| 1. Table Name | `estoque_lotes` (L393, 452, 552+) | Use `estoque_lotes` | UPDATE: 02-banco-dados.md |
| 2. Estoque Status | DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO (L267, 460, 938) | Use 4 values | UPDATE: EstoqueStatus enum in modulos/estoque.md |
| 3. Status Semantics | Reference shows 3 different status fields (L267, 518, 632) | Separate enums per field | UPDATE: Clarify in modulos/estoque.md & modulos/vendas.md |
| 4. FK Field Name | `lote_id` referenced (L257, 402, 504, 570) | Use `estoque_lote_id` | VERIFY: Naming consistency |
| 5. Tipo Overload | NOT addressed in reference | Document separately | DOCUMENT: Two different `tipo` fields |
| 6. Event Sourcing | `*_events` pattern (L81, 196) | Use `*_events` | UPDATE: Rename `estoque_movimentacoes` → `estoque_lotes_events` |
| 7. Allocation Status | ATIVO, PARCIALMENTE_ESTORNADO, (CANCELADA) (L632, 590) | Single enum | UPDATE: Merge/clarify AlocacaoStatus |
| 8. Phase Dependency | NOT addressed in reference | Clarify scope | CLARIFY: Cadastros/Financeiro phase alignment |
| 9. Origem Values | COMPRA, ESTOQUE (L361) | Create enum | CREATE: origem enum in modulos/vendas.md |
| 10. FK Consistency | References to `estoque_lotes.id` (L257, 504, 570) | Use `estoque_lote_id` FK | UPDATE: 02-banco-dados.md |

---

## Files to Update (Priority Order)

### 🔴 CRITICAL (Will Break Code)
1. **02-banco-dados.md**
   - Change table name: `estoques` → `estoque_lotes` (Line 443)
   - All FK references: `estoque_id` → `estoque_lote_id` and ensure they reference `estoque_lotes`
   - Verify SQL enum matches reference (DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO) ✓ Already correct

2. **modulos/estoque.md**
   - Update EstoqueStatus enum (Line 660): TEMP/ESTOQUE/CANCELADO → DISPONIVEL/RESERVADO/ESGOTADO/BLOQUEADO
   - Remove EstoqueStatus or repurpose it for venda_item status
   - Rename `estoque_movimentacoes` → `estoque_lotes_events`
   - Add AlocacaoStatus enum with proper values

### 🟠 HIGH (Prevents Phase 2)
3. **modulos/financeiro.md**
   - Document the two different `tipo` fields and their purposes
   - Ensure phase dependencies are clear

4. **modulos/vendas.md**
   - Create `VendaItemOrigem` enum: COMPRA, ESTOQUE
   - Define venda_item status enum (ESTOQUE, PENDENTE, ENTREGUE - implied from reference)

### 🟡 MEDIUM (Code Quality)
5. **cadastros.md**
   - Already updated with polymorphic enderecos design ✓
   - No further changes needed

---

## Conclusion

Using `03-decisoes/02-schema-visual-overview.md` as the authoritative reference resolves the majority of conflicts:

✅ **8 out of 10 issues are now RESOLVED** with clear decisions
⚠️ **2 issues remain** (phase dependency, some field naming conventions) that require team decision

The reference document is comprehensive and internally consistent. Other documents should be updated to align with it.
