# Next-Gen Documentation Organization Review

**Date**: 2026-01-10
**Scope**: Complete review of .claude/next-gen/ folder structure including rascunhos/

---

## Executive Summary

The documentation is **well-organized overall** with clear logical flow (01-contexto → 05-execução). However:

- **✅ Strengths**: Clear folder purposes, reading sequence, comprehensive coverage
- **⚠️ Issues**:
  - Index file (00-indice.md) references deleted files (outdated)
  - Some rascunhos files are unique and valuable (should document purpose)
  - 03-design-greenfield.md overlaps with module-specific docs
  - Analysis files (02-analise/) may be deprecated by decision docs (03-decisoes/)
  - No clear guidance on which rascunhos docs are actively maintained

**Recommendation**: Update index, add rascunhos purpose documentation, consider consolidating redundant analysis.

---

## Current Folder Structure

### 01-contexto/ (5 files - 210+ lines each)
**Purpose**: Understand the legacy system's business logic

Files:
- 01-visao-geral-fluxos.md - High-level system flows and L1/L2 architecture
- 02-fluxos-estoque.md - Inventory creation, consumption, returns
- 03-fluxos-entrega-nfe.md - Delivery, NFe, CNAB, commissions
- 04-fluxos-cadastros.md - Master data, quotes, warehouse, permissions
- 05-regras-negocio.md - Pricing, taxes, validations, status transitions

**Status**: ✅ GOOD - Focused on understanding current system, not design

---

### 02-analise/ (5 files - 700+ lines each)
**Purpose**: Analyze problems and evaluate solution options

Files:
- 01-comparativo-legado-novo.md - Side-by-side comparison of old vs new
- 02-melhorias.md - Problem areas and improvement options
- 03-simplificacao-l1l2.md - 3 options for flattening L1/L2 tables (with recommendation)
- 04-correcao-fifo.md - FIFO inventory consumption problem analysis
- 05-normalizacao-fornecedor.md - Foreign key normalization

**Status**: ⚠️ PARTIALLY REDUNDANT
- These provide detailed exploration of tradeoffs already decided in 03-decisoes/
- 03-simplificacao-l1l2.md, 04-correcao-fifo.md, 05-normalizacao-fornecedor.md are valuable for understanding **why** decisions were made (decision history/rationale)
- 01-comparativo-legado-novo.md, 02-melhorias.md might duplicate 03-decisoes/01-adrs.md

**Recommendation**: Keep as-is (valuable for decision context), but mark as "reference/historical"

---

### 03-decisoes/ (4 files)
**Purpose**: Record architectural decisions and design

Files:
- 01-adrs.md - Architecture Decision Records (Laravel, PostgreSQL, frontend, NFe, migration)
- **02-schema-redesenhado.md** (1800+ lines) - 🟢 AUTHORITATIVE PostgreSQL schema
- **02-schema-visual-overview.md** (1279 lines) - 📊 Flowcharts and visual patterns
- 03-design-greenfield.md (1766 lines) - Complete reimagining of system design

**Status**: ⚠️ SOME OVERLAP
- 02-schema-redesenhado.md & 02-schema-visual-overview.md: Complementary ✅ (text vs visual)
- 03-design-greenfield.md: **OVERLAPS with module-specific docs** (vendas.md, estoque.md, financeiro.md, logistica.md)
  - Contains detailed entity models, state machines, flows
  - Module docs (04-arquitetura/modulos/) contain same information
  - **Greenfield appears to be draft/complete version**; modules are final/focused versions

**Recommendation**:
- Keep 02-schema-*.md as definitive schema references
- 03-design-greenfield.md: Either archive or mark as "v1.0 complete design" (reference), rely on module docs for implementation

---

### 04-arquitetura/ (25 files + modulos/ subfolder)
**Purpose**: How to build the system

