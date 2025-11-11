# Option 3 Deep Dive: Supplier ID Resolution Strategy

**Document Date**: 2025-11-11
**Complexity Level**: High
**Estimated Effort**: 4-6 hours

---

## The Core Challenge

When deferring all database writes until `salvar()`, we face this problem:

```
Timeline:
────────────────────────────────────────────────────────

Phase 1 (processarArquivo):
  Read Excel with supplier name "ABC Ltda"
  Need to assign idFornecedor to products
  But suppliers haven't been created yet!

  Current code:
    const int idFornecedor = buscarCadastrarFornecedor();  // Creates supplier NOW
    produto.idFornecedor = idFornecedor;                   // Uses real ID

  With Option 3:
    ???? How do we assign idFornecedor????

Phase 3 (salvar):
  Create supplier "ABC Ltda" in database → Gets real idFornecedor (e.g., 42)
  Now need to update all products with idFornecedor = 42
  But we already created products with idFornecedor = ???
```

**The Problem**: We need supplier IDs before suppliers exist in the database.

---

## Solution 1: Temporary Negative IDs (SIMPLEST)

**Idea**: Use negative numbers as temporary supplier IDs during Phase 1, resolve to real IDs during Phase 3.

### How It Works

```
Phase 1: processarArquivo()
──────────────────────────

Suppliers in Excel:
  "ABC Ltda" (new)
  "XYZ Corp" (new)
  "Existing Supplier" (already in DB)

Build supplier map:
  {
    "ABC Ltda" → -1              (temporary ID)
    "XYZ Corp" → -2              (temporary ID)
    "Existing Supplier" → 42     (real ID from DB)
  }

Products in Excel:
  Product 1: supplier="ABC Ltda" → idFornecedor = -1
  Product 2: supplier="XYZ Corp" → idFornecedor = -2
  Product 3: supplier="Existing Supplier" → idFornecedor = 42

All stored in memory models with these IDs.

Phase 3: salvar()
─────────────────

Start transaction

For each pending operation:

  1. Create suppliers:
     INSERT INTO fornecedor (razaoSocial) VALUES ("ABC Ltda") → Gets idFornecedor = 100
     INSERT INTO fornecedor (razaoSocial) VALUES ("XYZ Corp") → Gets idFornecedor = 101

  2. Build ID mapping:
     tempIdMap[-1] = 100
     tempIdMap[-2] = 101

  3. Fix product references:
     For each product in modelProduto:
       if (product.idFornecedor < 0):
         product.idFornecedor = tempIdMap[product.idFornecedor]

  4. Submit all products:
     modelProduto.submitAll()

Commit transaction
```

### Implementation

**Header file changes**:

```cpp
class ImportaProdutos {
private:
    struct PendingSupplier {
        int tempId;           // Negative ID used during Phase 1
        QString razaoSocial;
        QVariant validadeProdutos;
    };

    QVector<PendingSupplier> pendingSuppliers;
    QMap<int, int> tempToRealSupplierIds;  // Maps -1→100, -2→101, etc
    int nextTempSupplierId = -1;           // Counter: -1, -2, -3, ...
};
```

**Phase 1: Modified `cadastraFornecedores()`**:

```cpp
void ImportaProdutos::cadastraFornecedores(QXlsx::Document &xlsx) {
    const int rows = xlsx.dimension().rowCount();
    QStringList fornecedores;
    int count = 0;

    // Extract unique suppliers from Excel
    for (int row = 2; row < rows; ++row) {
        const QString fornec = xlsx.readValue(row, 1).toString();
        if (not fornec.isEmpty()) { ++count; }
        if (fornec.isEmpty() or fornecedores.contains(fornec)) { continue; }
        fornecedores << xlsx.readValue(row, 1).toString();
    }

    progressDialog.setMaximum(count);

    // NOW: Just check which suppliers exist, queue creations for new ones
    for (auto const &fornecedor : qAsConst(fornecedores)) {
        m_fornecedor = fornecedor.left(100);

        // Try to find supplier in existing database (READ ONLY)
        SqlQuery queryFornecedor;
        queryFornecedor.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
        queryFornecedor.bindValue(":razaoSocial", m_fornecedor);

        int idFornecedor = -1;  // Default: not found

        if (queryFornecedor.exec() && queryFornecedor.first()) {
            // Supplier EXISTS in database - use real ID
            idFornecedor = queryFornecedor.value("idFornecedor").toInt();
        } else {
            // Supplier NEW - queue for creation with temp ID
            idFornecedor = nextTempSupplierId;  // e.g., -1
            nextTempSupplierId--;                // Next will be -2

            PendingSupplier pending;
            pending.tempId = idFornecedor;
            pending.razaoSocial = m_fornecedor;
            pending.validadeProdutos = (validade == -1) ? QVariant()
                                                        : qApp->serverDate().addDays(validade);
            pendingSuppliers.append(pending);
        }

        // Map supplier name to ID (could be real or temporary)
        m_fornecedores.insert(fornecedor.left(100), idFornecedor);
    }
}
```

