# venda.cpp — Freight Logic Analysis

## Context

Two bugs were fixed in the freight calculation path:

1. `cd12cda8` — `orcamento.cpp`: spinbox minimum was locked to the stored frete value on reload, silently clamping edits.
2. `08161723` — `venda.cpp` (`prepararVenda`): `calcularFrete` was called for gerentes before `freteManual` was read, setting `minimoGerente` as the spinbox minimum and clamping a manual zero freight.

This document records the broader analysis of all freight-related paths in `venda.cpp` done after those fixes.

---

## Findings

### 1. `viewRegister` never populates `canChangeFrete` — **real bug**

`setupMapper` binds `checkBoxFreteManual` to the `freteManual` DB column, so the checkbox renders correctly when viewing an existing Venda. But the in-memory flag `canChangeFrete` is never set from the model in `viewRegister`.

**Consequence:** if an admin opens an existing Venda where `freteManual=true` and interacts with the freight checkbox, the code hits `if (not canChangeFrete)` (line 824) and demands re-authorization — even though it was already manual.

**Fix:** add `canChangeFrete = data("freteManual").toBool();` inside the `viewRegister` lambda, analogous to what `prepararVenda` now does.

---

### 2. `on_itemBoxEndereco_idChanged` — `minimoGerente` stays 0 for NÃO HÁ/RETIRA — possibly intentional

```cpp
if (User::isGerente()) { minimoGerente = 0.; }      // line 1411 — reset
calcularFrete(true);                                  // line 1417 — sets minimoGerente only inside the non-NÃO-HÁ block
if (not representacao) {
    ui->doubleSpinBoxFrete->setMinimum(
        not qFuzzyIsNull(minimoGerente) ? minimoGerente : ui->doubleSpinBoxFrete->value());
}
```

When the address is "NÃO HÁ/RETIRA", `calcularFrete` skips the weight/Qualp block entirely, so `minimoGerente` stays 0. Line 1418 then falls to the `else` branch and sets `minimum = current_value`, locking gerentes to the base calculated freight with no flexibility to go lower. This appears intentional (pickup orders skip the gerente discount), but the logic is implicit and fragile.

---

### 3. `calcularFrete` early-returns skip `setMinimum` — low risk

```cpp
if (ui->checkBoxFreteManual->isChecked()) { return; }  // exits before setMinimum at line 1515
if (verificaServicosEspeciais()) { return; }           // exits before setMinimum at line 1515
```

If either guard triggers, the spinbox minimum is not updated and can carry over from a prior call. In practice this is mitigated:

- `on_itemBoxEndereco_idChanged` explicitly calls `setMinimum(0)` (line 1416) before calling `calcularFrete`.
- `verificaServicosEspeciais` sets its own minimum (line 1441).
- The `prepararVenda` path is now guarded by `not canChangeFrete`, so the early-return on the checkbox is no longer reachable there.

---

### 4. `User::valorMinimoFrete = -1` one-shot reset — by design

At `venda.cpp:847` (and identically at `orcamento.cpp:1333`), after using the authorizing admin's minimum freight value, the static is reset to `-1`. This is a deliberate one-shot token: the value is consumed and wiped so it cannot be reused without re-authorization. Safe because the checkbox is disabled (`setDisabled(true)`) after the authorization path, preventing a second click.

---

## Summary

| Location | Severity | Status |
|---|---|---|
| `prepararVenda` lines 345–355 | Bug | Fixed in `08161723` |
| `viewRegister` (line 589) | Bug | **Open** — `canChangeFrete` never set from DB |
| `on_itemBoxEndereco_idChanged` line 1418 | Possibly intentional | Not fixed — NÃO HÁ/RETIRA locks gerente minimum |
| `calcularFrete` early returns lines 1450–1452 | Low risk | Not fixed — mitigated by callers |
| `valorMinimoFrete = -1` line 847 | By design | No action needed |
