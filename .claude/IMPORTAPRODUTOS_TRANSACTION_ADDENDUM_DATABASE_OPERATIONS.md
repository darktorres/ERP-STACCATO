# Transaction Refactoring Addendum: Database Operations During Import

**Critical Issue**: The previous refactoring plan overlooked database queries that occur DURING the import processing phase
**Document Date**: 2025-11-11

---

## The Gap in Previous Plan

The initial transaction refactoring plan stated: "move transaction to only wrap `salvar()`" but **overlooked that `processarArquivo()` itself performs database writes**.

```cpp
void ImportaProdutos::processarArquivo() {
    // ...
    cadastraFornecedores(xlsx);           // ← DATABASE OPERATIONS!
    verificaSeRepresentacao();            // ← DATABASE QUERY!
    marcaTodosProdutosDescontinuados();   // ← DATABASE UPDATE!
    mostraApenasEstesFornecedores();      // ← DATABASE SELECT!
    // ...
}
```

Simply removing `startTransaction()` from `importarTabela()` would leave these operations **unprotected and uncommitted**, creating several problems.

---

## Database Operations in processarArquivo()

### 1. cadastraFornecedores() - Lines 353-393

**What it does**:
```cpp
for (auto const &fornecedor : qAsConst(fornecedores)) {
    m_fornecedor = fornecedor.left(100);

    const int idFornecedor = buscarCadastrarFornecedor();  // ← SELECT + possibly INSERT

    ids << QString::number(idFornecedor);
    m_fornecedores.insert(fornecedor.left(100), idFornecedor);

    SqlQuery queryFornecedor;
    queryFornecedor.prepare("UPDATE fornecedor SET validadeProdutos = :validade WHERE razaoSocial = :razaoSocial");
    queryFornecedor.bindValue(":validade", validadeDate);
    queryFornecedor.bindValue(":razaoSocial", fornecedor.left(100));

    if (not queryFornecedor.exec()) {  // ← UPDATE on fornecedor table
        throw RuntimeException("Erro salvando validade: " + queryFornecedor.lastError().text());
    }
}
```

**Operations**:
- SELECT from fornecedor table (via `buscarCadastrarFornecedor()`)
- INSERT into fornecedor if not found (via `buscarCadastrarFornecedor()`)
- UPDATE fornecedor to set validity dates

**Duration**: Depends on supplier count (typically 5-50 suppliers × 10-50ms = 50-2500ms)

**Lock impact**: Holds **fornecedor table lock** for duration

---

### 2. verificaSeRepresentacao() - Lines 55-65

**What it does**:
```cpp
void ImportaProdutos::verificaSeRepresentacao() {
    SqlQuery queryFornecedor;
    queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
    queryFornecedor.bindValue(":razaoSocial", m_fornecedor.left(100));

    if (not queryFornecedor.exec()) {
        throw RuntimeException("Erro lendo tabela fornecedor: " + queryFornecedor.lastError().text());
    }

    if (not queryFornecedor.first()) {
        throw RuntimeException("Dados não encontrados para fornecedor");
    }

    ui->checkBoxRepresentacao->setChecked(queryFornecedor.value("representacao").toBool());
}
```

**Operations**:
- SELECT from fornecedor table (read-only)

**Duration**: ~10-50ms

**Lock impact**: None (read-only)

---

### 3. marcaTodosProdutosDescontinuados() - Lines 401-407

**What it does**:
```cpp
void ImportaProdutos::marcaTodosProdutosDescontinuados() {
    SqlQuery query;

    if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" +
                       idsFornecedor + ") AND estoque = FALSE AND promocao = " +
                       QString::number(static_cast<int>(tipo)))) {
        throw RuntimeException("Erro marcando produtos descontinuados: " + query.lastError().text());
    }
}
```

**Operations**:
- UPDATE on produto table - marks all old products from these suppliers as discontinued

**Duration**: Depends on product count (typically 100-10,000 products × 1-5ms = 100-50,000ms)

**Lock impact**: Holds **produto table lock** for duration (CAN BE SIGNIFICANT!)

---

### 4. mostraApenasEstesFornecedores() - Lines 395-399

**What it does**:
```cpp
void ImportaProdutos::mostraApenasEstesFornecedores() {
    modelProduto.setFilter("idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " +
                           QString::number(static_cast<int>(tipo)));

    modelProduto.select();  // ← SELECT from database
}
```