**Phase 1: `leituraProduto()` unchanged**:

```cpp
void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
    produto = {};
    const QLocale locale(QLocale::Portuguese);

    // ... read all fields ...

    // Use the mapped supplier ID (could be real or temporary)
    produto.idFornecedor = m_fornecedores.value(fornecedor.toString().trimmed());
    // If supplier is pending: idFornecedor = -1 or -2, etc
    // If supplier exists: idFornecedor = real ID like 42

    // ... rest of code ...
}
```

**Phase 3: Modified `salvar()`**:

```cpp
void ImportaProdutos::salvar() {
    // Step 1: Create pending suppliers and build ID mapping
    for (const auto& pending : pendingSuppliers) {
        SqlQuery queryInsert;
        queryInsert.prepare("INSERT INTO fornecedor (razaoSocial, validadeProdutos) "
                           "VALUES (:razaoSocial, :validade)");
        queryInsert.bindValue(":razaoSocial", pending.razaoSocial);
        queryInsert.bindValue(":validade", pending.validadeProdutos);

        if (not queryInsert.exec()) {
            throw RuntimeException("Erro cadastrando fornecedor: " + queryInsert.lastError().text());
        }

        // Get the newly assigned ID
        int realId = queryInsert.lastInsertId().toInt();

        // Map temporary ID to real ID
        tempToRealSupplierIds[pending.tempId] = realId;
    }

    // Step 2: Update all products with temporary IDs to real IDs
    for (int row = 0; row < modelProduto.rowCount(); ++row) {
        int tempId = modelProduto.data(row, "idFornecedor").toInt();

        // Check if this is a temporary ID
        if (tempId < 0 && tempToRealSupplierIds.contains(tempId)) {
            int realId = tempToRealSupplierIds[tempId];
            modelProduto.setData(row, "idFornecedor", realId);
        }
    }

    // Step 3: Now submit all products with resolved IDs
    modelProduto.submitAll();

    // Step 4: Handle supplier validity dates (for existing suppliers)
    // This was deferred from Phase 1
    for (const auto& pending : pendingSuppliers) {
        int realId = tempToRealSupplierIds[pending.tempId];
        // Already set during INSERT, nothing more needed
    }

    // Step 5: Continue with existing salvar() logic
    SqlQuery queryPrecos;

    if (validade != -1) {
        queryPrecos.prepare(
            "INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) "
            "SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim "
            "FROM produto WHERE atualizarTabelaPreco = TRUE");
        queryPrecos.bindValue(":validadeInicio", qApp->serverDate().toString("yyyy-MM-dd"));
        queryPrecos.bindValue(":validadeFim", validadeString);

        if (not queryPrecos.exec()) {
            throw RuntimeException("Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text());
        }
    }

    // ... rest of existing salvar() code ...
}
```

### Advantages

✅ **Simple**: Straightforward mapping concept
✅ **No complex queuing**: Don't need elaborate operation queues
✅ **Easy to test**: Negative IDs are clearly temporary
✅ **Minimal changes**: Only changes to `cadastraFornecedores()` and `salvar()`
✅ **Works with existing model structure**: Models already have `idFornecedor` field

### Disadvantages

✗ **ID resolution overhead**: Must scan and update all products
✗ **Error prone**: If ID mapping fails, products have wrong supplier
✗ **Magic numbers**: Negative IDs are a bit of a hack
✗ **Debugging difficulty**: Hard to trace issues with temp IDs

