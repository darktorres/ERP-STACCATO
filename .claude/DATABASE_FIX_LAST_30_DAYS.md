# Database Fix Plan: Recent Duplicate Normal Products (Last 30 Days)

**Status:** FINAL CORRECTED
**Date:** 2026-01-09
**Cutoff:** 2025-12-10 (30 days ago)
**Scope:** Delete only recent duplicates created in last 30 days

---

## Executive Summary

**Delete:** 21,624 duplicate normal products created in last 30 days
**Keep:** All older products (canonical records from before last 30 days)
**Keep:** All special products (estoque, promotions, linked products)

These recent duplicates are copies of products that already existed before the last 30 days. The old canonical products will remain in the database.

---

## The Problem

**Recent Import (Last 30 Days):**
- 21,654 new normal products were created
- 21,624 of these (99.86%) are duplicates of existing older products
- Only 30 are actually new unique products

**Example:**
- Product ID 500208 created 2021-08-04 (canonical) ✓
- Product ID 685658 created 2026-01-02 (duplicate) ✗
- Product ID 687006 created 2026-01-02 (duplicate) ✗
- Product ID 690977 created 2026-01-02 (duplicate) ✗
- All 4 IDs have same: Furnecedor=2, Code=6050811A, UI=0

**Action:** DELETE the 3 recent (IDs 685658, 687006, 690977), KEEP the old one (ID 500208)

---

## Statistics

| Item | Count |
|------|-------|
| **Normal products created in last 30 days** | 21,654 |
| **Duplicates of older products** | 21,624 |
| **Unique new products** | 30 |
| **Duplicate product groups** | ~7,200 |

---

## Examples of Duplicates to Delete

| Furnecedor | Code | Canonical | Duplicates to Delete | All Created |
|-----------|------|-----------|---------------------|------------|
| 2 | 6050811A | 500208 (2021-08-04) | 685658, 687006, 690977 (2026-01-02) | 2021 → 2026 |
| 2 | 6050813A | 500204 (2021-08-04) | 685686, 687034, 691005 (2026-01-02) | 2021 → 2026 |
| 2 | 6050814A | 500212 (2021-08-04) | 685945, 687294, 691265 (2026-01-02) | 2021 → 2026 |
| 2 | 6050815A | 500205 (2021-08-04) | 685466, 686814, 690785 (2026-01-02) | 2021 → 2026 |
| 2 | 6050819A | 500211 (2021-08-04) | 685860, 687208, 691179 (2026-01-02) | 2021 → 2026 |

---

## Fix Strategy

### Step 1: Create Mapping (Recent Duplicates → Canonical)
For each product created in last 30 days that is a duplicate of an older product, map it to the canonical older product.

### Step 2: Update ALL References in Dependent Tables

**References to Update:**
| Table | Count | Action |
|-------|-------|--------|
| orcamento_has_produto | 88 | Update idProduto |
| pedido_fornecedor_has_produto | 2 | Update idProduto |
| venda_has_produto | 4 | Update idProduto |
| venda_has_produto2 | 4 | Update idProduto |
| pedido_fornecedor_has_produto2 | 2 | Update idProduto |
| produto_has_preco | 47,804 | Update idProduto |
| **Total** | **47,904** | **All must be updated before deletion** |

