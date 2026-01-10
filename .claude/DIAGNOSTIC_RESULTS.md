# Diagnostic Results: Product Database Duplicates

**Date:** 2026-01-09
**Database:** staccato
**Scope:** Analysis of producto table and referential integrity

---

## Executive Summary

**⚠️ CRITICAL ISSUE:** The database contains **367,498 duplicate product records** out of 395,244 total products - representing **92.98% duplication rate**.

This is NOT a recent problem from the import refactoring. This is a **systemic issue spanning 10 years** (2016-2026) where the application has been consistently creating duplicate products instead of updating existing ones.

---

## Key Statistics

| Metric | Value |
|--------|-------|
| **Total Products** | 395,244 |
| **Unique Business Keys** | 27,746 |
| **Duplicate Product Records** | 367,498 |
| **Duplicate Product Groups** | 24,580 |
| **Duplication Rate** | 92.98% |
| **Date Range** | 2016-01-05 to 2026-01-08 |
| **Years Affected** | 10 years |
| **Average Duplicates per Business Key** | 14.25 |

---

## Analysis by Product Type

| Type | Total Count | Unique Keys | Duplicates | Duplicate % |
|------|------------|------------|-----------|------------|
| **NULL (no type)** | 352,201 | 257,754 | 94,447 | 26.8% |
| **REVESTIMENTO** | 42,744 | 23,649 | 19,095 | 44.7% |
| **BANHEIRA** | 299 | 275 | 24 | 8.0% |

**Note:** NULL tipo comprises 89% of all products. Within NULL tipo group, there are still 94k duplicate records.

---

## Duplicate Distribution Over Time

### Recent Year Duplication Rates (Last 24 Months)

| Month | Total Products | Unique Keys | Duplicate % |
|-------|----------------|------------|------------|
| 2026-01 | 21,717 | 0 | **100.00%** |
| 2025-12 | 1,152 | 2 | 99.83% |
| 2025-11 | 1,080 | 2 | 99.81% |
| 2025-10 | 1,761 | 1 | 99.94% |
| 2025-09 | 2,050 | 3 | 99.85% |
| 2025-08 | 5,699 | 3 | 99.95% |
| 2025-07 | 1,450 | 5 | 99.66% |
| 2025-06 | 950 | 0 | **100.00%** |
| 2025-05 | 1,407 | 2 | 99.86% |
| 2025-04 | 2,572 | 0 | **100.00%** |
| 2025-03 | 1,058 | 2 | 99.81% |
| 2025-02 | 1,289 | 0 | **100.00%** |
| 2025-01 | 800 | 1 | 99.88% |

**Pattern:** 99-100% of products created monthly are duplicates. This has been consistent for the entire period analyzed.

---

## Top 20 Most Duplicated Products

| Supplier | Code | UI | Type | Duplicate Count | Oldest | Newest | Span |
|----------|------|----|----|---------|--------|--------|------|
| Furnecedor 19 | Q1648 | 0 | NULL | **315** | 2018-07-05 | 2022-01-18 | 3+ years |
| Furnecedor 283 | 20010802 | 0 | NULL | **311** | 2018-08-27 | 2025-12-19 | 7+ years |
| Furnecedor 19 | 101010031 | 0 | NULL | **240** | 2016-01-05 | 2023-12-14 | 8+ years |
| Furnecedor 19 | Q1633 | 0 | NULL | **228** | 2018-07-05 | 2022-02-03 | 4+ years |
| Furnecedor 266 | 25644BR-0 | 0 | NULL | **224** | 2021-10-04 | 2025-04-29 | 4+ years |
| Furnecedor 19 | Q1639 | 0 | NULL | **222** | 2018-07-05 | 2022-01-18 | 4+ years |
| Furnecedor 19 | 101020018 | 0 | NULL | **207** | 2016-01-05 | 2022-10-11 | 7+ years |
| Furnecedor 2 | 6057736A | 5 - L | REVESTIMENTO | **193** | 2016-01-05 | 2021-07-25 | 5+ years |
| Furnecedor 19 | 102370005 | 0 | NULL | **188** | 2016-01-05 | 2023-08-18 | 7+ years |
| Furnecedor 2 | 6059800A | 0 | NULL | **180** | 2021-08-04 | 2026-01-02 | 4+ years |

