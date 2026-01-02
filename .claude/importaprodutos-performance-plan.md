# ImportaProdutos Performance Analysis and Improvement Plan

## Executive Summary

The `ImportaProdutos` class in `src/importaprodutos.cpp` imports product data from Excel files into the database. Performance analysis reveals several critical bottlenecks that cause slow imports, especially with large files (1000+ rows).

---

## Performance Bottlenecks Identified

### 1. **QLocale Creation Inside Loop** (HIGH IMPACT)
**Location**: `leituraProduto()` - Line 412
```cpp
const QLocale locale(QLocale::Portuguese);  // Created for EVERY row!
```
**Problem**: A new `QLocale` object is created for each of the ~20,000 rows being processed.
**Fix**: Move to class member or static const.

---

### 2. **Multiple Individual Cell Reads Per Row** (HIGH IMPACT)
**Location**: `leituraProduto()` - Lines 414-466
```cpp
QVariant fornecedor = xlsx.readValue(row, 1);
QVariant descricao = xlsx.readValue(row, 2);
// ... 20 more calls
```
**Problem**: Each `xlsx.readValue()` performs two nested `QMap` lookups. With 20 columns and 20,000 rows = 400,000 map lookups.
**Fix**: Read entire row at once, or cache cell references.

---

### 3. **Double Iteration for Supplier Registration** (MEDIUM IMPACT)
**Location**: `cadastraFornecedores()` - Lines 360-367
```cpp
for (int row = 2; row < rows; ++row) {
  const QString fornec = xlsx.readValue(row, 1).toString();
  // ...
  fornecedores << xlsx.readValue(row, 1).toString();  // Reads SAME cell twice!
}
```
**Problem**:
- First loop reads column 1 twice per row
- Individual SQL queries per supplier instead of batch

---

### 4. **Repetitive Field Index Lookups** (HIGH IMPACT)
**Location**: `atualizaCamposProduto()` - Lines 527-724
```cpp
if (modelProduto.data(row, "fornecedor").toString() != produto.fornecedor) {
  modelProduto.setData(row, "fornecedor", produto.fornecedor);
  modelProduto.setData(row, "fornecedorUpd", yellow);
```
**Problem**:
- 24 field comparisons with string-based column lookups
- Each `data()` and `setData()` call does `fieldIndex()` lookup
- With 20,000 rows = 480,000 field index lookups (24 fields x 20,000 rows)
**Fix**: Pre-cache field indices, use loop with field array.

---

### 5. **String Operations on Every Cell** (MEDIUM IMPACT)
**Location**: `leituraProduto()` - Lines 468-488
```cpp
produto.descricao = descricao.toString().remove("*").remove("()").replace('_', ' ').toUpper().trimmed().left(250);
```
**Problem**: Multiple string operations create intermediate QString objects.
**Fix**: Single-pass character processing or pre-compiled regex.

---

### 6. **SQL Query Per Product (Promocao Type)** (HIGH IMPACT)
**Location**: `insereEmOk()` - Lines 906-914
```cpp
if (tipo == Tipo::Promocao) {
  SqlQuery query;
  query.prepare("SELECT idProduto FROM produto WHERE ...");
  // Executed for EVERY new product!
}
```
**Problem**: Individual SELECT query for each new product in Promocao mode.
**Fix**: Batch query or pre-load all related products.

---

### 7. **Qt Model Layer Overhead** (MEDIUM IMPACT)
**Location**: Throughout file
**Problem**: Using `QSqlTableModel` for bulk operations has significant overhead:
- Signal/slot emission on data changes
- Proxy model processing
- Row-by-row submitAll()

---

## Improvement Plan

### Phase 1: Quick Wins (Low Effort, High Impact)

#### 1.1 Move QLocale to Static/Member
```cpp
// In class header
static const QLocale s_locale;

// In implementation
const QLocale ImportaProdutos::s_locale(QLocale::Portuguese);
```
**Expected improvement**: 5-10%

#### 1.2 Cache Field Indices
```cpp
// Cache indices once after model setup
struct FieldIndices {
  int fornecedor, descricao, un, colecao, m2cx, pccx, kgcx;
  int formComercial, codComercial, codBarras, ncm, qtdPallet;
  int custo, precoVenda, ui, un2, minimo, mva, st, sticms;
  int quantCaixa, markup, validade, descontinuado, atualizarTabelaPreco;
  // ... all *Upd fields
};
FieldIndices m_indices;
```
**Expected improvement**: 15-20%

#### 1.3 Fix Double Cell Read in cadastraFornecedores
```cpp
const QString fornec = xlsx.readValue(row, 1).toString();
if (not fornec.isEmpty()) {
  ++count;
  if (not fornecedores.contains(fornec)) {
    fornecedores << fornec;  // Use already-read value
  }
}
```
**Expected improvement**: 5%

---

### Phase 2: Moderate Refactoring (Medium Effort, High Impact)

