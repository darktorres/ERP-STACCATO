# Tax Code Review - Staccato ERP
## Reforma Tributaria 2025 (LC 214/2025)
**Date:** 2025-12-25
**Business Type:** Construction Materials
**Total Issues Found:** 26

---

## Critical Issues (2)

### Issue #1: SQL Injection Vulnerability
**File:** `src/cadastrarnfe.cpp:577`
**Severity:** CRITICAL
**Type:** Security

```cpp
// VULNERABLE CODE:
if (not queryNfe.exec("SELECT COALESCE(MAX(numeroNFe), 0) + 1 AS numeroNFe FROM nfe WHERE cnpjOrig = '" + cnpj + "'"))
```

**Problem:** Direct string concatenation with CNPJ value instead of prepared statement with parameter binding.

**Fix:**
```cpp
queryNfe.prepare("SELECT COALESCE(MAX(numeroNFe), 0) + 1 AS numeroNFe FROM nfe WHERE cnpjOrig = :cnpj");
queryNfe.bindValue(":cnpj", cnpj);
queryNfe.exec();
```

---

### Issue #24: Missing Preferential Rate Logic for Construction Materials
**File:** Overall
**Severity:** CRITICAL
**Type:** Feature

**Problem:** LC 214/2025 Article 8 X provides 40% reduction for construction materials used in "Minha Casa Minha Vida" program, but:
- Database has classification code '200010' defined
- Code does NOT automatically apply 40% reduction when this classification is selected
- No logic to verify product NCM matches construction materials
- No validation that preferential rate is only applied to eligible materials

**Impact:** Could result in over-taxation if preferential rates aren't properly applied.

---

## High Priority Issues (6)

### Issue #3: updateComplemento() Excludes New Taxes
**File:** `src/cadastrarnfe.cpp:548-559`
**Severity:** HIGH
**Type:** Logic Error

```cpp
// CURRENT CODE - Missing IBS/CBS/IS:
const double total =
    ui->doubleSpinBoxBaseICMS->value() + ui->doubleSpinBoxValorICMS->value() +
    ui->doubleSpinBoxValorPIS->value() + ui->doubleSpinBoxValorCOFINS->value();
```

**Problem:** The "Total Aproximado de tributos" calculation only includes old taxes (ICMS, PIS, COFINS) and completely ignores new Reforma Tributaria taxes (IBS, CBS, IS).

**Fix:** Add new tax values to the total calculation.

---

### Issue #5: 2026 Test Rates Not Verified
**File:** `src/cadastrarnfe.h:55-60`
**Severity:** HIGH
**Type:** Configuration

```cpp
if (ano == 2026) {
    // 2026 Test period: fixed low rates for system testing (LC 214/2025 Art. 343)
    // Total: ~1% (IBS 0.1% + CBS 0.9%) - verify against latest official rates
    aliq.pIBSUF = 0.1;
    aliq.pIBSMun = 0.0;  // Municipalities not yet participating in 2026
    aliq.pCBS = 0.9;
}
```

**Problem:**
- Comment says "verify against latest official rates" - indicates uncertainty
- IBS Municipal rate hardcoded to 0.0 with assumption municipalities won't participate
- Rates should match official Ministry of Finance announcement (pending)
- Not configurable from database

---

### Issue #8: No Preferential Rate Calculation
**File:** Overall
**Severity:** HIGH
**Type:** Feature

**Problem:** The `2025_03_imposto_classificacao_data.sql` file includes tax classification data but:
- Only has generic classification entry for construction materials
- NO logic in code to APPLY the 40% reduction when classification is selected
- The reduction percentage (40%) is in comment but not implemented
- No automatic application based on product NCM

---

### Issue #13: No Construction Materials Tax Validation
**File:** Overall
**Severity:** HIGH
**Type:** Validation

**Problem:** Construction materials have special rules but no validation for:
- Some qualify for reduced rates under Minha Casa Minha Vida
- Some have exemptions
- No warnings when classification codes are mismatched to product type

---

### Issue #16: No SEFAZ NT 2025.002 Verification
**File:** Overall
**Severity:** HIGH
**Type:** Compliance

**Problem:** Code implements IBS/CBS/IS per LC 214/2025 but:
- No validation against official SEFAZ Nota Tecnica 2025.002
- No checking of mandatory fields for each CST type
- No validation of `obrigatorios` JSON field in `imposto_classificacao` table (never used)

---

### Issue #26: NCM Migration Incomplete for Construction Products
**File:** `db/2025_02_ncm_imposto_seletivo.sql`
**Severity:** HIGH
**Type:** Database

**Problem:** Migration only updates NCM codes for IS-subject items (tobacco, alcohol, vehicles), but does NOT update classification codes for construction material NCMs:
- 6810-6815 (Cement, concrete products)
- 6901-6915 (Ceramic products for construction)
- 7201-7307 (Iron/steel construction materials)

