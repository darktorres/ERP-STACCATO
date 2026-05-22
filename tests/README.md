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

Saída esperada (M2):

```
********* Start testing of TestSmoke *********
PASS   : TestSmoke::initTestCase()
PASS   : TestSmoke::buildPipelineWorks()
PASS   : TestSmoke::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped
********* Finished testing of TestSmoke *********
********* Start testing of TestValidators *********
PASS   : TestValidators::initTestCase()
PASS   : TestValidators::cpfValidoFormatado()
PASS   : TestValidators::cpfValidoSemFormatacao()
PASS   : TestValidators::cpfTamanhoErrado()
PASS   : TestValidators::cpfTodosDigitosIguais()
PASS   : TestValidators::cpfChecksumInvalido()
PASS   : TestValidators::cnpjValidoFormatado()
PASS   : TestValidators::cnpjValidoSemFormatacao()
PASS   : TestValidators::cnpjTamanhoErrado()
PASS   : TestValidators::cnpjChecksumInvalido()
PASS   : TestValidators::cleanupTestCase()
Totals: 11 passed, 0 failed, 0 skipped
********* Finished testing of TestValidators *********
```

## Layout do test_tier1.cpp

Todos os test cases vivem em `tests/tier1/test_tier1.cpp` como classes
`QObject` independentes. O `main()` ao final do arquivo instancia cada uma
e roda via `QTest::qExec`. Para adicionar uma nova suite, basta declarar
mais uma classe e um bloco no `main()` — sem mudança no `.pro`.

## Por que tests/tier1 não exige DLLs do projeto

`TestSmoke` e `TestValidators` referenciam apenas `validators::*` (puro)
e Qt — o linker não puxa o resto da `libstaccato`. Por isso o binário não
precisa de `libcurl.dll`/`zlib1.dll` para subir. Tests futuros que toquem
mais código de `libstaccato` vão herdar as mesmas dependências do
`Loja.exe` (`windeployqt` é o caminho usual de deploy).

## Próximos passos

- **M2**: extrair validadores para `src/validators.h/.cpp`; cobrir
  `Application::roundDouble`, `ajustarDiaUtil`, `sanitizeSQL`,
  `removerDiacriticos`, `Sql::contasPagar/contasReceber`, etc.
- **M3**: criar `tests/tier2/` com `IntegrationFixture` contra MySQL
  `staccato_test`. Bootstrap em `tests/scripts/reset-db.bat` chamando
  `mysql ... < initdb.sql` + `< tests/fixtures/fixtures.sql`.
- **M4**: smoke tests dos diálogos críticos (Venda, Orçamento, CadastroCliente).
