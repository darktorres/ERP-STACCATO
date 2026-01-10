# Rascunhos Folder Cleanup Analysis

**Date**: 2026-01-10
**Status**: Ready for cleanup decision

---

## Summary

7 markdown files in `rascunhos/` folder. Recommendation: **4 files can be deleted**, **3 files should be kept** (with status clarification).

| File | Purpose | Status | Action | Reason |
|------|---------|--------|--------|--------|
| `schema-proposto.md` | Deprecated schema | ARCHIVED | ❌ DELETE | Explicitly marked deprecated, replaced by schema-redesenhado.md |
| `schema-proposto-flowchart.md` | Visual schema reference | MOVED | ❌ DELETE | Duplicate copy - original is at 03-decisoes/02-schema-visual-overview.md |
| `1nn-problem-flowchart.md` | Root cause analysis | REFERENCED | ✅ KEEP | Essential reference explaining why M:N model is necessary |
| `schema-alternativo-2-entidades.md` | Alternative design | EVALUATED | ✅ KEEP | Decision documentation for future architectural discussions |
| `event-sourcing-analise.md` | Event Sourcing analysis | OUTDATED | ⚠️ UPDATE | Marked "not adopted for v1" but we DID implement Event Sourcing - needs status update |
| `alocacao-m2n-workflow.md` | M:N allocation workflow | COMPLETE | ✅ KEEP | Complete workflow guide - content merged into modules but valuable standalone reference |
| `ISSUES-REMAINING.md` | Issue tracking | ACTIVE | ✅ KEEP | Issue tracking and progress document |

---

## Detailed Analysis

### ❌ DELETE (2 files)

#### 1. `schema-proposto.md` (Deprecated Schema)

**Current Status**:
- Explicitly marked `⚠️ DEPRECATED ARCHIVE`
- Lines 3-11 redirect to schema-redesenhado.md and schema-visual-overview.md
- Contains only deprecation notice + historical reference

**Recommendation**: DELETE
- No ongoing value - purely historical archive
- All users directed to schema-redesenhado.md
- Deprecation notice is clear but having file creates confusion

**Action**: Delete file, add to .gitignore if needed for cleanup

---

#### 2. `schema-proposto-flowchart.md` (Duplicate Copy)

**Current Status**:
- 1279 lines identical to 03-decisoes/02-schema-visual-overview.md
- Was moved to decisions folder during Phase 1 consolidation
- Duplicate file in two locations

**Recommendation**: DELETE
- Authoritative location is 03-decisoes/02-schema-visual-overview.md
- Rascunhos version is redundant
- Keeping both creates maintenance burden

**Action**: Delete file from rascunhos

---

### ✅ KEEP (3 files)

#### 1. `1nn-problem-flowchart.md` (Root Cause Analysis)

**Current Status**:
- Header: "Essential Reading - Read this FIRST"
- Marked as "Referenced from schema-redesenhado.md"
- Explains: Old system structure, why M:N necessary, 3-entity model

**Value**:
- Critical context for understanding architectural decisions
- Referenced in schema documentation
- Helps new team members understand problem domain
- Flowchart visualization of 1:N:N problem

**Recommendation**: KEEP (no changes needed)
- Belongs in rascunhos as analysis/context document
- Consider: Add link to this from 00-indice.md under "Architectural Context" section

---

#### 2. `schema-alternativo-2-entidades.md` (Alternative Design Evaluation)

**Current Status**:
- Header: "2-Entity Model (Evaluated - Not Adopted)"
- Decision: "3-Entity model selected"
- When to read: "If reconsidering architectural tradeoffs or planning v2+ refactoring"

**Value**:
- Decision documentation: why 3-entity chosen over 2-entity
- Architectural tradeoff analysis
- Reference for future refactoring discussions

**Recommendation**: KEEP (no changes needed)
- This is exactly what rascunhos should contain: evaluated alternatives
- Valuable for architectural discussions and retrospectives

---

#### 3. `event-sourcing-analise.md` (Event Sourcing Analysis)