These NCMs should have `cClassTribIBS` and `cClassTribCBS` set appropriately.

---

## Medium Priority Issues (12)

### Issue #2: Debug File Written on Every NFe
**File:** `src/cadastrarnfe.cpp:212-219`
**Severity:** MEDIUM
**Type:** Security

```cpp
// DEBUG: Write command to file for inspection
QFile debugFile(QCoreApplication::applicationDirPath() + "/nfe_acbr_command.txt");
if (debugFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream debugStream(&debugFile);
    debugStream << nfe;
    debugFile.close();
}
```

**Problem:** Sensitive NFe commands written to disk in plain text on every generation. Security and compliance risk.

---

### Issue #4: fatorNovosTributos Never Used
**File:** `src/cadastrarnfe.h:23-24`
**Severity:** MEDIUM
**Type:** Logic

```cpp
double fatorNovosTributos;  // Percentage of new taxes to apply (0.0 to 1.0)
double fatorAntigosTributos; // Percentage of old taxes to apply (1.0 to 0.0)
```

**Problem:** These fields are calculated but never used. The transition schedule should blend old and new taxes during 2027-2032 period:
- 2027: 10% new, 90% old
- 2028: 20% new, 80% old
- ...
- 2033+: 100% new

---

### Issue #6: 2032 Transition Jump
**File:** `src/cadastrarnfe.h:44`
**Severity:** MEDIUM
**Type:** Configuration

```cpp
case 2032: fator = 0.90; break;
```

**Problem:** Jumps from 0.50 (2031) to 0.90 (2032). Needs verification against official CONFAZ schedule.

---

### Issue #7: No NCM-to-Classification Validation
**File:** `src/cadastrarnfe.cpp:1344-1372`
**Severity:** MEDIUM
**Type:** Validation

**Problem:** `validarClassTrib()` validates format but does NOT validate:
- Whether classification is appropriate for the product's NCM
- Whether construction materials are being classified under wrong codes

---

### Issue #9: creditoPresumido Field Never Used
**File:** Migration files
**Severity:** MEDIUM
**Type:** Feature

**Problem:** The `creditoPresumido` flag in `imposto_classificacao` table is never read or used in C++ code.

---

### Issue #10: Schema Inconsistency
**File:** `db/2025_01_reforma_tributaria_migration.sql`
**Severity:** MEDIUM
**Type:** Database

**Problem:** Migration adds tax fields to `estoque_has_consumo` and `estoque`, but NOT to `venda_has_produto2` where actual NFe items are stored. The view returns NULL for all new tax fields.

---

### Issue #11: NCM Defaults Mismatch
**File:** Various
**Severity:** MEDIUM
**Type:** Database

**Problem:** Database defaults ('000001') and C++ code defaults may get out of sync if either is changed independently.

---

### Issue #14: Empty CST Codes
**File:** `src/cadastrarnfe.cpp:1441`
**Severity:** MEDIUM
**Type:** Validation

```cpp
modelProduto.setData(row, "cstIS", "");  // Empty string for CST
```

**Problem:** SEFAZ requires specific CST codes. Empty CST might cause NFe rejection.

---

### Issue #15: No Error Handling for NCM Query
**File:** `src/cadastrarnfe.cpp:2358-2373`
**Severity:** MEDIUM
**Type:** Error Handling

**Problem:** If NCM query fails, error is silently ignored and defaults are used. No error message to user.

---

### Issue #18: Unaddressed TODOs
**File:** Various
**Severity:** MEDIUM
**Type:** Code Quality

Notable TODOs:
- Line 30: "emissao abertura tela de UserConfig"
- Line 136: "verificar porque essa view usa ABS()"
- Line 1651: "verificar a aliquota entre estados"
- Lines 2812-2835: 14+ TODOs including NFe devolucao, service NFe

---

### Issue #19: Hardcoded Company Name
**File:** `src/cadastrarnfe.cpp:2102`
**Severity:** MEDIUM
**Type:** Configuration

```cpp
const QString assunto = "NF-e - " + ui->lineEditNumero->text() + " - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA";
```

**Problem:** Company name should be read from configuration, not hardcoded.

---

### Issue #25: No IS Validation for Construction Materials
**File:** Overall
**Severity:** MEDIUM
**Type:** Validation

**Problem:** Construction materials are NOT subject to IS, but:
- No validation prevents accidental IS assignment
- No warnings if user assigns IS code to construction material NCM

---

## Low Priority Issues (6)

### Issue #12: Decimal Precision Inconsistency
**File:** Various
**Severity:** LOW-MEDIUM
**Type:** Database

