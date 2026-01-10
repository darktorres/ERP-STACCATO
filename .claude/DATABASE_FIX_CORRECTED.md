# Database Fix Plan: Duplicate Normal Products

**Status:** CORRECTED UNDERSTANDING
**Date:** 2026-01-09
**Scope:** Remove duplicates of NORMAL products only

---

## Executive Summary

The database contains **25,453 duplicate NORMAL product records** that need to be removed.

**Important:** Special products (estoque variants, promotions, linked products) are intentional and must be KEPT.

---

## Product Categories

### Normal Products (tipo=NULL, idProdutoRelacionado=NULL)
- **Total:** 282,425
- **Unique business keys:** 271,090
- **Duplicate records:** 11,335 groups, 25,453 total duplicates
- **These are what we DELETE**

### Special Products (DO NOT DELETE)
- **With estoque** (warehouse-specific variants): kept
- **With tipo** (REVESTIMENTO, BANHEIRA, PROMOCAO): kept
- **With idProdutoRelacionado** (linked products): kept
- **Total:** 112,819
- **These stay in database exactly as-is**

---

## Duplicate Groups Examples

**Furnecedor 334, Code 011113026501, UI 0:**
| idProduto | Created | Action |
|-----------|---------|--------|
| 477821 | 2021-04-06 14:22:22 | **KEEP (Canonical)** |
| 482241 | 2021-04-06 | DELETE |
| 562698 | 2021-04-06 | DELETE |
| 580832 | 2025-08-13 | DELETE |
| 592880 | 2025-08-13 | DELETE |
| 678549 | 2025-08-13 | DELETE |

**Furnecedor 6, Code 1622001290200, UI 0:**
| idProduto | Created | Action |
|-----------|---------|--------|
| 667807 | 2025-04-09 10:38:48 | **KEEP (Canonical)** |
| 695764 | 2025-04-09 | DELETE |
| 699509 | 2025-04-09 | DELETE |
| 703254 | 2025-06-17 | DELETE |
| 706999 | 2026-01-02 | DELETE |

---

## References to Duplicate Normal Products

These references need to be updated to point to canonical product:

| Table | Unique Duplicate IDs Referenced | Total References | Action |
|-------|--------------------------------|------------------|--------|
| `estoque` | 2,921 | 18,719 | Update idProduto to canonical |
| `orcamento_has_produto` | 5,809 | 274,899 | Update idProduto to canonical |
| `venda_has_produto2` | 3,703 | 37,390 | Update idProduto to canonical |
| **Total** | **~11,335** | **~330,008** | **Redirect to canonical** |

---

## Fix Strategy (Simple)

### Step 1: Generate Canonical Mapping
For each duplicate group of normal products:
- Select the OLDEST created product as canonical
- Map all other duplicates → canonical

**Query:**
```sql
CREATE TEMPORARY TABLE duplicate_mapping AS
SELECT
  p.idFornecedor,
  p.codComercial,
  IFNULL(p.ui, 'NULL') as ui,
  MIN(p.idProduto) as canonical_id,
  GROUP_CONCAT(p.idProduto ORDER BY p.idProduto SEPARATOR ',') as all_ids
FROM produto p
WHERE p.tipo IS NULL
  AND p.idProdutoRelacionado IS NULL
  AND CONCAT(p.idFornecedor, '-', p.codComercial, '-', IFNULL(p.ui, 'NULL')) IN (
    SELECT CONCAT(idFornecedor, '-', codComercial, '-', IFNULL(ui, 'NULL'))
    FROM produto
    WHERE tipo IS NULL AND idProdutoRelacionado IS NULL
    GROUP BY idFornecedor, codComercial, ui
    HAVING COUNT(*) > 1
  )
GROUP BY p.idFornecedor, p.codComercial, p.ui;
```

Result: 11,335 mappings of duplicates → canonical

### Step 2: Update References

For each dependent table, redirect to canonical:

