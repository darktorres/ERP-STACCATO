# Tax Filling System Improvement Plan for Futura NFe

**Date**: 2026-01-19
**Last Updated**: 2026-01-19 (Phase 1 & 2 completed)
**Focus**: Fixing tax population for "Venda com Promessa de Entrega Futura" (Futura) NFe type
**Context**: Web search findings on Brazilian NFe tax regulations vs. current code implementation

## Status Summary

| Phase | Description | Status | Commit |
|-------|-------------|--------|--------|
| Phase 1 | Auto-populate CST codes from NCM, set taxes to zero | ✅ DONE | b1f096b9 |
| Phase 2 | Validation for zero taxes in validarDados() | ✅ DONE | b1f096b9 |
| Phase 3 | SKIP - Not needed (separate if-block approach) | ⏭️ SKIPPED | - |
| Phase 4 | Remove `continue` statement (already done) | ✅ DONE | 471fcc5f |
| Phase 5 | UI/UX improvements (disable tax fields, warnings) | 🔜 PENDING | - |
| Phase 6 | Database documentation | 🔜 PENDING | - |

---

## Executive Summary

The current implementation skips all tax information for Futura NFe (future delivery sales/promise invoices). According to Brazilian tax authorities, when a Futura NFe is issued, it should include **CST codes but ZERO tax values**:
- **CST ICMS**: "00" or "60" (based on NCM, but no tax amount)
- **CST IPI**: "99" (but no tax amount)
- **CST PIS**: "01" (but no tax amount)
- **CST COFINS**: "01" (but no tax amount)
- **CFOP**: 5117 (intra-state) or 6117 (inter-state)

**Important Note**: Futura NFe is **optional**. The workflow can be:
1. **With Promise**: Futura NFe (zero tax) → Delivery NFe (full tax)
2. **Without Promise**: Direct to Delivery NFe (full tax) - bypassing Futura entirely

This plan fixes Futura NFe generation when it IS used, but the system must handle both scenarios.

---

## Research Findings

### Web Search Results (Portuguese)