### Data Flow Example

```
Excel File:
  Product 1: "ABC Ltda" | "Widget" | ...
  Product 2: "ABC Ltda" | "Gadget" | ...
  Product 3: "XYZ Corp" | "Doohickey" | ...
  Product 4: "Existing" | "Thing" | ...

Phase 1 (In Memory):
  m_fornecedores = {
    "ABC Ltda" → -1,
    "XYZ Corp" → -2,
    "Existing" → 42  ← Real ID from database
  }

  modelProduto contains:
    Row 0: idFornecedor=-1, nome="Widget"
    Row 1: idFornecedor=-1, nome="Gadget"
    Row 2: idFornecedor=-2, nome="Doohickey"
    Row 3: idFornecedor=42, nome="Thing"

Phase 3 (Database):
  CREATE "ABC Ltda" → idFornecedor=100
  CREATE "XYZ Corp" → idFornecedor=101

  tempToRealSupplierIds = {
    -1 → 100,
    -2 → 101
  }

  UPDATE modelProduto:
    Row 0: idFornecedor=-1 → 100
    Row 1: idFornecedor=-1 → 100
    Row 2: idFornecedor=-2 → 101
    Row 3: idFornecedor=42 (unchanged)

  INSERT all products with correct IDs
```

---

## Solution 2: Deferred Queue with Supplier References (MORE COMPLEX)

**Idea**: Queue supplier creations and product creations separately, resolve references in `salvar()`.

### How It Works

```cpp
struct PendingSupplier {
    QString razaoSocial;
    QVariant validadeProdutos;
    // Will receive idFornecedor after creation
};

struct PendingProduct {
    int pendingSupplierIndex;  // Index into pendingSuppliers vector
    QString nome;
    QString descricao;
    // ... other product fields ...
    // idFornecedor will be set before INSERT
};

QVector<PendingSupplier> pendingSuppliers;
QVector<PendingProduct> pendingProducts;
```

### Implementation Sketch

```cpp
void ImportaProdutos::cadastraFornecedores(QXlsx::Document &xlsx) {
    // Queue suppliers instead of creating
    for (auto const &fornecedor : qAsConst(fornecedores)) {
        SqlQuery queryCheck;
        queryCheck.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
        queryCheck.bindValue(":razaoSocial", fornecedor.left(100));

        if (queryCheck.exec() && queryCheck.first()) {
            // Existing supplier
            m_fornecedores[fornecedor] = queryCheck.value("idFornecedor").toInt();
        } else {
            // New supplier - queue it
            PendingSupplier pending;
            pending.razaoSocial = fornecedor.left(100);
            pending.validadeProdutos = (validade == -1) ? QVariant()
                                                        : qApp->serverDate().addDays(validade);

            int supplierIndex = pendingSuppliers.size();
            pendingSuppliers.append(pending);

            // Store index instead of real ID
            m_fornecedores[fornecedor] = -100 - supplierIndex;  // -100, -101, -102, etc
        }
    }
}

void ImportaProdutos::insereEmOk() {
    // Instead of directly inserting to model, queue the product

    int pendingSupplierIdx = -1;
    if (produto.idFornecedor < -99) {
        // This is a pending supplier reference
        pendingSupplierIdx = -(produto.idFornecedor) - 100;
    }

    PendingProduct pending;
    pending.pendingSupplierIndex = pendingSupplierIdx;
    pending.nome = produto.descricao;
    // ... copy other fields ...

    pendingProducts.append(pending);
    itensImported++;
}

void ImportaProdutos::salvar() {
    // Step 1: Create suppliers and collect their new IDs
    QVector<int> supplierRealIds;

    for (const auto& pending : pendingSuppliers) {
        SqlQuery queryInsert;
        queryInsert.prepare("INSERT INTO fornecedor (razaoSocial, validadeProdutos) "
                           "VALUES (:razaoSocial, :validade)");
        queryInsert.bindValue(":razaoSocial", pending.razaoSocial);
        queryInsert.bindValue(":validade", pending.validadeProdutos);

        if (not queryInsert.exec()) {
            throw RuntimeException("Error creating supplier");
        }

        supplierRealIds.append(queryInsert.lastInsertId().toInt());
    }

    // Step 2: Create products using resolved supplier IDs
    for (const auto& pending : pendingProducts) {
        int realSupplierId;
        if (pending.pendingSupplierIndex >= 0) {
            // Use newly created supplier ID
            realSupplierId = supplierRealIds[pending.pendingSupplierIndex];
        } else {
            // Use existing supplier ID
            realSupplierId = pending.realSupplierIdForExisting;
        }

        SqlQuery queryInsert;
        queryInsert.prepare("INSERT INTO produto (...) VALUES (...)");
        queryInsert.bindValue(":idFornecedor", realSupplierId);
        // ... bind other fields ...
        queryInsert.exec();
    }

    // ... rest of salvar() ...
}
```

