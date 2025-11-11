# ImportaProdutos.cpp - Detailed Code Review & Fix Plan

**Document Date**: 2025-11-11
**File Reviewed**: `src/importaprodutos.cpp`
**Total Issues Found**: 15
**Critical Issues**: 3 | High Priority: 3 | Medium Priority: 4 | Low Priority: 5

---

## Executive Summary

The `ImportaProdutos` class handles importing product data from Excel files into the database. While the overall structure is functional, there are **three critical issues** that pose security and stability risks:

1. **SQL Injection vulnerability** in supplier representation update
2. **Hash collision vulnerability** in product lookup dictionary
3. **Division by zero** in markup calculation

Additionally, several high-priority issues affect functionality and maintainability. This plan provides a prioritized roadmap for fixes with implementation details.

---

## CRITICAL ISSUES

### Issue #1: SQL Injection Vulnerability (Line 1025)

**Severity**: 🔴 CRITICAL
**Location**: `ImportaProdutos::on_checkBoxRepresentacao_toggled()`
**Lines**: 1025-1026

#### Problem Description

```cpp
if (not query.exec("UPDATE fornecedor SET representacao = " + QString(checked ? "TRUE" : "FALSE") +
                   " WHERE idFornecedor IN (" + idsFornecedor + ")"))
```

The `idsFornecedor` variable is directly concatenated into the SQL query string without parameterization. While the current code builds this from internal data (line 390: `idsFornecedor = ids.join(",")`), this violates secure coding practices and becomes a liability if:
- The data source is ever changed
- Future modifications introduce untrusted input
- The code is copied to other contexts

#### Risk Assessment

**Attack Vector**: If `idsFornecedor` ever contains user-controlled data, an attacker could inject arbitrary SQL.

**Current Impact**: Low (internal data only), but **Poor Security Practice** (High maintenance risk)

**Example Attack** (if future changes allow user input):
```
idsFornecedor = "1) OR 1=1 --"
Query becomes: "... WHERE idFornecedor IN (1) OR 1=1 --)"
```

#### Recommended Fix

**Option A: Parameterized Query (Preferred)**

Replace the string concatenation with parameterized binding:

```cpp
void ImportaProdutos::on_checkBoxRepresentacao_toggled(const bool checked) {
    for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
        modelProduto.setData(row, "representacao", checked);
    }

    SqlQuery query;

    // Build parameterized query with placeholders
    QStringList idsList = idsFornecedor.split(",");
    QString placeholders = "?" + QString(",?").repeated(idsList.size() - 1);

    query.prepare("UPDATE fornecedor SET representacao = :rep WHERE idFornecedor IN (" +
                  placeholders + ")");
    query.bindValue(":rep", checked);

    // Bind each ID
    for (int i = 0; i < idsList.size(); ++i) {
        query.addBindValue(idsList[i].toInt());
    }

    if (not query.exec()) {
        throw RuntimeException("Erro guardando 'Representacao' em Fornecedor: " +
                              query.lastError().text());
    }
}
```

**Option B: Validate and Sanitize (Fallback)**

If parameterized queries aren't compatible with the codebase:

```cpp
// Validate that idsFornecedor contains only comma-separated integers
QRegularExpression validIds("^\\d+(?:,\\d+)*$");
if (!validIds.match(idsFornecedor).hasMatch()) {
    throw RuntimeException("Invalid supplier ID format");
}
// Then proceed with query
```

#### Implementation Steps

1. Identify if SqlQuery class supports parameterized IN clauses with variable-length lists
2. Update `on_checkBoxRepresentacao_toggled()` with parameterized query
3. Add unit tests to verify:
   - Multiple suppliers are updated correctly
   - SQL injection attempts are prevented
4. Code review before merge

#### Estimated Effort
- **Implementation**: 30 minutes
- **Testing**: 20 minutes
- **Review**: 15 minutes
- **Total**: ~1 hour

---

### Issue #2: Hash Collision Vulnerability (Lines 68, 97-98, 123, 416, 916)

**Severity**: 🔴 CRITICAL
**Location**: Multiple locations building product lookup key
**Lines**: 68, 97-98, 123, 416, 916

#### Problem Description

The code uses concatenated strings as hash keys without delimiters:

```cpp
// Current approach (VULNERABLE)
hashModel[produto.fornecedor + produto.codComercial + produto.ui +
          QString::number(static_cast<int>(tipo))] = row;
```

This creates hash collisions when field boundaries align unintentionally:

| Scenario | Fornecedor | CodComercial | UI | Key (Current) | Collision! |
|----------|-----------|--------------|----|-|-|
| Product A | "AB" | "CD" | "1" | "ABCD1" | |
| Product B | "A" | "BCD" | "1" | "ABCD1" | ✓ COLLISION |

#### Risk Assessment

**Impact**: HIGH - Products can be incorrectly overwritten in lookup table, causing:
- Wrong products updated instead of created
- Data corruption
- Silent failures (no error thrown)

**Example Scenario**:
1. Import supplier "AB" + code "CD" + UI "1" → stored in hashModel
2. Import supplier "A" + code "BCD" + UI "1" → overwrites first entry
3. First product never updated, second product gets wrong data

#### Recommended Fix

Use delimiters in hash key construction:

```cpp
// Helper function to create safe keys
inline QString buildProductKey(const Produto& p, const Tipo tipo) {
    return p.fornecedor + "|" + p.codComercial + "|" + p.ui + "|" +
           QString::number(static_cast<int>(tipo));
}

// Apply everywhere the key is built:
// Line 68
const int row = hashModel.value(buildProductKey(produto, tipo));

// Lines 97-98
hashModel[buildProductKey(modelProduto.data(row, "fornecedor").toString(),
                         modelProduto.data(row, "codComercial").toString(),
                         modelProduto.data(row, "ui").toString(),
                         static_cast<Tipo>(modelProduto.data(row, "promocao").toInt()))] = row;

// Line 123
const bool existeNoModel = hashModel.contains(buildProductKey(produto, tipo));

// Line 416
hashModel[buildProductKey(produto, tipo)] = row;

// Line 916
hashModel[buildProductKey(produto, tipo)] = row;
```