**Problem:** Migration uses `DECIMAL(5,4)` but C++ uses `double`. Final rates are hardcoded as integers (12.0, 5.7, 8.8).

---

### Issue #17: Commented Code Block
**File:** `src/cadastrarnfe.cpp:2171-2214`
**Severity:** LOW
**Type:** Code Quality

**Problem:** Large block of commented code for "Entrada" invoice handling. Should be removed or documented.

---

### Issue #20: Incomplete Signal Blocking
**File:** `src/cadastrarnfe.cpp:2458-2584`
**Severity:** LOW-MEDIUM
**Type:** Performance

**Problem:** Signal blocking only applied for Saida type, not for Entrada, Futura, or SaidaAposFutura types. May cause expensive view recalculations.

---

### Issue #21: Repeated Aliquot Calculation
**File:** `src/cadastrarnfe.cpp` (lines 2420, 2546, 2684)
**Severity:** LOW
**Type:** Performance

**Problem:** `AliquotasReformaTributaria::calcular()` called three times with same date. Should be calculated once and reused.

---

### Issue #22: Hardcoded Final Tax Rates
**File:** `src/cadastrarnfe.h:31-33`
**Severity:** MEDIUM
**Type:** Configuration

```cpp
constexpr double IBSUF_FINAL = 12.0;
constexpr double IBSMUN_FINAL = 5.7;
constexpr double CBS_FINAL = 8.8;
```

**Problem:** Hardcoded in header, not configurable without recompilation.

---

### Issue #23: 2026 Test Rates Not Configurable
**File:** `src/cadastrarnfe.h:55-60`
**Severity:** MEDIUM
**Type:** Configuration

**Problem:** Test rates hardcoded instead of stored in database configuration.

---

## Summary Table

| # | Issue | Severity | Type | Location |
|---|-------|----------|------|----------|
| 1 | SQL Injection in preencherNumeroNFe | CRITICAL | Security | :577 |
| 24 | Missing preferential rate logic | CRITICAL | Feature | Overall |
| 3 | updateComplemento() excludes new taxes | HIGH | Logic | :548-559 |
| 5 | 2026 test rates not verified | HIGH | Config | .h:55-60 |
| 8 | No preferential rate calculation | HIGH | Feature | Overall |
| 13 | No construction materials validation | HIGH | Validation | Overall |
| 16 | No SEFAZ NT verification | HIGH | Compliance | Overall |
| 26 | NCM migration incomplete | HIGH | Database | Migration |
| 2 | Debug file writing | MEDIUM | Security | :212-219 |
| 4 | fatorNovosTributos unused | MEDIUM | Logic | .h:23-24 |
| 6 | 2032 transition jump | MEDIUM | Config | .h:44 |
| 7 | No NCM-to-classification validation | MEDIUM | Validation | :1344-1372 |
| 9 | creditoPresumido unused | MEDIUM | Feature | Migration |
| 10 | Schema inconsistency | MEDIUM | Database | Migration |
| 11 | NCM defaults mismatch | MEDIUM | Database | Various |
| 14 | Empty CST codes | MEDIUM | Validation | :1441 |
| 15 | No error handling for NCM query | MEDIUM | Error | :2358-2373 |
| 18 | Unaddressed TODOs | MEDIUM | Quality | Various |
| 19 | Hardcoded company name | MEDIUM | Config | :2102 |
| 25 | No IS validation | MEDIUM | Validation | Overall |
| 12 | Decimal precision inconsistency | LOW-M | Database | Various |
| 17 | Commented code block | LOW | Quality | :2171-2214 |
| 20 | Incomplete signal blocking | LOW-M | Performance | :2458 |
| 21 | Repeated aliquot calculation | LOW | Performance | 3 locations |
| 22 | Hardcoded final tax rates | MEDIUM | Config | .h:31-33 |
| 23 | 2026 test rates not configurable | MEDIUM | Config | .h:55-60 |

---

## Recommended Fix Order

### Immediate (Before Production)
1. Fix SQL injection vulnerability (#1)
2. Remove debug file writing (#2)
3. Fix updateComplemento() to include new taxes (#3)
4. Add error handling for NCM queries (#15)

### High Priority (Next Release)
1. Implement preferential rate logic for construction materials (#24, #8)
2. Verify and document 2026 test rates (#5)
3. Add NCM-to-classification validation (#7)
4. Update NCM migration for construction materials (#26)
5. Implement IS validation for non-subject products (#25)

### Medium Priority
1. Implement transition blending logic (#4)
2. Implement credit presumido calculation (#9)
3. Move hardcoded values to configuration (#19, #22, #23)
4. Resolve TODO comments (#18)

### Code Quality
1. Remove commented code (#17)
2. Fix signal blocking consistency (#20)
3. Consolidate repeated calculations (#21)