### Advantages

✅ **Explicit references**: Clear what's pending vs real
✅ **Index-based**: Avoids negative ID magic
✅ **Type safe**: Can create dedicated struct types

### Disadvantages

✗ **Complex**: Multiple reference types to track
✗ **Harder to debug**: Need to understand index mapping
✗ **Duplicates model data**: Models have product data in both models AND pending queue
✗ **Larger refactoring**: Changes to multiple functions

---

## Solution 3: Dual Key Map (MOST ROBUST)

**Idea**: Use supplier name as key until real ID assigned, then switch to real ID.

### How It Works

```cpp
// During Phase 1: Map supplier name → pending record
QMap<QString, PendingSupplier*> suppliersByName;
QMap<int, PendingSupplier*> suppliersById;  // Empty initially

// Product has:
struct Produto {
    QString supplierName;  // Use name as foreign key initially
    int idFornecedor;      // Updated after supplier created
};

// In salvar():
for (auto& product : products) {
    // Find supplier record by name
    PendingSupplier* supplier = suppliersByName[product.supplierName];
    if (supplier) {
        // Update product with new supplier ID
        product.idFornecedor = supplier->realIdAfterCreation;
    }
}
```

### Advantages

✅ **Clean semantics**: Products reference suppliers by name, resolved later
✅ **Flexible**: Supports duplicate supplier names
✅ **Less magic**: No negative IDs or complex indexing
✅ **Debuggable**: Can inspect supplier name

### Disadvantages

✗ **Requires schema changes**: Products need `supplierName` field
✗ **String comparisons**: Less efficient than integer IDs
✗ **Matching complexity**: Must handle name normalization

---

## Recommendation for Option 3

**Use Solution 1: Temporary Negative IDs** because:

1. ✅ **Simplest to implement**: 2-3 hours
2. ✅ **Minimal changes**: Only affects `cadastraFornecedores()` and `salvar()`
3. ✅ **Least intrusive**: Doesn't require model structure changes
4. ✅ **Works immediately**: Uses existing `idFornecedor` field
5. ✅ **Clear semantics**: Negative = temporary, positive = real
6. ✅ **Easy testing**: Can verify ID mapping

**Trade-off**: Uses negative IDs which feels like a hack, but is pragmatic and works.

---

## Complete Implementation: Option 3 with Temporary IDs

### Step 1: Update Header File

```cpp
// importaprodutos.h

class ImportaProdutos : public QDialog {
private:
    struct PendingSupplier {
        int tempId;              // Negative ID
        QString razaoSocial;
        QVariant validadeProdutos;
    };

    QVector<PendingSupplier> pendingSuppliers;
    QMap<int, int> tempToRealSupplierIds;  // Maps temp → real
    int nextTempSupplierId = -1;

    // ... other existing members ...
};
```

### Step 2: Refactor cadastraFornecedores()