**Observation:** Products with hundreds of duplicates created over 3-8 year periods. This indicates systematic, continuous creation of duplicates with every import cycle.

---

## Referential Integrity Status

### References in Dependent Tables

| Table | Total Records | Valid References | Orphaned |
|-------|--------------|-----------------|----------|
| `estoque` | 77,069 | 77,042 | **27** ⚠️ |
| `estoque_has_consumo` | 131,116 | 128,966 | **2,150** ⚠️ |
| `orcamento_has_produto` | 995,441 | 995,441 | **0** ✓ |
| `pedido_fornecedor_has_produto` | 115,516 | 115,513 | **3** ⚠️ |
| `venda_has_produto` | 177,812 | 177,812 | **0** ✓ |
| `venda_has_produto2` | 194,193 | 194,193 | **0** ✓ |
| `pedido_fornecedor_has_produto2` | 152,568 | 152,565 | **3** ⚠️ |
| `veiculo_has_produto` | 125,654 | 125,634 | **20** ⚠️ |
| `produto_has_preco` | 2,138,660 | 2,138,660 | **0** ✓ |

**Summary:** Mostly clean referential integrity (few orphans). However, the massive number of duplicates means the system has 24,580 different "canonical" product records that shouldn't exist.

---

## Root Cause Analysis

### Original Issue (Recent Fix)
The refactoring addressed `ON DUPLICATE KEY UPDATE` not working on composite business keys:
- Business Key: (idFornecedor, codComercial, ui, tipo)
- PRIMARY KEY: idProduto (auto-increment)

### Deeper Issue (Historical)
The duplication problem is NOT just from the recent import refactoring. Evidence:
1. **Timeline:** Duplicates date back to 2016 (10 years ago)
2. **Consistency:** 99-100% duplication rate for entire period
3. **Scale:** 367,498 total duplicates (not just recent)
4. **Pattern:** Multiple import cycles per product (many products have 100-300+ duplicates)

### Hypothesis
The application has **NEVER had working UPDATE logic** for products:
- Every import created new products (INSERTs) instead of updating existing ones
- No matter what logic was in place, it resulted in duplicates
- The recent fix improved the code structure but didn't retroactively clean the data

---

## Data Quality Issues

### Multiple IDs for Same Product
Example: Furnecedor 19, Code Q1648
- **Primary ID (canonical):** 270566 (created 2018-07-05)
- **Duplicate IDs:** 273068, 289811, 293802... (up to 315 different IDs)
- **Problem:**
  - Some sales use ID 270566
  - Some sales use ID 273068
  - Some sales use ID 289811
  - All refer to the SAME product
  - Reports and analytics are fragmented

### Example Impact
If you query sales of product with codComercial "Q1648":
```sql
SELECT * FROM venda_has_produto2 WHERE idProduto = 270566;  -- Some sales
SELECT * FROM venda_has_produto2 WHERE idProduto = 273068;  -- Other sales
SELECT * FROM venda_has_produto2 WHERE idProduto = 289811;  -- Other sales
-- Must query ALL 315 IDs to get complete picture!
```

---

## Business Impact

### Immediate Issues
1. **Reporting is Broken**
   - Sales reports split across hundreds of product IDs
   - Inventory counts fragmented
   - Revenue analytics inaccurate

2. **Data Maintenance Nightmare**
   - 367,498 extra product records consuming storage
   - Foreign keys point to wrong IDs
   - Price histories scattered across multiple product records

3. **System Performance**
   - 24,580 unnecessary product groups in database
   - Indexes bloated with duplicate data
   - Queries potentially slow due to data volume

### Risk if Not Fixed
1. **Future Imports:** Will continue creating duplicates (new issue is fixed in code, but old data remains)
2. **Data Integrity:** Orphaned records, inconsistent references
3. **Scalability:** Database will continue to grow with duplicates

---

## Fixability Assessment

### Challenges
1. **Scale:** 367,498 duplicate records to consolidate
2. **Complex Dependencies:** 9 tables with foreign keys
3. **Historical Data:** 10 years of mixed-ID transactions
4. **Automated Canon Selection:** Hard to determine which duplicate was "original"

### Feasibility
**FIXABLE but COMPLEX**