```sql
-- Update estoque references
UPDATE estoque e
JOIN produto p ON e.idProduto = p.idProduto
JOIN duplicate_mapping dm ON
  p.idFornecedor = dm.idFornecedor
  AND p.codComercial = dm.codComercial
  AND IFNULL(p.ui, 'NULL') = dm.ui
SET e.idProduto = dm.canonical_id
WHERE e.idProduto != dm.canonical_id;

-- Update orcamento_has_produto references
UPDATE orcamento_has_produto ohp
JOIN produto p ON ohp.idProduto = p.idProduto
JOIN duplicate_mapping dm ON
  p.idFornecedor = dm.idFornecedor
  AND p.codComercial = dm.codComercial
  AND IFNULL(p.ui, 'NULL') = dm.ui
SET ohp.idProduto = dm.canonical_id
WHERE ohp.idProduto != dm.canonical_id;

-- Update venda_has_produto2 references
UPDATE venda_has_produto2 vhp
JOIN produto p ON vhp.idProduto = p.idProduto
JOIN duplicate_mapping dm ON
  p.idFornecedor = dm.idFornecedor
  AND p.codComercial = dm.codComercial
  AND IFNULL(p.ui, 'NULL') = dm.ui
SET vhp.idProduto = dm.canonical_id
WHERE vhp.idProduto != dm.canonical_id;
```

### Step 3: Delete Duplicate Products

```sql
-- Delete duplicates (keep only canonical)
DELETE FROM produto
WHERE tipo IS NULL
  AND idProdutoRelacionado IS NULL
  AND idProduto NOT IN (SELECT canonical_id FROM duplicate_mapping);
```

### Step 4: Verify

```sql
-- Verify no more duplicates of normal products
SELECT
  idFornecedor,
  codComercial,
  ui,
  COUNT(*) as duplicate_count
FROM produto
WHERE tipo IS NULL AND idProdutoRelacionado IS NULL
GROUP BY idFornecedor, codComercial, ui
HAVING COUNT(*) > 1;
-- Should return 0 rows
```

---

## Impact Analysis

### Products Deleted
- **25,453 duplicate normal product records** will be removed
- **271,090 unique normal products** will remain

### Products NOT Touched (Safe)
- **112,819 special products** stay exactly as-is:
  - Estoque variants
  - Promotions
  - Linked products
  - Different tipos (REVESTIMENTO, BANHEIRA, etc.)

### References Updated
- ~330,008 references across 3 tables redirected to canonical products
- No data loss (just pointer updates)

### Verification
- Check for duplicate normal products: should be 0
- Check for orphaned references: should be 0
- Views and reports: should work correctly with canonical products

---

## Timeline & Risk

| Aspect | Assessment |
|--------|-----------|
| **Complexity** | LOW - Simple deletion of duplicates |
| **Risk** | VERY LOW - Only deleting unnecessary duplicates |
| **Data Loss** | NONE - Just consolidating pointers |
| **Execution Time** | 30 minutes to 1 hour |
| **Rollback** | Possible if transaction wraps entire operation |

---

## Implementation Steps

1. **Backup database** - Full backup before starting
2. **Disable foreign key checks** temporarily
3. **Create mapping table** - Identify canonical products
4. **Update references** - Redirect to canonical (estoque, orcamento, venda)
5. **Delete duplicates** - Remove non-canonical products
6. **Verify integrity** - Check no duplicates remain
7. **Re-enable foreign key checks**
8. **Commit transaction**

---

## Ready to Execute?

This fix is straightforward because:
- Only deleting unnecessary duplicate NORMAL products
- NOT touching any special products (estoque, promotions, linked)
- NOT dealing with estoque/promocao merging (they're kept separate)
- Simple pointer redirection (no complex business logic)

**Recommendation:** Execute immediately
- Scope is clear
- Risk is minimal
- Benefit is high (clean database, accurate reports)
- Rollback is simple if needed
