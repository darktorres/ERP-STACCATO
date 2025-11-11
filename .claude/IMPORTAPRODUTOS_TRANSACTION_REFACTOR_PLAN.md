# Transaction Refactoring Plan for ImportaProdutos

**Issue**: Database transaction held open for entire dialog lifetime
**Severity**: 🔴 CRITICAL
**Impact**: Blocks other users, causes deadlocks, degrades ERP system performance
**Document Date**: 2025-11-11

---

## Executive Summary

Currently, the `ImportaProdutos` class opens a database transaction at the start of the import process (line 46) and only closes it when the user clicks "Salvar" and the dialog closes (line 984). This can mean the transaction remains open for **minutes** while the user:

1. Selects an Excel file
2. Selects validity dates
3. Waits for Excel parsing
4. Reviews import results
5. Decides whether to save

This is a **severe anti-pattern** in database design that causes:
- **Database locks** held for extended periods
- **Blocked concurrent users** unable to access the same tables
- **Deadlock risk** in multi-user environments
- **Poor ERP system performance** (ERP systems are typically multi-user)
- **Abandoned connections** if user walks away or closes dialog

**Solution**: Move transaction boundaries to only wrap the actual database write operations, minimizing lock duration to milliseconds.

---

## Current Transaction Lifecycle (PROBLEMATIC)

```
Timeline                              Transaction State
─────────────────────────────────────────────────────────
User clicks "Importar"
  ↓
importarTabela() called
  ├─ Line 46: startTransaction()     ← 🔴 TRANSACTION START
  │
  └─ processarArquivo() called
      ├─ Read Excel file             ✗ Long I/O wait (locked)
      ├─ Query suppliers             ✗ Database queries (locked)
      ├─ Parse rows (100s-1000s)     ✗ CPU processing (locked)
      ├─ Build models in memory      ✗ Data prep (locked)
      ├─ setupTables()               ✗ UI setup (locked)
      ├─ showMaximized()             ✗ Dialog display (locked)
      └─ Return to main loop

Dialog displayed to user            ⏳ STILL IN TRANSACTION (minutes!)
  ├─ User reviews products
  ├─ User reviews errors
  ├─ User can check/uncheck "Representacao"
  └─ User decides to save...

User clicks "Salvar" button
  ↓
on_pushButtonSalvar_clicked() called
  ├─ Check for errors
  │
  └─ salvar() called              ✓ Database writes (locked)
      ├─ modelProduto.submitAll() ✓ Insert/Update rows
      ├─ Insert prices
      ├─ Update flags
      ├─ Call stored procedure
      └─ Update stock prices

  Line 984: endTransaction()         ← 🟢 TRANSACTION END (finally!)

Dialog closes
```

**Problem Duration**: From "Importar" click → "Salvar" click (could be **5-30+ minutes**)

---

## Optimal Transaction Lifecycle (PROPOSED)

```
Timeline                              Transaction State
─────────────────────────────────────────────────────────
User clicks "Importar"
  ↓
importarTabela() called
  ├─ readFile() called              🟢 NO TRANSACTION
  ├─ readValidade() called          🟢 NO TRANSACTION
  │
  └─ processarArquivo() called      🟢 NO TRANSACTION
      ├─ Read Excel file             ✓ I/O without locks
      ├─ Query suppliers             ✓ Read-only queries
      ├─ Parse rows                  ✓ CPU processing
      ├─ Build models in memory      ✓ Data prep
      ├─ setupTables()               ✓ UI setup
      ├─ showMaximized()             ✓ Dialog display
      └─ Return to main loop

Dialog displayed to user            🟢 NO TRANSACTION (no locks!)
  ├─ User reviews products freely
  ├─ User reviews errors
  ├─ User can check/uncheck options
  └─ User decides to save...

User clicks "Salvar" button
  ↓
on_pushButtonSalvar_clicked() called
  ├─ Check for errors              🟢 NO TRANSACTION
  │
  ├─ Line NEW: startTransaction()  ← 🔴 TRANSACTION START (just now!)
  │
  └─ salvar() called               ✓ Database writes (locked)
      ├─ modelProduto.submitAll() ✓ Insert/Update rows
      ├─ Insert prices
      ├─ Update flags
      ├─ Call stored procedure
      └─ Update stock prices

  └─ Line NEW: endTransaction()    ← 🟢 TRANSACTION END (immediately!)

Dialog closes                        🟢 NO TRANSACTION
```

