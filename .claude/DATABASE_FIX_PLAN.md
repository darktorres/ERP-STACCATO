# Database Fix Plan: Product Import Duplicates

## Executive Summary

Product imports that were incorrectly executed as INSERTs instead of UPDATEs have created duplicate products in the database. Existing products were inserted with new `idProduto` values rather than updating existing records. Since `idProduto` is referenced by 9 tables via foreign keys, we need a careful consolidation strategy to fix this without breaking referential integrity.

---

## Problem Analysis

### Root Cause
The original code used `INSERT INTO ... ON DUPLICATE KEY UPDATE` which only works on PRIMARY KEY (`idProduto`). However, products are identified by a **composite business key**:
- `idFornecedor` (Supplier)
- `codComercial` (Commercial Code)
- `ui` (UI field)
- `tipo` (Type: Estoque, Promocao, etc.)

Since `ON DUPLICATE KEY UPDATE` doesn't recognize this business key, new products were inserted with auto-generated `idProduto` instead of updating existing ones.

### Impact
- Database now contains duplicate product records
- Multiple `idProduto` values exist for the same logical product
- All dependent tables have references to both old and new duplicate IDs
- Data integrity is compromised

---

## Database Schema: idProduto Dependencies

### Tables with Foreign Key References to `produto(idProduto)`

| Table | Primary Key | Constraint Name | Notes |
|-------|------------|-----------------|-------|
| `estoque_has_consumo` | (idEstoque, idVendaProduto2) | `fk_EstoqueConsumo_Produto` | Inventory consumption tracking |
| `orcamento_has_produto` | idOrcamentoProduto | `fk_orcamentoproduto_produto` | Quote/budget items |
| `pedido_fornecedor_has_produto` | idPedido1 | `fk_PedidoForn_Produto` | Purchase order items (v1) |
| `venda_has_produto` | idVendaProduto1 | `fk_vendaproduto_produto` | Sales items (v1) |
| `venda_has_produto2` | idVendaProduto2 | `fk_vendaproduto2_produto` | Sales items (v2) |
| `pedido_fornecedor_has_produto2` | idPedido2 | `fk_PedidoForn2_Produto` | Purchase orders (v2) |
| `produto_has_preco` | idPreco | `fk_Produto_has_Preco_Produto1` | Price history (ON DELETE CASCADE) |
| `veiculo_has_produto` | (idVeiculo, idProduto) | `veiculo_idProduto` | Vehicle product assignments |
| `estoque` | idEstoque | Index only, no FK constraint | Inventory records |

### Special Self-Reference
- `produto.idProdutoRelacionado` - Links promotion products to base products

### Views Affected
Multiple views depend on `idProduto`:
- `view_produto_estoque`
- `view_orcamento_peso`
- `DEXCO_PRODUCTSTOCK`
- `EDU_view_EstoqueCD`
- `EDU_view_EstoquePortinari`
- `EDU_view_Vendas`
- `XL_ProdutosMes`
- `view_agendar_coleta`
- `view_estoque_contabil`
- `view_estoque_disponivel`
- `view_estoque_zerado`
- `view_financeiro`
- `view_galpao`
- `viewlucroreal`

---

## Fix Strategy

### Phase 1: Identify and Classify Duplicates

**Objective:** Determine which products are duplicates and which is the canonical record.

**Query Approach:**
```sql
-- Find products with same business key
SELECT
  idFornecedor,
  codComercial,
  ui,
  tipo,
  GROUP_CONCAT(idProduto ORDER BY idProduto) as duplicate_ids,
  COUNT(*) as count,
  MIN(created) as oldest_created,
  MAX(created) as newest_created
FROM produto
GROUP BY idFornecedor, codComercial, ui, tipo
HAVING COUNT(*) > 1
ORDER BY count DESC;
```

**Classification Logic:**
- **Canonical Product:** The one created FIRST (oldest `created` timestamp) or the one with most references in dependent tables
- **Duplicate Products:** All others created after the canonical one in the same import batch