**SQL Updates Required:**
```sql
-- Update orcamento_has_produto
UPDATE orcamento_has_produto ohp
JOIN produto p30 ON ohp.idProduto = p30.idProduto
JOIN produto p_old ON
  p30.idFornecedor = p_old.idFornecedor
  AND p30.codComercial = p_old.codComercial
  AND IFNULL(p30.ui, 'NULL') = IFNULL(p_old.ui, 'NULL')
SET ohp.idProduto = (
  SELECT MIN(p_old2.idProduto)
  FROM produto p_old2
  WHERE p_old2.idFornecedor = p30.idFornecedor
    AND p_old2.codComercial = p30.codComercial
    AND IFNULL(p_old2.ui, 'NULL') = IFNULL(p30.ui, 'NULL')
    AND p_old2.created < DATE_SUB(NOW(), INTERVAL 30 DAY)
    AND p_old2.tipo IS NULL
    AND p_old2.idProdutoRelacionado IS NULL
)
WHERE p30.created >= DATE_SUB(NOW(), INTERVAL 30 DAY)
  AND p30.tipo IS NULL
  AND p30.idProdutoRelacionado IS NULL;

-- Update pedido_fornecedor_has_produto
UPDATE pedido_fornecedor_has_produto pfhp
JOIN produto p30 ON pfhp.idProduto = p30.idProduto
SET pfhp.idProduto = (
  SELECT MIN(p_old.idProduto)
  FROM produto p_old
  WHERE p_old.idFornecedor = p30.idFornecedor
    AND p_old.codComercial = p30.codComercial
    AND IFNULL(p_old.ui, 'NULL') = IFNULL(p30.ui, 'NULL')
    AND p_old.created < DATE_SUB(NOW(), INTERVAL 30 DAY)
    AND p_old.tipo IS NULL
    AND p_old.idProdutoRelacionado IS NULL
)
WHERE p30.created >= DATE_SUB(NOW(), INTERVAL 30 DAY)
  AND p30.tipo IS NULL
  AND p30.idProdutoRelacionado IS NULL;

-- Update venda_has_produto
UPDATE venda_has_produto vhp
JOIN produto p30 ON vhp.idProduto = p30.idProduto
SET vhp.idProduto = (
  SELECT MIN(p_old.idProduto)
  FROM produto p_old
  WHERE p_old.idFornecedor = p30.idFornecedor
    AND p_old.codComercial = p30.codComercial
    AND IFNULL(p_old.ui, 'NULL') = IFNULL(p30.ui, 'NULL')
    AND p_old.created < DATE_SUB(NOW(), INTERVAL 30 DAY)
    AND p_old.tipo IS NULL
    AND p_old.idProdutoRelacionado IS NULL
)
WHERE p30.created >= DATE_SUB(NOW(), INTERVAL 30 DAY)
  AND p30.tipo IS NULL
  AND p30.idProdutoRelacionado IS NULL;

-- Update venda_has_produto2
UPDATE venda_has_produto2 vhp2
JOIN produto p30 ON vhp2.idProduto = p30.idProduto
SET vhp2.idProduto = (
  SELECT MIN(p_old.idProduto)
  FROM produto p_old
  WHERE p_old.idFornecedor = p30.idFornecedor
    AND p_old.codComercial = p30.codComercial
    AND IFNULL(p_old.ui, 'NULL') = IFNULL(p30.ui, 'NULL')
    AND p_old.created < DATE_SUB(NOW(), INTERVAL 30 DAY)
    AND p_old.tipo IS NULL
    AND p_old.idProdutoRelacionado IS NULL
)
WHERE p30.created >= DATE_SUB(NOW(), INTERVAL 30 DAY)
  AND p30.tipo IS NULL
  AND p30.idProdutoRelacionado IS NULL;

-- Update pedido_fornecedor_has_produto2
UPDATE pedido_fornecedor_has_produto2 pfhp2
JOIN produto p30 ON pfhp2.idProduto = p30.idProduto
SET pfhp2.idProduto = (
  SELECT MIN(p_old.idProduto)
  FROM produto p_old
  WHERE p_old.idFornecedor = p30.idFornecedor
    AND p_old.codComercial = p30.codComercial
    AND IFNULL(p_old.ui, 'NULL') = IFNULL(p30.ui, 'NULL')
    AND p_old.created < DATE_SUB(NOW(), INTERVAL 30 DAY)
    AND p_old.tipo IS NULL
    AND p_old.idProdutoRelacionado IS NULL
)
WHERE p30.created >= DATE_SUB(NOW(), INTERVAL 30 DAY)
  AND p30.tipo IS NULL
  AND p30.idProdutoRelacionado IS NULL;

-- Update produto_has_preco
UPDATE produto_has_preco php
JOIN produto p30 ON php.idProduto = p30.idProduto
SET php.idProduto = (
  SELECT MIN(p_old.idProduto)
  FROM produto p_old
  WHERE p_old.idFornecedor = p30.idFornecedor
    AND p_old.codComercial = p30.codComercial
    AND IFNULL(p_old.ui, 'NULL') = IFNULL(p30.ui, 'NULL')
    AND p_old.created < DATE_SUB(NOW(), INTERVAL 30 DAY)
    AND p_old.tipo IS NULL
    AND p_old.idProdutoRelacionado IS NULL
)
WHERE p30.created >= DATE_SUB(NOW(), INTERVAL 30 DAY)
  AND p30.tipo IS NULL
  AND p30.idProdutoRelacionado IS NULL;
```

### Step 3: Delete Recent Duplicate Products
Delete all 21,624 products from the last 30 days that are duplicates.

### Step 4: Verify
Confirm no duplicates remain and no orphaned references.

---

## Impact

### What Gets Deleted
- **21,624 recent duplicate normal products**
- Created in last 30 days (2025-12-10 to 2026-01-09)
- Are duplicates of older products

### What Stays Untouched
- **All 30 truly new products** created in last 30 days
- **All older canonical products** (from before 2025-12-10)
- **All special products** (112,819 estoque/promotion/linked variants)
- **All old data** - nothing before 2025-12-10 is affected

### Risk Assessment
- **Risk Level:** VERY LOW
- **Scope:** Only last 30 days of data
- **Data Loss:** None (just removing unnecessary duplicates)
- **Rollback:** Simple if transaction wraps entire operation

---

## Complete Execution Plan

1. **Update 47,904 references** across 6 dependent tables
   - orcamento_has_produto: 88
   - pedido_fornecedor_has_produto: 2
   - venda_has_produto: 4
   - venda_has_produto2: 4
   - pedido_fornecedor_has_produto2: 2
   - produto_has_preco: 47,804

2. **Delete 21,624 duplicate products** created in last 30 days

3. **Verify** no duplicates or orphans remain

---

## Ready?

Should I execute this to:
- Update 47,904 references in dependent tables
- Remove 21,624 duplicate products from the last 30 days?