```cpp
void ImportaProdutos::cadastraFornecedores(QXlsx::Document &xlsx) {
    const int rows = xlsx.dimension().rowCount();
    QStringList fornecedores;
    int count = 0;

    // Extract unique suppliers
    for (int row = 2; row < rows; ++row) {
        const QString fornec = xlsx.readValue(row, 1).toString();
        if (not fornec.isEmpty()) { ++count; }
        if (fornec.isEmpty() or fornecedores.contains(fornec)) { continue; }
        fornecedores << xlsx.readValue(row, 1).toString();
    }

    progressDialog.setMaximum(count);

    // Check suppliers without modifying database
    for (auto const &fornecedor : qAsConst(fornecedores)) {
        m_fornecedor = fornecedor.left(100);

        // READ: Try to find existing supplier
        SqlQuery queryFornecedor;
        queryFornecedor.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
        queryFornecedor.bindValue(":razaoSocial", m_fornecedor);

        int idFornecedor = -1;

        if (queryFornecedor.exec() && queryFornecedor.first()) {
            // EXISTS: Use real ID
            idFornecedor = queryFornecedor.value("idFornecedor").toInt();
        } else {
            // NEW: Queue for creation with temp ID
            idFornecedor = nextTempSupplierId;
            nextTempSupplierId--;

            PendingSupplier pending;
            pending.tempId = idFornecedor;
            pending.razaoSocial = m_fornecedor;
            pending.validadeProdutos = (validade == -1) ? QVariant()
                                                        : qApp->serverDate().addDays(validade);
            pendingSuppliers.append(pending);
        }

        // Store mapping (real or temporary ID)
        m_fornecedores.insert(fornecedor.left(100), idFornecedor);
    }
}
```

### Step 3: Keep Phase 1 Functions Mostly Unchanged

`verificaSeRepresentacao()`, `marcaTodosProdutosDescontinuados()`, `mostraApenasEstesFornecedores()` don't need changes - they work with queried data.

**Except**: They should NOT persist anything. They're just reading:

```cpp
// This is already read-only, no changes needed
void ImportaProdutos::verificaSeRepresentacao() {
    SqlQuery queryFornecedor;
    queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
    // ... existing code ...
}

// This needs to be DEFERRED (don't execute it)
void ImportaProdutos::marcaTodosProdutosDescontinuados() {
    // COMMENTED OUT - will do this in salvar()
    /*
    SqlQuery query;
    if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE ...")) {
        throw RuntimeException(...);
    }
    */

    // Instead, just store which suppliers had their products marked
    // (This will be done atomically in Phase 3)
}
```

### Step 4: Defer marcaTodosProdutosDescontinuados()

```cpp
// Store state needed for Phase 3
private:
    QStringList suppliersToMarkDiscontinued;
    int tipoForDiscontinued;

void ImportaProdutos::processarArquivo() {
    // ... existing code ...

    // Store which suppliers and type (instead of executing)
    suppliersToMarkDiscontinued = idsFornecedor;
    tipoForDiscontinued = static_cast<int>(tipo);

    // Don't call marcaTodosProdutosDescontinuados()
    // Will do it in Phase 3

    // ... rest of code ...
}
```

### Step 5: Update salvar() with Complete Atomic Execution

```cpp
void ImportaProdutos::salvar() {
    // ALL database operations happen here in single transaction

    // Step 1: Mark old products discontinued (deferred from Phase 1)
    SqlQuery queryDiscontinued;
    if (not queryDiscontinued.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" +
                                    suppliersToMarkDiscontinued + ") AND estoque = FALSE AND promocao = " +
                                    QString::number(tipoForDiscontinued))) {
        throw RuntimeException("Erro marcando produtos descontinuados: " + queryDiscontinued.lastError().text());
    }

    // Step 2: Create new suppliers
    for (const auto& pending : pendingSuppliers) {
        SqlQuery queryInsert;
        queryInsert.prepare("INSERT INTO fornecedor (razaoSocial, validadeProdutos) VALUES (:razaoSocial, :validade)");
        queryInsert.bindValue(":razaoSocial", pending.razaoSocial);
        queryInsert.bindValue(":validade", pending.validadeProdutos);

        if (not queryInsert.exec()) {
            throw RuntimeException("Erro cadastrando fornecedor: " + queryInsert.lastError().text());
        }

        int realId = queryInsert.lastInsertId().toInt();
        tempToRealSupplierIds[pending.tempId] = realId;
    }

    // Step 3: Resolve temporary IDs to real IDs in products
    for (int row = 0; row < modelProduto.rowCount(); ++row) {
        int tempId = modelProduto.data(row, "idFornecedor").toInt();

        if (tempId < 0 && tempToRealSupplierIds.contains(tempId)) {
            int realId = tempToRealSupplierIds[tempId];
            modelProduto.setData(row, "idFornecedor", realId);
        }
    }

    // Step 4: Submit all products
    modelProduto.submitAll();

    // Step 5: Insert price records
    SqlQuery queryPrecos;
    if (validade != -1) {
        queryPrecos.prepare(
            "INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) "
            "SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim "
            "FROM produto WHERE atualizarTabelaPreco = TRUE");
        queryPrecos.bindValue(":validadeInicio", qApp->serverDate().toString("yyyy-MM-dd"));
        queryPrecos.bindValue(":validadeFim", validadeString);

        if (not queryPrecos.exec()) {
            throw RuntimeException("Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text());
        }
    }

    // Step 6: Update flags
    if (not queryPrecos.exec("UPDATE produto SET atualizarTabelaPreco = FALSE")) {
        throw RuntimeException("Erro comunicando com banco de dados: " + queryPrecos.lastError().text());
    }

    // Step 7: Call stored procedure
    SqlQuery queryExpirar;
    if (not queryExpirar.exec("CALL invalidar_produtos_expirados()")) {
        throw RuntimeException("Erro executando invalidar_produtos_expirados: " + queryExpirar.lastError().text());
    }

    // Step 8: Update stock prices
    atualizaPrecoEstoque();

    // All done - transaction will commit when this function returns
}
```