**Metadata to Track:**
- Map of duplicate_idProduto → canonical_idProduto
- Count of references in each dependent table for each duplicate

---

### Phase 2: Consolidate References

**Objective:** Update all foreign key references from duplicates to canonical products.

For each dependent table, perform updates in this order (respecting foreign key dependencies):

#### Step 2.1: `estoque`
```sql
UPDATE estoque
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.2: `estoque_has_consumo`
```sql
UPDATE estoque_has_consumo
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.3: `orcamento_has_produto`
```sql
UPDATE orcamento_has_produto
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.4: `pedido_fornecedor_has_produto`
```sql
UPDATE pedido_fornecedor_has_produto
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.5: `venda_has_produto`
```sql
UPDATE venda_has_produto
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.6: `pedido_fornecedor_has_produto2`
```sql
UPDATE pedido_fornecedor_has_produto2
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.7: `venda_has_produto2`
```sql
UPDATE venda_has_produto2
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.8: `veiculo_has_produto`
```sql
UPDATE veiculo_has_produto
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

#### Step 2.9: `produto_has_preco`
```sql
UPDATE produto_has_preco
SET idProduto = ?canonical_id
WHERE idProduto = ?duplicate_id;
```

**Potential Conflict:** If both canonical and duplicate products have price records, we may need to:
- Keep prices from both (different date ranges)
- Keep prices from canonical only
- Merge price records intelligently

---

### Phase 3: Handle Promotion Linking

**Objective:** Fix `idProdutoRelacionado` references for promotion products.

```sql
-- Find promotion products pointing to duplicates
SELECT
  p1.idProduto,
  p1.descricao,
  p1.idProdutoRelacionado as old_related_id,
  ?canonical_id as new_related_id
FROM produto p1
WHERE p1.idProdutoRelacionado = ?duplicate_id;

-- Update to canonical
UPDATE produto
SET idProdutoRelacionado = ?canonical_id
WHERE idProdutoRelacionado = ?duplicate_id;
```

---

### Phase 4: Delete Duplicates

**Objective:** Remove incorrect product records after all references are consolidated.

**Pre-deletion Checks:**
1. Verify no remaining foreign key references to duplicate products
2. Verify all dependent tables have been updated
3. Create backup/export of duplicate product data (for audit trail)

**Deletion Steps:**
```sql
-- Delete products in specific order
DELETE FROM produto
WHERE idProduto = ?duplicate_id;
```

**Notes:**
- `produto_has_preco` has `ON DELETE CASCADE`, so prices will auto-delete
- Verify no orphaned references remain after deletion
- Check that all foreign key constraints are satisfied

---

### Phase 5: Verification & Integrity Checks

**Objective:** Ensure data integrity after consolidation.

#### Check 5.1: No More Duplicates
```sql
SELECT
  idFornecedor,
  codComercial,
  ui,
  COUNT(*) as count
FROM produto
GROUP BY idFornecedor, codComercial, ui
HAVING COUNT(*) > 1;
-- Should return 0 rows
```

#### Check 5.2: No Orphaned References
```sql
-- Check each dependent table
SELECT 'estoque' as table_name, COUNT(*) as orphaned_count
FROM estoque
WHERE idProduto NOT IN (SELECT idProduto FROM produto)
UNION ALL
SELECT 'estoque_has_consumo', COUNT(*)
FROM estoque_has_consumo
WHERE idProduto NOT IN (SELECT idProduto FROM produto)
-- ... repeat for all dependent tables
```

#### Check 5.3: Foreign Key Constraint Check
```sql
-- Enable foreign key checks
SET FOREIGN_KEY_CHECKS=1;
-- Run consistency check
ANALYZE TABLE produto, estoque, estoque_has_consumo,
  orcamento_has_produto, pedido_fornecedor_has_produto,
  venda_has_produto, pedido_fornecedor_has_produto2,
  venda_has_produto2, veiculo_has_produto, produto_has_preco;
```