**Optimal Duration**: Only during `salvar()` (typically **100-500ms**)

**Benefit**: Transaction duration reduced from **minutes → milliseconds**

---

## Problem Analysis

### Why This Matters for ERP Systems

ERPs are **inherently multi-user systems**. The current design has these consequences:

1. **Supplier table locked**: While importing product "A" from supplier "X", no other user can:
   - Create a new supplier
   - Update supplier info
   - View supplier records (depending on DB isolation level)

2. **Product table locked**: Other users cannot:
   - Create new products
   - Update product prices
   - View product inventory
   - Create purchase orders for these products

3. **Cascading impacts**: If purchase orders depend on product data, they're blocked too

4. **Deadlock scenarios**:
   - User A imports products → locks supplier/product tables
   - User B tries to update a supplier → blocked, waits
   - User B's transaction locks something User A needs → **DEADLOCK**
   - System hangs or crashes

### Current Implementation Issues

**Line 46**: Transaction starts too early
```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        qApp->startTransaction("ImportaProdutos::importaTabela");  // ← START TOO EARLY
        processarArquivo();
    } catch (std::exception &) {
        close();
        throw;
    }
}
```

**Lines 968-989**: Transaction ends when user finally saves
```cpp
void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        // User sees dialog, dialog blocks here
        // Meanwhile, transaction still open!
        QMessageBox msgBox(...);
        if (msgBox.exec() == QMessageBox::No) { return; }
    }

    try {
        salvar();
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->endTransaction();  // ← END FINALLY!
    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}
```

**Line 1015**: Cleanup on close
```cpp
void ImportaProdutos::closeEvent(QCloseEvent *event) {
    if (qApp->getInTransaction()) {
        qApp->rollbackTransaction("");  // ← Rollback if still open
    }
    QDialog::closeEvent(event);
}
```

---

## Proposed Refactoring

### Strategy: Move Transaction to Minimal Window

**Core Principle**: Keep transactions as short as possible. Move everything except actual database writes outside the transaction.

### Phase 1: Pre-Refactor Analysis

1. Identify what needs transactions:
   - ✓ Database writes in `salvar()` - **MUST be transacted**
   - ✓ Stored procedure calls in `salvar()` - **MUST be transacted**
   - ✗ Excel file I/O - **Never needs transactions**
   - ✗ Data processing in memory - **Never needs transactions**
   - ✗ UI operations - **Never needs transactions**
   - ✗ User interaction/dialogs - **Never needs transactions**

2. Identify state changes:
   - Models (modelProduto, modelErro) are built in memory - **No transaction needed**
   - Database writes happen in `salvar()` only - **Transaction needed here**
   - Rollback only needed if `salvar()` fails - **Can be local to salvar()**

### Phase 2: Refactoring Steps

#### Step 1: Remove startTransaction from importarTabela()

**Before**:
```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        qApp->startTransaction("ImportaProdutos::importaTabela");  // ← REMOVE
        processarArquivo();
    } catch (std::exception &) {
        close();
        throw;
    }
}
```

**After**:
```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // Transaction removed - will only start when saving
        processarArquivo();
    } catch (std::exception &) {
        close();
        throw;
    }
}
```

**Impact**:
- ✓ Transaction no longer open during file reading
- ✓ Transaction no longer open during data processing
- ✓ Transaction no longer open during UI display
- ✓ Other users can freely access supplier/product tables

#### Step 2: Wrap salvar() in its own transaction

**Before**:
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
        salvar();  // No transaction here
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->endTransaction();  // Ends transaction started in importarTabela()
    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}