### Step 6: Update importarTabela()

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // Phase 1: No transaction - just read and prepare
        processarArquivo();

        // Phase 2: Dialog shown - still no transaction

    } catch (std::exception &) {
        close();
        throw;
    }
}
```

### Step 7: Update on_pushButtonSalvar_clicked()

```cpp
void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        QMessageBox msgBox(QMessageBox::Question, "Atenção!",
                          "Produtos com erro não serão salvos. Deseja continuar?",
                          QMessageBox::Yes | QMessageBox::No, this);
        msgBox.button(QMessageBox::Yes)->setText("Continuar");
        msgBox.button(QMessageBox::No)->setText("Voltar");

        if (msgBox.exec() == QMessageBox::No) { return; }
    }

    try {
        // Single transaction wrapping ALL writes
        qApp->startTransaction("ImportaProdutos::salvar");

        try {
            salvar();
            qApp->endTransaction();  // COMMIT everything
        } catch (std::exception &) {
            qApp->rollbackTransaction("");  // ROLLBACK everything
            throw;
        }
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}
```

---

## Testing Strategy for Option 3

### Unit Tests

```cpp
void testTempIdResolution() {
    // Verify temporary IDs correctly map to real IDs
    QCOMPARE(tempToRealSupplierIds[-1], 100);
    QCOMPARE(tempToRealSupplierIds[-2], 101);
}

void testMixedSupplierIds() {
    // Products should have mix of real and temporary IDs after Phase 1
    // Real supplier: idFornecedor = 42
    // Temp supplier: idFornecedor = -1

    // After Phase 3, all should be real
    // Supplier 42: unchanged
    // Supplier -1: → 100
}

void testNewSupplierCreation() {
    // Verify pending suppliers inserted with correct names
    // Verify IDs assigned sequentially
}

void testCancelRollsBackEverything() {
    // Start import, cancel before save
    // Verify:
    //   - No new suppliers created
    //   - No products inserted
    //   - No products marked discontinued
}
```

### Integration Tests

```cpp
void testFullImportWithNewSuppliers() {
    // Import Excel with:
    //   - 2 new suppliers
    //   - 1 existing supplier
    //   - 10 products across them

    // Verify:
    //   - Suppliers created with correct names
    //   - Products assigned correct supplier IDs
    //   - All atomically committed
}

