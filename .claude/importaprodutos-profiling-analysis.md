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

### Regression Testing
```bash
cd /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/tools/import-benchmark

# Generate golden file (baseline)
./import-benchmark ../../CASTELATTO.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 \
    --dry-run --generate-golden golden-castelatto.json

# Verify after code changes
./import-benchmark ../../CASTELATTO.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 \
    --verify golden-castelatto.json
```

### Running Commands from Windows (via WSL)
```batch
:: Profile with perf
wsl -d Ubuntu -- bash -c "cd /tmp && /usr/local/bin/perf record -g -o bench.data /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/tools/import-benchmark/import-benchmark /mnt/c/Users/Torres/Dropbox/Projeto_Staccato/erp-staccato/CASTELATTO.xlsx --user torres --pass 1234 --host 172.31.240.1 --dry-run"

:: View perf report
wsl -d Ubuntu -- bash -c "cd /tmp && /usr/local/bin/perf report -i bench.data --stdio --no-children --percent-limit 0.5"
```

## Profiling Results (2026-01-02)

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

## Optimization Results

### Tested Approaches (Incremental)

| Approach | Result | Notes |
|----------|--------|-------|
| QSqlRecord Batch | **-27% SLOWER** | `setRecord()` internally calls `setData()` per field |
| Field Index Cache | **0% (no impact)** | String lookup wasn't the bottleneck |
| Disable Proxy | **+7% faster** | 58s → 54s, eliminates proxy overhead |

### Final Solution: Bypass QSqlTableModel (5.8x FASTER)

**Architecture Change:**
```
Before: Excel → QSqlTableModel.setData() ×187,000 → Preview → submitAll()
                           ↓
              80% CPU in QMapNodeBase::nextNode()

After:  Excel → Compare in memory → QVector<ProductChange> → QStandardItemModel → Preview
                                              ↓
                                    Batch SQL on save
```

**Implementation:**
1. **Phase 1 - Load:** Direct SQL query into `QHash<QString, Produto>`
2. **Phase 2 - Compare:** In-memory comparison, build `QVector<ProductChange>`
3. **Phase 3 - Preview:** Populate `QStandardItemModel` from change vector (read-only)
4. **Phase 4 - Save:** Batch prepared statements for INSERT/UPDATE

**Results (Verified via Regression Testing):**
- **Before:** 55-62 seconds, 80% CPU in Qt map traversal
- **After:** 8.7 seconds, 11% CPU in DB string conversion
- **Speedup:** 6.3x (86% faster)
- **Regression Test:** PASSED - all 16,020 rows match field-by-field

**Profile After Optimization:**
| Overhead | Function | Location |
|----------|----------|----------|
| 11% | `QUtf8::convertToUnicode` | DB value conversion |
| 6% | `malloc` | Memory allocation |
| ~5% | `loadExistingProducts` | Loading products from DB |

The Qt model map traversal bottleneck has been **completely eliminated**.

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