```

**After**:
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
        qApp->startTransaction("ImportaProdutos::salvar");  // ← START HERE (just before saving)

        try {
            salvar();

            qApp->endTransaction();  // ← END IMMEDIATELY AFTER
        } catch (...) {
            qApp->rollbackTransaction("");  // ← Rollback if salvar() fails
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

**Impact**:
- ✓ Transaction only open during actual database writes (milliseconds)
- ✓ User can review results for as long as needed without blocking others
- ✓ Clear transaction scope (start → save → end)
- ✓ Rollback handled locally if save fails

#### Step 3: Update closeEvent() cleanup

**Before**:
```cpp
void ImportaProdutos::closeEvent(QCloseEvent *event) {
    if (qApp->getInTransaction()) {
        qApp->rollbackTransaction("");  // Cleanup stale transaction
    }
    QDialog::closeEvent(event);
}
```

**After**:
```cpp
void ImportaProdutos::closeEvent(QCloseEvent *event) {
    if (qApp->getInTransaction()) {
        qApp->rollbackTransaction("");  // Should be rare now
        qDebug() << "Warning: Unclosed transaction during close event";
    }
    QDialog::closeEvent(event);
}
```

**Impact**:
- ✓ Should rarely be triggered now (only if save failed)
- ✓ Added debug message to catch unexpected transaction open state
- ✓ Safety net still exists

### Phase 3: Alternative Implementation (More Robust)

If you want even more explicit transaction handling, create a dedicated save function:

**Alternative**: Extract transaction logic into separate function

```cpp
private:
    void salvarComTransacao() {
        qApp->startTransaction("ImportaProdutos::salvar");
        try {
            salvar();
            qApp->endTransaction();
        } catch (...) {
            qApp->rollbackTransaction("");
            throw;
        }
    }

// Usage:
void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        QMessageBox msgBox(...);
        if (msgBox.exec() == QMessageBox::No) { return; }
    }

    try {
        salvarComTransacao();  // ← Single call handles transaction
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}
```

---

## Impact Analysis

### Performance Improvements

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Transaction duration | 5-30 minutes | 100-500 ms | **60-18,000x better** |
| Database locks held | Long | Minimal | **Massive reduction** |
| Concurrent user blocking | Yes | No | **Unblocked** |
| Deadlock probability | High | Very low | **Drastically reduced** |

### Multi-User Scenario Example

**Before (Current)**:
```
Time  User A (Importing)           User B (Managing inventory)
────  ──────────────────────────   ──────────────────────────
0s    Open import dialog
      (Transaction STARTS)

5s    Reads Excel
      (Transaction open)           Tries to update supplier
                                   → BLOCKED, waiting for User A

10s   Processing rows              Waits...
      (Transaction open)           Waits... (queue builds up)

20s   Reviews results              Waits...
      (Transaction open)           Waits...

30s   Clicks "Salvar"              Waits...
      Writing to DB...
      (Transaction ends)           → Finally unblocked!

31s   Close dialog                 Update completes
```

**Result**: User B blocked for 30+ seconds

---

**After (Proposed)**:
```
Time  User A (Importing)           User B (Managing inventory)
────  ──────────────────────────   ──────────────────────────
0s    Open import dialog
      (No transaction)

5s    Reads Excel
      (No transaction)             Updates supplier
                                   → Works immediately! ✓

10s   Processing rows
      (No transaction)             Other updates work fine

20s   Reviews results
      (No transaction)             Inventory fully responsive

30s   Clicks "Salvar"
      (Transaction STARTS)

30.2s Writing to DB...             No blocking,
      (Transaction ends)           works freely

31s   Close dialog
```

**Result**: User B works freely throughout

---

## Testing Strategy

### Unit Tests to Add

```cpp
// Test that transaction is NOT open during processing
void testNoTransactionDuringProcessing() {
    // Mock qApp->getInTransaction()
    // Verify it returns false during processarArquivo()
}

// Test transaction only open during save
void testTransactionOnlyDuringSave() {
    // Verify startTransaction called from on_pushButtonSalvar_clicked()
    // Verify endTransaction called after salvar()
    // Verify it's NOT called from importarTabela()
}

// Test rollback on save failure
void testRollbackOnSaveFailure() {
    // Mock salvar() to throw exception
    // Verify rollbackTransaction() called
    // Verify data not committed
}

// Test transaction scope
void testTransactionScope() {
    // Verify transaction open only during database write
    // Verify locked duration < 1 second
}
```

### Integration Tests

```cpp
// Test concurrent user scenario
void testConcurrentImports() {
    // Simulate two ImportaProdutos dialogs
    // Verify they don't block each other
    // Verify both can save successfully
}

