# Manual Test Procedure - Tax Reform Implementation

## Prerequisites

1. ACBrMonitorPLUS running on localhost:3434
2. Environment set to **Homologação** (staging)
3. Application compiled successfully
4. Database migration scripts executed

---

## Test 1: UI Verification

### Steps:
1. Open the application
2. Navigate to NFe creation screen (Cadastrar NFe)
3. Select a sale (Venda) with products
4. Verify the new tax tabs are visible:
   - **IBS tab**: Should show fields for cClassTrib, vBC, pIBSUF, pIBSMun, vTribOpIBSUF, vTribOpIBSMun
   - **CBS tab**: Should show fields for cClassTrib, vBC, pCBS, vCBS
   - **IS tab**: Should show fields for cClassTrib, vBC, pIS, vIS

### Expected Result:
- Three new tabs visible in the tax section
- Fields should be editable for IBS/CBS, read-only for calculated values

---

## Test 2: Tax Auto-Population (Tipo::Entrada)

### Steps:
1. Create a "Entrada" type NFe (input/return)
2. Select products that have old tax data (ICMS/PIS/COFINS)
3. Observe the auto-population of IBS/CBS fields

### Expected Result:
For products without existing IBS/CBS data:
- **vBCIBS** = total + proportional freight
- **pIBSUF** = 12.0%
- **vTribOpIBSUF** = vBC × 12%
- **pIBSMun** = 5.7%
- **vTribOpIBSMun** = vBC × 5.7%
- **pCBS** = 8.8%
- **vCBS** = vBC × 8.8%
- **cstIBS** = "00"
- **cstCBS** = "00"

---

## Test 3: Manual Tax Calculation

### Steps:
1. In the IBS tab, change the base value (vBCIBS)
2. Verify the calculated values update automatically
3. Repeat for CBS and IS tabs

### Expected Result:
- Changing vBC should recalculate all derived values
- Changing aliquot should recalculate tax values
- Total values at bottom should update accordingly

---

## Test 4: NFe Preview (DANFE)

### Steps:
1. Fill all required NFe fields
2. Click "Prévia" button
3. ACBr should generate the DANFE preview

### Expected Result:
- No errors during XML generation
- DANFE should display correctly
- Check if new tax sections appear in XML (may be ignored by current ACBr version)

---

## Test 5: NFe Validation (without SEFAZ submission)

### Steps:
1. With NFe data filled, observe the validation process
2. Check for any alerts or errors from ACBr

### Expected Result:
- ACBr should validate the NFe structure
- May show warnings about unknown sections (IBS/CBS/IS) if ACBr version doesn't support them yet
- Core NFe should still be valid

---

## Test 6: XML Structure Verification

### Steps:
1. Generate an NFe preview
2. Check the generated XML file at `C:/ACBrMonitorPLUS/nfe.xml`
3. Search for the new tax sections

### Expected Structure:
```xml
<!-- Inside each product -->
<IBSCBS>
  <CST>00</CST>
  <cClassTrib>000000</cClassTrib>
  <vBC>100.00</vBC>
</IBSCBS>
<IBSUFReg>
  <pIBSUF>12.00</pIBSUF>
  <vTribOp>12.00</vTribOp>
</IBSUFReg>
<IBSMunReg>
  <pIBSMun>5.70</pIBSMun>
  <vTribOp>5.70</vTribOp>
</IBSMunReg>
<CBS>
  <CST>00</CST>
  <cClassTrib>000000</cClassTrib>
  <vBC>100.00</vBC>
  <pCBS>8.80</pCBS>
  <vCBS>8.80</vCBS>
  <vTribOp>8.80</vTribOp>
</CBS>
```

---

## Known Limitations

1. **ACBr Version**: Current ACBr may not fully support the new tax reform fields yet
2. **SEFAZ Validation**: The SEFAZ homologation environment may not have updated schemas
3. **IS (Imposto Seletivo)**: Must be manually configured per product - no auto-population

---

## Troubleshooting

### "Erro conectando ao emissor de NF-e ACBr"
- Verify ACBrMonitorPLUS is running
- Check port 3434 is accessible
- Confirm server configuration in UserConfig

### Validation Errors on New Tax Fields
- Expected if ACBr version predates tax reform
- Core NFe functionality should still work
- Wait for ACBr update to support new fields

### Missing Tax Values
- Ensure products have either old (ICMS/PIS/COFINS) or new (IBS/CBS) data
- Check preencherImpostos() is being called for the transaction type

---

## Files Modified

- `src/cadastrarnfe.cpp` - Main implementation
- `src/cadastrarnfe.h` - Method declarations
- `ui/cadastrarnfe.ui` - UI widgets for new taxes
- `db/2025_01_reforma_tributaria_migration.sql` - Database schema
- `db/2025_01_reforma_tributaria_view_update.sql` - View updates