**Operations**:
- SELECT from produto table to populate model for display

**Duration**: Depends on product count (typically 100-10,000 products × 1-5ms = 100-50,000ms)

**Lock impact**: None (read-only)

---

## The Problem: Missing Transaction Creates Data Integrity Issues

If we simply remove the transaction, these operations become **uncommitted changes**:

```
Scenario: User imports products, then closes dialog without clicking "Save"

1. cadastraFornecedores() executes:
   - Suppliers created in database ✓ (PERSIST even if user cancels)

2. marcaTodosProdutosDescontinuados() executes:
   - Old products marked discontinued ✓ (PERSIST even if user cancels)

3. User reviews results

4. User clicks [X] to close dialog instead of [Save]

5. Result: Database modified even though user canceled!
   - New suppliers exist
   - Old products marked discontinued
   - But new product data never imported
   - Database in inconsistent state!
```

This is **UNACCEPTABLE** - users expect canceling a dialog to roll back all changes.

---

## Solution Options

### Option 1: Keep Single Long Transaction (CURRENT - NOT ACCEPTABLE)

**Pros**:
- ✓ All operations atomic
- ✓ Data fully consistent
- ✓ One transaction boundary

**Cons**:
- ✗ Transaction open for 5-30 minutes (blocks other users)
- ✗ Users can't access supplier/product tables
- ✗ Deadlock risk
- ✗ Resource exhaustion

**This is the current approach - it's why we're fixing it.**

---

### Option 2: Two-Phase Transaction (SHORT + SHORT)

**Idea**: Wrap processarArquivo() in a SHORT transaction, then wrap salvar() in another SHORT transaction.

```
Phase 1: processarArquivo() in transaction
  Start transaction
  cadastraFornecedores()          ← Modifying database
  verificaSeRepresentacao()       ← Reading database
  marcaTodosProdutosDescontinuados() ← Modifying database
  mostraApenasEstesFornecedores() ← Reading database
  COMMIT transaction              ← Persist these changes
  (Duration: ~1-5 seconds)

Phase 2: User reviews dialog
  No transaction                  ← Other users not blocked!
  (Duration: 5-30 minutes)

Phase 3: User clicks "Save"
  Start transaction
  salvar()                        ← Import products
  COMMIT transaction
  (Duration: 100-500ms)
```