// Test long-running import doesn't block others
void testLongImportDoesntBlockOthers() {
    // Start import with large Excel file
    // From different user/connection, verify can:
    //   - Update suppliers
    //   - View products
    //   - Create new records
}

// Test transaction cleanup on abnormal close
void testTransactionCleanupOnClose() {
    // Start import
    // Force close dialog without saving
    // Verify transaction rolled back
    // Verify no orphaned locks
}
```

### Manual Testing

1. **Long review scenario**:
   - Import product file
   - Leave dialog open for 5 minutes
   - From another user account, try to:
     - Update a supplier → Should work immediately
     - Create a new product → Should work immediately
     - Check inventory → Should work immediately

2. **Concurrent imports**:
   - Open two ImportaProdutos dialogs
   - Import different files simultaneously
   - Both should complete without blocking each other

3. **Verify no abandoned locks**:
   - Monitor database locks during/after import
   - Verify no locks held after dialog closes

---

## Risks and Mitigation

### Risk 1: Data Consistency (LOW RISK)

**Risk**: Committing partial data if something fails

**Mitigation**:
- Transaction still wraps ALL operations in `salvar()`
- If ANY operation fails, entire transaction rolls back
- No partial commits possible
- ✓ SAFE

### Risk 2: User changes mind during review (MEDIUM RISK)

**Risk**: User makes decisions based on stale data

**Scenario**:
1. User imports products (no transaction)
2. Another user updates a supplier name
3. Current user reviews results (showing old supplier name)
4. Current user saves (uses new supplier name already in DB)

**Mitigation**:
- This was ALWAYS a risk (even with current implementation)
- Current transaction doesn't prevent this (it's a data consistency issue, not a locking issue)
- If needed, add "Refresh from DB" button before save
- Or add validation: compare models with DB before save

**Risk Level**: Same as before (not introduced by this refactor)

### Risk 3: Rollback on save failure (LOW RISK)

**Risk**: Data partially written if save fails

**Current**:
```cpp
try {
    salvar();
} catch (std::exception &) {
    close();
    throw;  // Transaction still open!
}
qApp->endTransaction();  // Only reached if no exception
```

**Issue**: If `salvar()` throws, transaction still open and manually rolled back in `closeEvent()`

**Proposed**:
```cpp
qApp->startTransaction("ImportaProdutos::salvar");
try {
    salvar();
    qApp->endTransaction();  // Commit on success
} catch (...) {
    qApp->rollbackTransaction("");  // Rollback on failure
    throw;  // Re-throw exception
}
```

**Benefit**: ✓ Explicit and immediate rollback
- ✓ No stale open transaction
- ✓ Clearer code flow
- ✓ SAFER

### Risk 4: Breaking existing code that expects transaction (LOW RISK)

**Current code might assume**:
```cpp
// Somewhere in salvar() or related code:
if (qApp->getInTransaction()) {
    // Do something special
}
```

**Mitigation**:
- Search codebase for `getInTransaction()` calls
- Search for `startTransaction()` / `endTransaction()` calls
- Verify no assumptions about transaction state during processing
- Add asserts to catch unexpected states

**Effort**: 30 minutes to audit and fix if needed

---

## Implementation Roadmap

### Phase 1: Code Review (30 minutes)
1. Audit all `qApp->startTransaction()` calls in codebase
2. Audit all uses of `qApp->getInTransaction()`
3. Verify no code depends on transaction being open during processing
4. Check for any other dialog with same pattern

### Phase 2: Implement Changes (1-2 hours)
1. Remove `startTransaction()` from `importarTabela()` [10 min]
2. Add `startTransaction()` to `on_pushButtonSalvar_clicked()` [10 min]
3. Add proper error handling and rollback [15 min]
4. Update `closeEvent()` cleanup [5 min]
5. Code review [20 min]

### Phase 3: Testing (2-3 hours)
1. Unit tests [1 hour]
2. Integration tests [1 hour]
3. Manual testing (concurrent scenarios) [30 min]
4. Performance testing [30 min]

### Phase 4: Documentation (30 minutes)
1. Update code comments explaining transaction boundaries
2. Document the why/how of transaction scope
3. Add to coding guidelines

**Total Effort**: 4-6 hours

---

## Checklist for Implementation

- [ ] Audit codebase for transaction usage patterns
- [ ] Verify no code assumes transaction open during processing
- [ ] Remove `startTransaction()` from `importarTabela()`
- [ ] Add `startTransaction()/endTransaction()` to `on_pushButtonSalvar_clicked()`
- [ ] Add proper try-catch for transaction safety
- [ ] Update `closeEvent()` cleanup logic
- [ ] Add unit tests for transaction scope
- [ ] Add integration tests for concurrent access
- [ ] Perform manual testing with concurrent users
- [ ] Monitor database locks during testing
- [ ] Update code comments
- [ ] Update developer documentation
- [ ] Code review by team lead
- [ ] Deploy to test environment
- [ ] Monitor for issues in production

---

## Code Examples

### Before Refactoring

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        qApp->startTransaction("ImportaProdutos::importaTabela");  // ← START (too early!)
        processarArquivo();  // Takes 5-30 seconds
    } catch (std::exception &) {
        close();
        throw;
    }
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
    // ... user review dialog ...
    try {
        salvar();
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->endTransaction();  // ← END (finally!)
    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}

void ImportaProdutos::closeEvent(QCloseEvent *event) {
    if (qApp->getInTransaction()) {
        qApp->rollbackTransaction("");  // Cleanup stale transaction
    }
    QDialog::closeEvent(event);
}
```