void testProductSupplierIntegrity() {
    // After import, verify FK constraints:
    // All products.idFornecedor reference valid fornecedor.idFornecedor
}
```

---

## Comparison: Solutions for Option 3

| Aspect | Solution 1 (Negative IDs) | Solution 2 (Queue Index) | Solution 3 (Name Map) |
|--------|--------------------------|-------------------------|----------------------|
| **Complexity** | Simple | Moderate | Moderate |
| **Implementation Time** | 2-3 hours | 4 hours | 3-4 hours |
| **Code Changes** | Minimal | Moderate | Moderate |
| **Testability** | Easy | Moderate | Moderate |
| **Debugging** | Harder (magic IDs) | Moderate | Easier (names) |
| **Performance** | Best | Best | Slower (string compare) |
| **Maintainability** | Fair | Fair | Good |
| **Risk Level** | Low | Medium | Low |

**Recommended**: **Solution 1 (Temporary Negative IDs)** - Best balance of simplicity and effectiveness.

---

## Complete Example: Import with New Suppliers

### Scenario

```
Excel file contains:
  - Row 2: "NEW SUPPLIER 1" | "Product A" | ...
  - Row 3: "NEW SUPPLIER 1" | "Product B" | ...
  - Row 4: "EXISTING" | "Product C" | ...
  - Row 5: "NEW SUPPLIER 2" | "Product D" | ...

Database has:
  - Supplier "EXISTING" with idFornecedor = 42
  - No suppliers "NEW SUPPLIER 1" or "NEW SUPPLIER 2"
```

### Execution

**Phase 1: processarArquivo() - NO TRANSACTION**

```
Read suppliers:
  "NEW SUPPLIER 1" → Not in DB → tempId = -1, queue for creation
  "EXISTING" → In DB → realId = 42, use as-is
  "NEW SUPPLIER 2" → Not in DB → tempId = -2, queue for creation

m_fornecedores = {
  "NEW SUPPLIER 1" → -1,
  "EXISTING" → 42,
  "NEW SUPPLIER 2" → -2
}

pendingSuppliers = [
  {tempId: -1, razaoSocial: "NEW SUPPLIER 1", ...},
  {tempId: -2, razaoSocial: "NEW SUPPLIER 2", ...}
]

Products in modelProduto:
  Row 0: idFornecedor = -1, nome = "Product A"
  Row 1: idFornecedor = -1, nome = "Product B"
  Row 2: idFornecedor = 42, nome = "Product C"
  Row 3: idFornecedor = -2, nome = "Product D"

User reviews dialog for 10 minutes...
(No database locks held!)
```

**Phase 3: salvar() - SINGLE TRANSACTION**

```
Step 1: Mark old products discontinued
  UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (...) ...

Step 2: Create suppliers
  INSERT INTO fornecedor (razaoSocial) VALUES ("NEW SUPPLIER 1") → idFornecedor = 100
  INSERT INTO fornecedor (razaoSocial) VALUES ("NEW SUPPLIER 2") → idFornecedor = 101

  tempToRealSupplierIds = {-1 → 100, -2 → 101}

Step 3: Resolve temp IDs in products
  Row 0: idFornecedor = -1 → 100
  Row 1: idFornecedor = -1 → 100
  Row 2: idFornecedor = 42 (unchanged)
  Row 3: idFornecedor = -2 → 101

Step 4: INSERT all products
  INSERT product (idFornecedor=100, nome="Product A", ...)
  INSERT product (idFornecedor=100, nome="Product B", ...)
  INSERT product (idFornecedor=42, nome="Product C", ...)
  INSERT product (idFornecedor=101, nome="Product D", ...)

Step 5-8: Insert prices, update flags, call procedures, etc.

COMMIT transaction

Result:
  - Suppliers 100 and 101 created
  - All 4 products inserted with correct supplier IDs
  - All atomically committed
  - No possibility of partial inserts
```

---

## Summary

**To handle supplier ID resolution in Option 3**:

1. **Use temporary negative IDs** during Phase 1 for new suppliers
2. **Queue new suppliers** to be created in Phase 3
3. **Resolve IDs** at save time by:
   - Creating suppliers (get real IDs)
   - Building temp→real mapping
   - Updating products with correct IDs
   - Submitting all atomically

**Result**:
- Clean separation of read (Phase 1) from write (Phase 3)
- New suppliers don't exist until user confirms save
- All operations atomic
- User can cancel without side effects
- Zero database locks except during final save (100-500ms)

**Estimated Implementation**: 4-6 hours (vs 1-2 hours for Option 2)
**Complexity**: High (but manageable)
**Effort Worth**: Only if strict requirement that cancel = no DB changes