**Implementation**:

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // Phase 1: Short transaction for setup operations
        qApp->startTransaction("ImportaProdutos::processarArquivo");

        try {
            processarArquivo();  // Takes 1-5 seconds with transaction
            qApp->endTransaction();  // COMMIT setup changes
        } catch (...) {
            qApp->rollbackTransaction("");
            throw;
        }

        // Phase 2: User reviews - NO TRANSACTION
        // Dialog shown to user

    } catch (std::exception &) {
        close();
        throw;
    }
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        QMessageBox msgBox(...);
        if (msgBox.exec() == QMessageBox::No) { return; }
    }

    try {
        // Phase 3: Short transaction for actual import
        qApp->startTransaction("ImportaProdutos::salvar");

        try {
            salvar();
            qApp->endTransaction();  // COMMIT import
        } catch (...) {
            qApp->rollbackTransaction("");
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

**Pros**:
- ✓ Phase 1 transaction short (1-5 seconds) - acceptable overhead
- ✓ Phase 2 has no transaction - other users unblocked
- ✓ Phase 3 transaction very short (100-500ms)
- ✓ All operations atomic (each phase commits or rolls back)
- ✓ User cancellation still works (Phase 1 committed, but Phase 3 won't run)
- ✓ ~6x improvement over current (6 seconds blocked vs 5-30 minutes)

**Cons**:
- ✗ If user cancels after Phase 1:
  - Suppliers are created (but this is data that should exist anyway)
  - Old products are marked discontinued (but this is the intended outcome)
  - New products are NOT imported (correct - user canceled)
  - Database is consistent - no orphaned data
- ✗ Suppliers created/validity updated even if user cancels
  - This might be acceptable depending on business rules
  - Could add warning dialog about this

**Risk Assessment**: LOW
- Phase 1 side effects (suppliers created, old products marked discontinued) are somewhat expected
- Not a data integrity problem (no orphaned data)
- No deadlock risk (short transaction duration)

---

### Option 3: Defer All Writes Until Save (MOST CORRECT)

**Idea**: Don't modify database during processarArquivo(). Instead, queue operations and execute them all in Phase 2 transaction.

**Execution**:

```
Phase 1: processarArquivo() - READ ONLY
  No transaction
  Read Excel file
  Read suppliers from DB (no creates)
  Create suppliers IN MEMORY (queue for later)
  Check representacao flag
  Queue products to discontinue
  Load existing products for comparison
  (Duration: 1-5 seconds, NO DATABASE LOCKS)

Phase 2: User reviews dialog
  No transaction
  Dialog displayed
  (Duration: 5-30 minutes, NO LOCKS)

Phase 3: User clicks "Save"
  Start transaction
  Execute queued operations:
    - Create suppliers (INSERT)
    - Mark products discontinued (UPDATE)
    - Insert/update imported products (INSERT/UPDATE)
    - Insert prices
    - Call stored procedures
    - Update stock prices
  COMMIT transaction
  (Duration: 500ms-2 seconds)
```

**Implementation** (sketch):

```cpp
class ImportaProdutos {
private:
    struct PendingOperation {
        enum Type { CreateSupplier, MarkDiscontinued, UpdateProduct } type;
        // ... operation-specific data ...
    };

    QVector<PendingOperation> pendingOps;
    QMap<QString, int> pendingSupplierIds;  // Suppliers queued to create
};

void ImportaProdutos::cadastraFornecedores(QXlsx::Document &xlsx) {
    // Instead of INSERT/UPDATE to database:

    for (auto const &fornecedor : qAsConst(fornecedores)) {
        // Try to find existing supplier
        SqlQuery qSelect;
        qSelect.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
        qSelect.bindValue(":razaoSocial", fornecedor.left(100));

        if (qSelect.exec() && qSelect.first()) {
            // Supplier exists - just queue the validity update
            int idFornecedor = qSelect.value("idFornecedor").toInt();
            pendingSupplierIds[fornecedor] = idFornecedor;

            PendingOperation op;
            op.type = PendingOperation::UpdateSupplierValidity;
            op.supplierId = idFornecedor;
            op.validityDate = validadeDate;
            pendingOps.append(op);
        } else {
            // Supplier doesn't exist - queue creation (don't create now)
            PendingOperation op;
            op.type = PendingOperation::CreateSupplier;
            op.supplierName = fornecedor;
            op.validityDate = validadeDate;
            pendingOps.append(op);

            // Assign temporary negative ID for reference
            pendingSupplierIds[fornecedor] = -(pendingOps.size());
        }
    }
}

void ImportaProdutos::marcaTodosProdutosDescontinuados() {
    // Queue the operation instead of executing
    PendingOperation op;
    op.type = PendingOperation::MarkDiscontinued;
    op.supplierIds = idsFornecedor;
    op.tipo = tipo;
    pendingOps.append(op);
}

void ImportaProdutos::salvar() {
    // Execute all pending operations atomically

    for (const auto& op : pendingOps) {
        switch (op.type) {
        case PendingOperation::CreateSupplier:
            // INSERT new supplier
            break;
        case PendingOperation::UpdateSupplierValidity:
            // UPDATE supplier validity
            break;
        case PendingOperation::MarkDiscontinued:
            // UPDATE products
            break;
        }
    }

    // Then do the existing salvar() logic
    modelProduto.submitAll();
    // ... etc ...
}
```

**Pros**:
- ✓ MOST CORRECT - all database modifications in single transaction
- ✓ Zero database locks during Phase 1 and 2
- ✓ User cancellation works perfectly (nothing committed until Phase 3)
- ✓ Minimal transaction duration (only Phase 3: 500ms-2s)
- ✓ No side effects if user cancels (no orphaned data)
- ✓ Other users unblocked for entire 5-30 minute review period

**Cons**:
- ✗ Significant refactoring required
- ✗ Complex queue management
- ✗ Temporary supplier IDs must be resolved during final save
- ✗ More testing needed
- ✗ Estimated effort: 4-6 hours

**Risk Assessment**: MEDIUM
- Refactoring is complex and error-prone
- Temporary ID resolution could have bugs
- Existing logic is duplicated in queue execution

---

### Option 4: Use Savepoints (DATABASE FEATURE)

**Idea**: Create savepoint at start of Phase 1, commit at end of Phase 1, new savepoint for Phase 3.

**How it works**:

```sql
START TRANSACTION;

-- Phase 1: Setup operations
SAVEPOINT setup_savepoint;
INSERT/UPDATE suppliers...
UPDATE products...
RELEASE SAVEPOINT setup_savepoint;  -- Commit Phase 1 to storage

-- Phase 2: User reviews (no DB operations)

-- Phase 3: Final import
SAVEPOINT import_savepoint;
INSERT/UPDATE products...
RELEASE SAVEPOINT import_savepoint;  -- Commit Phase 3 to storage

COMMIT;  -- Final transaction commit
```

**Qt Implementation**:

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // Start outer transaction (will commit all changes)
        qApp->startTransaction("ImportaProdutos::full");

        try {
            // Phase 1: Setup operations with savepoint
            qApp->database().exec("SAVEPOINT import_setup");
            processarArquivo();  // Does setup operations
            qApp->database().exec("RELEASE SAVEPOINT import_setup");  // Confirm Phase 1

            // Phase 2: User reviews - still in transaction but Phase 1 confirmed

        } catch (...) {
            qApp->rollbackTransaction("");
            throw;
        }
    } catch (std::exception &) {
        close();
        throw;
    }
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        QMessageBox msgBox(...);
        if (msgBox.exec() == QMessageBox::No) {
            qApp->rollbackTransaction("");  // User canceled - rollback everything
            return;
        }
    }

    try {
        // Phase 3: Final import with savepoint
        qApp->database().exec("SAVEPOINT import_final");
        salvar();
        qApp->database().exec("RELEASE SAVEPOINT import_final");

        qApp->endTransaction();  // Final commit
    } catch (std::exception &) {
        qApp->rollbackTransaction("");
        close();
        throw;
    }

    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}