**Better: Use Composite Key Class**

For maximum clarity and type safety:

```cpp
// In header file (importaprodutos.h)
struct ProductKey {
    QString fornecedor;
    QString codComercial;
    QString ui;
    int tipo;

    bool operator==(const ProductKey& other) const {
        return fornecedor == other.fornecedor &&
               codComercial == other.codComercial &&
               ui == other.ui &&
               tipo == other.tipo;
    }
};

// Define hash function for ProductKey
uint qHash(const ProductKey& key) {
    return qHash(key.fornecedor) ^ qHash(key.codComercial) ^
           qHash(key.ui) ^ qHash(key.tipo);
}

// In .cpp file
QHash<ProductKey, int> hashModel;

// Usage becomes cleaner:
ProductKey key{produto.fornecedor, produto.codComercial, produto.ui, static_cast<int>(tipo)};
hashModel[key] = row;
```

#### Implementation Steps

1. **Option A (Quick Fix)**: Add delimiter-based key function
   - Create `buildProductKey()` helper function
   - Replace all 5 locations with calls to helper
   - Add unit tests for collision detection

2. **Option B (Preferred)**: Implement ProductKey struct
   - Define ProductKey struct in header
   - Implement operator== and qHash
   - Replace QHash<QString, int> with QHash<ProductKey, int>
   - Update all usage sites
   - Add comprehensive collision tests

#### Testing Strategy

```cpp
// Unit test pseudo-code
void testHashCollisions() {
    QHash<QString, int> hashModel;

    // Add first product
    QString key1 = "AB|CD|1|0";
    hashModel[key1] = 1;

    // Try to add collision candidate
    QString key2 = "A|BCD|1|0";
    hashModel[key2] = 2;

    // Both should exist
    QCOMPARE(hashModel.size(), 2);
    QCOMPARE(hashModel[key1], 1);
    QCOMPARE(hashModel[key2], 2);
}
```

#### Estimated Effort
- **Quick Fix (Option A)**: 1.5 hours
  - Function creation: 15 min
  - Finding/replacing all 5 locations: 30 min
  - Testing: 45 min

- **Preferred Fix (Option B)**: 2.5 hours
  - Struct definition: 15 min
  - Hash function implementation: 20 min
  - Refactoring usage: 45 min
  - Testing: 1 hour

---

### Issue #3: Division by Zero in Markup Calculation (Line 488)

**Severity**: 🔴 CRITICAL
**Location**: `ImportaProdutos::leituraProduto()`
**Line**: 488

#### Problem Description

```cpp
produto.markup = qApp->roundDouble(((produto.precoVenda / produto.custo) - 1.) * 100);
```

If `produto.custo` is 0.0 (or very close to it due to floating-point precision), this results in:
- Division by zero (undefined behavior)
- NaN or Infinity propagation
- Potential application crash

While validation exists later (line 775: `if (produto.custo <= 0.)` in `camposForaDoPadrao()`), validation happens AFTER the calculation, which is too late.

#### Risk Assessment

**Impact**: HIGH - Application crash on invalid input
- **Trigger**: Any row in Excel with cost = 0
- **Current Flow**:
  1. `leituraProduto()` calculates markup (crash here)
  2. `camposForaDoPadrao()` would validate it (never reached)

**Example**:
```
Cost: 0
Sale Price: 100
Calculation: (100 / 0) - 1 → Exception/Crash
```

#### Recommended Fix

**Option A: Validate Before Calculation (Preferred)**

```cpp
void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
    // ... existing code ...

    // Perform minimal validation IMMEDIATELY after reading
    if (custo.toDouble() <= 0.0) {
        custo = 0.0;  // Normalize
    }
    if (precoVenda.toDouble() <= 0.0) {
        precoVenda = 0.0;  // Normalize
    }

    // Then assign to product struct
    produto.custo = custo.toDouble();
    produto.precoVenda = precoVenda.toDouble();

    // Now safe to calculate
    if (produto.custo > 0.0) {
        produto.markup = qApp->roundDouble(((produto.precoVenda / produto.custo) - 1.) * 100);
    } else {
        produto.markup = 0.0;  // Default for zero cost
    }

    // ... rest of consistency data ...
}
```

**Option B: Safe Division Helper**

```cpp
// In header or utility file
inline double safeDivide(double dividend, double divisor, double defaultValue = 0.0) {
    return (divisor != 0.0) ? (dividend / divisor) : defaultValue;
}

// In leituraProduto()
produto.markup = qApp->roundDouble((safeDivide(produto.precoVenda, produto.custo) - 1.) * 100);
```

**Option C: Separate Validation Function**

```cpp
void ImportaProdutos::validateProdutoNumericFields() {
    // Normalize and validate all numeric fields
    if (produto.custo <= 0.0) {
        produto.custo = 0.0;
    }
    if (produto.precoVenda <= 0.0) {
        produto.precoVenda = 0.0;
    }
    if (produto.m2cx < 0.0) {
        produto.m2cx = 0.0;
    }
    // ... etc for all numeric fields

    // Calculate derived fields safely
    if (produto.custo > 0.0) {
        produto.markup = qApp->roundDouble(((produto.precoVenda / produto.custo) - 1.) * 100);
    } else {
        produto.markup = 0.0;
    }
}

// Call immediately after assigning primitive fields in leituraProduto()
void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
    produto = {};
    // ... read all values ...

    // Assign to struct
    produto.idFornecedor = /* ... */;
    produto.fornecedor = /* ... */;
    // ... other string fields ...

    produto.custo = custo.toDouble();
    produto.precoVenda = precoVenda.toDouble();
    produto.ui = /* ... */;
    produto.un2 = /* ... */;
    produto.minimo = minimo.toDouble();
    produto.mva = mva.toDouble();
    produto.st = st.toDouble();
    produto.sticms = sticms.toDouble();

    // THEN validate numeric fields and calculate derived fields
    validateProdutoNumericFields();

    // consistency dados
    if (produto.ui.isEmpty()) { produto.ui = "0"; }
    // ... rest of code ...
}
```