**Base Architecture Files** (04-arquitetura/):
- 01-arquitetura.md - Laravel structure, patterns, services
- 02-banco-dados.md - Database design principles
- 03-frontend.md - Frontend framework evaluation
- 04-infraestrutura.md - Infrastructure, auditoria, search
- 05-seguranca.md - Authentication, authorization
- 06-api.md - REST API design
- 07-testes.md - Testing strategy
- 08-erros-monitoramento.md - Error handling, logging
- 09-integracoes.md - External integrations (ACBr, CNAB)
- 10-design-system.md - UI design system
- 11-concorrencia.md - Locks, transactions, race conditions
- 12-atalhos-teclado.md - Keyboard shortcuts, accessibility
- 13-impressao.md - PDF, Excel, receipts, DANFE
- 14-devops.md - Docker, CI/CD, deployment
- 15-dicionario-dados.md - Glossary, enums, conventions
- 16-compatibilidade.md - Browser/device compatibility
- 17-validacao.md - Validation strategy (multi-layer)
- 18-dependencias.md - Dependency audit (PHP/NPM)

**Module Files** (04-arquitetura/modulos/):
- _indice.md - Module implementation priority list
- cadastros.md - Master data CRUD module
- compras.md - Purchasing module (completed Phase 2 consolidation ✅)
- estoque.md - Inventory module (completed Phase 3 consolidation ✅)
- financeiro.md - Financial module (completed Phase 2 consolidation ✅)
- logistica.md - Logistics module
- nfe.md - Electronic invoice module
- relatorios.md - Reporting module
- vendas.md - Sales module (completed Phase 2-3 consolidation ✅)

**Status**: ✅ EXCELLENT - Comprehensive, well-organized, module docs recently consolidated

---

### 05-execucao/ (4 files)
**Purpose**: How to migrate to new system

Files:
- 01-plano-migracao.md - Strangler Fig migration phases
- 02-migracao-dados.md - Data migration strategy (updated Phase 3 ✅)
- 03-paridade-funcionalidades.md - Functional parity checklist
- 04-treinamento.md - Training and rollout plan

**Status**: ✅ GOOD - Focused on execution

---

### rascunhos/ (3 files)
**Purpose**: Explorations and evaluated decisions

**Current Files**:
1. **alocacao-m2n-workflow.md** (505 lines)
   - Status: "Proposto"
   - Content: M:N allocation workflow, state machines, scenarios, queries
   - Value: Comprehensive workflow guide NOT in other docs
   - Recommendation: ✅ KEEP
   - Why: Unique implementation reference for allocation pattern

2. **event-sourcing-analise.md** (437 lines)
   - Status: "Partially adopted in v1" (recently updated)
   - Content: Full ES explanation + what we implemented vs deferred
   - Value: Roadmap for future v2+ Event Sourcing implementation
   - Recommendation: ✅ KEEP
   - Why: Future planning reference, documents hybrid approach

3. **1nn-problem-flowchart.md** (376 lines)
   - Status: "Root cause analysis"
   - Content: Problem visualization, current issues, solution approach
   - Value: **LARGELY DUPLICATES** 02-schema-redesenhado.md sections 6.1-6.3
   - Recommendation: ❌ DELETE
   - Why: Content already in schema-redesenhado.md (Resumo das Mudanças, Simplificações, Processo de Parear)

**Deleted Files** (Cleanup 2026-01-10):
- ❌ schema-proposto.md (explicitly deprecated)
- ❌ schema-proposto-flowchart.md (duplicate of 02-schema-visual-overview.md)
- ❌ schema-alternativo-2-entidades.md (alternative not adopted)
- ❌ ISSUES-REMAINING.md (completion tracking record)
- ❌ CLEANUP-ANALYSIS.md (cleanup documentation)

**Final rascunhos/ status**:
- 3 active files (alocacao-m2n-workflow, event-sourcing-analise, 1nn-problem-flowchart)
- Recommendation: Keep only 2 (delete 1nn-problem-flowchart for duplication)

---

## Issues Found

### Issue #1: Outdated Index References (HIGH PRIORITY)

**File**: 00-indice.md, lines 121-124