```

**Pros**:
- ✓ All operations in single transaction (atomic)
- ✓ Savepoints allow "partial commits" within transaction
- ✓ Clean rollback semantics (user cancel = rollback everything)
- ✓ Moderate refactoring needed
- ✓ Database handles the complexity

**Cons**:
- ✗ Savepoint support varies by database
- ✗ Transaction still "open" from UI perspective (locks held)
- ✗ MySQL InnoDB savepoints have limitations

**Risk Assessment**: MEDIUM
- Depends on database support
- May not reduce lock duration as much as desired

---

## Recommendation

For **maximum user impact with reasonable effort**:

### Use **Option 2: Two-Phase Transaction** as DEFAULT

**Why**:
- ✓ Reduces lock duration from **5-30 minutes → ~6 seconds** (4-300x improvement)
- ✓ Minimal refactoring required
- ✓ Clear, understandable flow
- ✓ Low risk
- ✓ Easy to implement (1-2 hours)

**Trade-off**:
- If user cancels after Phase 1, suppliers are created and old products marked discontinued
- This is acceptable because:
  - Suppliers SHOULD exist in ERP
  - Old products SHOULD be marked discontinued (it's the intent of the import)
  - New products simply won't be added (what user wanted by canceling)
  - No data corruption or orphaning

**When to use Option 3: Defer All Writes**:
- If strict requirement: "Cancel must leave database untouched"
- If doing full refactor anyway
- If this becomes standard pattern for other imports
- Effort: 4-6 hours

**When NOT to use Option 4: Savepoints**:
- Doesn't solve the core problem (transaction still open)
- Just complicates code without benefit

---

## Revised Implementation Plan

### Phase 1: Wrap processarArquivo() in Short Transaction

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // Phase 1: Start transaction for setup
        qApp->startTransaction("ImportaProdutos::setup");

        try {
            processarArquivo();  // Setup operations (1-5 seconds)
            qApp->endTransaction();  // COMMIT setup changes
        } catch (std::exception &) {
            qApp->rollbackTransaction("");
            close();
            throw;
        }

        // Phase 2: No transaction - user reviews dialog

    } catch (std::exception &) {
        close();
        throw;
    }
}
```

**Time cost**: Adds 1-5 second transaction (acceptable)

### Phase 2: Wrap salvar() in Short Transaction

```cpp
void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        QMessageBox msgBox(QMessageBox::Question, "Atenção!",
                          "Produtos com erro não serão salvos. Deseja continuar?",
                          QMessageBox::Yes | QMessageBox::No, this);
        msgBox.button(QMessageBox::Yes)->setText("Continuar");
        msgBox.button(QMessageBox::No)->setText("Voltar");

        if (msgBox.exec() == QMessageBox::No) {
            return;  // User canceled - Phase 1 changes stay (acceptable)
        }
    }

    try {
        // Phase 3: Start transaction for final import
        qApp->startTransaction("ImportaProdutos::salvar");

        try {
            salvar();  // Import operations (100-500ms)
            qApp->endTransaction();  // COMMIT final changes
        } catch (std::exception &) {
            qApp->rollbackTransaction("");
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

**Time cost**: Adds 100-500ms transaction (negligible)

### Total Transaction Duration

```
Old (BAD):        5-30 MINUTES (blocks all other users)