### After Refactoring

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // No transaction here - let user review results
        processarArquivo();  // Takes 5-30 seconds, NO LOCKS
    } catch (std::exception &) {
        close();
        throw;
    }
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
    // ... user review dialog ...

    try {
        // Transaction only wraps the actual database write
        qApp->startTransaction("ImportaProdutos::salvar");  // ← START (just before saving)

        try {
            salvar();  // Takes 100-500ms

            qApp->endTransaction();  // ← END (immediately!)
        } catch (std::exception &) {
            qApp->rollbackTransaction("");  // Rollback on save failure
            throw;
        }
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}

void ImportaProdutos::closeEvent(QCloseEvent *event) {
    if (qApp->getInTransaction()) {
        // Should rarely happen now
        qWarning() << "WARNING: Transaction still open at close event!";
        qApp->rollbackTransaction("");
    }
    QDialog::closeEvent(event);
}
```

---

## Why This Matters

### For Users
- **Better responsiveness**: Other users can work while import is running
- **Faster operations**: No waiting for import-related locks
- **Better collaboration**: Multi-user ERP actually works as intended

### For Database
- **Reduced load**: Fewer long-running transactions
- **Lower memory usage**: Fewer open connections holding state
- **Fewer deadlocks**: Reduced lock contention
- **Better concurrency**: MVCC and lock mechanisms work efficiently

### For Operations
- **Higher availability**: System more responsive during imports
- **Better monitoring**: Clear transaction boundaries
- **Easier debugging**: Transaction scope is obvious and minimal
- **Industry best practice**: Aligned with database design principles

---

## Related Issues to Address

This refactoring should be done alongside:

1. **Transaction Scope in Similar Dialogs**
   - Search codebase for other dialogs with similar pattern
   - Apply same refactoring to all of them
   - Create coding guideline: "Transactions should be as short as possible"

2. **Error Handling Improvements**
   - Add better error messages when save fails
   - Consider partial save scenarios
   - Add rollback confirmation to user

3. **Performance Monitoring**
   - Add timing logs to understand salvar() duration
   - Monitor database lock durations
   - Alert if transaction exceeds 5 seconds (unexpected)

---

## Questions to Answer Before Starting

1. Are there other dialogs with the same transaction pattern?
2. Does the codebase have any code that depends on transaction being open during processing?
3. What's the typical duration of `salvar()`? (Should be < 1 second)
4. Are there any test environments to validate concurrent access?
5. Is there monitoring in place for database locks and transaction duration?

---

## Conclusion

This refactoring is **critical for a multi-user ERP system**. Holding transactions open for minutes at a time is a severe anti-pattern that blocks other users and risks deadlocks.

The solution is straightforward:
- Move transaction start to just before `salvar()`
- Keep transaction scope minimal (< 1 second)
- Ensure proper rollback on failure
- Other users remain unblocked

**Estimated effort**: 4-6 hours
**Impact**: Transformational (60-18,000x reduction in lock duration)
**Risk**: Low (clear, well-defined scope)
