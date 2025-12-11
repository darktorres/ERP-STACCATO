# Orcamento Product Totals Calculation Bug Analysis

## Summary

A logic bug in `orcamento.cpp` causes incorrect product total calculations when the user changes the discount percentage on items that have a `multiplo` (multiple) value defined. This results in validation failures when saving budgets.

## Root Cause

The `on_doubleSpinBoxDesconto_valueChanged()` function uses `doubleSpinBoxQuantCx->value()` to calculate quantity, while all other handlers correctly use `doubleSpinBoxQuant->singleStep()`.

When a product has a `multiplo` value, these two values differ:
- `doubleSpinBoxQuantCx->value()` = `quantCaixa` (quantity per box)
- `doubleSpinBoxQuant->singleStep()` = `multiplo` (the multiple, which overrides the step)

## Affected Code

**File:** `src/orcamento.cpp`

**Buggy function:** `on_doubleSpinBoxDesconto_valueChanged()` (lines 1499-1517)

```cpp
void Orcamento::on_doubleSpinBoxDesconto_valueChanged(const double desconto) {
  const double caixas = ui->doubleSpinBoxCaixas->value();
  const double caixas2 = not qFuzzyIsNull(fmod(caixas, ui->doubleSpinBoxCaixas->singleStep())) ? ceil(caixas) : caixas;
  const double quant = caixas2 * ui->doubleSpinBoxQuantCx->value();  // BUG: should use singleStep()

  unsetConnections();

  try {
    const double prcUn = ui->doubleSpinBoxPrecoUn->value();
    const double itemBruto = quant * prcUn;

    ui->doubleSpinBoxTotalItem->setValue(itemBruto * (1. - (desconto / 100)));
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}
```

## Comparison with Correct Implementations

**`on_doubleSpinBoxQuant_valueChanged()` (lines 764-780) - CORRECT:**
```cpp
const double stepQt = ui->doubleSpinBoxQuant->singleStep();  // Correct
// ...
const double itemBruto2 = quant2 * prcUn;
ui->doubleSpinBoxTotalItem->setValue(itemBruto2 * (1. - desc));
```

**`on_doubleSpinBoxCaixas_valueChanged()` (lines 971-988) - CORRECT:**
```cpp
const double stepQt = ui->doubleSpinBoxQuant->singleStep();  // Correct
// ...
const double quant2 = caixas2 * stepQt;
const double itemBruto2 = quant2 * prcUn;
ui->doubleSpinBoxTotalItem->setValue(itemBruto2 * (1. - desc));
```

## How the Bug Manifests

### Setup in `setarParametrosProduto()` (lines 1079-1102):

```cpp
ui->doubleSpinBoxQuantCx->setValue(quantCaixa);           // e.g., 2
ui->doubleSpinBoxQuant->setSingleStep(quantCaixa);        // e.g., 2

if (not qFuzzyIsNull(multiplo)) {
  ui->doubleSpinBoxCaixas->setSingleStep(multiplo / quantCaixa);  // e.g., 3
  ui->doubleSpinBoxQuant->setSingleStep(multiplo);                // e.g., 6 (overrides!)
}
```

### Example Scenario:

Product configuration:
- `quantCaixa` = 2 (units per box)
- `multiplo` = 6 (must buy in multiples of 6)
- `precoVenda` = R$ 100.00

User actions:
1. Adds 1 box of the product
2. Actual quantity = 1 box × 6 (singleStep) = 6 units
3. User changes discount to 10%

What happens:
- `on_doubleSpinBoxDesconto_valueChanged()` calculates: `quant = 1 × 2 = 2` (WRONG)
- Sets `doubleSpinBoxTotalItem` to: `2 × 100 × 0.9 = R$ 180.00` (WRONG)
- Correct value should be: `6 × 100 × 0.9 = R$ 540.00`

When saving:
- `parcialDesc` stored = R$ 180.00 (from buggy calculation)
- `calcPrecoGlobalTotal()` calculates: `6 × 100 × 0.9 = R$ 540.00` (correct)
- Difference = R$ 360.00 (way more than 0.1 tolerance)
- `verificarTotais()` fails, `corrigirValores()` cannot fix it
- Save blocked with "Erro nos valores!" error

## Fix

**Line 1502** should be changed from:
```cpp
const double quant = caixas2 * ui->doubleSpinBoxQuantCx->value();
```

To:
```cpp
const double quant = caixas2 * ui->doubleSpinBoxQuant->singleStep();
```

## Manual Reproduction Steps

### Prerequisites
1. Find or create a product in the database with a `multiplo` value set:
   ```sql
   -- Find products with multiplo
   SELECT idProduto, produto, quantCaixa, multiplo, precoVenda
   FROM produto
   WHERE multiplo > 0 AND multiplo != quantCaixa;

   -- Or set multiplo on an existing product for testing
   UPDATE produto SET multiplo = 6, quantCaixa = 2 WHERE idProduto = <id>;
   ```

### Steps to Reproduce
1. Open the application and create a new Orçamento (budget)
2. Fill in required fields (Cliente, Vendedor, Profissional, Endereço)
3. Select a product that has `multiplo` set (e.g., `quantCaixa=2`, `multiplo=6`)
4. Set quantity to 1 box (actual quantity will be 6 units due to multiplo)
5. **Change the discount percentage** (e.g., set to 10%)
6. Observe the "Total" field for the item - it will show the wrong value
7. Click "Adicionar" to add the item
8. Try to save the budget

### Expected vs Actual

With `quantCaixa=2`, `multiplo=6`, `precoVenda=100`, 1 box, 10% discount:

| Field | Expected | Actual (Bug) |
|-------|----------|--------------|
| Quantity | 6 units | 6 units (correct in spinbox) |
| Item Total | R$ 540.00 | R$ 180.00 |
| Validation | Pass | Fail - "Erro nos valores!" |

### Why It Happens
- When you change quantity/boxes: total is calculated correctly using `singleStep` (=6)
- When you change discount: total is calculated using `quantCx` (=2) instead
- The wrong total gets stored, but validation uses the correct calculation
- Mismatch > 0.1 triggers the error

### Workaround
After changing discount, change the quantity (up then back down) to force recalculation with the correct formula.

## Impact

- Only affects products with a non-zero `multiplo` value in the database
- Bug triggers when user modifies discount percentage after setting quantity
- Results in budgets that cannot be saved
- The error message "Erro nos valores! Entre em contato com o suporte!" is shown