#### Implementation Steps

1. Choose validation approach (Option A recommended for this codebase)
2. Add null/zero checks in `leituraProduto()` before calculating `markup`
3. Move validation to happen BEFORE calculations
4. Add unit tests:
   - Test with cost = 0
   - Test with cost = very small number (floating-point edge case)
   - Test with valid cost
5. Code review

#### Testing Strategy

```cpp
void testMarkupCalculation() {
    // Simulate invalid data
    Produto p;

    // Test 1: Zero cost
    p.custo = 0.0;
    p.precoVenda = 100.0;
    // Should NOT crash, should set markup = 0.0 or handle gracefully

    // Test 2: Negative cost (from Excel input)
    p.custo = -5.0;
    p.precoVenda = 100.0;
    // Should normalize to 0.0

    // Test 3: Valid data
    p.custo = 50.0;
    p.precoVenda = 100.0;
    // Should calculate markup = 100%
}
```

#### Estimated Effort
- **Implementation (Option A)**: 45 minutes
  - Add validation before calculation: 20 min
  - Move/refactor code: 15 min
  - Testing: 10 min

- **Implementation (Option B - Helper)**: 30 minutes
  - Create helper function: 10 min
  - Update calculation: 10 min
  - Testing: 10 min

- **Implementation (Option C - Separate function)**: 1 hour
  - Create validation function: 20 min
  - Integrate into flow: 20 min
  - Testing: 20 min

---

## HIGH PRIORITY ISSUES

### Issue #4: Progress Dialog Configuration Contradiction (Lines 144-151)

**Severity**: 🟡 HIGH
**Location**: `ImportaProdutos::setProgressDialog()`
**Lines**: 144-151

#### Problem Description

```cpp
void ImportaProdutos::setProgressDialog() {
    progressDialog.reset();
    progressDialog.setCancelButton(nullptr);           // Line 145: Remove cancel button
    progressDialog.setLabelText("Importando...");
    progressDialog.setWindowTitle("ERP Staccato");
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimum(0);
    progressDialog.setMaximum(0);                      // Line 150: Indeterminate progress
    progressDialog.setCancelButtonText("Cancelar");    // Line 151: Set cancel button text
}
```

**Contradiction**: Line 145 removes the cancel button, but line 151 sets its text. This is contradictory and likely results in:
- Cancel button never appearing
- `setCancelButtonText()` call being ineffective
- Confusing code maintenance

#### Risk Assessment

**Impact**: MEDIUM - UI doesn't match intended behavior
- User cannot cancel import process even though code supports it (line 107 checks `wasCanceled()`)
- Code is confusing and contradictory

#### Recommended Fix

**Clarify intent and make consistent:**

```cpp
void ImportaProdutos::setProgressDialog() {
    progressDialog.reset();
    progressDialog.setLabelText("Importando...");
    progressDialog.setWindowTitle("ERP Staccato");
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimum(0);
    progressDialog.setMaximum(0);  // Indeterminate progress bar

    // Option A: If cancel button should be shown
    progressDialog.setCancelButtonText("Cancelar");
    // Don't call setCancelButton(nullptr)

    // Option B: If cancel button should NOT be shown
    // progressDialog.setCancelButton(nullptr);
    // Don't call setCancelButtonText()
}
```

Since the code checks `if (progressDialog.wasCanceled())` on line 107, **the cancel button is expected to be visible**. Therefore:

```cpp
void ImportaProdutos::setProgressDialog() {
    progressDialog.reset();
    progressDialog.setLabelText("Importando...");
    progressDialog.setWindowTitle("ERP Staccato");
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimum(0);
    progressDialog.setMaximum(0);  // Indeterminate progress bar
    progressDialog.setCancelButtonText("Cancelar");
    // Remove the setCancelButton(nullptr) call
}
```

#### Implementation Steps

1. Identify intended behavior: Should cancel button be visible?
2. Remove contradictory call
3. Test that cancel button:
   - Is visible and clickable
   - Properly cancels the import process
4. Verify the progress dialog behavior during import

#### Estimated Effort
- **Investigation**: 10 minutes
- **Fix**: 5 minutes
- **Testing**: 15 minutes
- **Total**: ~30 minutes

---

### Issue #5: Incorrect Progress Maximum (Lines 358-369)

**Severity**: 🟡 HIGH
**Location**: `ImportaProdutos::processarArquivo()`
**Lines**: 358-369 (progress max set) and 104-125 (actual loop)

#### Problem Description

```cpp
// Lines 358-369: Count unique suppliers
int count = 0;
for (int row = 2; row < rows; ++row) {
    const QString fornec = xlsx.readValue(row, 1).toString();
    if (not fornec.isEmpty()) { ++count; }
}
progressDialog.setMaximum(count);  // Sets max to SUPPLIER count

// Lines 104-125: Actual processing loop
const int rowCount = xlsx.dimension().rowCount();
for (int row = 2; row <= rowCount; ++row) {  // Iterates through ALL rows
    progressDialog.setValue(current++);      // Increments once per row
    // ... process row ...
}
```