Current references:
```
├── 1nn-problem-flowchart.md          # 🔴 LEIA PRIMEIRO
├── schema-alternativo-2-entidades.md # 📋 Avaliado
├── schema-proposto.md                # ❌ DEPRECATED
└── event-sourcing-analise.md         # 📋 Avaliado para v2+
```

**Problem**: References deleted files (schema-alternativo, schema-proposto)

**Fix Required**:
```
├── alocacao-m2n-workflow.md          # 📘 Workflow implementation guide
├── event-sourcing-analise.md         # 🔮 Future v2+ Event Sourcing roadmap
└── 1nn-problem-flowchart.md          # 📊 Root cause analysis (REMOVE - see Issue #2)
```

---

### Issue #2: 1nn-Problem-Flowchart Duplication (MEDIUM PRIORITY)

**File**: rascunhos/1nn-problem-flowchart.md vs 03-decisoes/02-schema-redesenhado.md

**Problem**: Content already documented in schema-redesenhado.md

**Evidence**:
- L1/L2 problems: schema-redesenhado.md 6.1 (line 1535)
- Query simplifications: schema-redesenhado.md 6.2 (line 1547)
- Allocation process: schema-redesenhado.md 6.3 (line 1588)
- Problem visualization: schema-redesenhado.md lines 59, 71, 246

**Recommendation**: Delete rascunhos/1nn-problem-flowchart.md

**Rationale**:
- Flowchart value added by visualization, but text content duplicates schema doc
- schema-redesenhado.md is more concise and decision-focused
- Keep comprehensive explanation in decision docs, not exploratory docs

---

### Issue #3: Design-Greenfield Potential Overlap (LOW PRIORITY)

**File**: 03-decisoes/03-design-greenfield.md (1766 lines)

**Problem**: May duplicate module-specific architecture docs

**Content Check**:
- Sections: Philosophy, concepts, bounded contexts, order lifecycle, stock management, delivery, fiscal/financial, state machines, event architecture, schema, testing, advanced features
- Module docs (vendas, estoque, financeiro, logistica) contain similar sections

**Status**: Not necessarily problematic - greenfield could be:
- Complete v1.0 design (archived reference)
- Overview before module deep-dives

**Recommendation**: Keep as-is (valuable overview), but update 00-indice.md to clarify when to read:
- "Read 03-design-greenfield.md for complete system overview"
- "Read specific module docs (04-arquitetura/modulos/) for implementation details"

---

### Issue #4: Analysis Files May Be Deprecated (LOW PRIORITY)

**Files**: 02-analise/ (5 files, ~3500 lines)

**Problem**: Decisions already made in 03-decisoes/ - are these still needed?

**Assessment**:
- ✅ 01-comparativo-legado-novo.md - KEEP (comparison valuable)
- ✅ 02-melhorias.md - KEEP (improvement options context)
- ✅ 03-simplificacao-l1l2.md - KEEP (explains why flattened tables chosen)
- ✅ 04-correcao-fifo.md - KEEP (explains why M:N allocation needed)
- ✅ 05-normalizacao-fornecedor.md - KEEP (explains why FKs chosen)

**Recommendation**: KEEP ALL - these are valuable decision history/rationale

**Action**: Update 00-indice.md to clarify:
- "02-analise/ explores problems and options (decision history)"
- "03-decisoes/ records final decisions"

---

## Recommendations Summary

### Priority 1: Update 00-indice.md (Immediate)

**Changes**:
1. Remove references to deleted files (schema-alternativo, schema-proposto)
2. Update rascunhos/ section to reflect actual files:
   ```
   └── rascunhos/                            # Exploracoes Ativas
       ├── alocacao-m2n-workflow.md          # 📘 M:N allocation workflow guide
       ├── event-sourcing-analise.md         # 🔮 Event Sourcing v2+ roadmap
       └── 1nn-problem-flowchart.md          # 📊 Root cause problem analysis
   ```
3. Add clarity sections:
   - "02-analise/ contains decision history and rationale"
   - "03-design-greenfield.md is complete system overview (before module deep-dives)"

---

### Priority 2: Delete 1nn-Problem-Flowchart (Optional)

