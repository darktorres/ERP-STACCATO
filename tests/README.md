# tests — ERP Staccato

Pirâmide de testes em três tiers. Estado atual: **M1 (pipeline proof)** — apenas
`tier1/test_validators.cpp` rodando, suficiente para provar que o build de testes
funciona end-to-end. M2 amplia tier1; M3 adiciona tier2 (MySQL); M4 adiciona tier3
(UI smoke).

Plano completo em `.claude/test-infrastructure-plan.md`.

## Layout

```
tests/
├── tests.pro          (SUBDIRS aggregator)
├── tier1/             (M1+) — unit, no DB, no event loop além do QApplication padrão
│   ├── tier1.pro
│   └── test_validators.cpp
├── tier2/             (M3) — integração contra MySQL staccato_test local
└── tier3/             (M4) — UI smoke via QTest::keyClicks/mouseClick
```

## Build

Requer o ambiente MSVC + Qt 5.15.2 já configurado (ver `CLAUDE.md`).

### Build completo (lib + app + testes)

```batch
:: 1) ative o MSVC dev shell
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

:: 2) gere Makefiles a partir do orchestrator
"C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe" staccato.pro

:: 3) compile
nmake
```

Isto compila, nesta ordem: `libstaccato/` → `Loja.exe` → `tests/tier1/tier1_tests.exe`.

### Build apenas dos testes (após libstaccato já estar compilado)

```batch
cd tests\tier1
"C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe" tier1.pro
nmake
```

## Run

Por padrão, qmake com `CONFIG += testcase` adiciona um target `check` que executa
o binário:

```batch
cd tests\tier1
nmake check
```

Ou rode o executável diretamente. Importante: Qt e cURL DLLs precisam estar no
PATH (use `windeployqt` para deploy permanente ou adicione manualmente):

```batch
set "PATH=C:\Qt\5.15.2\msvc2019\bin;3rdparty\cURL_x86-msvc\bin;%PATH%"
tests\tier1\debug\tier1_tests.exe
:: ou para JUnit XML (útil para CI futuro):
tests\tier1\debug\tier1_tests.exe -o results.xml,junitxml
```

Saída esperada (M1):

```
********* Start testing of TestSmoke *********
PASS   : TestSmoke::initTestCase()
PASS   : TestSmoke::buildPipelineWorks()
PASS   : TestSmoke::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped
********* Finished testing of TestSmoke *********
```

## Escopo do M1

M1 prova apenas que o pipeline build → link → run de testes funciona
end-to-end. O teste `test_smoke.cpp` deliberadamente **não** referencia código
de `libstaccato`.

### Por que não há ainda teste de validadores em M1

A tentativa inicial era subclassar `RegisterDialog` (`Probe`) para expor
`validaCPF`/`validaCNPJ` protected. Esbarrou em três problemas, todos
resolvidos pela refactor leve do M2:

1. `RegisterDialog` é abstrato — exigia stub das 9 puras virtuais.
2. Construtor chama `model.setTable(...)` que lança se não houver
   `QSqlDatabase` default. Workaround: abrir QSQLITE in-memory no
   `initTestCase`.
3. **DLL_INIT_FAILED em runtime**: ao referenciar `RegisterDialog` o linker
   puxa transitivamente código de `Application` + QSimpleUpdater + LimeReport
   para dentro do .exe de teste, e algum desses .obj's depende de recursos/
   inicializadores que só vivem em `Loja.exe`. O processo morre antes
   mesmo do `main()` rodar (`STATUS_DLL_INIT_FAILED`, código 0xC0000142).

A solução limpa é extrair `validaCPF/CNPJ` para funções livres em
`src/validators.h/.cpp` (sem `RuntimeError`, sem QMessageBox no caminho
inválido — apenas retornam `bool`). Aí o teste só puxa `validators.obj`,
sem cascata.

## Próximos passos

- **M2**: extrair validadores para `src/validators.h/.cpp`; cobrir
  `Application::roundDouble`, `ajustarDiaUtil`, `sanitizeSQL`,
  `removerDiacriticos`, `Sql::contasPagar/contasReceber`, etc.
- **M3**: criar `tests/tier2/` com `IntegrationFixture` contra MySQL
  `staccato_test`. Bootstrap em `tests/scripts/reset-db.bat` chamando
  `mysql ... < initdb.sql` + `< tests/fixtures/fixtures.sql`.
- **M4**: smoke tests dos diálogos críticos (Venda, Orçamento, CadastroCliente).