**Mismatch**:
- Progress max = number of unique suppliers (likely ~5-50)
- Progress increments = number of rows in Excel (likely ~500-5000)
- Result: Progress bar maxes out almost instantly

#### Risk Assessment

**Impact**: MEDIUM - UX degradation
- User sees progress bar fill to 100% in milliseconds
- Cannot estimate remaining time
- No useful progress feedback

#### Recommended Fix

**Set max to actual row count:**

```cpp
void ImportaProdutos::processarArquivo() {
    QXlsx::Document xlsx(file, this);
    xlsx.selectSheet("BASE");
    verificaTabela(xlsx);
    progressDialog.show();

    cadastraFornecedores(xlsx);
    verificaSeRepresentacao();
    marcaTodosProdutosDescontinuados();
    mostraApenasEstesFornecedores();

    itensExpired = modelProduto.rowCount();

    for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
        hashModel[/* ... */] = row;
    }

    int current = 0;
    bool canceled = false;

    const int rowCount = xlsx.dimension().rowCount();

    // FIX: Set max to total rows - header row
    progressDialog.setMaximum(rowCount - 2);  // Subtract header and starting row 2

    for (int row = 2; row <= rowCount; ++row) {
        if (progressDialog.wasCanceled()) {
            canceled = true;
            break;
        }

        progressDialog.setValue(current++);

        if (xlsx.readValue(row, 1).toString().isEmpty()) { continue; }

        // ... rest of loop ...
    }

    // ... rest of function ...
}
```

**Alternative: Two-phase Progress**

If supplier registration is time-consuming, show two-phase progress:

```cpp
void ImportaProdutos::processarArquivo() {
    QXlsx::Document xlsx(file, this);
    xlsx.selectSheet("BASE");
    verificaTabela(xlsx);

    // Phase 1: Register suppliers
    progressDialog.setLabelText("Registrando fornecedores...");
    progressDialog.show();
    cadastraFornecedores(xlsx);

    verificaSeRepresentacao();
    marcaTodosProdutosDescontinuados();
    mostraApenasEstesFornecedores();

    itensExpired = modelProduto.rowCount();

    for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
        hashModel[/* ... */] = row;
    }

    // Phase 2: Process products
    progressDialog.setLabelText("Importando produtos...");
    int current = 0;
    bool canceled = false;
    const int rowCount = xlsx.dimension().rowCount();
    progressDialog.setMaximum(rowCount - 2);

    for (int row = 2; row <= rowCount; ++row) {
        // ... rest of loop ...
    }

    // ... rest of function ...
}
```

#### Implementation Steps

1. Identify the correct progress bar max (total rows - 2 for header)
2. Update `progressDialog.setMaximum()` call
3. Test progress display:
   - Progress should advance smoothly
   - Should reach 100% when import completes
   - Time estimation should be accurate
4. Consider two-phase progress for better UX

#### Estimated Effort
- **Implementation**: 20 minutes
- **Testing**: 20 minutes
- **Total**: ~40 minutes

---

### Issue #6: Memory Leak - ValidadeDialog Stack Allocation (Line 165)

**Severity**: 🟡 HIGH
**Location**: `ImportaProdutos::readValidade()`
**Line**: 165

#### Problem Description

```cpp
bool ImportaProdutos::readValidade() {
    auto *validadeDlg = new ValidadeDialog(this);  // Heap allocation

    if (validadeDlg->exec() == QDialog::Rejected) { return false; }  // May leak if true!

    validade = validadeDlg->getValidade();

    return true;
    // Object never deleted - relies on parent cleanup
}
```

**Issues**:
1. Dialog allocated on heap with `new`
2. Never explicitly deleted
3. Relies on parent cleanup (happens when `this` is destroyed)
4. If `exec()` returns `Rejected`, the function returns early without deleting the object

While Qt's parent-child hierarchy ensures eventual cleanup, this is:
- Unnecessary heap allocation for a temporary object
- Potential early return leak (line 167)
- Not idiomatic Qt

#### Risk Assessment

**Impact**: LOW to MEDIUM
- **Actual memory leak**: No (parent cleanup works eventually)
- **Resource waste**: Yes (object lives longer than needed)
- **Code style issue**: Yes (not idiomatic Qt)
- **Potential issue**: If parent is deleted before dialog completion

#### Recommended Fix

**Use stack allocation (idiomatic Qt):**

```cpp
bool ImportaProdutos::readValidade() {
    ValidadeDialog validadeDlg(this);  // Stack allocation

    if (validadeDlg.exec() == QDialog::Rejected) {
        return false;  // Dialog auto-destroyed here
    }

    validade = validadeDlg.getValidade();

    return true;
    // Dialog auto-destroyed when leaving scope
}
```

**Why this is better**:
- Dialog is automatically destroyed when it goes out of scope
- No reliance on parent cleanup timing
- Lower memory overhead (stack allocation is faster)
- More idiomatic Qt code
- Clearer ownership semantics

#### Implementation Steps

1. Change `new ValidadeDialog(this)` to stack allocation
2. Update pointer dereferencing if needed (but `.` works for stack objects)
3. Test the validade dialog flow
4. Check if other dialogs in the codebase have the same issue

#### Similar Issues to Check

Search for other instances of this pattern:
```
new.*Dialog(this)
```

This may be a codebase-wide pattern that should be standardized.

#### Estimated Effort
- **Implementation**: 15 minutes
- **Testing**: 10 minutes
- **Codebase-wide audit**: 30 minutes (optional)
- **Total**: 25-55 minutes

---

## MEDIUM PRIORITY ISSUES

### Issue #7: Hardcoded Column Numbers (Lines 414-464)

**Severity**: 🟠 MEDIUM
**Location**: `ImportaProdutos::leituraProduto()`
**Lines**: 414-464

#### Problem Description

