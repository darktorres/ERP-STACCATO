# Naming Improvements Analysis

## Functions with Potentially Misleading Names

### 1. `leituraProduto()` → `lerProdutoDaLinha()` ⭐ STRONG CANDIDATE

**Current Name**: `leituraProduto()`
- Meaning: "product reading" (noun form)
- Misleading: Sounds like it reads/loads product data

**What it actually does**:
- Reads a SINGLE ROW from Excel
- Parses fields from that row
- Converts/validates values
- Stores in temporary `produto` struct
- Does NOT access database

**Better Name**: `lerProdutoDaLinha(int row)`
- Meaning: "read product from line/row"
- Clear: It's reading from a LINE (Excel row), not from database
- Verb form makes intent clear

**Change**: One line update in processarArquivo()
```cpp
// Before
leituraProduto(xlsx, row);

// After
lerProdutoDaLinha(xlsx, row);
```

---

### 2. `mostraApenasEstesFornecedores()` → `filtrarPorFornecedores()` ⭐ GOOD CANDIDATE

**Current Name**: `mostraApenasEstesFornecedores()`
- Meaning: "show only these suppliers"
- Misleading: Sounds like UI operation (show/display)

**What it actually does**:
- Sets a FILTER on the model
- Selects products from database
- NOT a display/show operation

**Better Name**: `filtrarPorFornecedores()`
- Meaning: "filter by suppliers"
- Clear: It's a database filter operation
- Explains what the model does

**Current Usage**: Lines 104, 475
```cpp
// Before
mostraApenasEstesFornecedores();

// After
filtrarPorFornecedores();
```

---

### 3. `marcaTodosProdutosDescontinuados()` → `marcarProdutosDescontinuados()` ⭐ MEDIUM CANDIDATE

**Current Name**: `marcaTodosProdutosDescontinuados()`
- Meaning: "mark ALL products discontinued"
- Problem: It doesn't actually mark ALL products - only those for specific suppliers

**What it actually does**:
- Marks products as discontinued
- Only for specific suppliers (filtered by idsFornecedor)
- Only non-stock items (estoque = FALSE)
- Only for specific promotion type

**Better Name**: `marcarProdutosDescontinuados()`
- Meaning: "mark products discontinued" (implicit: the filtered ones)
- More accurate: Doesn't claim to mark "all"

**Why change**:
- Current name is technically inaccurate
- Shorter and clearer
- Still obvious what it does

**Current Usage**: Line 487 (in `commitarSetupFornecedores()`)
```cpp
// Before (but moved to commitarSetupFornecedores)
marcaTodosProdutosDescontinuados();

// After
marcarProdutosDescontinuados();
```

---

### 4. `insereEmOk()` / `insereEmErro()` → `adicionarProdutoValido()` / `adicionarProdutoComErro()` ⭐ GOOD CANDIDATE

**Current Names**:
- `insereEmOk()` - means "insert in OK"
- `insereEmErro()` - means "insert in error"
- Problem: Unclear what "em" means - "in"? "to"?

**What they actually do**:
- Add a product to a model
- `insereEmOk()`: adds to `modelProduto` (valid products)
- `insereEmErro()`: adds to `modelErro` (error products)

**Better Names**:
- `adicionarProdutoValido()` - "add valid product"
- `adicionarProdutoComErro()` - "add product with error"

**Why change**:
- Crystal clear what they do
- English speakers can understand ("add product valid/with error")
- "insere em" is ambiguous and awkward

**Current Usage**: Lines 135-140, 919-920
```cpp
// Before
existeNoModel ? atualizaProduto() : insereEmOk();
// ... later ...
insereEmErro();

// After
existeNoModel ? atualizaProduto() : adicionarProdutoValido();
// ... later ...
adicionarProdutoComErro();
```

---

### 5. `marcaProdutoNaoDescontinuado()` → `marcarProdutoComoAtivo()` ⭐ GOOD CANDIDATE

**Current Name**: `marcaProdutoNaoDescontinuado()`
- Meaning: "mark product not discontinued"
- Problem: Double negative ("not discontinued" = complicated)

**What it actually does**:
- Sets `descontinuado = 0` (false)
- Decrements `itensExpired` counter
- Marks product as "active" in import

**Better Name**: `marcarProdutoComoAtivo()`
- Meaning: "mark product as active"
- Positive form: much clearer
- Domain language: "active" vs "not discontinued"

**Current Usage**: Line 89
```cpp
// Before
marcaProdutoNaoDescontinuado(row);

// After
marcarProdutoComoAtivo(row);
```

---

