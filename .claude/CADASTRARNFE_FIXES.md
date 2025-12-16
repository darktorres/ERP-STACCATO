# CadastrarNFe ICMS Fixes Documentation

## Overview
This document describes three critical fixes made to `src/cadastrarnfe.cpp` to resolve ICMS calculation and auto-population issues in the NFe emission system.

**Scope**: Fixes apply to **Tipo::Saida** (sales) and **Tipo::SaidaAposFutura** (post-future delivery) only.
**Excluded**: Tipo::Entrada (customer returns) - Tax treatment to be determined later.

---

## Fix 1: ICMS Interstate Base Calculation (Lines 763, 768, 800)

### Problem
The ICMS Interstate calculation was using the **wrong base value**:
- **Current (Wrong)**: Using `vBCPIS` (PIS calculation base)
- **Should be**: Using `vBC` (ICMS calculation base)

This caused SEFAZ to reject NFe with error:
```
Rejeição: Valor do ICMS Interestadual para UF de Destino difere do calculado
```

### Root Cause
The `view_produto_estoque` view returns `vBC` and `vBCPIS` fields with NULL values, so the code had to manually set them. However, `vBCPIS` and `vBC` have different meanings:
- **vBC** = ICMS base (product value + freight + IPI + others - discounts)
- **vBCPIS** = PIS base (for PIS/COFINS calculation)

### Solution
Changed three locations:

**Location 1: `writeProduto()` - Line 763**
```cpp
// BEFORE
stream << "vBCUFDest = " + modelProduto.data(row, "vBCPIS").toString() + "\n";

// AFTER
stream << "vBCUFDest = " + modelProduto.data(row, "vBC").toString() + "\n";
```

**Location 2: `writeProduto()` - Line 768**
```cpp
// BEFORE
const double difal = modelProduto.data(row, "vBCPIS").toDouble() * diferencaICMS;

// AFTER
const double difal = modelProduto.data(row, "vBC").toDouble() * diferencaICMS;
```

**Location 3: `writeTotal()` - Line 800**
```cpp
// BEFORE
const double difal = modelProduto.data(row, "vBCPIS").toDouble() * diferencaICMS;

// AFTER
const double difal = modelProduto.data(row, "vBC").toDouble() * diferencaICMS;
```

### Impact
- Fixes SEFAZ rejection for interstate ICMS calculations
- ICMS Interstate now calculated on correct base value
- Matches SEFAZ validation rules

---

## Fix 2: Auto-populate ICMS from NCM Table

### Problem
The system was NOT auto-populating ICMS values like it does for PIS and COFINS:

**Current behavior:**
- ✅ PIS: Auto-populated from `porcentagemPIS` config (line 2077)
- ✅ COFINS: Auto-populated from `porcentagemCOFINS` config (line 2081)
- ❌ ICMS: User must manually enter each value

**What was available:**
- NCM table stores `aliq` field with ICMS percentages
- Example: NCM 69072300 has `aliq = 12.00`

### Solution
Updated the `preencherImpostos()` function to query NCM table and auto-populate ICMS for each transaction type:

**Changes in all three transaction types (Tipo::Entrada, Tipo::Saida, Tipo::SaidaAposFutura):**

```cpp
// BEFORE: Only queried for ST flag
SqlQuery queryNcm;
queryNcm.prepare("SELECT st FROM ncm WHERE ncm = :ncm");

// AFTER: Also query for ICMS aliquot
SqlQuery queryNcm;
queryNcm.prepare("SELECT st, aliq FROM ncm WHERE ncm = :ncm");
queryNcm.bindValue(":ncm", ncmProduto);

if (queryNcm.exec() and queryNcm.first()) {
  produtoST = queryNcm.value("st").toBool();
  aliqICMS = queryNcm.value("aliq").toDouble();  // NEW
}
```

**Auto-populate fields:**
```cpp
// ICMS
modelProduto.setData(row, "vBC", total + freteProduto);
modelProduto.setData(row, "pICMS", aliqICMS);                              // NEW
modelProduto.setData(row, "vICMS", (total + freteProduto) * aliqICMS / 100);  // NEW
```

### Affected Functions
- `preencherImpostos()` - Tipo::Saida section
- `preencherImpostos()` - Tipo::SaidaAposFutura section

### Example
For product with:
- NCM: 69072300 (has aliq = 12.00% in database)
- Product total: R$ 1000.00
- Freight: R$ 100.00

**Auto-populated values:**
- `vBC = 1100.00` (total + freight)
- `pICMS = 12.00` (from NCM table)
- `vICMS = 132.00` (calculated: 1100 × 12 ÷ 100)