```cpp
void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
    produto = {};
    const QLocale locale(QLocale::Portuguese);

    QVariant fornecedor = xlsx.readValue(row, 1);      // Hard-coded column 1
    QVariant descricao = xlsx.readValue(row, 2);       // Hard-coded column 2
    QVariant un = xlsx.readValue(row, 3);              // Hard-coded column 3
    QVariant colecao = xlsx.readValue(row, 4);         // Hard-coded column 4
    QVariant m2cx = xlsx.readValue(row, 5);            // Hard-coded column 5
    // ... 15 more hard-coded columns ...
    QVariant sticms = xlsx.readValue(row, 20);         // Hard-coded column 20
    // ...
}
```

**Risks**:
1. **Fragile to changes**: If Excel template column order changes, code silently reads wrong data
2. **No validation**: No check that header row matches expected schema
3. **Error detection**: Errors only manifest as invalid data or validation failures, not immediately
4. **Maintainability**: Hard to understand which columns are being read
5. **Header verification exists** (lines 992-1011) but only validates names, not used for column lookup

#### Risk Assessment

**Impact**: MEDIUM
- Data corruption if Excel template changes
- Silent failures (bad data imported instead of error thrown)
- Header verification exists but is incomplete

#### Recommended Fix

**Use dynamic column lookup based on headers:**

```cpp
class ImportaProdutos {
private:
    // Cache column indices after verifying headers
    struct ColumnMap {
        int fornecedor;
        int descricao;
        int un;
        int colecao;
        int m2cx;
        int pccx;
        int kgcx;
        int formComercial;
        int codComercial;
        int codBarras;
        int ncm;
        int qtdPallet;
        int custo;
        int precoVenda;
        int ui;
        int un2;
        int minimo;
        int mva;
        int st;
        int sticms;
    };

    ColumnMap columnMap;

    // Function to build column map from headers
    void buildColumnMap(QXlsx::Document &xlsx);
};

// Implementation
void ImportaProdutos::buildColumnMap(QXlsx::Document &xlsx) {
    // Define expected headers
    QMap<QString, int*> headerMap{
        {"fornecedor", &columnMap.fornecedor},
        {"descricao", &columnMap.descricao},
        {"un", &columnMap.un},
        {"colecao", &columnMap.colecao},
        {"m2cx", &columnMap.m2cx},
        {"pccx", &columnMap.pccx},
        {"kgcx", &columnMap.kgcx},
        {"formComercial", &columnMap.formComercial},
        {"codComercial", &columnMap.codComercial},
        {"codBarras", &columnMap.codBarras},
        {"ncm", &columnMap.ncm},
        {"qtdPallet", &columnMap.qtdPallet},
        {"custo", &columnMap.custo},
        {"precoVenda", &columnMap.precoVenda},
        {"ui", &columnMap.ui},
        {"un2", &columnMap.un2},
        {"minimo", &columnMap.minimo},
        {"mva", &columnMap.mva},
        {"st", &columnMap.st},
        {"sticms", &columnMap.sticms},
    };

    // Scan header row (row 1) and build column index map
    int col = 1;
    while (!xlsx.readValue(1, col).toString().isEmpty()) {
        QString headerName = xlsx.readValue(1, col).toString();
        if (headerMap.contains(headerName)) {
            *headerMap[headerName] = col;
        }
        col++;
    }

    // Verify all required headers found
    for (const auto& headerName : headerMap.keys()) {
        if (*headerMap[headerName] == 0) {
            throw RuntimeException(QString("Column '%1' not found in Excel header").arg(headerName));
        }
    }
}

// Usage in processarArquivo()
void ImportaProdutos::processarArquivo() {
    QXlsx::Document xlsx(file, this);
    xlsx.selectSheet("BASE");
    verificaTabela(xlsx);
    buildColumnMap(xlsx);  // Build column map from headers

    // ... rest of function ...
}

// Usage in leituraProduto()
void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
    produto = {};
    const QLocale locale(QLocale::Portuguese);

    // Use column map instead of hard-coded numbers
    QVariant fornecedor = xlsx.readValue(row, columnMap.fornecedor);
    QVariant descricao = xlsx.readValue(row, columnMap.descricao);
    QVariant un = xlsx.readValue(row, columnMap.un);
    QVariant colecao = xlsx.readValue(row, columnMap.colecao);
    QVariant m2cx = xlsx.readValue(row, columnMap.m2cx);
    // ... etc ...
}
```

**Alternative: Simple Validation Approach**

If refactoring to dynamic column lookup is too extensive, add validation that hard-coded columns match header:

```cpp
void ImportaProdutos::verificaTabela(QXlsx::Document &xlsx) {
    // Existing checks
    if (xlsx.readValue(1, 1).toString() != "fornecedor") {
        throw RuntimeError("Column 1 should be 'fornecedor'");
    }
    if (xlsx.readValue(1, 2).toString() != "descricao") {
        throw RuntimeError("Column 2 should be 'descricao'");
    }
    // ... etc for all 20 columns ...

    qDebug() << "Excel template structure validated successfully";
}
```

#### Implementation Steps

**Option A: Quick fix (validation only)**
1. Update `verificaTabela()` to validate ALL column positions
2. Test with correctly and incorrectly formatted Excel files
3. Time: 30 minutes

**Option B: Proper fix (dynamic lookup)**
1. Design ColumnMap structure
2. Implement `buildColumnMap()` function
3. Refactor `leituraProduto()` to use ColumnMap
4. Update `processarArquivo()` to call `buildColumnMap()`
5. Test thoroughly with different column orders
6. Time: 2-3 hours

#### Estimated Effort
- **Option A (validation)**: 30 minutes
- **Option B (full refactor)**: 2-3 hours

---

### Issue #8: TODO - File Not Closed on Error (Line 1036)