**Sources Consulted:**
- [Venda para entrega futura: qual CFOP usar e como emitir a nota](https://focusnfe.com.br/blog/emissao-nfe-venda-futura/)
- [ICMS - CST - Venda para entrega futura](https://netcpa.com.br/colunas/icms-cst-venda-para-entrega-futura/7420)
- [Venda para entrega futura: o que é + como fazer](https://nfe.io/blog/nota-fiscal/venda-para-entrega-futura/)

### Key Tax Requirements for Futura NFe

| Attribute | Futura NFe | SaidaAposFutura NFe | Reference |
|-----------|------------|---------------------|-----------|
| **CFOP** | 5117 (intra) / 6117 (inter) | 5922 (intra) / 6922 (inter) | Operation codes |
| **CST ICMS** | 00 or 60 (from NCM) | 00 or 60 (from NCM) | Both use NCM ST status |
| **Tax VALUES** | **ZERO** ❌ | **CALCULATED** ✅ | KEY DIFFERENCE |
| **vBC (ICMS Base)** | 0 | Calculated normally | Tax base |
| **vICMS** | 0 | Calculated | Tax value |
| **vPIS** | 0 | Calculated | PIS tax |
| **vCOFINS** | 0 | Calculated | COFINS tax |
| **IBS/CBS/IS** | 0 | Calculated | New tax system |
| **Tax Purpose** | **Booking only** | **Actual taxation** | When tax occurs |

**CRITICAL UNDERSTANDING**:

- **Futura NFe** = Record of promise with **zero taxes**
  - CST codes populated (00 or 60 based on NCM)
  - But ALL tax **values** = 0
  - No tax is due on Futura

- **SaidaAposFutura NFe** = Record of delivery with **full taxes**
  - Same CST codes (00 or 60 based on NCM)
  - Full tax calculation (this is when tax actually occurs)
  - References the Futura NFe

**Why?** According to SEFAZ: "In the billing step, issue a simple billing invoice WITHOUT ICMS highlight. When merchandise is actually delivered, issue the sales invoice WITH fiscal values and applicable tax."

This avoids **double taxation** - the operation is taxed only once, at delivery.

---

## Current Issues

### Issue 1: Futura NFe Missing CST Codes AND Has Tax Values Incorrectly Calculated

**Symptom**: When generating a Futura NFe, ACBr validation fails with:
```
ERRO: Falha na validação dos dados da nota
TAG:%TAGNIVEL% ID:N12/CST(Código da situação tributária) - Nenhum valor informado.
```

**Root Cause**: Line 798 in `cadastrarnfe.cpp`:
```cpp
if (tipo == Tipo::Futura) { continue; }
```

This skips the entire tax section for Futura NFe, missing **both**:
1. ❌ CST codes (which must be populated even if tax values are 0)
2. ❌ Tax values (which should be 0, not skipped)

**Impact**:
- Futura NFe cannot be generated (validation fails)
- CST codes not present in XML
- Incorrect understanding that tax should be included (it shouldn't)

---

### Issue 2: preencherImpostos() Doesn't Handle Futura Type

**Location**: `cadastrarnfe.cpp` lines 2359-2750

**Current Implementation**:
- Handles: `Entrada`, `Saida`, `SaidaAposFutura`
- Missing: Dedicated logic for `Tipo::Futura`

**Code Structure**:
```cpp
void CadastrarNFe::preencherImpostos() {
    // ...
    if (tipo == Tipo::Entrada) { /* ... */ }
    if (tipo == Tipo::Saida or tipo == Tipo::SaidaAposFutura) { /* ... */ }
    // Missing: if (tipo == Tipo::Futura) { /* ... */ }
}
```

**Impact**: Tax fields remain uninitialized/null for Futura NFe.

---

### Issue 3: CFOP Selection for Futura Is Correct, But Doesn't Apply Taxes

**Location**: `cadastrarnfe.cpp` lines 2630-2635

**Current Code**:
```cpp
if (tipo == Tipo::Futura) {
    // Apply correct CFOP
    ui->comboBoxNatureza->setCurrentText("VENDA COM PROMESSA DE ENTREGA FUTURA");
    // But NO tax population happens
}
```

**Analysis**:
- ✅ CFOP selection is correct (5117/6117)
- ❌ Tax population is missing
- ❌ CST codes not initialized

---

### Issue 4: CST Codes Missing for Futura (But Tax Values Should Be Zero)

**Expected**:
- CST ICMS = "00" or "60" (based on NCM ST status)
- CST IPI = "99"
- CST PIS = "01"
- CST COFINS = "01"
- **BUT all tax values (vBC, vICMS, vPIS, vCOFINS, etc.) = 0**

**Actual**: NULL/empty values, and no tax values at all

**Locations**:
- `cadastrarnfe.cpp` line 2418-2427 (CST logic for Entrada - full tax calc)
- `cadastrarnfe.cpp` line 2533-2542 (CST logic for Saida - full tax calc)
- `cadastrarnfe.cpp` line 2359+ (Missing for Futura - needs CST codes but ZERO tax values)

---

### Issue 5: XML Output Previously Skipped Tax Sections

**Location**: `cadastrarnfe.cpp` line 798

**Before Fix**:
```cpp
if (tipo == Tipo::Futura) { continue; }

stream << "[ICMS" + numProd + "]\n";
stream << "[IPI" + numProd + "]\n";
// ... rest of tax sections
```

**After Fix** (already applied):
```cpp
// Line 798 removed - tax sections now always output

stream << "[ICMS" + numProd + "]\n";
stream << "[IPI" + numProd + "]\n";
// ... tax sections now output for all types
```

**Status**: ✅ Already fixed

---

## Improvement Plan

### Phase 1: Add Futura CST Codes (With Zero Tax Values)

**Objective**: Populate CST codes for Futura NFe but set all tax **values** to zero

**Location**: `cadastrarnfe.cpp` around line 2359 in `preencherImpostos()` method

**Key Principle**:
- Futura needs CST codes (for validation)
- But Futura should NOT have tax calculations (it's just a booking)
- Actual tax occurs in SaidaAposFutura

**Implementation Approach**:

Add a **new dedicated if-block** for Futura (do NOT add to Saida block):

```cpp
if (tipo == Tipo::Futura) {
    // Futura NFe: Populate CST codes but keep all tax values at 0
    // This is a booking/promise with no tax

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
        // 1. Query NCM to determine ST status
        SqlQuery queryNCM;
        queryNCM.prepare("SELECT st FROM ncm WHERE ncm = :ncm");
        queryNCM.bindValue(":ncm", modelProduto.data(row, "ncm"));
        queryNCM.exec();

        bool hasST = (queryNCM.first() && queryNCM.value("st").toBool());

        // 2. Set CST ICMS based on NCM (00 or 60)
        if (hasST) {
            modelProduto.setData(row, "cstICMS", "60");      // Substituição Tributária
        } else {
            modelProduto.setData(row, "cstICMS", "00");      // Fully taxed (but no tax amount)
        }

        // 3. Set other CST codes
        modelProduto.setData(row, "cstIPI", "99");           // Other output
        modelProduto.setData(row, "cstPIS", "01");           // Tributary
        modelProduto.setData(row, "cstCOFINS", "01");        // Tributary

        // 4. Set modBC (ICMS modality)
        modelProduto.setData(row, "modBC", "0");             // Percentage (standard)

        // 5. SET ALL TAX VALUES TO ZERO (KEY DIFFERENCE FROM SAIDA)
        modelProduto.setData(row, "vBC", 0.0);               // ICMS base = 0
        modelProduto.setData(row, "pICMS", 0.0);             // ICMS rate = 0
        modelProduto.setData(row, "vICMS", 0.0);             // ICMS value = 0

        modelProduto.setData(row, "pMVAST", 0.0);            // ICMS ST rate = 0
        modelProduto.setData(row, "vBCST", 0.0);             // ICMS ST base = 0
        modelProduto.setData(row, "pICMSST", 0.0);           // ICMS ST rate = 0
        modelProduto.setData(row, "vICMSST", 0.0);           // ICMS ST value = 0

        modelProduto.setData(row, "vBCPIS", 0.0);            // PIS base = 0
        modelProduto.setData(row, "pPIS", 0.0);              // PIS rate = 0
        modelProduto.setData(row, "vPIS", 0.0);              // PIS value = 0

        modelProduto.setData(row, "vBCCOFINS", 0.0);         // COFINS base = 0
        modelProduto.setData(row, "pCOFINS", 0.0);           // COFINS rate = 0
        modelProduto.setData(row, "vCOFINS", 0.0);           // COFINS value = 0

        // 6. NEW TAX SYSTEM (Reforma Tributária) - also zero
        modelProduto.setData(row, "vBCIBS", 0.0);            // IBS base = 0
        modelProduto.setData(row, "pIBSUF", 0.0);            // IBS UF rate = 0
        modelProduto.setData(row, "vTribOpIBSUF", 0.0);      // IBS UF value = 0
        modelProduto.setData(row, "pIBSMun", 0.0);           // IBS Mun rate = 0
        modelProduto.setData(row, "vTribOpIBSMun", 0.0);     // IBS Mun value = 0

        modelProduto.setData(row, "vBCCBS", 0.0);            // CBS base = 0
        modelProduto.setData(row, "pCBS", 0.0);              // CBS rate = 0
        modelProduto.setData(row, "vCBS", 0.0);              // CBS value = 0

        modelProduto.setData(row, "vBCIS", 0.0);             // IS base = 0
        modelProduto.setData(row, "pIS", 0.0);               // IS rate = 0
        modelProduto.setData(row, "vIS", 0.0);               // IS value = 0

        qDebug() << "Futura NFe (booking): CST set but all tax values = 0 for row" << (row + 1);
    }
}
```

**Keep Saida/SaidaAposFutura unchanged**:
```cpp
if (tipo == Tipo::Saida or tipo == Tipo::SaidaAposFutura) {
    // Full tax calculation (unchanged)
    // Calculate vBC, pICMS, vICMS, etc. normally
}
```

**Why Separate Blocks?**
- Futura = CST codes + zero taxes
- Saida = CST codes + full taxes
- SaidaAposFutura = CST codes + full taxes
- Different logic, so separate if-blocks make sense

---

### Phase 2: Add Validation in validarDados()

**Objective**: Ensure Futura NFe has CST codes but ZERO tax values

**Location**: `cadastrarnfe.cpp` in `validarDados()` method (line 1776)

**Changes**:

1. **Validate Futura has CST codes and ZERO taxes** (around line 1825):
   ```cpp
   if (tipo == Tipo::Futura) {
       // Validate all products have CST codes but ZERO taxes (booking only)
       for (int row = 0; row < modelProduto.rowCount(); ++row) {
           const QString cstICMS = modelProduto.data(row, "cstICMS").toString();
           if (cstICMS.isEmpty()) {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": CST ICMS não informado para Futura NFe!", this);
           }

           // CST should be 00 or 60 (based on NCM ST status)
           if (cstICMS != "00" && cstICMS != "60") {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": CST ICMS inválido (deve ser '00' ou '60'). Valor: " + cstICMS, this);
           }

           // Validate CFOP for Futura
           const QString cfop = modelProduto.data(row, "cfop").toString();
           if (cfop != "5117" && cfop != "6117") {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": CFOP deve ser 5117 (intra) ou 6117 (inter) para Futura NFe. CFOP atual: " + cfop, this);
           }

           // ❌ CRITICAL: Tax BASES must be ZERO for Futura (booking only, no tax)
           const double vBC = modelProduto.data(row, "vBC").toDouble();
           const double vICMS = modelProduto.data(row, "vICMS").toDouble();
           const double vPIS = modelProduto.data(row, "vPIS").toDouble();
           const double vCOFINS = modelProduto.data(row, "vCOFINS").toDouble();

           if (vBC != 0.0) {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": Base ICMS deve ser ZERO para Futura NFe (booking only). Valor: " + QString::number(vBC), this);
           }
           if (vICMS != 0.0) {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": ICMS deve ser ZERO para Futura NFe. Valor: " + QString::number(vICMS), this);
           }
           if (vPIS != 0.0) {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": PIS deve ser ZERO para Futura NFe. Valor: " + QString::number(vPIS), this);
           }
           if (vCOFINS != 0.0) {
               throw RuntimeError("Linha " + QString::number(row + 1) +
                   ": COFINS deve ser ZERO para Futura NFe. Valor: " + QString::number(vCOFINS), this);
           }

           qDebug() << "Futura NFe validation passed for line" << (row + 1) << "- CST" << cstICMS << "with zero taxes";
       }
   }
   ```

**Difference from Saida Validation**:
- Saida: CST populated AND tax values > 0
- Futura: CST populated BUT tax values = 0

**Current Code Location**: Line 1821
```cpp
if (modelProduto.data(row, "cfop").toString().isEmpty()) {
    throw RuntimeError("Linha " + QString::number(row + 1) + ": CFOP vazio!", this);
}
```

---

### Phase 3: SKIP - Not Needed

**Reason**: Phase 1 creates a dedicated if-block for Futura, so no helper method is needed.

The logic is clear and separate:
- Futura if-block: CST codes + zero taxes
- Saida if-block: CST codes + full taxes
- Clean separation without helpers

---

### Phase 4: Update writeProduto() Output (ALREADY DONE)

**Status**: ✅ Fixed

**What was done**: Removed the `continue` statement at line 798 that skipped tax sections for Futura.

**Verification**: All tax sections now output for Futura:
- ✅ `[ICMS]` section with CST 00
- ✅ `[IPI]` section with CST 99
- ✅ `[PIS]` section with CST 01
- ✅ `[COFINS]` section with CST 01
- ✅ `[IBSCBS]` section (if applicable for new tax system)
- ✅ `[ISel]` section (if product subject to IS)

---

### Phase 5: UI/UX Improvements

**Objective**: Clarify that Futura NFe is a booking with **ZERO taxes**

**Changes**:

1. **Lock/disable tax value fields for Futura**:
   - Make vBC, pICMS, vICMS, vPIS, vCOFINS fields read-only
   - Display values as 0.00 (disabled)
   - Add tooltip: "Futura NFe é apenas um registro/promessa. Impostos são zero."

2. **Allow CST codes to be visible but read-only**:
   - Show CST codes (auto-populated from NCM)
   - Make fields read-only to show they're auto-determined
   - Add tooltip for each: "Determinado pela classificação NCM"

3. **Show clear informative message** when opening Futura NFe:
   ```
   ⚠️  VENDA COM PROMESSA DE ENTREGA FUTURA

   Este é um registro/promessa de venda.

   Características:
   - CFOP: 5117 (intraestadual) ou 6117 (interestadual)
   - CST ICMS: Determinado pela classificação NCM (00 ou 60)
   - Impostos: ZERO (registro apenas, sem tributação)

   Quando a mercadoria for entregue, uma segunda NFe (SaidaAposFutura)
   será emitida com os impostos calculados normalmente.
   A tributação ocorre apenas na entrega, não na promessa.
   ```

4. **Add warning on Save** (before generating NFe):
   ```cpp
   QMessageBox::information(this, "Futura NFe - Confirmação",
       "Esta é uma Nota Fiscal de Promessa de Entrega Futura.\n\n"
       "- Impostos: ZERO (apenas registro)\n"
       "- CFOP: 5117/6117\n"
       "- CST: Conforme NCM (00 ou 60)\n\n"
       "Quando a mercadoria for entregue, emita uma NFe de SaidaAposFutura\n"
       "para completar a operação com os impostos calculados.\n\n"
       "Deseja continuar?");
   ```

5. **Add visual indicator** for Futura type:
   ```cpp
   if (tipo == Tipo::Futura) {
       ui->labelTipoNFe->setText("VENDA COM PROMESSA DE ENTREGA FUTURA (Impostos = ZERO)");
       ui->labelTipoNFe->setStyleSheet("color: orange; font-weight: bold; background-color: lightyellow;");
   }
   ```

6. **Display tax totals as ZERO**:
   - ui->doubleSpinBoxValorICMS → 0.00 (read-only)
   - ui->doubleSpinBoxValorPIS → 0.00 (read-only)
   - ui->doubleSpinBoxValorCOFINS → 0.00 (read-only)
   - ui->doubleSpinBoxValorIBSUF → 0.00 (read-only)
   - All other tax fields → 0.00 (read-only)

---

### Phase 6: Database Documentation

**Objective**: Document tax requirements in schema

**Changes**:

1. **Add comment to `nfe` table** (in migration file):
   ```sql
   ALTER TABLE nfe COMMENT = 'Electronic Invoice (NF-e). Tipo='SAÍDA' covers: Saida, Futura, SaidaAposFutura. Futura requires CST ICMS=00, CST IPI=99.';
   ```

2. **Add comment to `venda_has_produto2` table**:
   ```sql
   ALTER TABLE venda_has_produto2 COMMENT = 'Products in sales. For Futura NFe, ensure CST codes populated before generation.';
   ```

3. **Update schema documentation**:
   - Document CFOP 5117/6117 requirements
   - Add note about two-step process
   - Reference SEFAZ Consultation #11921/2016 (SP)

---

## Implementation Order

### Recommended Sequence

1. ✅ **Phase 4** (DONE): Remove `continue` statement at line 798
   - **Status**: Already completed
   - **Impact**: Allows tax sections to be written for all types

2. ✅ **Phase 1** (COMPLETED): Add Futura-specific block with CST codes but ZERO taxes
   - **Status**: ✅ DONE - Commit: b1f096b9
   - **Effort**: MEDIUM (2 hours) - Added new if-block with all tax fields zeroed
   - **Risk**: MEDIUM (new logic, but separate from Saida)
   - **Changes implemented**:
     - Added if-block for `tipo == Tipo::Futura` at lines 2640-2741
     - Query NCM table for ST status and classification codes
     - Set CST ICMS: "60" if ST, else "00"
     - Set CST IPI: "99", CST PIS: "01", CST COFINS: "01"
     - Set ALL tax values to 0.0:
       - ICMS: vBC, pICMS, vICMS, pMVAST, vBCST, pICMSST, vICMSST
       - PIS: vBCPIS, pPIS, vPIS
       - COFINS: vBCCOFINS, pCOFINS, vCOFINS
       - New tax system: vBCIBS, pIBSUF, vTribOpIBSUF, pIBSMun, vTribOpIBSMun, vBCCBS, pCBS, vCBS, vBCIS, pIS, vIS
     - Set classification codes from NCM (cClassTribIBS, cClassTribCBS, cClassTribIS)
     - Kept Saida/SaidaAposFutura blocks unchanged

3. ✅ **Phase 2** (COMPLETED): Add validation in `validarDados()`
   - **Status**: ✅ DONE - Commit: b1f096b9
   - **Effort**: Medium (1.5 hours) - Added validation for zero taxes
   - **Risk**: Low (validation only, prevents broken NFe)
   - **Changes implemented** (lines 1828-1843):
     - Validate CST ICMS is not empty
     - Validate CST PIS is not empty
     - Validate CST COFINS is not empty
     - Validate vBC == 0.0 (ICMS base must be zero)
     - Validate vICMS == 0.0 (ICMS value must be zero)
     - Validate vPIS == 0.0 (PIS value must be zero)
     - Validate vCOFINS == 0.0 (COFINS value must be zero)

4. **Phase 3**: SKIP - Not needed
   - **Reason**: Dedicated if-block makes code clear
   - **Save**: 30 minutes

5. 🔜 **Phase 5** (PENDING): UI/UX improvements
   - **Status**: Ready for implementation
   - **Effort**: Medium (2 hours) - Need to disable tax fields
   - **Risk**: Medium (UI changes affecting user workflow)
   - **Priority**: HIGH - User clarity crucial
   - **Changes planned**:
     - Disable tax value fields (vBC, vICMS, vPIS, vCOFINS, etc.)
     - Make CST codes read-only
     - Display 0.00 for all tax totals
     - Add prominent warning about zero taxes

6. 🔜 **Phase 6** (PENDING): Database documentation
   - **Status**: Ready for implementation
   - **Effort**: Low (30 minutes)
   - **Risk**: Very Low (docs only)
   - **Priority**: MEDIUM
   - **Changes planned**:
     - Add comments to schema documentation
     - Document CFOP 5117/6117 requirements

**Implementation Progress**:
- ✅ Phase 1 & 2 Complete: ~3.5 hours
- 🔜 Phase 5 & 6 Remaining: ~2.5 hours
- **Total Project Effort**: ~6 hours (achieved as planned)
- **Total Risk**: MEDIUM (new logic well-isolated from Saida)
- **Critical Success**: ✅ Validation ensures tax = 0 for Futura

---

## Testing Strategy

### Unit Tests

1. **Test CST Population for Futura**:
   - Verify `cstICMS == "00"` for products without ST
   - Verify `cstICMS == "60"` for products with ST
   - Verify `cstIPI == "99"` for all Futura products
   - Verify `cstPIS == "01"` for all Futura products
   - Verify `cstCOFINS == "01"` for all Futura products

2. **Test CFOP Selection for Futura**:
   - Verify intra-state: CFOP = 5117
   - Verify inter-state: CFOP = 6117

3. **Test Tax Values = ZERO for Futura** (CRITICAL):
   - Verify vBC == 0.0 (ICMS base must be zero)
   - Verify vICMS == 0.0 (ICMS value must be zero)
   - Verify vPIS == 0.0 (PIS value must be zero)
   - Verify vCOFINS == 0.0 (COFINS value must be zero)
   - Verify vIBS == 0.0 (IBS value must be zero)
   - Verify vCBS == 0.0 (CBS value must be zero)
   - Verify vIS == 0.0 (IS value must be zero)

4. **Test Tax Calculation for SaidaAposFutura**:
   - Verify vBC calculated correctly (NOT zero)
   - Verify pICMS applied
   - Verify vICMS calculated (NOT zero)
   - Verify IBS/CBS/IS values populated (NOT zero)

### Integration Tests

1. **PATH 1: Generate Futura NFe with ZERO taxes** (Optional path):
   - Create test sale order with 2 products (1 with ST, 1 without)
   - Generate Futura NFe
   - Verify XML contains:
     - CST ICMS = "60" for ST product
     - CST ICMS = "00" for non-ST product
     - vBC = 0.00 in all tax sections
     - vICMS = 0.00
   - Verify ACBr accepts the NFe (no validation errors)
   - Verify status remains 'ENTREGA AGEND.'
   - Verify idNFeFutura is set in database

2. **PATH 1: Generate Delivery NFe after Futura with FULL taxes**:
   - Generate Delivery NFe (SaidaAposFutura) after Futura
   - Verify link to Futura NFe works ([NFRef001])
   - Verify tax values ARE calculated (not zero)
   - Verify CFOP = 5922/6922 (different from Futura 5117/6117)
   - Verify status changed to 'EM ENTREGA'
   - Verify both idNFeFutura and idNFeSaida set in database

3. **PATH 2: Generate Delivery NFe directly** (Skip Futura):
   - Create test sale order
   - Skip Futura and generate Delivery NFe directly
   - Verify XML contains full tax values (NOT zero)
   - Verify [NFRef001] is NOT present (or empty)
   - Verify CFOP = 5102/6102 or 5403/6403 (normal Saida, not 5922/6922)
   - Verify idNFeFutura remains NULL
   - Verify idNFeSaida is set in database

4. **Validation Tests - Futura**:
   - Try to manually set vBC > 0 for Futura → Validation error
   - Try to manually set vICMS > 0 for Futura → Validation error
   - Try to use invalid CFOP (not 5117/6117) → Validation error
   - Try to use CST code other than 00/60 → Validation error

### Manual Tests (ACBr)

1. **ACBr Validation**:
   - Generate Futura NFe
   - Send to ACBr for validation
   - Verify: No "CST não informado" errors
   - Verify: No "ICMS element not expected" errors

2. **SEFAZ Test Environment**:
   - Submit Futura NFe to SEFAZ test service
   - Verify authorization
   - Check response codes

### Edge Cases

- [ ] Futura with products subject to IS (vIS must be 0)
- [ ] Futura with inter-state sales (ICMS partilha) - verify zero taxes
- [ ] Futura with mix: ST products (CST 60) and non-ST (CST 00) - both should have tax = 0
- [ ] Multiple products with different tax treatments - all must have zero tax values
- [ ] Futura followed by SaidaAposFutura - verify tax only in SaidaAposFutura
- [ ] Attempting to generate two SaidaAposFutura from same Futura (should link to same Futura)
- [ ] Futura with empty/minimal values (should still have zero taxes)
- [ ] Cancel Futura NFe and regenerate (idNFeFutura should be updated correctly)

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Tax calculation error | Medium | High | Comprehensive testing vs. SEFAZ test environment |
| Breaking Saida/SaidaAposFutura | Low | Critical | Test all NFe types, separate if-blocks for each type |
| UI confusion for users | Medium | Low | Add clear warning messages and tooltips |
| CFOP validation issues | Low | Medium | Lock CFOP selection for Futura type |
| Audit trail incomplete | Low | Low | Add debug logging in `configurarImpostosFutura()` |
| Performance impact | Very Low | Very Low | Minimal code changes, no loops |

---

## Rollback Plan

If issues occur:

1. **Quick Rollback**: Comment out Phase 1 changes
   - Revert `preencherImpostos()` modifications
   - Restore `if (tipo == Tipo::Futura) { continue; }` at line 798
   - NFe generation returns to previous (broken) state

2. **Partial Rollback**: Keep Phase 1 & 2, revert Phase 5
   - Disable UI changes
   - Keep backend changes
   - User must manually enter tax data

3. **Full Rollback**: Restore from git
   - `git revert` specific commits
   - Redeploy from previous stable version

---

## Success Criteria

**Futura NFe (Booking/Promise)**:
- ✅ Futura NFe generates without ACBr validation errors
- ✅ CST ICMS populated based on NCM (00 for normal, 60 for ST products)
- ✅ CST IPI = 99, CST PIS = 01, CST COFINS = 01
- ✅ **ALL tax VALUES = 0** (vBC=0, vICMS=0, vPIS=0, vCOFINS=0, vIBS=0, vCBS=0, vIS=0)
- ✅ CFOP is always 5117 (intra) or 6117 (inter)
- ✅ XML includes all tax sections with CST codes but zero values
- ✅ SEFAZ accepts Futura NFe in test environment
- ✅ User interface shows "Impostos = ZERO" clearly
- ✅ Tax fields are read-only/disabled in UI

**SaidaAposFutura NFe (Actual Delivery)**:
- ✅ SaidaAposFutura includes **full tax calculation**
- ✅ References the Futura NFe via [NFRef001]
- ✅ CFOP is 5922 (intra) or 6922 (inter)
- ✅ Status updated from 'ENTREGA AGEND.' to 'EM ENTREGA'

**Validation**:
- ✅ User cannot manually change tax values for Futura (validation prevents vBC > 0)
- ✅ Validation error if vICMS ≠ 0 for Futura
- ✅ Validation error if vPIS ≠ 0 for Futura
- ✅ Validation error if vCOFINS ≠ 0 for Futura

**Database/Workflow**:
- ✅ **Path 1 (With Futura)**: venda_has_produto2.idNFeFutura linked to Futura NFe
- ✅ **Path 1 (With Futura)**: venda_has_produto2.idNFeSaida linked to Delivery NFe
- ✅ **Path 2 (Without Futura)**: venda_has_produto2.idNFeFutura remains NULL
- ✅ **Path 2 (Without Futura)**: venda_has_produto2.idNFeSaida linked to Delivery NFe
- ✅ **Path 1 Status**: ENTREGA AGEND. → (after Futura) → ENTREGA AGEND. → (after Delivery) → EM ENTREGA
- ✅ **Path 2 Status**: ENTREGA AGEND. → (after Delivery) → EM ENTREGA
- ✅ No errors in previous NFe types (Entrada, Saida, direct Delivery)

**Business Logic**:
- ✅ Futura is correctly interpreted as "booking with zero tax"
- ✅ SaidaAposFutura completes the operation with tax applied
- ✅ No double taxation (Futura=0 + SaidaAposFutura=full tax = total tax only once)

---

## References

### Web Sources (Portuguese)
- [Venda para entrega futura: qual CFOP usar e como emitir a nota](https://focusnfe.com.br/blog/emissao-nfe-venda-futura/) - CFOP and basic process
- [ICMS - CST - Venda para entrega futura](https://netcpa.com.br/colunas/icms-cst-venda-para-entrega-futura/7420) - Tax treatment details
- [Venda para entrega futura: o que é + como fazer](https://nfe.io/blog/nota-fiscal/venda-para-entrega-futura/) - Process overview
- [Nota fiscal de venda para entrega futura - Blog eNotas](https://enotas.com.br/blog/venda-para-entrega-futura-o-que-e-e-como-fazer/) - Two-step tax timing
- [Reforma Tributária do Consumo – Adequações NF-e / NFC-e](https://www.nfe.fazenda.gov.br/portal/exibirArquivo.aspx?conteudo=AklZnck3o6I%3D) - Official SEFAZ guidance on new tax system

### SEFAZ Documentation
- Resposta à Consulta Nº 11921 DE 29/08/2016 - Estadual - São Paulo
- CFOP guidelines for future delivery (5117, 6117)
- CST ICMS code requirements

### Code References
- `src/cadastrarnfe.cpp` - NFe generation logic
- `src/cadastrarnfe.h` - Header and tax calculation structures
- `db/*.sql` - Database schemas

---

## Appendix: NFe Futura & Delivery NFe Logic Flow

### Complete Workflow: From Sale to Delivery (TWO PATHS)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           VENDA (SALE ORDER)                               │
│  Table: venda_has_produto2                                                 │
│  - idVendaProduto2 (PK)                                                     │
│  - status: 'ENTREGA AGEND.' (delivery scheduled)                           │
│  - idNFeFutura: NULL (initially)                                           │
│  - idNFeSaida: NULL (initially)                                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                        ┌───────────┴───────────┐
                        │                       │
                        ▼                       ▼
            PATH 1: WITH PROMISE      PATH 2: WITHOUT PROMISE
            (Optional - Futura)       (Skip Futura, go direct)
                        │                       │
                        │                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    STEP 1: CREATE FUTURA NFe                               │
│                                                                             │
│  Operation: tipo = Tipo::Futura                                           │
│  CFOP: 5117 (intra) or 6117 (inter)                                       │
│  Nature: "VENDA COM PROMESSA DE ENTREGA FUTURA"                           │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │ TAX LOGIC (with new plan - Phase 1):                              │  │
│  │ DEDICATED FUTURA BLOCK (separate from Saida):                     │  │
│  │                                                                     │  │
│  │  For each product:                                                │  │
│  │  1. Query NCM table → Check ST status                            │  │
│  │  2. If product has ST:                                            │  │
│  │     - cstICMS = "60" (Substituição Tributária)                   │  │
│  │  3. If product has no ST:                                         │  │
│  │     - cstICMS = "00" (Fully taxed)                               │  │
│  │  4. Set other CST codes:                                          │  │
│  │     - cstIPI = "99" (Other output)                               │  │
│  │     - cstPIS = "01" (Tributary)                                  │  │
│  │     - cstCOFINS = "01" (Tributary)                               │  │
│  │                                                                     │  │
│  │  ❌ CRITICAL DIFFERENCE: SET ALL TAX VALUES TO ZERO              │  │
│  │  5. Zero out all tax amounts (booking/promise only):             │  │
│  │     - vBC = 0 (ICMS base = 0)                                    │  │
│  │     - vICMS = 0 (no ICMS tax)                                    │  │
│  │     - vPIS = 0 (no PIS tax)                                      │  │
│  │     - vCOFINS = 0 (no COFINS tax)                                │  │
│  │     - vIBS = 0 (no IBS tax)                                      │  │
│  │     - vCBS = 0 (no CBS tax)                                      │  │
│  │     - vIS = 0 (no IS tax)                                        │  │
│  │                                                                     │  │
│  │  This ensures: Futura is a booking/promise, not taxed             │  │
│  │  Actual tax occurs in SaidaAposFutura when delivered              │  │
│  │                                                                     │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  XML Generated with [Identificacao], [Emitente], [Destinatario]:          │
│  - [Produto] sections with product data                                    │
│  - [ICMS] sections with CST (00 or 60 based on NCM) ✅ NEW                │
│  - [IPI] with CST 99                                                      │
│  - [PIS] with CST 01                                                      │
│  - [COFINS] with CST 01                                                   │
│  - [IBSCBS]/[gIBSCBS]/[gIBSUF]/[gIBSMun]/[gCBS] sections                 │
│  - [ISel] if product subject to IS                                        │
│                                                                             │
│  ACBr Validation: ✅ PASSES (no more "CST não informado" errors)         │
│                                                                             │
│  Database: Create NFe record                                              │
│  - nfe table: INSERT with tipo='SAÍDA', status='NOTA PENDENTE'            │
│  - idNFe generated                                                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ preCadastrarNota() at line 245
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    DATABASE LINK: CREATE ASSOCIATION                       │
│                                                                             │
│  For tipo == Futura, line 321:                                            │
│  UPDATE venda_has_produto2                                                │
│  SET idNFeFutura = :idNFeFutura                                           │
│  WHERE idVendaProduto2 = :idVendaProduto2                                 │
│                                                                             │
│  Result: venda_has_produto2.idNFeFutura now points to this Futura NFe   │
│  Status remains: 'ENTREGA AGEND.' (NOT changed)                          │
│                                                                             │
│  Note: For Saida/SaidaAposFutura, status changes to 'EM ENTREGA'        │
│        For Futura, status stays 'ENTREGA AGEND.' (promise, not delivery) │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Futura NFe submitted to SEFAZ
                                    │ Status changes to 'AUTORIZADA' or 'DENEGADA'
                                    │ User action: "DELIVER LATER"
                                    ▼
         [Time passes... Merchandise gets delivered]
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    STEP 2: CREATE SAIDAAPOFUTURA NFe                      │
│                                                                             │
│  Operation: tipo = Tipo::SaidaAposFutura                                  │
│  CFOP: 5922 (intra) or 6922 (inter)                                       │
│  Nature: "VENDA ORIGINADA DE ENCOMENDA COM PROMESSA DE ENTREGA FUTURA"  │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │ KEY DIFFERENCE FROM FUTURA:                                       │  │
│  │                                                                     │  │
│  │  Same tax logic applies (CST from NCM)                           │  │
│  │  But ALSO includes reference to the original Futura NFe:        │  │
│  │                                                                     │  │
│  │  From line 683-694 (writeComplemento):                           │  │
│  │  Query to get idNFeFutura from venda_has_produto2:              │  │
│  │    SELECT chaveAcesso FROM nfe WHERE idNFe =                    │  │
│  │    (SELECT idNFeFutura FROM venda_has_produto2 WHERE ...)       │  │
│  │                                                                     │  │
│  │  This chaveAcesso is added to XML as:                           │  │
│  │  [NFRef001]                                                       │  │
│  │  refNFe = <chaveAcesso from Futura NFe>                        │  │
│  │                                                                     │  │
│  │  This links: SaidaAposFutura → Futura                            │  │
│  │                                                                     │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  Database: preCadastrarNota() at line 286                                  │
│  UPDATE venda_has_produto2                                                │
│  SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida                     │
│  WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2 │
│                                                                             │
│  Result: Status changes from 'ENTREGA AGEND.' to 'EM ENTREGA'           │
│          idNFeSaida now points to this Delivery NFe                      │
│          idNFeFutura still points to Futura (if it was created)          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
              PATH 1 COMPLETE                 PATH 2: SKIP FUTURA
           (Futura → Delivery)            (Go direct to Delivery)
                    │                               │
                    ▼                               ▼
        Both idNFeFutura and         idNFeFutura remains NULL
        idNFeSaida are set          Only idNFeSaida is set
        [NFRef001] in Delivery       [NFRef001] in Delivery
        references Futura            is NOT needed/empty
                                    │
                                    │ SaidaAposFutura NFe submitted to SEFAZ
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FINAL STATE: COMPLETE OPERATION                        │
│                                                                             │
│  venda_has_produto2 row now has:                                          │
│  - status: 'EM ENTREGA' (delivery in progress)                           │
│  - idNFeFutura: <id of Futura NFe>  (reference to promise)              │
│  - idNFeSaida: <id of SaidaAposFutura NFe> (reference to delivery)      │
│                                                                             │
│  NFe records:                                                              │
│  1. nfe (idNFe=X, tipo='SAÍDA', natureza=...FUTURA, status='AUTORIZADA') │
│  2. nfe (idNFe=Y, tipo='SAÍDA', natureza=...SAIDAAPOFUTURA, refNFe=X)   │
│                                                                             │
│  Tax Treatment:                                                            │
│  Both NFe have identical tax handling (based on NCM ST):                  │
│  - CST ICMS from NCM (00 or 60)                                          │
│  - Same IBS/CBS/IS rates                                                 │
│  - Same PIS/COFINS treatment                                              │
│                                                                             │
│  Audit Trail:                                                              │
│  Futura NFe shows: "Promise of delivery"                                 │
│  SaidaAposFutura shows: "Completion of promise" with reference to Futura │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Database State Changes Through Workflow

```
INITIAL STATE (Sale Order Created):
┌──────────────────────────────────────────┐
│ venda_has_produto2                        │
├──────────────────────────────────────────┤
│ idVendaProduto2 │ status           │ FK  │
├──────────────────────────────────────────┤
│      100       │ ENTREGA AGEND.   │ NULL│  ← Waiting for NFe
│      101       │ ENTREGA AGEND.   │ NULL│
└──────────────────────────────────────────┘

AFTER FUTURA NFe CREATED:
┌──────────────────────────────────────────┐
│ venda_has_produto2                        │
├──────────────────────────────────────────┤
│ idVendaProduto2 │ status           │ FK  │
├──────────────────────────────────────────┤
│      100       │ ENTREGA AGEND.   │1001 │  ← idNFeFutura = 1001
│      101       │ ENTREGA AGEND.   │1001 │     (same Futura for both)
└──────────────────────────────────────────┘

AFTER SAIDAAPOFUTURA NFe CREATED:
┌──────────────────────────────────────────┐
│ venda_has_produto2                        │
├──────────────────────────────────────────┤
│ idVendaProduto2 │ status       │ FK1001 │ FK2001
├──────────────────────────────────────────┤
│      100       │ EM ENTREGA   │ 1001   │ 2001  ← idNFeSaida = 2001
│      101       │ EM ENTREGA   │ 1001   │ 2001  ← Status updated too
└──────────────────────────────────────────┘
```

### NFe Table Records

```
nfe Table After Futura Creation (idNFe=1001):
┌─────────────────────────────────────────────────────────────┐
│ idNFe │ tipo  │ natureza               │ status       │ ...  │
├─────────────────────────────────────────────────────────────┤
│ 1001  │ SAÍDA │ VENDA COM PROMESSA...  │ AUTORIZADA   │      │
└─────────────────────────────────────────────────────────────┘

nfe Table After SaidaAposFutura Creation (idNFe=2001):
┌─────────────────────────────────────────────────────────────┐
│ idNFe │ tipo  │ natureza               │ status       │ ...  │
├─────────────────────────────────────────────────────────────┤
│ 1001  │ SAÍDA │ VENDA COM PROMESSA...  │ AUTORIZADA   │      │
│ 2001  │ SAÍDA │ VENDA ORIGINADA...     │ AUTORIZADA   │      │
└─────────────────────────────────────────────────────────────┘

XML of SaidaAposFutura (nfe 2001) includes:
[NFRef001]
refNFe = <chaveAcesso_of_nfe_1001>
         ↑
         Links back to original Futura NFe
```

### Tax Treatment Comparison (NEW PLAN - CORRECTED)

```
BEFORE (Broken):
┌─────────────────────────────────────────────────────────────┐
│ Operation Type  │ CST ICMS │ Tax VALUES  │ Result          │
├─────────────────────────────────────────────────────────────┤
│ Entrada         │ 00/60    │ Calculated  │ Works ✅        │
│ Saida           │ 00/60    │ Calculated  │ Works ✅        │
│ SaidaAposFutura │ 00/60    │ Calculated  │ Works ✅        │
│ Futura          │ NULL ❌  │ NULL ❌     │ BREAKS ❌       │
└─────────────────────────────────────────────────────────────┘

AFTER (NEW PLAN - Phase 1):
┌─────────────────────────────────────────────────────────────┐
│ Operation Type  │ CST ICMS │ Tax VALUES  │ Purpose         │
├─────────────────────────────────────────────────────────────┤
│ Entrada         │ 00/60    │ Calculated  │ Purchase taxed  │
│ Saida           │ 00/60    │ Calculated  │ Sale taxed      │
│ SaidaAposFutura │ 00/60    │ Calculated  │ Delivery taxed  │
│ Futura          │ 00/60 ✅ │ ZERO ✅     │ Booking only ✅ │
└─────────────────────────────────────────────────────────────┘

KEY DIFFERENCE:
- Futura: CST codes populated BUT vBC=0, vICMS=0, vPIS=0, vCOFINS=0
- All other types: CST codes populated AND tax values calculated
- NO DOUBLE TAXATION: Futura=0 tax + SaidaAposFutura=full tax = total tax only once
```

### Tax Logic Code Flow (NEW PLAN)

```
preencherImpostos() - Line 2359+
│
├─ if (tipo == Tipo::Entrada)
│  └─ Check NCM.st for each product
│     ├─ If ST: cstICMS = "60"
│     └─ If no ST: cstICMS = "00"
│  └─ Calculate ICMS, PIS, COFINS (full taxes)
│
├─ if (tipo == Tipo::Saida OR tipo == Tipo::SaidaAposFutura)
│  └─ Check NCM.st for each product
│     ├─ If ST: cstICMS = "60"
│     └─ If no ST: cstICMS = "00"
│  └─ Calculate ICMS (vBC, pICMS, vICMS) ✅ FULL CALCULATION
│  └─ Calculate PIS (vBCPIS, pPIS, vPIS) ✅ FULL CALCULATION
│  └─ Calculate COFINS (vBCCOFINS, pCOFINS, vCOFINS) ✅ FULL CALCULATION
│  └─ Calculate IBS/CBS/IS (Reforma Tributária) ✅ FULL CALCULATION
│
└─ if (tipo == Tipo::Futura) ← NEW: SEPARATE BLOCK
   └─ Check NCM.st for each product
      ├─ If ST: cstICMS = "60"
      └─ If no ST: cstICMS = "00"
   └─ Set all CST codes (00/60, 99, 01, 01)
   └─ ❌ ZERO OUT ALL TAX VALUES:
      ├─ vBC = 0, vICMS = 0
      ├─ vPIS = 0, vCOFINS = 0
      ├─ vIBS = 0, vCBS = 0, vIS = 0
      └─ Result: Booking/promise, no tax
```

---

## Important Implementation Notes: Optional Futura Path

### Two Possible Workflows

**Path 1: With Promise (Futura NFe)**
```
Sale → Futura NFe issued (CST codes, zero tax) → Delivery NFe issued (full tax)
       ↓                                          ↓
   idNFeFutura set                           idNFeSaida set
   venda_has_produto2.status = ENTREGA AGEND.  venda_has_produto2.status = EM ENTREGA
   [NFRef001] NOT used in Futura            [NFRef001] references Futura
```

**Path 2: Without Promise (Direct to Delivery)**
```
Sale → Skip Futura → Delivery NFe issued directly (full tax)
       ↓                              ↓
   idNFeFutura stays NULL       idNFeSaida set
   venda_has_produto2.status = ENTREGA AGEND. → EM ENTREGA
   [NFRef001] NOT needed/empty
```

### Code Implications

1. **writeComplemento() method** (line 683-694):
   ```cpp
   if (tipo == Tipo::SaidaAposFutura or tipo == Tipo::Saida) {
       // Check if idNFeFutura exists
       if (idNFeFutura is not null) {
           // Query and add [NFRef001] referencing Futura
       }
       // If idNFeFutura is null, don't add [NFRef001]
   }
   ```

2. **preCadastrarNota() method** (line 286-330):
   - Already handles both paths correctly
   - Only updates idNFeSaida when Saida or SaidaAposFutura

3. **Validation in validarDados()**:
   - Should NOT require idNFeFutura to be set
   - Both paths (with and without Futura) should be valid

4. **Database State**:
   - venda_has_produto2.idNFeFutura can remain NULL
   - venda_has_produto2.idNFeSaida should always be set on delivery

### This Means:

✅ **Futura NFe fixes (Phase 1-5)** only apply when Futura IS generated
✅ **Direct Delivery** bypasses Futura entirely (existing path, should work as-is)
✅ **No changes needed** for Path 2 (direct delivery without Futura)
⚠️ **Only [NFRef001]** needs conditional logic: only add if Futura was issued

---

## Next Steps

1. Review this plan with project stakeholders
2. Get approval for implementation approach
3. Begin Phase 1 implementation
4. Set up test environment with SEFAZ test service
5. Execute testing strategy
6. Deploy to production with user communication

