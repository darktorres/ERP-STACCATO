# ImportaProdutos Performance Benchmark Tool

A CLI tool to measure and profile the performance of the product import functionality.
Uses the REAL `importaprodutos.cpp` code with database connection.

## Building

### Linux (WSL2 recommended for profiling)

```bash
cd tools/import-benchmark
qmake import-benchmark.pro
make -j$(nproc)

# The executable will be at: ./import-benchmark
```

### Windows (MSVC)

```batch
:: Setup environment
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

:: Navigate to tool directory
cd tools\import-benchmark

:: Generate Makefile
"C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe" import-benchmark.pro

:: Build
nmake release
```

## Usage

### Basic Benchmark

```bash
./import-benchmark <excel-file> --user <db-user> --pass <db-pass> [options]
```

Options:
- `-H, --host <host>` - Database host (default: localhost)
- `-u, --user <user>` - Database user (required)
- `-p, --pass <pass>` - Database password
- `-d, --validade <days>` - Validade in days (default: 30)
- `-t, --tipo <tipo>` - Tipo: 0=Normal, 1=Promocao (default: 0)
- `-n, --dry-run` - Don't commit changes (rollback at end)
- `-g, --generate-golden <file>` - Generate golden file for regression testing
- `-V, --verify <file>` - Verify results against golden file

### Examples

Run benchmark with dry-run (no database changes):

```bash
./import-benchmark ../../CASTELATTO.xlsx --user torres --pass 1234 --host 172.31.240.1 --dry-run
```

Generate golden file for regression testing:

```bash
./import-benchmark ../../CASTELATTO.xlsx --user torres --pass 1234 --host 172.31.240.1 \
    --dry-run --generate-golden golden-castelatto.json
```

Verify optimized code against baseline:

```bash
./import-benchmark ../../CASTELATTO.xlsx --user torres --pass 1234 --host 172.31.240.1 \
    --verify golden-castelatto.json
```

## Regression Testing

The benchmark tool supports regression testing to ensure optimizations don't break functionality.

### Workflow

1. **Generate baseline golden file** (with current/known-good code):
   ```bash
   ./import-benchmark data.xlsx --user X --pass Y --dry-run --generate-golden baseline.json
   ```

2. **Make optimizations** to the code

3. **Verify optimizations** don't change results:
   ```bash
   ./import-benchmark data.xlsx --user X --pass Y --verify baseline.json
   ```

### What's Compared

The verification compares:
- **Counters**: itensImported, itensUpdated, itensNotChanged, itensExpired, itensError
- **Model row counts**: Number of rows in modelProduto and modelErro
- **Field values**: All product fields for each row, matched by key (fornecedor + codComercial + ui)

Floating-point values are compared with tolerance (0.0001).

## Profiling (Linux/WSL2)

### Using perf

```bash
cd /tmp  # Use Linux filesystem for perf.data

# Record profile
perf record -g ./import-benchmark /path/to/data.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 --dry-run

# View report
perf report --stdio --no-children
```

### Using valgrind/callgrind

```bash
valgrind --tool=callgrind --callgrind-out-file=/tmp/callgrind.out \
    ./import-benchmark /path/to/data.xlsx \
    --user torres --pass 1234 --host 172.31.240.1 --dry-run

# View results
kcachegrind /tmp/callgrind.out
```

## WSL2 Network Notes

When running from WSL2, MySQL runs on Windows host:
- Get host IP: `ip route show default | grep -oP 'via \K\S+'`
- MySQL needs GRANT for WSL2 subnet: `GRANT ALL ON db.* TO 'user'@'172.31.%'`

## Output Example

```
=== ImportaProdutos CLI Benchmark ===
File: CASTELATTO.xlsx
Database: torres@172.31.240.1
Validade: 30 days
Tipo: Normal
Dry run: Yes

Connecting to database...
Connected in 147 ms

Starting import...
Produtos importados: 0 | Atualizados: 3745 | Não modificados: 0 | Descontinuados: 12275 | Com erro: 0
Import completed in 55514 ms

=== Results ===
Imported:    0
Updated:     3745
Not changed: 0
Expired:     12275
Errors:      0
Model rows:  16020
Error rows:  0

Total time: 56000 ms
Rolling back transaction...
```

## Performance Baseline

With CASTELATTO.xlsx (3,745 products to update, 12,275 to mark discontinued):
- **Import time**: ~55 seconds
- **Throughput**: ~287 products/second

See `.claude/importaprodutos-profiling-analysis.md` for detailed profiling results.
