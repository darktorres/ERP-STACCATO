# ImportaProdutos Profiling Analysis

## Environment Setup

### WSL2 with Ubuntu
```bash
# Install Ubuntu in WSL2
wsl --install -d Ubuntu
wsl --set-default Ubuntu

# Install profiling tools
sudo apt update
sudo apt install -y valgrind heaptrack qtbase5-dev libqt5sql5-mysql \
    libqt5charts5-dev libqt5xml5 libcurl4-openssl-dev libssl-dev build-essential

# Install Qt debug symbols (for better profiling)
echo 'deb http://ddebs.ubuntu.com noble main restricted universe multiverse' | sudo tee /etc/apt/sources.list.d/ddebs.list
echo 'deb http://ddebs.ubuntu.com noble-updates main restricted universe multiverse' | sudo tee -a /etc/apt/sources.list.d/ddebs.list
sudo apt install -y ubuntu-dbgsym-keyring
sudo apt update
sudo apt install -y libqt5core5t64-dbgsym libqt5gui5t64-dbgsym libqt5widgets5t64-dbgsym libqt5sql5t64-dbgsym
```

### Build perf from WSL2 Kernel Source
The default perf doesn't match the WSL2 kernel. Build from source:
```bash
cd /tmp
git clone --depth 1 --branch linux-msft-wsl-5.15.y https://github.com/microsoft/WSL2-Linux-Kernel.git wsl-kernel
cd wsl-kernel/tools/perf
sudo apt install -y flex bison libelf-dev libdw-dev libaudit-dev libslang2-dev \
    libperl-dev libnuma-dev libunwind-dev libcap-dev
make -j$(nproc) WERROR=0 NO_LIBPYTHON=1
sudo cp perf /usr/local/bin/perf
```

### Build Benchmark with Debug Symbols
```bash
cd /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/tools/import-benchmark
qmake import-benchmark.pro  # .pro has -g -fno-omit-frame-pointer flags
make -j$(nproc)
```

## Profiling Commands

### Using perf (CPU Sampling)
```bash
cd /tmp  # Use Linux filesystem for perf.data

# Record profile (without dwarf - dwarf has compatibility issues)
/usr/local/bin/perf record -o bench.data -g \
    /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/tools/import-benchmark/import-benchmark \
    /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/CASTELATTO.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 --dry-run

# View report (flat view)
/usr/local/bin/perf report -i bench.data --stdio --no-children

# View report with call graph
/usr/local/bin/perf report -i bench.data --stdio

# Top functions only
/usr/local/bin/perf report -i bench.data --stdio --no-children -g none
```

### Using valgrind/callgrind (Call Graph Analysis)
```bash
cd /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/tools/import-benchmark

# Run callgrind (slower but more detailed)
valgrind --tool=callgrind --callgrind-out-file=/tmp/callgrind.out \
    ./import-benchmark ../../CASTELATTO.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 --dry-run

# View with kcachegrind (GUI)
kcachegrind /tmp/callgrind.out

# Or view text summary
callgrind_annotate /tmp/callgrind.out
```

### Using heaptrack (Memory Profiling)
```bash
heaptrack ./import-benchmark ../../CASTELATTO.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 --dry-run

# View results
heaptrack_gui heaptrack.import-benchmark.*.gz
```

## Profiling Results (2025-01-02)

### Test Configuration
- **File:** CASTELATTO.xlsx
- **Products Updated:** 3,745
- **Products Discontinued:** 12,275
- **Total Time:** ~56 seconds
- **Throughput:** ~287 products/sec

### CPU Profile Summary

| Overhead | Function | Library |
|----------|----------|---------|
| **80.88%** | `QMapNodeBase::nextNode()` | libQt5Core.so |
| 7.32% | `QSqlTableModelPrivate::insertCount()` | libQt5Sql.so |
| 4.24% | `QSqlTableModel::rowCount()` | libQt5Sql.so |
| 0.35% | `QSqlField::name()` | libQt5Sql.so |
| 0.29% | `QXmlStreamReaderPrivate::parse()` | libQt5Core.so |
| 0.28% | `malloc` | libc.so |

### Call Stack Analysis

The dominant bottleneck (`80.88%`) traces to:

```
ImportaProdutos::atualizaCamposProduto()
  └── SqlTableModel::setData()
        └── QSortFilterProxyModel::setData()
              └── QSqlTableModel::setData()
                    └── QSqlTableModel::data() / indexInQuery()
                          └── QMapNodeBase::nextNode()  ← 80% CPU time
```

Secondary path:
```
ImportaProdutos::atualizaCamposProduto()
  └── SqlTableModel::data()
        └── QSortFilterProxyModel::data()
              └── QSqlTableModel::data()
                    └── QMapNodeBase::nextNode()
```

### Root Cause

Each call to `setData()` or `data()` on the Qt SQL model triggers:
1. Proxy model index translation (`proxy_to_source`)
2. SQL table model index lookup (`indexInQuery`)
3. Map traversal to find the correct row (`QMapNodeBase::nextNode`)

With ~3,745 products and ~20+ fields per product, this results in:
- ~75,000+ calls to setData()
- ~150,000+ map traversals
- Each traversal is O(log n) but the constant factor is high

## Optimization Recommendations

### 1. Batch Updates (High Impact)
Instead of calling `setData()` for each field individually:
```cpp
// Current: Multiple setData calls per row
model.setData(row, "field1", value1);
model.setData(row, "field2", value2);
// ... 20+ more calls

// Better: Use QSqlRecord for batch update
QSqlRecord record = model.record(row);
record.setValue("field1", value1);
record.setValue("field2", value2);
model.setRecord(row, record);
```

### 2. Direct SQL for Bulk Operations (High Impact)
For large imports, bypass the model entirely:
```cpp
// Use prepared statement with batch execution
QSqlQuery query;
query.prepare("UPDATE produto SET field1=?, field2=? WHERE id=?");
for (const auto& product : products) {
    query.addBindValue(product.field1);
    query.addBindValue(product.field2);
    query.addBindValue(product.id);
    query.exec();
}
```

### 3. Cache Field Indices (Medium Impact)
```cpp
// Current: Lookup field index by name each time
int idx = model.fieldIndex("fieldName");  // String comparison

// Better: Cache indices at start
QHash<QString, int> fieldIndices;
for (int i = 0; i < model.columnCount(); i++) {
    fieldIndices[model.headerData(i).toString()] = i;
}
```

### 4. Disable Sorting During Import (Medium Impact)
```cpp
// Disable proxy model updates during bulk import
proxyModel->setDynamicSortFilter(false);
// ... do import ...
proxyModel->setDynamicSortFilter(true);
proxyModel->invalidate();
```

### 5. Use Transactions Wisely (Low-Medium Impact)
Already using transactions, but ensure they're not committed too frequently.

## Network Considerations (WSL2)

When running from WSL2, MySQL connections go through virtual networking:
- WSL2 IP: `172.31.x.x` (varies)
- Windows host IP: Get from `ip route show default` (gateway IP)
- MySQL needs GRANT for WSL2 subnet: `GRANT ALL ON db.* TO 'user'@'172.31.%'`

## Files Modified for Benchmarking

The following files have `#ifdef BENCHMARK_BUILD` sections:
- `src/application.cpp` - Disables GUI dialogs, config file reads
- `src/importaprodutos.cpp` - Disables progress dialog, results popup

These changes allow CLI-only execution for profiling without GUI interference.