**Current Status**:
- Header: "Not adopted for v1 - deferred to v2+"
- Reason: "Team size/complexity tradeoff"
- Note: "schema-proposto-flowchart.md explores pg_ivm"

**⚠️ PROBLEM**: Status is OUTDATED
- **FACT**: We implemented Event Sourcing in Phase 3!
  - `estoque_movimentacoes` table (append-only audit trail)
  - `alocacoes_eventos` table (allocation event log)
  - `financeiro_parcelas_events` table (financial event log)
  - `venda_itens_events` table (sales item event log)
- All documented in: estoque.md, vendas.md, financeiro.md

**Recommendation**: UPDATE STATUS
- Change "Not adopted for v1" → "Partially adopted for v1"
- Document: We ARE using append-only event tables + immutability triggers
- We are NOT using full Event Sourcing (CQRS pattern with full state reconstruction)
- This is a hybrid approach: Event Sourcing for audit trail, but normal tables for current state
- Keep file as reference for future full ES implementation in v2+

**Action**:
1. Update header status to clarify "Partially adopted"
2. Document what we implemented vs full ES pattern
3. Note this as foundation for v2+ full ES migration

---

#### 4. `alocacao-m2n-workflow.md` (M:N Allocation Workflow)

**Current Status**:
- Status: "Proposto"
- Marked as COMPLETED in ISSUES-REMAINING.md
- Complete workflow guide: entities, state machines, scenarios, queries

**Value**:
- Comprehensive workflow documentation
- State machine diagrams for venda_item and alocacao
- Step-by-step allocation process
- Validation rules and transaction patterns
- Antipatterns and benefits documented

**Overlap**: Content has been merged into module documentation (vendas.md, estoque.md, logistica.md)

**Recommendation**: KEEP as reference document
- Valuable standalone workflow guide
- Module documentation is more code-focused
- This provides process-level understanding
- Helps with implementation and training

**Action**: No changes needed - keep as-is

---

#### 5. `ISSUES-REMAINING.md` (Issue Tracking)

**Current Status**:
- Active tracking document
- Just updated with all 9 issues verified as COMPLETED
- Comprehensive progress tracking

**Recommendation**: KEEP as reference archive
- Now serves as completion record for consistency issues
- Useful for retrospectives
- Could be moved to completed-tasks or archived folder if wanted

**Action**: No changes needed

---

## Cleanup Plan

### Phase 1: Delete Duplicates & Deprecated (immediate)
```bash
# Remove deprecated schema file
rm rascunhos/schema-proposto.md

# Remove duplicate flowchart
rm rascunhos/schema-proposto-flowchart.md
```

### Phase 2: Update Status Documentation (immediate)
```bash
# Update event-sourcing-analise.md status
# Change "Not adopted for v1" to "Partially adopted for v1"
# Document hybrid approach: Event Sourcing for audit, normal state tables
```

### Phase 3: Link Context Documents (optional)
```bash
# Add references in 00-indice.md:
# - 1nn-problem-flowchart.md under "Architectural Context"
# - schema-alternativo-2-entidades.md under "Alternative Designs"
# - event-sourcing-analise.md under "Future Architecture"
```

---

## Files Summary After Cleanup

**Remaining in rascunhos/** (5 files):
1. `1nn-problem-flowchart.md` - Root cause analysis (context)
2. `schema-alternativo-2-entidades.md` - Alternative design evaluation (decision docs)
3. `event-sourcing-analise.md` - Event sourcing analysis (future planning, UPDATED)
4. `alocacao-m2n-workflow.md` - M:N workflow guide (reference)
5. `ISSUES-REMAINING.md` - Issue tracking archive (completion record)

**Moved/Deleted** (2 files):
- ❌ `schema-proposto.md` → DELETE (deprecated)
- ❌ `schema-proposto-flowchart.md` → DELETE (duplicate of 03-decisoes/02-schema-visual-overview.md)

---

## Decision Points

1. **Delete schema-proposto.md?** → YES
2. **Delete schema-proposto-flowchart.md?** → YES (duplicate)
3. **Update event-sourcing-analise.md?** → YES (status outdated)
4. **Add index references to remaining files?** → OPTIONAL (improves discoverability)