#### Check 5.4: Reference Counts
```sql
-- Count references by dependent table for canonical product
SELECT
  'estoque' as table_name,
  COUNT(*) as reference_count
FROM estoque
WHERE idProduto = ?canonical_id
UNION ALL
SELECT 'estoque_has_consumo', COUNT(*)
FROM estoque_has_consumo
WHERE idProduto = ?canonical_id
-- ... repeat for all dependent tables
```

---

## Implementation Options

### Option A: Automated SQL Script
**Pros:**
- Fast execution
- Reproducible
- Can be version controlled

**Cons:**
- Requires high confidence in duplicate detection logic
- Less opportunity to review individual cases

**When to use:** If you have clear criteria for identifying duplicates (e.g., all products created in last 24 hours with same business key)

### Option B: Interactive Review & Fix
**Approach:**
1. Generate list of all duplicate groups
2. Manual review and approval of canonical vs. duplicate designation
3. Generate and execute migration scripts per duplicate group
4. Verify after each group

**Pros:**
- Maximum control and visibility
- Can handle edge cases
- Audit trail of decisions

**Cons:**
- Time-consuming for many duplicates
- Requires manual review

**When to use:** If you're unsure about which duplicate should be canonical

### Option C: Diagnostic Only First
**Approach:**
1. Run diagnostic queries to show:
   - All duplicate groups
   - Reference counts per table
   - Date ranges of created timestamps
   - Data differences between duplicates

2. Show results for review
3. Once approved, run Phase 2-5

**Pros:**
- Understand scope before committing
- Make informed decisions about canonical records
- Minimal risk

**Cons:**
- Additional step before actual fix

**When to use:** Recommended as first step regardless

---

## Recommended Approach

**Step 1:** Run diagnostic queries to understand the scope
- How many duplicates?
- When were they created?
- Which tables are most affected?

**Step 2:** Decide on canonical product selection criteria
- Oldest (original record)?
- Most referenced (most used)?
- Manual selection?

**Step 3:** Generate fix script
- Create parameterized SQL for Phase 2-4
- Include verification checks
- Add rollback capability

**Step 4:** Execute with verification
- Run in transaction with checkpoints
- Verify at each phase
- Rollback if issues detected

**Step 5:** Validate final state
- Run all Phase 5 verification queries
- Check application data display
- Verify views work correctly

---

## Risk Assessment

### High Risk Areas
- **Foreign Key Constraints:** 9 tables depend on idProduto
  - Mitigation: Update in correct order, verify after each step

- **Views and Reports:** 14+ views depend on product data
  - Mitigation: Re-run view queries after consolidation to verify

- **Data Loss:** Deleting duplicate products
  - Mitigation: Export duplicates before deletion, keep transaction rollback available

### Medium Risk Areas
- **Price History:** `produto_has_preco` may have records for both canonical and duplicate
  - Mitigation: Decide merge strategy upfront (keep all, keep canonical only, etc.)

- **Promotion Linking:** `idProdutoRelacionado` chain could be broken
  - Mitigation: Verify all promotion products point to valid canonical products

### Low Risk Areas
- **estoque:** No foreign key, can be updated safely
- **Views:** Auto-update since they query current data

---

## Rollback Plan

### Pre-Fix
1. Create full database backup
2. Export duplicate product data and all reference records
3. Save duplicate ID mappings in temporary tables

### If Issues Occur
1. Restore from backup
2. Analyze failure point
3. Adjust strategy and retry

### Proof of Correctness
Keep:
- Original duplicate detection queries and results
- Mapping of old_idProduto → canonical_idProduto
- Count of records updated in each table
- Verification query results

---

## Next Steps

Please review this plan and confirm which approach you'd like:

1. **Diagnostic First:** Run diagnostic queries to see scope and nature of duplicates
2. **Full Automated Fix:** Provide list of duplicate mappings and execute fix script
3. **Interactive Fix:** Review each duplicate group and decide canonical record per group
4. **Custom Criteria:** Specify exactly how to identify canonical vs. duplicate products

What would you prefer?