Options:
- ✓ Automated fix using "oldest created = canonical" rule
- ✓ Automated fix using "most referenced = canonical" rule
- ✓ Manual review + automated consolidation (safest but slowest)
- ✓ Per-supplier fix (handle suppliers with most duplicates first)

### Recommended Approach
1. **Assess criticality** - Does business need ALL old duplicates or just recent data?
2. **Choose consolidation rule** - Oldest, most-referenced, or mixed
3. **Backup database** - Full backup before starting
4. **Fix in batches** - By supplier group to manage risk
5. **Verify after each batch** - Check data integrity before moving to next

---

---

## CRITICAL ISSUES DISCOVERED (Post-Analysis)

### 1. Estoque Duplication: Same Inventory Across Multiple Product IDs

**Finding:** Inventory (estoque) records for the SAME lote, location, and bloco are split across multiple duplicate product IDs.

**Examples:**
- Fornecedor 19, Code 660371: lote "N/D", local "CD", bloco 1
  - idProduto 564173: quant=5341, restante=1112
  - idProduto 580438: same lote/location, different idProduto
  - **PROBLEM:** Two separate estoque records for same physical inventory

- Furnecedor 334, Code 0401010103: lote "N/D", local "CD", bloco 1
  - idProduto 562696
  - idProduto 580830
  - idProduto 592878
  - **3 different product IDs for same inventory location**

**Scale:** 30 examples found with detailed analysis, likely hundreds more

**Impact on Consolidation:**
- Cannot simply redirect estoque.idProduto to canonical product
- Must MERGE estoque records when consolidating
- Risk of:
  - Duplicate quantity counting (sum vs. keep largest)
  - Loss of inventory history
  - Unclear which record is "correct"

**This requires business logic decision:**
- Should quantities be summed?
- Should oldest or newest record be kept?
- Should all estoque records be merged into one?

---

### 2. idProdutoRelacionado: Cross-Product Linking to Duplicates

**Finding:** 84,319 products link to other products via `idProdutoRelacionado` field. **83,091 (98.5%) of these links point to products that ARE themselves duplicated.**

**What This Means:**
- Product A (id 12345) might link to Product B's canonical ID (id 67890)
- But Product B has 100 duplicate IDs
- After consolidation, the link is only valid if we pick the right canonical B
- If we pick a different duplicate ID as canonical, the links break

**Consolidation Challenge:**
- After selecting canonical product for Product B's duplicates
- Must update ALL 83,091 links to point to the newly-selected canonical
- If consolidation logic changes canonical product selection, links cascade to break

---

### 3. Promotion Logic (tipo Field)

**Finding:** Database has 3 tipos:
- NULL: 352,201 products (89%)
- REVESTIMENTO: 42,744 products
- BANHEIRA: 299 products
- Promocao: 0 products (feature appears unused currently)

**Note:** Promotion linking via `idProdutoRelacionado` exists but is not using explicit "Promocao" tipo. The linking is cross-tipo.

---

## Consolidation Complexity Matrix

| Issue | Complexity | Impact | Decision Needed |
|-------|-----------|--------|-----------------|
| **Duplicate Product IDs** | High | 367k duplicates to consolidate | Choose canonical selection rule |
| **Estoque Merging** | CRITICAL | Same inventory across IDs | Merge strategy (sum/keep/oldest/newest) |
| **idProdutoRelacionado Links** | High | 83k links to duplicates | Update strategy (cascade, validate) |
| **Price History** | Medium | producto_has_preco split across IDs | Keep all/merge/oldest only |
| **Sales History** | Medium | venda_has_produto* split across IDs | Informational (no action needed) |

---

## Next Steps

### For You to Decide:

**Option A: Emergency Import Fix Only** (Minimal Risk, Partial Solution)
- Fix the recent import code (✓ already done)
- Leave historical duplicates as-is
- Prevents FUTURE duplicates
- Doesn't fix EXISTING 367k duplicates
- **Timeline:** Done
- **Risk:** Low
- **Benefit:** Prevents new duplicates from here forward

**Option B: Full Cleanup** (Higher Risk, Complete Solution)
- Fix recent import code (✓ already done)
- Consolidate all 367,498 duplicate records
- Delete duplicate product records
- Update all dependent tables
- **Timeline:** 2-6 hours execution + verification
- **Risk:** High (large data modification)
- **Benefit:** Eliminates all duplicates, fixes reporting, clean data