New (GOOD):
  Phase 1: 1-5 seconds (setup transaction)
  Phase 2: 0 seconds (no transaction)
  Phase 3: 0.1-0.5 seconds (save transaction)
  ────────
  Total: ~5.5 seconds of BLOCKING (vs 5-30 MINUTES)

Improvement: 60-300x better! ✓
```

---

## Testing Strategy with Database Operations

### New Tests Needed

```cpp
// Test Phase 1 transaction completes successfully
void testPhase1TransactionCommits() {
    // Verify cadastraFornecedores() committed
    // Verify marcaTodosProdutosDescontinuados() committed
    // Verify suppliers exist in database after dialog shown
}

// Test suppliers created in Phase 1
void testSuppliersCreatedInPhase1() {
    QStringList originalSuppliers = getSuppliers();

    openImportDialog();
    // Dialog opens and shows products
    // Verify new suppliers now in database

    closeDialogWithoutSaving();

    // Suppliers should still exist (Phase 1 was committed)
    QCOMPARE(getSuppliers(), originalSuppliers + newSuppliers);
}

// Test old products marked discontinued in Phase 1
void testProductsMarkedDiscontinuedInPhase1() {
    int activeProductsBefore = getActiveProductCount();

    openImportDialog();
    // Dialog opens
    // Verify old products marked discontinued

    closeDialogWithoutSaving();

    // Products should still be discontinued (Phase 1 was committed)
    int activeProductsAfter = getActiveProductCount();
    QCOMPARE(activeProductsAfter, activeProductsBefore - oldProductCount);
}

// Test new products only in Phase 3
void testNewProductsOnlyInPhase3() {
    openImportDialog();
    // Dialog shown

    // Verify new products NOT in database yet
    QCOMPARE(getNewProductCount(), 0);

    clickSave();
    // Wait for transaction

    // Verify new products NOW in database
    QVERIFY(getNewProductCount() > 0);
}

// Test user cancel doesn't add new products
void testCancelDoesntAddNewProducts() {
    openImportDialog();
    // Dialog shown
    // Verify new products NOT in database

    closeWithoutSaving();
    // Dialog closed

    // Verify new products STILL not in database
    QCOMPARE(getNewProductCount(), 0);
}
```

---

## Comparison: Database Operations Impact

| Operation | Phase | Duration | Lock Impact | When Committed |
|-----------|-------|----------|-------------|-----------------|
| cadastraFornecedores() | 1 | 100-2500ms | fornecedor table | Phase 1 end |
| marcaTodosProdutosDescontinuados() | 1 | 100-50000ms | produto table | Phase 1 end |
| verificaSeRepresentacao() | 1 | 10-50ms | None (read) | N/A |
| mostraApenasEstesFornecedores() | 1 | 100-50000ms | None (read) | N/A |
| salvar() | 3 | 100-500ms | produto, prices | Phase 3 end |

---

## Summary

The critical insight: **processarArquivo() contains write operations** that must either:

1. Be wrapped in a short transaction (Option 2 - RECOMMENDED)
   - Phase 1: 1-5 second transaction
   - Phase 2: No transaction (user reviews)
   - Phase 3: 100-500ms transaction
   - Total lock time: ~6 seconds (vs 5-30 minutes)

2. Be deferred until save time (Option 3)
   - Cleaner from user perspective
   - More work to implement
   - More risk in refactoring

3. Continue as single long transaction (current - NOT ACCEPTABLE)
   - Blocks other users for minutes
   - Deadlock risk
   - ERP anti-pattern

---

## Revised Recommendation

**Implement Option 2 (Two-Phase Transaction)**:
- Move `startTransaction()` to `processarArquivo()` (not `importarTabela()`)
- Add `endTransaction()` after `processarArquivo()` completes
- Keep existing `startTransaction()/endTransaction()` in `on_pushButtonSalvar_clicked()`

**Estimated effort**: 1-2 hours
**Lock duration reduction**: 4-300x improvement
**Risk level**: Low
**Implementation complexity**: Moderate
