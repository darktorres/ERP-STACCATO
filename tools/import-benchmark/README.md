# ImportaProdutos Performance Benchmark Tool

A CLI tool to measure and profile the performance of the product import functionality.

## Building

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

:: The executable will be at: release\import-benchmark.exe
```

### Linux

```bash
cd tools/import-benchmark
qmake import-benchmark.pro
make

# The executable will be at: ./import-benchmark
```

## Usage

### Generate Test Data

Create a test Excel file with synthetic product data:

```bash
# Generate file with 1000 products
import-benchmark --generate 1000 -o test_1000.xlsx

# Generate file with 10000 products
import-benchmark --generate 10000 -o test_10000.xlsx

# Generate file with 50000 products (stress test)
import-benchmark --generate 50000 -o test_50000.xlsx
```

### Run Benchmark

Basic benchmark:

```bash
import-benchmark test_1000.xlsx
```

Verbose output with phase timings:

```bash
import-benchmark test_1000.xlsx --verbose
```

Compare baseline vs optimized implementation:

```bash
import-benchmark test_1000.xlsx --compare
```

Multiple iterations (takes best result):

```bash
import-benchmark test_1000.xlsx --iterations 5 --compare
```

## Output Example

```
Benchmarking: test_10000.xlsx
Iterations: 3

Running baseline benchmark...

=== BASELINE (Current Implementation) ===
Total rows:         10001
Valid products:     10000
Error products:     0
Unique suppliers:   5

Timing breakdown:
  File open:          234 ms
  Sheet select:       1 ms
  Dimension read:     0 ms
  Header validation:  2 ms
  Supplier extract:   156 ms
  Product parsing:    1823 ms
  Field validation:   12 ms
  ---
  TOTAL:              2228 ms

Throughput: 4488.3 products/second

Running optimized benchmark...

=== OPTIMIZED (With Improvements) ===
Total rows:         10001
Valid products:     10000
Error products:     0
Unique suppliers:   5

Timing breakdown:
  File open:          231 ms
  Sheet select:       1 ms
  Dimension read:     0 ms
  Header validation:  2 ms
  Supplier extract:   89 ms
  Product parsing:    1456 ms
  Field validation:   11 ms
  ---
  TOTAL:              1790 ms

Throughput: 5586.6 products/second

=== COMPARISON ===
File open:         234 -> 231 (1.3% improvement)
Supplier extract:  156 -> 89 (42.9% improvement)
Product parsing:   1823 -> 1456 (20.1% improvement)
TOTAL:             2228 -> 1790 (19.7% improvement)
```

## Measured Bottlenecks

The benchmark measures the following phases:

1. **File open** - Time to parse the Excel file into memory
2. **Sheet select** - Time to locate and select the "BASE" sheet
3. **Dimension read** - Time to determine row/column count
4. **Header validation** - Time to verify column headers
5. **Supplier extraction** - Time to collect unique suppliers (first pass)
6. **Product parsing** - Time to read and transform product data
7. **Field validation** - Time to validate product fields

## Optimizations Demonstrated

The `--compare` flag runs both the baseline (mimicking current code) and an
optimized version that demonstrates:

1. **Static QLocale** - Moving `QLocale` creation outside the loop
2. **Single cell read** - Avoiding duplicate `readValue()` calls
3. **QSet for suppliers** - Using `QSet` instead of `QStringList::contains()`

These are the "quick win" optimizations from Phase 1 of the improvement plan.