**Option C: Hybrid Approach** (Medium Risk, Reasonable Solution)
- Fix recent import code (✓ already done)
- Clean duplicates for last N years only (e.g., last 2 years)
- Keep ancient historical data as-is
- **Timeline:** 1-2 hours execution + verification
- **Risk:** Medium
- **Benefit:** Cleans recent data, reduces risk, manageable scope

### If You Choose Full Cleanup:
1. I'll create detailed consolidation script
2. Run it in transaction mode (can rollback if needed)
3. Verify at each phase
4. Provide audit trail of all changes

---

## CRITICAL BUSINESS DECISIONS REQUIRED

Before implementing any fix, you must decide on these:

### Decision 1: Estoque Merging Strategy

**Problem:** Same physical inventory (lote, location, bloco) is split across multiple product duplicate IDs.

**Option A: Sum Quantities (Recommended for Inventory)**
- When merging estoque records, ADD quantities together
- Example: record1 (quant=100) + record2 (quant=50) = merged (quant=150)
- Keeps all inventory information
- **Risk:** May double-count if records are duplicates of each other

**Option B: Keep Largest Remaining Quantity**
- Keep only the estoque record with highest `restante` value
- Delete other duplicates
- **Benefit:** Conservative approach, avoids over-counting
- **Risk:** Lose historical quantities

**Option C: Keep Oldest Record Only**
- Keep the first-created estoque record, delete others
- Simplest approach
- **Risk:** Lose inventory history for newer records

### Decision 2: Canonical Product Selection

**Problem:** 367k duplicates need to be consolidated to ~27k canonical products.

**Option A: Oldest Record as Canonical (Recommended)**
- Use the product created earliest as canonical
- Logic: Original record is most likely correct
- **Pro:** Simple, repeatable, clear audit trail
- **Con:** Oldest might not have most recent data

**Option B: Most Referenced as Canonical**
- Use the product ID referenced by most sales/inventory
- Logic: Most-used ID is probably "correct" one
- **Pro:** Minimizes impact (fewer references to update)
- **Con:** Complex to calculate, business logic dependent

**Option C: Newest Record as Canonical**
- Use most recently created product
- Logic: Latest import is most current
- **Pro:** Has newest data
- **Con:** Contradicts "original is correct" assumption

### Decision 3: idProdutoRelacionado Linking Strategy

**Problem:** 83,091 products link to products that have duplicates.

**Challenge:** After consolidating duplicates, links must point to the canonical product.

**Questions:**
- Should we validate that ALL links point to valid canonical products after consolidation?
- Should we audit which links changed during consolidation?
- Is it acceptable if some links become invalid (unlikely but possible)?

### Decision 4: Scope of Fix

**Problem:** 367k duplicates dating back 10 years.

**Option A: Full Historical Cleanup**
- Fix all 367k duplicates from 2016-2026
- Scope: Everything
- **Timeline:** 4-6 hours
- **Risk:** Highest (largest data modification)
- **Benefit:** Complete cleanup

**Option B: Recent-Only Cleanup (Last 2 Years)**
- Fix duplicates created in last 2 years (~50k duplicates)
- Keep ancient data unchanged
- **Timeline:** 1-2 hours
- **Risk:** Lower (smaller scope)
- **Benefit:** Fixes recent business impact

**Option C: Per-Supplier Cleanup**
- Start with highest-impact suppliers (most inventory)
- Expand gradually
- **Timeline:** Iterative (1 hour per supplier batch)
- **Risk:** Lower (can stop at any point)
- **Benefit:** Phased approach, can validate

---

## Questions for You

To proceed with a fix, I need answers to:

1. **Estoque Merge Strategy:** A (Sum), B (Keep Largest), or C (Keep Oldest)?
2. **Canonical Selection:** A (Oldest), B (Most Referenced), or C (Newest)?
3. **Linking Validation:** Should we audit idProdutoRelacionado links?
4. **Scope:** A (Full), B (Recent 2 Years), or C (Phased by Supplier)?

Once you provide these decisions, I can:
- Generate detailed consolidation script
- Show exact SQL changes before executing
- Run in transaction mode (can rollback if needed)
- Provide detailed audit trail of all changes