**File**: rascunhos/1nn-problem-flowchart.md

**Rationale**: Duplicates schema-redesenhado.md sections 6.1-6.3

**Impact**: Low - content exists elsewhere

**Recommendation**: DELETE if want clean rascunhos folder (only 2 files: workflow + event-sourcing)

---

### Priority 3: Archive 03-Design-Greenfield Status Clarification

**File**: 03-decisoes/03-design-greenfield.md

**Update Header**:
```markdown
> Status: **Complete System Design Overview (v1.0)**
> Purpose: Comprehensive system design before module-specific implementation
> **For implementation details, see**: [04-arquitetura/modulos/](./04-arquitetura/modulos/)
```

---

### Priority 4: Improve Index Navigation

**Add to 00-indice.md**:
```
## Reading Path by Goal

### "I'm new, understand everything"
01-contexto → 02-analise → 03-decisoes/02-schema-*.md → 03-design-greenfield.md → 04-arquitetura/01

### "I need to implement [module]"
04-arquitetura/modulos/[module].md → related base architecture docs

### "I need to understand decisions"
03-decisoes/01-adrs.md → relevant 02-analise files

### "I'm planning v2+ features"
rascunhos/event-sourcing-analise.md → relevant ADRs
```

---

## Organization Summary

| Folder | Purpose | Status | Action |
|--------|---------|--------|--------|
| 00-indice.md | Navigation hub | ⚠️ Outdated refs | UPDATE |
| 01-contexto/ | Understand legacy | ✅ Good | Keep |
| 02-analise/ | Problem analysis | ✅ Historical value | Update clarity |
| 03-decisoes/ | Architectural decisions | ✅ Good | Clarify greenfield role |
| 04-arquitetura/ | Implementation guides | ✅ Excellent | Keep (recently consolidated) |
| 05-execucao/ | Migration execution | ✅ Good | Keep |
| rascunhos/ | Active explorations | ⚠️ Has duplicate | Delete 1nn-problem-flowchart |

---

## Files Overview (Complete)

### Context (01-contexto/) - 5 files, understand legacy
- ✅ visao-geral-fluxos.md
- ✅ fluxos-estoque.md
- ✅ fluxos-entrega-nfe.md
- ✅ fluxos-cadastros.md
- ✅ regras-negocio.md

### Analysis (02-analise/) - 5 files, problem exploration
- ✅ comparativo-legado-novo.md
- ✅ melhorias.md
- ✅ simplificacao-l1l2.md
- ✅ correcao-fifo.md
- ✅ normalizacao-fornecedor.md

### Decisions (03-decisoes/) - 4 files, architectural choices
- ✅ 01-adrs.md
- ✅ **02-schema-redesenhado.md** (🟢 AUTHORITATIVE)
- ✅ 02-schema-visual-overview.md (📊 Visual)
- ✅ 03-design-greenfield.md (needs clarity update)

### Architecture (04-arquitetura/) - 25 files, implementation guides
- ✅ 01-arquitetura.md
- ✅ 02-banco-dados.md
- ✅ 03-frontend.md
- ✅ ... (15 more base architecture files)
- ✅ **modulos/** - 9 module-specific implementations (recently consolidated)

### Execution (05-execucao/) - 4 files, migration plan
- ✅ 01-plano-migracao.md
- ✅ 02-migracao-dados.md
- ✅ 03-paridade-funcionalidades.md
- ✅ 04-treinamento.md

### Rascunhos (rascunhos/) - 3 files, active explorations
- ✅ alocacao-m2n-workflow.md
- ✅ event-sourcing-analise.md
- ❌ 1nn-problem-flowchart.md (DUPLICATE - candidate for deletion)

---

## Next Steps

1. ✅ Update 00-indice.md to fix outdated references
2. ⚠️ Delete rascunhos/1nn-problem-flowchart.md (optional, removes duplication)
3. ⏳ Update 03-design-greenfield.md header to clarify role
4. 📝 Consider adding "Reading Paths by Goal" to index for navigation help