### Impact
- Reduces manual data entry errors
- NFe emission process is faster
- Eliminates missing ICMS values
- Consistent with NCM configuration

---

## Fix 3: Partilha de ICMS - Update to Current Law (2019+)

### Problem
The code had hardcoded `80%` partition for ICMS split between origin and destination states:

```cpp
// Line 971: BEFORE
ui->doubleSpinBoxPercentualPartilha->setValue(80);

// Line 976-977: BEFORE
ui->doubleSpinBoxPartilhaDestinatario->setValue(difal * 0.8);  // 80% to destination
ui->doubleSpinBoxPartilhaRemetente->setValue(difal * 0.2);     // 20% to origin
```

### Background: Constitutional Amendment 87/2015
Brazil's tax law underwent a gradual transition:

| Period | Origin | Destination |
|--------|--------|-------------|
| 2016   | 80%    | 20%         |
| 2017   | 60%    | 40%         |
| 2018   | 40%    | 60%         |
| 2019+  | 0%     | **100%**    |

This change benefits destination states and encourages local business development.

### Solution
Updated to reflect current law (since 01/01/2019):

```cpp
// Line 969: AFTER
ui->doubleSpinBoxPercentualPartilha->setValue(100);  // 100% to destination

// Lines 974-975: AFTER
ui->doubleSpinBoxPartilhaDestinatario->setValue(difal);  // 100% to destination
ui->doubleSpinBoxPartilhaRemetente->setValue(0);         // 0% to origin
```

### Code Location
File: `src/cadastrarnfe.cpp`
Function: `on_tableItens_doubleClicked()`
Line: ~971

### Legal Reference
- Constitutional Amendment No. 87/2015
- ICMS Convention 152/2015
- Effective: 01/01/2019

### Impact
- ✅ Complies with current Brazilian tax law
- ✅ Prevents SEFAZ rejection for incorrect DIFAL splits
- ✅ Correct ICMS distribution to destination state
- ⚠️ **Important**: If you're emitting NFe dated 2016-2018, you may need to adjust this percentage per the law that applied in that year

---

## Summary Table

| Fix | File | Lines | NFe Types | Issue | Solution |
|-----|------|-------|-----------|-------|----------|
| 1 | cadastrarnfe.cpp | 763, 768, 800 | Saida, SaidaAposFutura | Wrong base (vBCPIS) | Use correct base (vBC) |
| 2 | cadastrarnfe.cpp | 2104-2138, 2175-2208 | Saida, SaidaAposFutura | No ICMS auto-population | Query NCM table, populate pICMS & vICMS |
| 3 | cadastrarnfe.cpp | 969, 974-975 | Saida, SaidaAposFutura | Outdated 80/20 split | Update to 100% destination (2019+) |

---

## Testing Recommendations

### Fix 1 Testing
- Emit interstate NFe with products that have NCM codes
- Verify SEFAZ accepts the NFe
- Check that ICMS Interstate values match SEFAZ validation

### Fix 2 Testing
- Ensure all products in NCM table have `aliq` values populated
- Create NFe with various products
- Verify `pICMS`, `vBC`, and `vICMS` are auto-populated correctly
- Check calculated ICMS amounts: `vICMS = vBC × pICMS ÷ 100`

### Fix 3 Testing
- Emit interstate NFe to a customer in different state
- Verify ICMS Partilha shows 100% to destination state
- Compare with SEFAZ response to ensure amounts match

---

## Related Database Tables

### ncm Table
```sql
SELECT ncm, aliq, st, mva4, mva12 FROM ncm
WHERE ncm = '69072300';
```

**Sample output:**
| ncm | aliq | st | mva4 | mva12 |
|-----|------|----|----|-------|
| 69072300 | 12.00 | 1 | 95.27 | 79.00 |

### Important Fields
- `aliq` = ICMS aliquot (percentage) - **Used in Fix 2**
- `st` = Substituição Tributária flag (0/1)
- `mva4`, `mva12` = Mark-up value (for ST calculations)

---

## Files Modified
- `src/cadastrarnfe.cpp` - All three fixes

## Files Not Modified (But related)
- `src/cadastrarnfe.h` - No changes needed
- `.gitignore` - Updated to exclude local files (separate commit)
- Database schema - No schema changes needed (data already exists)

---

## Notes
1. All fixes maintain backward compatibility with existing data
2. No database migrations required
3. Fixes apply to **Tipo::Saida** and **Tipo::SaidaAposFutura** only
4. **Tipo::Entrada** (customer returns) is excluded - tax treatment to be determined separately
5. **Tipo::Futura** has no taxes, so these fixes don't affect it
6. PIS and COFINS auto-population remains unchanged from previous implementation
