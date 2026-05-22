# tests — ERP Staccato

Pirâmide de testes em três tiers. Estado atual: **M1 (pipeline proof)** — apenas
`tier1/test_validators.cpp` rodando, suficiente para provar que o build de testes
funciona end-to-end. M2 amplia tier1; M3 adiciona tier2 (MySQL); M4 adiciona tier3
(UI smoke).

Plano completo em `.claude/test-infrastructure-plan.md`.

## Layout

```
tests/
├── tests.pro                       (SUBDIRS aggregator)
├── common/                         (M3+) — shared test infrastructure
│   ├── integration_fixture.h       — connection helper + IntegrationFixture base
│   └── integration_fixture.cpp
├── fixtures/                       (M3+) — curated seed data (optional)
├── tier1/                          (M1+) — unit, no DB
│   ├── tier1.pro
│   └── test_tier1.cpp
├── tier2/                          (M3+) — integração contra staccato_test local
│   ├── tier2.pro
│   └── test_tier2.cpp
└── tier3/                          (M4) — UI smoke via QTest::keyClicks/mouseClick
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

## DLL deploy do test binary

Desde M2.2 (que adiciona testes de `Sql::*` builders), o linker puxa para
o test exe arquivos como `sql.obj` que referenciam `SqlQuery` / `Application`
transitivamente — mesmo que os testes não cheguem a chamar nada com DB. O
result: `tier1_tests.exe` importa `libcurl.dll`, `zlib1.dll` etc. em tempo
de carga.

A guarda de DB-safety no `main()` continua impedindo qualquer `QSqlDatabase`
de ser aberta — então testes que tentassem (acidentalmente) conectar
falhariam mesmo com os DLLs presentes. O deploy completo é só para o exe
*conseguir subir*.

O `QMAKE_POST_LINK` de `tests/tier1/tier1.pro` invoca
`tools/deploy.cmd ... app` para cobrir esses DLLs no diretório do `.exe`.

## Tier 2 — staccato_test local

`tests/tier2/` roda contra um MySQL local. O binary se auto-bootstrapa:
ao iniciar, abre uma conexão admin no banco `mysql`, e se `staccato_test`
não existir cria com `CREATE DATABASE … utf8mb4`. Se existir mas sem a
tabela sentinela `loja`, invoca `mysql.exe < initdb.sql` para carregar o
schema todo (216 tabelas + 106 procedures/triggers/views — leva ~10 s na
primeira execução).

**Variáveis de ambiente** (todas opcionais):

| Var                          | Default       | Uso                                  |
|------------------------------|---------------|--------------------------------------|
| `STACCATO_TEST_DB_HOST`      | `127.0.0.1`   | Host do MySQL                        |
| `STACCATO_TEST_DB_PORT`      | `3306`        | Porta                                |
| `STACCATO_TEST_DB_USER`      | `root`        | Usuário (precisa CREATE DATABASE na 1ª vez) |
| `STACCATO_TEST_DB_PASS`      | (vazia)       | Senha                                |
| `STACCATO_TEST_DB_NAME`      | `staccato_test` | Schema; `staccato`/`staccato_staging` são **rejeitados em runtime** |
| `STACCATO_TEST_MYSQL_BIN`    | auto-detect   | Caminho do `mysql.exe` para bootstrap |

**Salvaguardas:**

- `readEnvConnectionInfo()` lança `std::runtime_error` se o nome do schema
  bater com `staccato` ou `staccato_staging`. Não há override.
- A conexão admin é só no banco `mysql` (não na produção).
- Cada teste roda dentro de uma transação que é rolled-back no `cleanup()`.
- Tests que chamem stored procedures que abrem transações próprias
  precisam do padrão `[procedure]` (truncate-snapshot — a documentar quando
  o primeiro for adicionado).

**Seed canônico (`tests/fixtures/fixtures.sql`):** após o load do
schema, o bootstrap também aplica um dataset mínimo (1 loja, 1 admin,
1 cliente PF com CPF válido, 1 cliente PJ com CNPJ válido). IDs são
fixos (`idLoja=1`, `idUsuario=1`, `idCliente=1` PF, `idCliente=2` PJ)
para que os testes possam referenciar diretamente. O .sql é
idempotente (`INSERT IGNORE`); o load só roda quando o bootstrap
acabou de criar o schema do zero — em corridas subsequentes a presença
do `loja` table já indica setup completo.

**Pré-requisito — usuário com `mysql_native_password`:**

O projeto inteiro usa `mysql_native_password` (a `libmysql.dll` vendorada,
da Connector/C 6.1, não tem o plugin `caching_sha2_password.dll`). O default
do MySQL 8.4 para `root@localhost` é `caching_sha2_password` — então o root
de uma instalação fresca *não* funciona. Crie um usuário dedicado para os
testes (uma vez):

```sql
CREATE USER 'staccato_test'@'localhost'
  IDENTIFIED WITH mysql_native_password BY 'staccato_test';
GRANT ALL PRIVILEGES ON *.* TO 'staccato_test'@'localhost';
FLUSH PRIVILEGES;
```

O `GRANT ALL PRIVILEGES ON *.*` é necessário porque o bootstrap precisa
de `CREATE DATABASE` no primeiro run. Depois dá pra apertar pra apenas
`staccato_test.*`.

**Rodar:**

```batch
set "STACCATO_TEST_DB_USER=staccato_test"
set "STACCATO_TEST_DB_PASS=staccato_test"
cd tests\tier2
nmake check
:: ou:
tests\tier2\debug\tier2_tests.exe
```

A primeira execução cria o schema; corridas subsequentes detectam que
`loja` já existe e pulam o carregamento (~10 ms de overhead).

**Sem creds configuradas:** os testes que dependem de DB usam `QSKIP` no
`initTestCase` com a mensagem do `lastError()` do driver. Os testes que
não tocam DB (`TestConnectionSafety`) seguem rodando normalmente.

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
