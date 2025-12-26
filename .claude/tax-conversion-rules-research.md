# Reforma Tributária 2025 - Tax Conversion Rules Research

## Summary of Findings

Research conducted for the 2025 Brazilian Tax Reform (Reforma Tributária) implementation in the Staccato ERP system.

---

## Official Tax Rates

### IBS (Imposto sobre Bens e Serviços)
- **Reference Rate**: 17.7%
- **Nature**: Replaces ICMS (state tax) + ISS (municipal tax)
- **Structure**: Divided between state and municipal portions
  - Each state defines its own rate
  - Each municipality defines its own rate
  - Rates are summed together for total IBS
- **Example** (São Paulo): 12% (state) + 8% (municipal) = 20% total
- **Transition Period (2026-2028)**: 0.1% (0.05% state + 0.05% municipal)
- **Default Implementation**: Using São Paulo rates (12% + 5.7% = 17.7%) as reference

### CBS (Contribuição sobre Bens e Serviços)
- **Rate**: 8.8%
- **Nature**: Replaces combined PIS + COFINS (federal contributions)
- **Type**: Non-cumulative system (applies only to value added)
- **Implementation**: Fixed 8.8% for all jurisdictions

### IS (Imposto Seletivo - Selective Tax)
- **Nature**: Targets harmful products and environmental impacts
- **Rates vary significantly by product**:
  - **Cigarettes**: 250%
  - **Alcoholic beverages**: 46-62% (scales with alcohol content)
  - **Sweet beverages**: Initially proposed at 2%, but rejected by Chamber of Deputies (Dec 16, 2025) - NOT APPROVED
  - **Vehicles**: ~26.5% (varies by efficiency, emissions, recyclability)
  - **Mineral goods**: Max 2.5%
  - **Other products**: As low as 0.25%
- **Implementation Note**: Must be manually specified per product as rates are product-specific

---

## Implementation Details

### Tax Conversion Logic (preencherImpostos)

For incoming stock (Tipo::Entrada) with old tax data:

1. **Detection**: Checks if product has IBS/CBS data already set
   ```
   if (!temIBS && !temCBS) // Only convert if not already new system
   ```

2. **Calculation Basis**: Same as old system
   - Base = total product value + proportional freight allocation

3. **IBS Calculation** (17.7% default):
   - State portion: 12% (configurável per state)
   - Municipal portion: 5.7% (17.7% - state portion)
   - Amounts calculated separately for federal/state reporting

4. **CBS Calculation** (8.8% fixed):
   - Single rate for all products
   - Applied to same base as IBS

5. **IS Calculation**:
   - NOT automatically populated
   - User must manually specify per product
   - Required for products in selective tax categories

### Configuration Notes

- IBS split (state/municipal) can be configured per user/state in future enhancements
- Currently defaults to São Paulo reference rates (12%+8% = 20%, but code uses 12%+5.7% = 17.7%)
- CBS rate is fixed at 8.8% system-wide
- IS rates must be maintained in product classification tables (imposto_classificacao)

---

## Transition Timeline

- **2026-2028**: CBS and IBS tested with minimal rates (0.1% IBS, actual CBS not specified)
- **2027**: CBS comes into effect as actual collection begins
- **2026-2033**: Gradual transition period where old system (ICMS/PIS/COFINS) and new system coexist
- **2033+**: Full transition to new system

---

## Sources

- [Reforma Tributária: guia completo sobre IVA, IBS, CBS e IS - Tax Group](https://www.taxgroup.com.br/intelligence/reforma-tributaria-guia-completo-sobre-iva-ibs-cbs-e-is/)
- [Como ficam as alíquotas estaduais e municipais com o IBS?](https://clicknotas.com.br/aliquotas-estaduais-municipais-ibs/)
- [Imposto Seletivo na Reforma Tributária - Tax Group](https://www.taxgroup.com.br/intelligence/imposto-seletivo-na-reforma-tributaria-saiba-os-principais-pontos/)
- [Reforma Tributária 2025: LC 214/2025, IBS, CBS e Imposto Seletivo](https://veiga.law/reforma-tributaria-2025-lc-214-2025-ibs-cbs-imposto-seletivo/)
- [Proposta aprovada detalha percentuais da arrecadação do Imposto sobre Bens e Serviços](https://www.camara.leg.br/noticias/1233904-proposta-aprovada-detalha-percentuais-da-arrecadacao-do-imposto-sobre-bens-e-servicos)

---

## Implementation Status

✅ Tax rate research completed
✅ preencherImpostos() method updated with correct rates
✅ IBS/CBS calculations implemented
⏳ IS manual specification interface (user configurable)
⏳ Compilation and testing pending
