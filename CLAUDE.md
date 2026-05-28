# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Development Commands

This is a Qt C++ application using the qmake build system.

### Building the Application

```bash
qmake Loja.pro
make  # or nmake on Windows with MSVC
```

### Project Structure

- **Target**: `Loja` (executable name)
- **Template**: Qt application
- **Qt Modules**: core, gui, sql, network, xml, charts, widgets
- **C++ Standard**: Latest (c++latest)
- **Precompiled Header**: `pch.h`

### Platform-Specific Notes

- **Windows**: Uses MSVC or MinGW compilers
- **Version**: Currently 0.10.136
- **Icon**: Staccato.ico
- **Dependencies**: OpenSSL, cURL, ACBr libraries

## MSVC Build Tools Configuration

### MSVC Build Tools Path

**Base Path**: `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` (Build Tools 2026, v18.6 — the old `\2022\BuildTools\` path no longer exists)

**Key Directories**:

- **MSVC Version**: Pinned to **`14.44.35207`**. Two toolsets are installed side-by-side — `14.44.35207` and `14.51.36231` — and **14.44 must be used**: 14.51 removed `stdext::checked_array_iterator`, which Qt 5.15.2's container headers still reference, so 14.51 cannot compile this project's PCH.
- **Toolset pin**: Set via the user-scope env var `VCVARS_USER_VERSION=14.44.35207`, which `vcvars`/`VsDevCmd` honor ahead of their default-version files. The VS Installer will not let you uninstall the "latest" (14.51) toolset (workload dependency), so this env-var pin is the resolution. Override per-build with `-vcvars_ver=14.51` if ever needed; undo with `setx VCVARS_USER_VERSION ""`.
- **Compiler Path**: `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x86\` (use `x86` for the 32-bit build; `x64` subfolder for 64-bit)
- **Tools Available**: `cl.exe`, `link.exe`, `nmake.exe`
- **Environment Setup**: `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat`

### Qt Installation Paths

Two kits are installed; the **32-bit `msvc2019` kit is the one for the shipping build** (required for the 32-bit target + legacy `mysql_native_password` auth — see CLAUDE memory). Pair it with `VsDevCmd.bat -arch=x86`.

- **Qt 5.15.2 (32-bit, primary)**: `C:\Qt\5.15.2\msvc2019\` — qmake at `C:\Qt\5.15.2\msvc2019\bin\qmake.exe`
- **Qt 5.15.2 (64-bit)**: `C:\Qt\5.15.2\msvc2019_64\` — qmake at `C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe`

### Build Environment Setup

To compile with MSVC, first run the Visual Studio Developer Command Prompt. With the `VCVARS_USER_VERSION` pin in place, this auto-selects toolset 14.44 (no `-vcvars_ver` needed); use `-arch=x86` for the 32-bit build:

```batch
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86
```

Or use the PowerShell version:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch x86
```

If the env-var pin is ever missing (e.g. fresh machine), add `-vcvars_ver=14.44` to force the working toolset.

### Test Suite Compilation

```batch
# Setup MSVC environment first (toolset 14.44 auto-selected via VCVARS_USER_VERSION pin)
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x86

# Navigate to tests directory
cd tests

# Generate Makefile with correct Qt (match the arch above — 32-bit kit shown)
"C:\Qt\5.15.2\msvc2019\bin\qmake.exe" tests.pro

# Compile tests
nmake

# Run tests
debug\tests.exe
```

## Architecture Overview

### Core Application Structure

- **Application Class**: Custom QApplication subclass with database connectivity, transaction management, and error handling
- **Main Window**: Central interface with tabbed modules
- **Module Organization**: Separated into functional areas (Compras, Estoque, Financeiro, Logística, NFe, etc.)

### Key Modules

1. **Compras (Purchases)**: Purchase order management, supplier interactions
2. **Estoque (Inventory)**: Stock management, product tracking
3. **Financeiro (Financial)**: Financial transactions, accounts management
4. **Logística (Logistics)**: Delivery scheduling, transportation management
5. **NFe (Electronic Invoice)**: Brazilian electronic invoice system integration
6. **Galpão (Warehouse)**: Warehouse layout and management
7. **Relatórios (Reports)**: Report generation using LimeReport

### Database Architecture

- **Database**: MySQL/MariaDB
- **Connection Management**: Centralized through Application class
- **Transaction Support**: Built-in transaction management with rollback capabilities
- **Models**: Custom SQL table models extending Qt's model classes

### Third-Party Dependencies

- **LimeReport 1.5.68**: Report generation and design
- **QtXlsxWriter**: Excel file generation
- **QSimpleUpdater**: Application auto-update functionality
- **ACBr**: Brazilian fiscal/accounting compliance library
- **OpenSSL**: Cryptographic operations
- **cURL**: HTTP client functionality

### UI Framework

- **Qt Widgets**: Traditional desktop UI components
- **Custom Delegates**: Specialized cell editors for tables
- **Proxy Models**: Filtering and sorting for large datasets
- **Custom Widgets**: Collapsible widgets, specialized input controls

### File Organization

- **src/**: All source code (.cpp/.h files)
- **ui/**: Qt Designer UI forms (.ui files)
- **qrs/**: Resources (images, translations, etc.)
- **3rdparty/**: External libraries and dependencies
- **modelos/**: Report templates (.lrxml, .xlsx)
- **db/**: Database schema and migration files
- **tests/**: Comprehensive test suite with unit and integration tests

### Key Design Patterns

- **Model-View Architecture**: Extensive use of Qt's model/view framework
- **Proxy Models**: For filtering and data transformation
- **Custom Delegates**: For specialized table cell editing
- **Exception Handling**: Custom exception classes for business logic errors
- **Transaction Management**: Database transactions with automatic rollback

### Development Workflow

1. Use Qt Creator or compatible IDE for development
2. Follow existing naming conventions (Portuguese business terms, English technical terms)
3. Database changes require corresponding model updates
4. UI changes should be made in .ui files when possible
5. Custom delegates and proxy models for complex table interactions

### Brazilian Business Context

This is an ERP system specifically designed for Brazilian businesses, with features for:

- NFe (Nota Fiscal Eletrônica) - Electronic invoice compliance
- Brazilian tax calculations and reporting
- Integration with Brazilian banking systems (CNAB)
- Portuguese language interface and business terminology

### Code Style Notes

- Mixed Portuguese/English naming (Portuguese for business concepts, English for technical concepts)
- Extensive use of auto keyword for type deduction
- Modern C++ features where supported by Qt 5.15+
- Consistent indentation and formatting

### Testing Framework

- **Test Directory**: `tests/`
- **Unit Tests**: Status management, validators, SQL operations
- **Integration Tests**: Purchase workflow, sales workflow, inventory management
- **Test Infrastructure**: Qt Test framework with mock objects and helpers
- **Coverage**: 97 test methods covering critical functionality

### Process Improvements

- **Status Management**: Enum-based status transitions to replace hard-coded strings
- **Input Validation**: Brazilian compliance validation (CPF/CNPJ/phone)
- **Database Safety**: Parameterized queries and transaction management
- **Workflow Simplification**: Reduced status states and cleaner business logic

## Important Instruction Reminders

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.
NEVER proactively create documentation files (\*.md) or README files. Only create documentation files if explicitly requested by the User.

## File Location Preferences

- **Markdown Files**: When asked to write MD files, place them in the `.claude/` folder.