### 6. `pintarCamposForaDoPadrao()` → `colorirCamposInvalidos()` ⭐ MEDIUM CANDIDATE

**Current Name**: `pintarCamposForaDoPadrao()`
- Meaning: "paint fields out of standard"
- Problem: "fora do padrão" is vague

**What it actually does**:
- Colors cells in error table
- Identifies which fields are INVALID
- Red for critical errors
- Gray for non-standard but acceptable

**Better Name**: `colorirCamposInvalidos()`
- Meaning: "color invalid fields"
- Clear: identifying invalid data
- Purpose is clear

**Current Usage**: Line 925
```cpp
// Before
pintarCamposForaDoPadrao(row);

// After
colorirCamposInvalidos(row);
```

---

### 7. `verificaSeRepresentacao()` → `carregarFlagRepresentacao()` ⭐ MEDIUM CANDIDATE

**Current Name**: `verificaSeRepresentacao()`
- Meaning: "verify if representacao"
- Problem: Doesn't VERIFY anything - just reads and sets a checkbox

**What it actually does**:
- Reads representacao flag from database
- Sets checkbox state based on it
- No validation/verification

**Better Name**: `carregarFlagRepresentacao()`
- Meaning: "load representacao flag"
- Clear: It's loading data from DB
- No false implication of verification

**Current Usage**: Line 105
```cpp
// Before
verificaSeRepresentacao();

// After
carregarFlagRepresentacao();
```

---

### 8. `verificaTabela()` - NAME IS GOOD ✅

**Current Name**: `verificaTabela()`
- Meaning: "verify table"
- Actual behavior: Validates Excel sheet structure
- ✅ Name is accurate!

---

## Summary Table

| Current Name | Better Name | Priority | Impact |
|---|---|---|---|
| `leituraProduto()` | `lerProdutoDaLinha()` | ⭐⭐⭐ HIGH | Prevents confusion about data source (Excel vs DB) |
| `insereEmOk()` | `adicionarProdutoValido()` | ⭐⭐⭐ HIGH | "insere em" is confusing |
| `insereEmErro()` | `adicionarProdutoComErro()` | ⭐⭐⭐ HIGH | Same as above |
| `mostraApenasEstesFornecedores()` | `filtrarPorFornecedores()` | ⭐⭐ MEDIUM | Clarifies it's a DB filter, not UI display |
| `marcaProdutoNaoDescontinuado()` | `marcarProdutoComoAtivo()` | ⭐⭐ MEDIUM | Double negative is hard to understand |
| `marcaTodosProdutosDescontinuados()` | `marcarProdutosDescontinuados()` | ⭐⭐ MEDIUM | Name is technically inaccurate |
| `pintarCamposForaDoPadrao()` | `colorirCamposInvalidos()` | ⭐ LOW | "fora do padrão" is vague |
| `verificaSeRepresentacao()` | `carregarFlagRepresentacao()` | ⭐ LOW | Avoid false implication of verification |

---

## Implementation Notes

### If Implementing All Changes

**Files to modify**:
1. `importaprodutos.h` - Update 8 function declarations
2. `importaprodutos.cpp` - Update function definitions and all call sites

**Scope**:
- ~40 function calls to update
- ~8 function definitions to rename
- No logic changes - pure refactoring

**Risk**: Very low (safe refactoring)
**Benefit**: Significantly improved code clarity

### Recommended Priority Order

If implementing gradually:

1. **Phase 1 (Critical for transaction refactoring)**:
   - `leituraProduto()` → `lerProdutoDaLinha()` ✅ ALREADY DOCUMENTED
   - `insereEmOk()` / `insereEmErro()` → better names
   - `mostraApenasEstesFornecedores()` → `filtrarPorFornecedores()`

2. **Phase 2 (Nice to have)**:
   - `marcaProdutoNaoDescontinuado()` → `marcarProdutoComoAtivo()`
   - `verificaSeRepresentacao()` → `carregarFlagRepresentacao()`
   - `marcaTodosProdutosDescontinuados()` → `marcarProdutosDescontinuados()`

3. **Phase 3 (Polish)**:
   - `pintarCamposForaDoPadrao()` → `colorirCamposInvalidos()`

---

## Conclusion

These naming improvements would:
- ✅ Make code self-documenting
- ✅ Reduce cognitive load for future developers
- ✅ Clarify the transaction boundaries and data flow
- ✅ Improve maintainability

Particularly `leituraProduto()` → `lerProdutoDaLinha()` and the `insereEm*` functions are STRONG candidates for immediate improvement because they clarify a critical aspect: **where data is coming from and what's being modified**.