#### 2.1 Data-Driven Field Updates
Replace 24 if-else blocks with loop:
```cpp
struct FieldMapping {
  int index;
  int indexUpd;
  std::function<QVariant()> getValue;
  bool isDouble;
};

std::vector<FieldMapping> fields = {
  {m_indices.fornecedor, m_indices.fornecedorUpd, [&]{ return produto.fornecedor; }, false},
  {m_indices.descricao, m_indices.descricaoUpd, [&]{ return produto.descricao; }, false},
  // ...
};

for (const auto& f : fields) {
  QVariant oldVal = modelProduto.data(row, f.index);
  QVariant newVal = f.getValue();
  bool changed = f.isDouble ? !qFuzzyCompare(oldVal.toDouble(), newVal.toDouble())
                            : oldVal != newVal;
  if (changed) {
    modelProduto.setData(row, f.index, newVal);
    modelProduto.setData(row, f.indexUpd, yellow);
    hasChanges = true;
  } else {
    modelProduto.setData(row, f.indexUpd, white);
  }
}
```
**Expected improvement**: 20-30%

#### 2.2 Batch Supplier Lookup
```cpp
// Pre-fetch all supplier IDs in one query
QHash<QString, int> existingSuppliers;
SqlQuery query;
query.exec("SELECT razaoSocial, idFornecedor FROM fornecedor");
while (query.next()) {
  existingSuppliers[query.value("razaoSocial").toString()] = query.value("idFornecedor").toInt();
}

// Then batch insert new suppliers
QStringList newSuppliers;
for (const auto& fornec : fornecedores) {
  if (!existingSuppliers.contains(fornec)) {
    newSuppliers << fornec;
  }
}
if (!newSuppliers.isEmpty()) {
  // Single INSERT with VALUES
}
```
**Expected improvement**: 10%

#### 2.3 Pre-Load Related Products for Promocao
```cpp
if (tipo == Tipo::Promocao) {
  // Load all at once
  QHash<QString, int> relatedProducts;
  SqlQuery query;
  query.exec("SELECT idFornecedor, codComercial, idProduto FROM produto WHERE promocao = FALSE AND estoque = FALSE");
  while (query.next()) {
    QString key = QString::number(query.value(0).toInt()) + "|" + query.value(1).toString();
    relatedProducts[key] = query.value(2).toInt();
  }
}
```
**Expected improvement**: 15% (in Promocao mode)

---

### Phase 3: Architectural Changes (High Effort, Highest Impact)

#### 3.1 Direct SQL Operations Instead of Model
For bulk import, bypass QSqlTableModel entirely:
```cpp
void ImportaProdutos::batchInsertProducts(const QVector<Produto>& products) {
  SqlQuery query;
  query.prepare("INSERT INTO produto (fornecedor, descricao, un, ...) VALUES (?, ?, ?, ...)");

  for (const auto& p : products) {
    query.bindValue(0, p.fornecedor);
    query.bindValue(1, p.descricao);
    // ...
    query.exec();
  }
}
```
Or use multi-row INSERT:
```cpp
QString sql = "INSERT INTO produto (cols) VALUES ";
QStringList valueSets;
for (const auto& p : products) {
  valueSets << QString("('%1', '%2', ...)").arg(escape(p.fornecedor), escape(p.descricao));
}
sql += valueSets.join(",");
```
**Expected improvement**: 40-60%

#### 3.2 Read Excel Rows in Batches
Instead of individual cell reads:
```cpp
QVector<QVariant> readRow(QXlsx::Document& xlsx, int row, int colCount) {
  QVector<QVariant> result(colCount);
  for (int col = 1; col <= colCount; ++col) {
    result[col-1] = xlsx.readValue(row, col);
  }
  return result;
}
```
**Expected improvement**: 5-10%

#### 3.3 Parallel Processing
Split file into chunks for multi-threaded processing:
```cpp
QtConcurrent::map(chunks, [](const Chunk& chunk) {
  // Process chunk of rows
});
```
**Expected improvement**: 50-200% (on multi-core)

---

## Implementation Priority

| Priority | Change | Effort | Impact | Risk |
|----------|--------|--------|--------|------|
| 1 | Move QLocale to static | Low | 5-10% | None |
| 2 | Cache field indices | Low | 15-20% | Low |
| 3 | Fix double cell read | Low | 5% | None |
| 4 | Data-driven field updates | Medium | 20-30% | Low |
| 5 | Batch supplier operations | Medium | 10% | Low |
| 6 | Pre-load Promocao products | Medium | 15% | Low |
| 7 | Direct SQL for inserts | High | 40-60% | Medium |
| 8 | Parallel processing | High | 50-200% | High |

---

## Metrics to Track

1. **Total import time** (end-to-end)
2. **Excel parsing time** (file open to all rows read)
3. **Database operation time** (queries + submitAll)
4. **Memory usage** (peak RSS)
5. **Rows processed per second**

---

## CLI Benchmark Tool

A CLI tool will be created in `tools/import-benchmark/` to:
1. Measure baseline performance
2. Test individual optimizations
3. Profile without UI overhead
4. Generate test Excel files of various sizes

---

## Expected Overall Improvement

With all Phase 1-2 changes: **50-70% faster**
With Phase 3 changes: **70-90% faster**