**Severity**: 🟠 MEDIUM
**Location**: Comment at line 1036
**Issue**: Excel file not properly closed if exception thrown during read

#### Problem Description

```cpp
// TODO: 0se der erro durante a leitura o arquivo nao é fechado
```

The Excel document (`QXlsx::Document xlsx`) is created on line 82:

```cpp
void ImportaProdutos::processarArquivo() {
    QXlsx::Document xlsx(file, this);  // Line 82: Created here

    xlsx.selectSheet("BASE");
    verificaTabela(xlsx);

    // ... lots of processing that might throw ...

    // If exception thrown, document might not be properly closed
}
```

#### Risk Assessment

**Impact**: LOW to MEDIUM
- Qt typically handles resource cleanup automatically
- `QXlsx::Document` is a Qt object, may have auto-cleanup
- **Risk**: File handle remains open temporarily, could affect:
  - Ability to re-import same file immediately
  - File locking issues on Windows
  - Resource exhaustion if multiple imports fail

#### Recommended Fix

**Use RAII pattern with explicit cleanup:**

```cpp
void ImportaProdutos::processarArquivo() {
    QXlsx::Document xlsx(file, this);

    try {
        xlsx.selectSheet("BASE");
        verificaTabela(xlsx);

        progressDialog.show();

        cadastraFornecedores(xlsx);
        verificaSeRepresentacao();
        marcaTodosProdutosDescontinuados();
        mostraApenasEstesFornecedores();

        itensExpired = modelProduto.rowCount();

        for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
            hashModel[/* ... */] = row;
        }

        int current = 0;
        bool canceled = false;
        const int rowCount = xlsx.dimension().rowCount();

        for (int row = 2; row <= rowCount; ++row) {
            if (progressDialog.wasCanceled()) {
                canceled = true;
                break;
            }

            progressDialog.setValue(current++);

            if (xlsx.readValue(row, 1).toString().isEmpty()) { continue; }

            leituraProduto(xlsx, row);

            if (camposForaDoPadrao()) {
                insereEmErro();
                continue;
            }

            const bool existeNoModel = hashModel.contains(/* ... */);
            existeNoModel ? atualizaProduto() : insereEmOk();
        }

        progressDialog.cancel();

        if (canceled) { throw std::exception(); }

        setupTables();
        ui->tableProdutos->sortByColumn("descontinuado", Qt::AscendingOrder);
        showMaximized();

        const QString resultado = /* ... */;
        QMessageBox::information(this, "Aviso!", resultado);

    } catch (...) {
        // Explicit cleanup on error
        // QXlsx::Document may auto-cleanup, but be explicit
        throw;
    }
}
```

**Better approach - Check QXlsx documentation:**

1. Verify `QXlsx::Document` auto-cleanup behavior
2. If manual close needed, add:

```cpp
xlsx.close();  // Or similar method if available
```

3. If relying on destructor, add comment explaining why

#### Implementation Steps

1. Check QXlsx documentation for proper cleanup
2. If needed, add explicit close/cleanup call
3. Test with forced exceptions during import
4. Verify file is properly closed and can be re-imported

#### Estimated Effort
- **Investigation**: 15 minutes
- **Implementation**: 15 minutes
- **Testing**: 15 minutes
- **Total**: 45 minutes

---

### Issue #9: TODO - Markup Display/Calculation Issues (Line 1034)

**Severity**: 🟠 MEDIUM
**Location**: Line 1034 TODO comment
**Issue**: Known bug with markup calculation or display

#### Problem Description

```cpp
// TODO: 4markup esta exibindo errado ou salvando errado
```

The markup is calculated on line 488:

```cpp
produto.markup = qApp->roundDouble(((produto.precoVenda / produto.custo) - 1.) * 100);
```

But the display or saving is incorrect. Without more context, possible issues:
- Calculation: `(precoVenda / custo - 1) * 100` gives percentage markup
- Display: May be showing wrong decimal places or format
- Storage: May not be persisting to database correctly
- Delegate: The percentage delegate might be displaying incorrectly

#### Risk Assessment

**Impact**: MEDIUM
- Data integrity: Markup values may be incorrect in database
- User confusion: UI shows different value than stored value
- Calculations: If markup used in other calculations, they're wrong

#### Recommended Fix

**Investigation steps**:

1. **Verify formula**:
   ```
   Markup % = ((Sale Price / Cost) - 1) × 100
   Example: Sale Price = 150, Cost = 100
   Markup = ((150/100) - 1) × 100 = 50%  ✓ Correct
   ```

2. **Check display delegate** (line 297):
   ```cpp
   ui->tableProdutos->setItemDelegateForColumn("markup", porcDelegate);
   ```
   - Is `PorcentagemDelegate` formatting correctly?
   - Does it match the stored values?

3. **Check database schema**:
   - Field type: Should be decimal/float
   - Scale: How many decimal places?
   - Does stored value match displayed value?

4. **Check rounding**:
   ```cpp
   produto.markup = qApp->roundDouble(/* ... */);
   ```
   - How many decimal places does `roundDouble()` round to?
   - Is this consistent with database storage?

5. **Create test cases**:
   ```cpp
   // Cost = 100, Sale Price = 150
   // Expected markup: 50%
   Produto p{/*...*/};
   QCOMPARE(p.markup, 50.0);

   // Cost = 50, Sale Price = 75
   // Expected markup: 50%
   QCOMPARE(p.markup, 50.0);
   ```

6. **Integration test**:
   - Import product with known cost/price
   - Verify markup calculation
   - Save and reload
   - Verify markup still correct
   - Check delegate displays correct value

#### Implementation Steps

1. Create a debug script to:
   - Calculate markup manually for test products
   - Compare with what's shown in UI
   - Compare with what's in database
2. Identify discrepancy
3. Fix root cause (calculation, display, or storage)
4. Add regression tests
5. Document the correct behavior

