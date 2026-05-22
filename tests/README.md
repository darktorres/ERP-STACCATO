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

O `nmake` invoca `tools/deploy.cmd` no `QMAKE_POST_LINK` de cada `.pro`,
copiando os DLLs do Qt (via `windeployqt`) para o lado do `.exe`. O test
binary roda sem mexer no PATH:

```batch
cd tests\tier1
nmake check
```

Ou direto:

```batch
tests\tier1\debug\tier1_tests.exe
:: ou para JUnit XML (útil para CI futuro):
tests\tier1\debug\tier1_tests.exe -o results.xml,junitxml
```

`Loja.exe` recebe um deploy mais completo (3rdparty DLLs além dos do Qt),
mas para subir de verdade ainda precisa de arquivos de config que não vão
para o repo (ver "Config files" abaixo).

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
`Loja.exe`.

## Config files (NÃO entram no repo)

`Loja.exe` lê os seguintes arquivos do diretório de trabalho ao iniciar.
Eles contêm segredos / credenciais e **não devem ser commitados**:

| Arquivo            | Lido por                             | Conteúdo                        |
|--------------------|--------------------------------------|---------------------------------|
| `mysql.txt`        | `Application::genericLogin` (application.cpp:127) | senha do MySQL                  |
| `lojas.txt`        | `Application::readSettingsFile`      | mapa nome → hostname das lojas  |
| `google_api.txt`   | `Application::googleMapsApi`         | chave da API do Google Maps     |
| `ACBrLib.ini`      | ACBr (NFe)                           | configuração da DLL fiscal      |
| `webdav.txt`       | uploads WebDAV                       | credenciais de armazenamento    |

Para rodar `debug\Loja.exe` localmente, copie esses arquivos do seu
build do Qt Creator (geralmente
`C:\Builds\build-Loja-Desktop_Qt_5_15_2_MSVC2019_32bit-Debug\debug\`) para
o `debug\` deste repo. `tools/deploy.cmd` deliberadamente não toca neles.

## Próximos passos

- **M2**: extrair validadores para `src/validators.h/.cpp`; cobrir
  `Application::roundDouble`, `ajustarDiaUtil`, `sanitizeSQL`,
  `removerDiacriticos`, `Sql::contasPagar/contasReceber`, etc.
- **M3**: criar `tests/tier2/` com `IntegrationFixture` contra MySQL
  `staccato_test`. Bootstrap em `tests/scripts/reset-db.bat` chamando
  `mysql ... < initdb.sql` + `< tests/fixtures/fixtures.sql`.
- **M4**: smoke tests dos diálogos críticos (Venda, Orçamento, CadastroCliente).