#### Estimated Effort
- **Investigation**: 1 hour
- **Fix (if formula)**: 30 minutes
- **Fix (if display)**: 1 hour
- **Fix (if storage)**: 1 hour
- **Testing**: 30 minutes
- **Total**: 2-3 hours (depending on root cause)

---

## LOW PRIORITY ISSUES

### Issue #10: Magic Numbers Without Constants

**Severity**: 🔵 LOW
**Location**: Throughout file

#### Examples

- `.left(100)` - unclear if this is database column size (line 468, 469, etc.)
- `.left(45)` - unit field length (line 470, 475, 482, 483)
- `.left(250)` - description length (line 469)
- `.left(200)` - collection length (line 471)
- `.left(10)` - NCM code length (line 478)
- NCM length validation: `product.ncm.length() == 6` (line 498)
- Vector indices: `xlsx.readValue(row, 1)` through `xlsx.readValue(row, 20)`

#### Problem

Magic numbers make code hard to understand and maintain:

```cpp
produto.fornecedor = fornecedor.toString().toUpper().trimmed().left(100);  // What's 100?
produto.descricao = descricao.toString().remove("*").remove("()").replace('_', ' ').toUpper().trimmed().left(250);
produto.un = un.toString().remove("*").toUpper().trimmed().left(45);
```

#### Recommended Fix

Define constants:

```cpp
// In importaprodutos.h
namespace ImportConstants {
    constexpr int MAX_RAZAO_SOCIAL = 100;
    constexpr int MAX_DESCRICAO = 250;
    constexpr int MAX_UNIDADE = 45;
    constexpr int MAX_COLECAO = 200;
    constexpr int MAX_FORMA_COMERCIAL = 100;
    constexpr int MAX_COD_COMERCIAL = 100;
    constexpr int MAX_COD_BARRAS = 100;
    constexpr int MAX_NCM = 10;

    constexpr int MIN_NCM_DIGITS = 8;  // For non-representacao products

    // Excel column indices
    constexpr int COL_FORNECEDOR = 1;
    constexpr int COL_DESCRICAO = 2;
    constexpr int COL_UN = 3;
    // ... etc
}

// In .cpp file
using namespace ImportConstants;

produto.fornecedor = fornecedor.toString().toUpper().trimmed().left(MAX_RAZAO_SOCIAL);
produto.descricao = descricao.toString().remove("*").remove("()").replace('_', ' ')
                            .toUpper().trimmed().left(MAX_DESCRICAO);
```

#### Estimated Effort
- **Define constants**: 20 minutes
- **Replace magic numbers**: 30 minutes
- **Testing**: 10 minutes
- **Total**: 1 hour

---

### Issue #11: Repetitive Field Update Code (Lines 537-723)

**Severity**: 🔵 LOW
**Location**: `ImportaProdutos::atualizaCamposProduto()` (Lines 527-724)

#### Problem

The function repeats nearly identical code 24 times:

```cpp
if (modelProduto.data(row, "fornecedor").toString() != produto.fornecedor) {
    modelProduto.setData(row, "fornecedor", produto.fornecedor);
    modelProduto.setData(row, "fornecedorUpd", yellow);
    changed = true;
} else {
    modelProduto.setData(row, "fornecedorUpd", white);
}

if (modelProduto.data(row, "descricao").toString() != produto.descricao) {
    modelProduto.setData(row, "descricao", produto.descricao);
    modelProduto.setData(row, "descricaoUpd", yellow);
    changed = true;
} else {
    modelProduto.setData(row, "descricaoUpd", white);
}

// ... repeats 22 more times ...
```

#### Risk

- **High maintenance burden**: Changes require updating 24+ locations
- **Bug propagation**: Errors duplicate across all instances
- **Code complexity**: Function is 200+ lines of near-identical code
- **Hard to read**: Difficult to see the actual logic

#### Recommended Fix

Use data-driven approach:

```cpp
void ImportaProdutos::atualizaCamposProduto(const int row) {
    modelProduto.setData(row, "atualizarTabelaPreco", true);

    const int yellow = static_cast<int>(FieldColors::Yellow);
    const int white = static_cast<int>(FieldColors::White);

    // Define field definitions
    struct FieldDef {
        QString fieldName;
        QString updateFieldName;
        QVariant currentValue;
        QVariant newValue;
    };

    QVector<FieldDef> fields{
        {"fornecedor", "fornecedorUpd",
         modelProduto.data(row, "fornecedor").toString(),
         produto.fornecedor},

        {"descricao", "descricaoUpd",
         modelProduto.data(row, "descricao").toString(),
         produto.descricao},

        {"un", "unUpd",
         modelProduto.data(row, "un").toString(),
         produto.un},

        // ... etc for all 24 fields ...
    };

    bool changed = false;
    for (const auto& field : fields) {
        if (field.currentValue != field.newValue) {
            modelProduto.setData(row, field.fieldName, field.newValue);
            modelProduto.setData(row, field.updateFieldName, yellow);
            changed = true;
        } else {
            modelProduto.setData(row, field.updateFieldName, white);
        }
    }

    changed ? itensUpdated++ : itensNotChanged++;
}
```

Or even better, use a macro:

```cpp
#define UPDATE_FIELD(fieldName, fieldValue) \
    if (modelProduto.data(row, fieldName).toString() != fieldValue) { \
        modelProduto.setData(row, fieldName, fieldValue); \
        modelProduto.setData(row, QString(fieldName) + "Upd", yellow); \
        changed = true; \
    } else { \
        modelProduto.setData(row, QString(fieldName) + "Upd", white); \
    }

// Usage:
UPDATE_FIELD("fornecedor", produto.fornecedor);
UPDATE_FIELD("descricao", produto.descricao);
UPDATE_FIELD("un", produto.un);
// ... etc ...
```

#### Estimated Effort
- **Data-driven approach**: 2 hours
  - Design structure: 30 minutes
  - Implement: 1 hour
  - Test: 30 minutes

- **Macro approach**: 1.5 hours
  - Create macros: 20 minutes
  - Replace code: 45 minutes
  - Test: 30 minutes

---

### Issue #12: Exception Type Inconsistency

**Severity**: 🔵 LOW
**Location**: Lines 60, 992, and others

#### Problem

The code uses both `RuntimeException` and `RuntimeError`:

```cpp
throw RuntimeException("Error message");  // Line 60
throw RuntimeError("Error message");      // Line 992
```

#### Risks

- Inconsistent error handling
- Unclear which exception types exist
- Difficult to catch specific exceptions

#### Recommended Fix

1. Define both exception types clearly in header
2. Establish convention:
   - `RuntimeException`: Business logic errors (invalid data, database errors)
   - `RuntimeError`: System errors (file I/O, missing resources)
   Or vice versa

3. Use consistently throughout

#### Estimated Effort
- 30 minutes to audit and standardize

---

### Issue #13: TODO - Discontinued Promotion Products (Line 1035)

**Severity**: 🔵 LOW
**Location**: Line 1035 TODO comment

#### Problem

```cpp
// TODO: 4nao mostrar promocao descontinuado
```

Discontinued promotion products should be filtered out of the UI display.

#### Recommended Fix

Add filter when displaying promotion products:

```cpp
// In setupTables() or a filter function
if (tipo == Tipo::Promocao) {
    modelProduto.setFilter("descontinuado = FALSE AND promocao = TRUE");
}
```

#### Estimated Effort
- 30 minutes

---

## Implementation Priority Matrix

```
PRIORITY | ISSUES | EFFORT | IMPACT
---------|--------|--------|--------
URGENT   | #1,#2,#3| 3h   | CRITICAL
HIGH     | #4,#5,#6| 2h   | MAJOR
MEDIUM   | #7,#8,#9| 6h   | MODERATE
LOW      | #10-13  | 4h   | MINOR
```

---

## Recommended Implementation Order

### Phase 1: Critical Fixes (6-8 hours)
**Do these immediately** - they affect security and stability

1. **Issue #2 (Hash Collision)** - 1.5-2 hours
   - Use delimited keys or ProductKey struct
   - Test thoroughly

2. **Issue #3 (Division by Zero)** - 45 minutes
   - Add validation before calculation
   - Test with edge cases

3. **Issue #1 (SQL Injection)** - 1 hour
   - Parameterize the representation query
   - Add security tests

4. **Issue #4 (Progress Dialog)** - 30 minutes
   - Remove contradictory code
   - Test cancel functionality

5. **Issue #5 (Progress Max)** - 40 minutes
   - Fix progress counting
   - Test with realistic data

### Phase 2: High-Priority Fixes (1-2 hours)
**Complete within next sprint** - improve functionality and maintainability

6. **Issue #6 (ValidadeDialog)** - 25 minutes
   - Convert to stack allocation
   - Check for similar patterns

7. **Issue #7 (Hardcoded Columns)** - 30-60 minutes
   - Choose validation or refactor approach
   - Test with different Excel layouts

### Phase 3: Medium-Priority Fixes (4-6 hours)
**Include in next iteration** - improve code quality

8. **Issue #8 (File Cleanup)** - 45 minutes
9. **Issue #9 (Markup Bug)** - 1.5-2 hours (depends on root cause)
10. **Issue #11 (Repetitive Code)** - 2 hours

### Phase 4: Low-Priority Improvements (3-4 hours)
**Nice-to-have** - improve maintainability and readability

11. **Issue #10 (Magic Numbers)** - 1 hour
12. **Issue #12 (Exception Consistency)** - 30 minutes
13. **Issue #13 (Filter TODOs)** - 30 minutes

---

## Testing Strategy

### Unit Tests to Add

```cpp
// Collision detection
void testProductKeyCollisions();

// Division by zero
void testMarkupWithZeroCost();
void testMarkupWithValidCost();

// Hash functions
void testProductKeyHash();
void testProductKeyEquality();

// SQL injection prevention
void testRepresentacaoUpdateWithInvalidInput();

// Progress tracking
void testProgressMaxCalculation();
```

### Integration Tests

```cpp
// Full import flow with various error conditions
void testImportWithZeroCosts();
void testImportWithInvalidColumns();
void testImportCancellation();
void testImportWithDuplicateProducts();
```

### Manual Testing

1. Import with valid Excel file
2. Import with invalid columns (test column validation)
3. Import with zero cost products (test division by zero)
4. Import with duplicate product keys (test collisions)
5. Cancel mid-import (test progress cancellation)
6. Verify progress bar accuracy
7. Test representacao checkbox update

---

## Checklist for Implementation

- [ ] All CRITICAL issues fixed and tested
- [ ] All HIGH priority issues fixed and tested
- [ ] Code review completed
- [ ] Unit tests added for critical areas
- [ ] Integration tests pass
- [ ] Manual testing completed
- [ ] Documentation updated
- [ ] Performance impact assessed
- [ ] Backward compatibility verified
- [ ] Related TODOs addressed

---

## Notes for Developers

1. **Hash key changes**: If changing from simple string concatenation to delimiters, update ALL 5 locations (lines 68, 97-98, 123, 416, 916)

2. **Database compatibility**: Ensure any changes to field validation don't break existing database records

3. **Excel file format**: If making changes to hardcoded column numbers, ensure existing Excel templates are documented

4. **Testing coverage**: Add tests for error conditions, not just happy path

5. **Performance**: The import process reads entire Excel file and iterates rows. Monitor performance impact of any refactoring.

---

**Document prepared for code review and implementation planning**
