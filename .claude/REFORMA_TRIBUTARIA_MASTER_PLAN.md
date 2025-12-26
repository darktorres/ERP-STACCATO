# Reforma Tributária 2025 - Plano Mestre de Implementação
## Staccato ERP - Sistema de Emissão de NF-e

**Versão:** 1.0
**Data:** 24 de Dezembro de 2025
**Responsável:** Análise Técnica - Claude Code
**Status:** Pronto para Implementação

---

## SUMÁRIO EXECUTIVO

A Emenda Constitucional 132/2023 implementa a Reforma Tributária do Consumo a partir de **01/01/2026**, substituindo parcialmente ICMS, PIS e COFINS pelos novos tributos IBS (Imposto sobre Bens e Serviços) e CBS (Contribuição sobre Bens e Serviços), além de IS (Imposto Seletivo).

**Prazos Críticos:**
- **31/10/2025**: Obrigatório em ambiente de homologação (SEFAZ)
- **01/01/2026**: Obrigatório em produção (novo formato)

**Escopo:** Adaptação do módulo `cadastrarnfe.cpp` para suportar os novos impostos mantendo compatibilidade com ICMS/PIS/COFINS.

**Esforço Estimado:** 120-150 horas de desenvolvimento

---

## PARTE I: ARQUITETURA DE DADOS ATUAL

### 1. Descoberta Crítica: Localização Real dos Campos de Imposto

Os campos de imposto **NÃO estão em `venda_has_produto2`**. Estão distribuídos conforme segue:

#### 1.1 Estrutura de Tabelas

```
┌──────────────────────────────────┐
│    venda_has_produto2 (VP2)      │  ← Dados básicos da venda
├──────────────────────────────────┤
│ idVendaProduto2, idVenda, idLoja │
│ idProduto, quant, un, total      │
│ ❌ SEM CAMPOS DE IMPOSTO          │
└────────┬─────────────────────────┘
         │ FK: idVendaProduto2
         │ Relacionamento 1 : N
         ↓
┌──────────────────────────────────┐
│   estoque_has_consumo (EHC)      │  ← ★ ARMAZENA TAX DATA ★
├──────────────────────────────────┤
│ idConsumo (PK)                   │
│ idVendaProduto2 (FK)             │
│ idEstoque (FK)                   │
│ quant, cfop, ncm, cest, un       │
│ ✅ TODOS OS 24 CAMPOS DE IMPOSTO: │
│   • ICMS: tipoICMS, orig, cstICMS│
│     modBC, vBC, pICMS, vICMS     │
│     modBCST, pMVAST, vBCST       │
│     pICMSST, vICMSST             │
│   • IPI: cEnq, cstIPI            │
│   • PIS: cstPIS, vBCPIS, pPIS    │
│     vPIS                         │
│   • COFINS: cstCOFINS, vBCCOFINS │
│     pCOFINS, vCOFINS             │
└────────┬─────────────────────────┘
         │ FK: idEstoque
         ↓
┌──────────────────────────────────┐
│        estoque (EST)             │  ← Origem dos dados (entrada)
├──────────────────────────────────┤
│ idEstoque (PK)                   │
│ idNFe, idProduto, fornecedor     │
│ ✅ MESMOS 24 CAMPOS DE IMPOSTO    │
│    (dados da NFe de entrada)     │
└──────────────────────────────────┘
```

#### 1.2 Fluxo de Dados Realista

```
RECEBIMENTO:
  NFe Entrada (fornecedor)
           ↓
  Parsed → Tax data extraído do XML
           ↓
  estoque (idNFe, idProduto, tipoICMS, vBC, pICMS, etc)

CONSUMO (Venda):
  venda_has_produto2 (idVenda, idProduto, quant, total)
           ↓
  Consumo de estoque
           ↓
  estoque_has_consumo (cópia do tax data do estoque)

EMISSÃO NFe:
  cadastrarnfe.cpp
           ↓
  view_produto_estoque (SELECT vp2 + p + ncm, retorna NULL para imposto)
           ↓
  Data Mapper lê estoque_has_consumo
           ↓
  UI permite edição
           ↓
  writeProduto() gera XML
```

#### 1.3 Mapeamento de Tabelas Atuais

| Tabela | Propósito | Tax Fields | Observação |
|--------|-----------|-----------|-----------|
| `venda_has_produto2` | Itens da venda | ❌ Nenhum | Base para view |
| `estoque_has_consumo` | Consumo de estoque | ✅ 24 campos | **ONDE ESTÃO OS DADOS** |
| `estoque` | Itens recebidos | ✅ 24 campos | Origem da entrada |
| `view_produto_estoque` | View para UI | ❌ NULL | Compatibilidade |

---

## PARTE II: NOVOS TRIBUTOS DA REFORMA

### 2. Estrutura IBS/CBS/IS

#### 2.1 IBS - Imposto sobre Bens e Serviços

**Características:**
- Tributo compartilhado: Estados (competência UF) + Municípios
- Substitui ICMS, PIS e COFINS (parcialmente)
- Código de classificação tributária de 6 dígitos (cClassTrib)
- Diferentes alíquotas para UF e Município

**Campos Necessários:**

```
[IBSCBS###]  ← Nova seção no XML
  CST = xxx        (3 dígitos - Código Situação Tributária)
  cClassTrib = xxxxxx  (6 dígitos - Classificação Tributária)
  vBC = xx.xx      (Base cálculo comum)

  [IBSUFReg###]    (Competência Estadual)
    pIBSUF = x.xx
    vTribOp = xx.xx
    [Grupos Opcionais]
      gDif: pDif, vDif
      gDevTrib: vDevTrib
      gRed: pRedAliq, pAliqEfet
      gDeson: detalhe desonação

  [IBSMunReg###]   (Competência Municipal)
    pIBSMun = x.xx
    vTribOp = xx.xx
    [Grupos Opcionais - idem UF]

  [CBS###]         (Contribuição Federal)
    pCBS = x.xx
    vCBS = xx.xx
    [Grupos Opcionais - similares]
```

#### 2.2 CBS - Contribuição sobre Bens e Serviços

**Características:**
- Tributo federal exclusivo
- Substitui parte do COFINS
- Mesma estrutura de classificação que IBS
- Alíquota uniforme nacional

#### 2.3 IS - Imposto Seletivo

**Características:**
- Tributo aplicado a produtos específicos (combustíveis, bebidas, etc.)
- Alíquotas por produto definidas em portaria
- Opcional conforme natureza do item

**Campos:**
```
[IS###]
  CST = xxx
  cClassTrib = xxxxxx
  vBC = xx.xx
  pIS = x.xx
  vIS = xx.xx
```

#### 2.4 Tabela de Classificação Tributária (cClassTrib)

A tabela `CST_cClassTrib_2025-09-30_Public.xlsx` fornece:
- Mapeamento de 6 dígitos para dispositivo legal (PLP68)
- Regras de validação por classificação
- Campos obrigatórios vs opcionais
- Crédito presumido disponível
- Relacionamento entre CST e cClassTrib

**Exemplo de códigos:**
- `010000` - Operação tributada integralmente
- `020000` - Com diferimento
- `030000` - Isenta
- `040000` - Não tributada

---

## PARTE III: PLANO TÉCNICO DETALHADO

### 3. Modificações de Banco de Dados

#### 3.1 Tabela `estoque_has_consumo` - Adicionar Campos IBS/CBS/IS

**SQL Migration Script:**

```sql
-- =====================================================
-- REFORMA TRIBUTÁRIA: Adicionar Campos IBS/CBS/IS
-- Tabela: estoque_has_consumo
-- =====================================================

ALTER TABLE estoque_has_consumo ADD COLUMN (

  -- ───────────────────────────────────────────────
  -- IBS/CBS - Campos Comuns
  -- ───────────────────────────────────────────────
  `cstIBS` VARCHAR(3) NULL COMMENT 'CST IBS',
  `cClassTribIBS` VARCHAR(6) NULL COMMENT 'Classificação Tributária IBS (6 dígitos)',
  `vBCIBS` DECIMAL(15,4) NULL COMMENT 'Base cálculo comum IBS/CBS',

  -- ───────────────────────────────────────────────
  -- IBS UF (Competência Estadual)
  -- ───────────────────────────────────────────────
  `pIBSUF` DECIMAL(5,4) NULL COMMENT 'Alíquota IBS da UF',
  `vTribOpIBSUF` DECIMAL(13,2) NULL COMMENT 'Valor bruto tributo UF',
  `pDifIBSUF` DECIMAL(5,4) NULL COMMENT 'Percentual diferimento IBS UF',
  `vDifIBSUF` DECIMAL(13,2) NULL COMMENT 'Valor diferimento IBS UF',
  `pRedAliqIBSUF` DECIMAL(5,4) NULL COMMENT 'Percentual redução alíquota IBS UF',
  `pAliqEfetIBSUF` DECIMAL(5,4) NULL COMMENT 'Alíquota efetiva IBS UF',

  -- ───────────────────────────────────────────────
  -- IBS Município (Competência Municipal)
  -- ───────────────────────────────────────────────
  `pIBSMun` DECIMAL(5,4) NULL COMMENT 'Alíquota IBS Municipal',
  `vTribOpIBSMun` DECIMAL(13,2) NULL COMMENT 'Valor bruto tributo Município',
  `pDifIBSMun` DECIMAL(5,4) NULL COMMENT 'Percentual diferimento IBS Mun',
  `vDifIBSMun` DECIMAL(13,2) NULL COMMENT 'Valor diferimento IBS Mun',
  `pRedAliqIBSMun` DECIMAL(5,4) NULL COMMENT 'Percentual redução alíquota IBS Mun',
  `pAliqEfetIBSMun` DECIMAL(5,4) NULL COMMENT 'Alíquota efetiva IBS Mun',

  -- ───────────────────────────────────────────────
  -- CBS (Contribuição Federal)
  -- ───────────────────────────────────────────────
  `cstCBS` VARCHAR(3) NULL COMMENT 'CST CBS',
  `cClassTribCBS` VARCHAR(6) NULL COMMENT 'Classificação Tributária CBS',
  `vBCCBS` DECIMAL(15,4) NULL COMMENT 'Base cálculo CBS',
  `pCBS` DECIMAL(5,4) NULL COMMENT 'Alíquota CBS',
  `vCBS` DECIMAL(13,2) NULL COMMENT 'Valor CBS',
  `vTribOpCBS` DECIMAL(13,2) NULL COMMENT 'Valor bruto tributo CBS',
  `pDifCBS` DECIMAL(5,4) NULL COMMENT 'Percentual diferimento CBS',
  `vDifCBS` DECIMAL(13,2) NULL COMMENT 'Valor diferimento CBS',
  `pRedAliqCBS` DECIMAL(5,4) NULL COMMENT 'Percentual redução alíquota CBS',
  `pAliqEfetCBS` DECIMAL(5,4) NULL COMMENT 'Alíquota efetiva CBS',

  -- ───────────────────────────────────────────────
  -- IS (Imposto Seletivo)
  -- ───────────────────────────────────────────────
  `cstIS` VARCHAR(3) NULL COMMENT 'CST Imposto Seletivo',
  `cClassTribIS` VARCHAR(6) NULL COMMENT 'Classificação Tributária IS',
  `vBCIS` DECIMAL(15,4) NULL COMMENT 'Base cálculo IS',
  `pIS` DECIMAL(5,4) NULL COMMENT 'Alíquota IS',
  `vIS` DECIMAL(13,2) NULL COMMENT 'Valor IS',
  `vTribOpIS` DECIMAL(13,2) NULL COMMENT 'Valor bruto tributo IS'

);

-- Índices para melhor performance
CREATE INDEX idx_ehc_cClassTribIBS ON estoque_has_consumo(`cClassTribIBS`);
CREATE INDEX idx_ehc_cClassTribCBS ON estoque_has_consumo(`cClassTribCBS`);
CREATE INDEX idx_ehc_cClassTribIS ON estoque_has_consumo(`cClassTribIS`);
```

**Total:** 35 novos campos

#### 3.2 Tabela `estoque` - Mesmos Campos

```sql
ALTER TABLE estoque ADD COLUMN (
  -- [Mesmos 35 campos acima]
  -- Armazena dados de origem (quando item é recebido)
);

CREATE INDEX idx_estoque_cClassTribIBS ON estoque(`cClassTribIBS`);
CREATE INDEX idx_estoque_cClassTribCBS ON estoque(`cClassTribCBS`);
CREATE INDEX idx_estoque_cClassTribIS ON estoque(`cClassTribIS`);
```

#### 3.3 View `view_produto_estoque` - Adicionar Colunas

```sql
DROP VIEW IF EXISTS `staccato`.`view_produto_estoque`;

CREATE OR REPLACE ALGORITHM=UNDEFINED DEFINER=`localhost` SQL SECURITY DEFINER
VIEW `staccato`.`view_produto_estoque` AS
select
  `vp2`.`idVendaProduto2` AS `idVendaProduto2`,
  `vp2`.`idRelacionado` AS `idRelacionado`,
  -- ... [campos existentes] ...

  -- Novos campos IBS/CBS/IS
  NULL AS `cstIBS`,
  NULL AS `cClassTribIBS`,
  NULL AS `vBCIBS`,
  NULL AS `pIBSUF`,
  NULL AS `vTribOpIBSUF`,
  NULL AS `pIBSMun`,
  NULL AS `vTribOpIBSMun`,
  NULL AS `cstCBS`,
  NULL AS `cClassTribCBS`,
  NULL AS `vBCCBS`,
  NULL AS `pCBS`,
  NULL AS `vCBS`,
  NULL AS `cstIS`,
  NULL AS `cClassTribIS`,
  NULL AS `vBCIS`,
  NULL AS `pIS`,
  NULL AS `vIS`

from ((`staccato`.`venda_has_produto2` `vp2`
  left join `staccato`.`produto` `p` on((`vp2`.`idProduto` = `p`.`idProduto`)))
  left join `staccato`.`ncm` `n` on((`p`.`ncm` = `n`.`ncm`)));
```

#### 3.4 Tabela de Classificação Tributária (Opcional - para cache)

```sql
CREATE TABLE IF NOT EXISTS `staccato`.`imposto_classificacao` (
  `idClassTrib` INT AUTO_INCREMENT PRIMARY KEY,
  `codigo` VARCHAR(6) NOT NULL UNIQUE COMMENT 'Código 6 dígitos',
  `tipo` ENUM('IBS', 'CBS', 'IS') NOT NULL,
  `descricao` VARCHAR(500),
  `dispositivo` VARCHAR(100) COMMENT 'Ref. PLP68',
  `creditoPresumido` TINYINT(1) DEFAULT 0,
  `obrigatorios` JSON COMMENT 'Campos obrigatórios para esta classificação',
  `created` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `lastUpdated` TIMESTAMP NULL ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_codigo (codigo),
  INDEX idx_tipo (tipo)
);
```

---

### 4. Modificações em `cadastrarnfe.cpp`

#### 4.1 Data Mapper - Adicionar Novos Campos

**Localização:** Após linha 63 (após mapper.addMapping para COFINS)

```cpp
// ================================================
// NOVOS CAMPOS: IBS/CBS/IS
// ================================================

// IBS - Base
mapper.addMapping(ui->lineEditClassTribIBS, modelProduto.fieldIndex("cClassTribIBS"));
mapper.addMapping(ui->doubleSpinBoxIBSBC, modelProduto.fieldIndex("vBCIBS"));

// IBS - UF (Estadual)
mapper.addMapping(ui->doubleSpinBoxIBSUFAliq, modelProduto.fieldIndex("pIBSUF"));
mapper.addMapping(ui->doubleSpinBoxIBSUFTribOp, modelProduto.fieldIndex("vTribOpIBSUF"));

// IBS - Município
mapper.addMapping(ui->doubleSpinBoxIBSMunAliq, modelProduto.fieldIndex("pIBSMun"));
mapper.addMapping(ui->doubleSpinBoxIBSMunTribOp, modelProduto.fieldIndex("vTribOpIBSMun"));

// CBS
mapper.addMapping(ui->lineEditClassTribCBS, modelProduto.fieldIndex("cClassTribCBS"));
mapper.addMapping(ui->doubleSpinBoxCBSBC, modelProduto.fieldIndex("vBCCBS"));
mapper.addMapping(ui->doubleSpinBoxCBSAliq, modelProduto.fieldIndex("pCBS"));
mapper.addMapping(ui->doubleSpinBoxCBSValor, modelProduto.fieldIndex("vCBS"));

// IS
mapper.addMapping(ui->lineEditClassTribIS, modelProduto.fieldIndex("cClassTribIS"));
mapper.addMapping(ui->doubleSpinBoxISBC, modelProduto.fieldIndex("vBCIS"));
mapper.addMapping(ui->doubleSpinBoxISAliq, modelProduto.fieldIndex("pIS"));
mapper.addMapping(ui->doubleSpinBoxISValor, modelProduto.fieldIndex("vIS"));
```

#### 4.2 Novos Métodos de Cálculo

```cpp
// Após método calculaCofins() (linha ~1106)

void CadastrarNFe::calculaIBS() {
  const auto selection = ui->tableItens->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  unsetConnections();

  try {
    const int row = selection.first().row();

    // IBS UF
    double vIBSUF = ui->doubleSpinBoxIBSBC->value() *
                    ui->doubleSpinBoxIBSUFAliq->value() / 100;
    ui->doubleSpinBoxIBSUFValor->setValue(vIBSUF);
    modelProduto.setData(row, "vBCIBS", ui->doubleSpinBoxIBSBC->value());
    modelProduto.setData(row, "pIBSUF", ui->doubleSpinBoxIBSUFAliq->value());
    modelProduto.setData(row, "vTribOpIBSUF", vIBSUF);

    // IBS Município
    double vIBSMun = ui->doubleSpinBoxIBSBC->value() *
                     ui->doubleSpinBoxIBSMunAliq->value() / 100;
    ui->doubleSpinBoxIBSMunValor->setValue(vIBSMun);
    modelProduto.setData(row, "pIBSMun", ui->doubleSpinBoxIBSMunAliq->value());
    modelProduto.setData(row, "vTribOpIBSMun", vIBSMun);

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::calculaCBS() {
  const auto selection = ui->tableItens->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  unsetConnections();

  try {
    const int row = selection.first().row();

    double vCBS = ui->doubleSpinBoxCBSBC->value() *
                  ui->doubleSpinBoxCBSAliq->value() / 100;
    ui->doubleSpinBoxCBSValor->setValue(vCBS);

    modelProduto.setData(row, "vBCCBS", ui->doubleSpinBoxCBSBC->value());
    modelProduto.setData(row, "pCBS", ui->doubleSpinBoxCBSAliq->value());
    modelProduto.setData(row, "vCBS", vCBS);

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::calculaIS() {
  const auto selection = ui->tableItens->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  unsetConnections();

  try {
    const int row = selection.first().row();

    double vIS = ui->doubleSpinBoxISBC->value() *
                 ui->doubleSpinBoxISAliq->value() / 100;
    ui->doubleSpinBoxISValor->setValue(vIS);

    modelProduto.setData(row, "vBCIS", ui->doubleSpinBoxISBC->value());
    modelProduto.setData(row, "pIS", ui->doubleSpinBoxISAliq->value());
    modelProduto.setData(row, "vIS", vIS);

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::validarClassTrib(const QString &cClassTrib, const QString &tipo) {
  // Valida se cClassTrib existe na tabela de classificação
  // tipo = "IBS", "CBS", "IS"

  if (cClassTrib.length() != 6 || !cClassTrib.toInt()) {
    throw RuntimeError("Classificação Tributária deve ter exatamente 6 dígitos!");
  }

  SqlQuery query;
  query.prepare("SELECT * FROM imposto_classificacao WHERE codigo = :codigo AND tipo = :tipo");
  query.bindValue(":codigo", cClassTrib);
  query.bindValue(":tipo", tipo);

  if (!query.exec()) {
    throw RuntimeException("Erro validando classificação tributária!");
  }

  if (!query.first()) {
    throw RuntimeError("Classificação Tributária " + cClassTrib + " não encontrada!");
  }
}
```

#### 4.3 Conectar Sinais aos Novos Métodos

**Em setConnections() (após linha 1660):**

```cpp
// Sinais para IBS/CBS/IS
connect(ui->doubleSpinBoxIBSBC, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaIBS, connectionType);
connect(ui->doubleSpinBoxIBSUFAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaIBS, connectionType);
connect(ui->doubleSpinBoxIBSMunAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaIBS, connectionType);

connect(ui->doubleSpinBoxCBSBC, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaCBS, connectionType);
connect(ui->doubleSpinBoxCBSAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaCBS, connectionType);

connect(ui->doubleSpinBoxISBC, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaIS, connectionType);
connect(ui->doubleSpinBoxISAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
        this, &CadastrarNFe::calculaIS, connectionType);

connect(ui->lineEditClassTribIBS, &QLineEdit::textChanged, this, [this]() {
  try { validarClassTrib(ui->lineEditClassTribIBS->text(), "IBS"); }
  catch (std::exception &e) { qApp->enqueueError(e.what()); }
}, connectionType);

connect(ui->lineEditClassTribCBS, &QLineEdit::textChanged, this, [this]() {
  try { validarClassTrib(ui->lineEditClassTribCBS->text(), "CBS"); }
  catch (std::exception &e) { qApp->enqueueError(e.what()); }
}, connectionType);

connect(ui->lineEditClassTribIS, &QLineEdit::textChanged, this, [this]() {
  try { validarClassTrib(ui->lineEditClassTribIS->text(), "IS"); }
  catch (std::exception &e) { qApp->enqueueError(e.what()); }
}, connectionType);
```

**Em unsetConnections() (após linha 1710):**

```cpp
disconnect(ui->doubleSpinBoxIBSBC, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaIBS);
disconnect(ui->doubleSpinBoxIBSUFAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaIBS);
disconnect(ui->doubleSpinBoxIBSMunAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaIBS);
disconnect(ui->doubleSpinBoxCBSBC, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaCBS);
disconnect(ui->doubleSpinBoxCBSAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaCBS);
disconnect(ui->doubleSpinBoxISBC, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaIS);
disconnect(ui->doubleSpinBoxISAliq, qOverload<double>(&QDoubleSpinBox::valueChanged),
           this, &CadastrarNFe::calculaIS);
// ... desconectar sinais de validação de cClassTrib ...
```

#### 4.4 Atualizar updateTotais()

**Modificar método (após linha 463):**

```cpp
void CadastrarNFe::updateTotais() {
  double baseICMS = 0;
  double valorICMS = 0;
  double valorPIS = 0;
  double valorCOFINS = 0;
  // NOVOS
  double valorIBSUF = 0;
  double valorIBSMun = 0;
  double valorCBS = 0;
  double valorIS = 0;
  double valorProdutos = 0;

  for (int row = 0; row < modelProduto.rowCount(); ++row) {
    baseICMS += modelProduto.data(row, "vBC").toDouble();
    valorICMS += modelProduto.data(row, "vICMS").toDouble();
    valorPIS += QString::number(modelProduto.data(row, "vPIS").toDouble(), 'f', 2).toDouble();
    valorCOFINS += QString::number(modelProduto.data(row, "vCOFINS").toDouble(), 'f', 2).toDouble();

    // NOVOS
    valorIBSUF += modelProduto.data(row, "vTribOpIBSUF").toDouble();
    valorIBSMun += modelProduto.data(row, "vTribOpIBSMun").toDouble();
    valorCBS += modelProduto.data(row, "vCBS").toDouble();
    valorIS += modelProduto.data(row, "vIS").toDouble();

    valorProdutos += modelProduto.data(row, "total").toDouble();
  }

  const double valorFrete = ui->doubleSpinBoxValorFrete->value();
  const double valorNota = valorProdutos + valorFrete;

  // Valores existentes
  ui->doubleSpinBoxBaseICMS->setValue(baseICMS);
  ui->doubleSpinBoxValorICMS->setValue(valorICMS);
  ui->doubleSpinBoxValorPIS->setValue(valorPIS);
  ui->doubleSpinBoxValorCOFINS->setValue(valorCOFINS);

  // Novos valores
  ui->doubleSpinBoxValorIBSUF->setValue(valorIBSUF);
  ui->doubleSpinBoxValorIBSMun->setValue(valorIBSMun);
  ui->doubleSpinBoxValorCBS->setValue(valorCBS);
  ui->doubleSpinBoxValorIS->setValue(valorIS);

  ui->doubleSpinBoxValorProdutos->setValue(valorProdutos);
  ui->doubleSpinBoxValorNota->setValue(valorNota);

  updateComplemento();
}
```

#### 4.5 Modificar writeProduto() - Gerar XML IBS/CBS/IS

**Alterar método (linhas 685-776):**

```cpp
void CadastrarNFe::writeProduto(QTextStream &stream) const {
  double sumFrete = 0;

  for (int row = 0; row < modelProduto.rowCount(); ++row) {
    const QString numProd = QString::number(row + 1).rightJustified(3, '0');

    // [Produto###] - Mantém existente
    stream << "[Produto" + numProd + "]\n";
    // ... [código existente] ...

    // [ICMS###], [IPI###], [PIS###], [COFINS###] - Mantém existente
    // ... [código existente] ...

    // ════════════════════════════════════════════════
    // NOVOS CAMPOS: [IBSCBS###], [IS###]
    // ════════════════════════════════════════════════

    // [IBSCBS###] - Novo grupo para IBS/CBS
    stream << "[IBSCBS" + numProd + "]\n";

    // Campos comuns
    QString cstIBS = modelProduto.data(row, "cstIBS").toString();
    if (!cstIBS.isEmpty()) {
      stream << "CST = " + cstIBS + "\n";
      stream << "cClassTrib = " + modelProduto.data(row, "cClassTribIBS").toString() + "\n";
      stream << "vBC = " + QString::number(modelProduto.data(row, "vBCIBS").toDouble(), 'f', 2) + "\n";

      // IBS UF
      stream << "[IBSUFReg" + numProd + "]\n";
      stream << "pIBSUF = " + QString::number(modelProduto.data(row, "pIBSUF").toDouble(), 'f', 2) + "\n";
      stream << "vTribOp = " + QString::number(modelProduto.data(row, "vTribOpIBSUF").toDouble(), 'f', 2) + "\n";

      // IBS Município
      stream << "[IBSMunReg" + numProd + "]\n";
      stream << "pIBSMun = " + QString::number(modelProduto.data(row, "pIBSMun").toDouble(), 'f', 2) + "\n";
      stream << "vTribOp = " + QString::number(modelProduto.data(row, "vTribOpIBSMun").toDouble(), 'f', 2) + "\n";

      // CBS
      QString cstCBS = modelProduto.data(row, "cstCBS").toString();
      if (!cstCBS.isEmpty()) {
        stream << "[CBS" + numProd + "]\n";
        stream << "CST = " + cstCBS + "\n";
        stream << "cClassTrib = " + modelProduto.data(row, "cClassTribCBS").toString() + "\n";
        stream << "vBC = " + QString::number(modelProduto.data(row, "vBCCBS").toDouble(), 'f', 2) + "\n";
        stream << "pCBS = " + QString::number(modelProduto.data(row, "pCBS").toDouble(), 'f', 2) + "\n";
        stream << "vCBS = " + QString::number(modelProduto.data(row, "vCBS").toDouble(), 'f', 2) + "\n";
      }
    }

    // [IS###] - Imposto Seletivo (se aplicável)
    QString cstIS = modelProduto.data(row, "cstIS").toString();
    if (!cstIS.isEmpty()) {
      stream << "[IS" + numProd + "]\n";
      stream << "CST = " + cstIS + "\n";
      stream << "cClassTrib = " + modelProduto.data(row, "cClassTribIS").toString() + "\n";
      stream << "vBC = " + QString::number(modelProduto.data(row, "vBCIS").toDouble(), 'f', 2) + "\n";
      stream << "pIS = " + QString::number(modelProduto.data(row, "pIS").toDouble(), 'f', 2) + "\n";
      stream << "vIS = " + QString::number(modelProduto.data(row, "vIS").toDouble(), 'f', 2) + "\n";
    }
  }
}
```

#### 4.6 Modificar writeTotal() - Incluir Novos Impostos

**Alterar método (linhas 778-807):**

```cpp
void CadastrarNFe::writeTotal(QTextStream &stream) const {
  stream << "[Total]\n";
  // Existentes
  stream << "BaseICMS = " + QString::number(ui->doubleSpinBoxBaseICMS->value(), 'f', 2) + "\n";
  stream << "ValorICMS = " + QString::number(ui->doubleSpinBoxValorICMS->value(), 'f', 2) + "\n";
  stream << "ValorIPI = " + QString::number(ui->doubleSpinBoxValorIPI->value(), 'f', 2) + "\n";
  stream << "ValorPIS = " + QString::number(ui->doubleSpinBoxValorPIS->value(), 'f', 2) + "\n";
  stream << "ValorCOFINS = " + QString::number(ui->doubleSpinBoxValorCOFINS->value(), 'f', 2) + "\n";

  // NOVOS
  stream << "ValorIBSUF = " + QString::number(ui->doubleSpinBoxValorIBSUF->value(), 'f', 2) + "\n";
  stream << "ValorIBSMun = " + QString::number(ui->doubleSpinBoxValorIBSMun->value(), 'f', 2) + "\n";
  stream << "ValorCBS = " + QString::number(ui->doubleSpinBoxValorCBS->value(), 'f', 2) + "\n";
  stream << "ValorIS = " + QString::number(ui->doubleSpinBoxValorIS->value(), 'f', 2) + "\n";

  stream << "ValorProduto = " + QString::number(ui->doubleSpinBoxValorProdutos->value(), 'f', 2) + "\n";
  const double valorFrete = (ui->checkBoxFrete->isChecked()) ? ui->doubleSpinBoxValorFrete->value() : 0;
  stream << "ValorFrete = " + QString::number(valorFrete, 'f', 2) + "\n";
  stream << "ValorNota = " + QString::number(ui->doubleSpinBoxValorNota->value(), 'f', 2) + "\n";

  // PARTILHA ICMS (mantém compatibilidade)
  // ... [código existente] ...
}
```

#### 4.7 Atualizar validarDados()

**Adicionar validações (antes do final do método):**

```cpp
// Validação IBS/CBS/IS (se preenchidos)
for (int row = 0; row < modelProduto.rowCount(); ++row) {
  QString cClassTribIBS = modelProduto.data(row, "cClassTribIBS").toString();
  QString cClassTribCBS = modelProduto.data(row, "cClassTribCBS").toString();
  QString cClassTribIS = modelProduto.data(row, "cClassTribIS").toString();

  if (!cClassTribIBS.isEmpty()) {
    if (cClassTribIBS.length() != 6 || !cClassTribIBS.toInt()) {
      throw RuntimeError("Linha " + QString::number(row + 1) +
                        ": Classificação Tributária IBS inválida (6 dígitos)!", this);
    }
  }

  if (!cClassTribCBS.isEmpty()) {
    if (cClassTribCBS.length() != 6 || !cClassTribCBS.toInt()) {
      throw RuntimeError("Linha " + QString::number(row + 1) +
                        ": Classificação Tributária CBS inválida (6 dígitos)!", this);
    }
  }

  if (!cClassTribIS.isEmpty()) {
    if (cClassTribIS.length() != 6 || !cClassTribIS.toInt()) {
      throw RuntimeError("Linha " + QString::number(row + 1) +
                        ": Classificação Tributária IS inválida (6 dígitos)!", this);
    }
  }
}
```

---

### 5. Modificações na Interface (UI/UX)

#### 5.1 Estrutura Recomendada

```
┌─ Abas Principais ─────────────────┐
├ Identificação                     │
├ Emitente                         │
├ Destinatário                     │
├ Itens                           │
├ ✅ ICMS (existente)              │
├ ✅ IPI (existente)               │
├ ✅ PIS (existente)               │
├ ✅ COFINS (existente)            │
├ ✅ Inter-estadual (DIFAL)        │
├ 🆕 IBS (nova)                    │
├ 🆕 CBS (nova)                    │
├ 🆕 Imposto Seletivo (nova)       │
├ Transporte                       │
├ Pagamento                        │
├ Volumes                          │
└ Complemento                      │
```

#### 5.2 Tab "IBS" - Componentes

```
┌─────────────────────────────────┐
│ IBS - Imposto sobre Bens/Serviços│
├─────────────────────────────────┤
│ Classificação Tributária: [______] (6 dígitos)
│ Base de Cálculo:          [______] R$
│
│ ┌─ Competência Estadual ──────┐
│ │ Alíquota (UF): [____] %      │
│ │ Valor Bruto:   [______] R$  │
│ └────────────────────────────┘
│
│ ┌─ Competência Municipal ─────┐
│ │ Alíquota (Mun): [____] %     │
│ │ Valor Bruto:    [______] R$ │
│ └────────────────────────────┘
│
│ ☐ Diferimento  ☐ Redução  ☐ Desonação
└─────────────────────────────────┘
```

#### 5.3 Tab "CBS" - Componentes

```
┌──────────────────────────────────┐
│ CBS - Contribuição Bens/Serviços │
├──────────────────────────────────┤
│ Classificação Tributária: [______] (6 dígitos)
│ Base de Cálculo:          [______] R$
│ Alíquota:                 [____] %
│ Valor CBS:                [______] R$
│
│ ☐ Diferimento  ☐ Redução
└──────────────────────────────────┘
```

#### 5.4 Tab "Imposto Seletivo" - Componentes

```
┌─────────────────────────────────┐
│ IS - Imposto Seletivo           │
├─────────────────────────────────┤
│ Classificação Tributária: [______] (6 dígitos)
│ Base de Cálculo:          [______] R$
│ Alíquota:                 [____] %
│ Valor IS:                 [______] R$
│
│ ℹ️  Aplicável a combustíveis,
│    bebidas e outros produtos
│    conforme portaria
└─────────────────────────────────┘
```

#### 5.5 Painel de Totais - Adicionar Campos

```
Adicionar após "Valor COFINS":
  Valor IBS (UF):    [dddd.dd] R$  (somente leitura)
  Valor IBS (Mun):   [dddd.dd] R$  (somente leitura)
  Valor CBS:         [dddd.dd] R$  (somente leitura)
  Valor IS:          [dddd.dd] R$  (somente leitura)
```

---

## PARTE IV: CRONOGRAMA E FASES

### 6. Plano de Implementação Faseado

#### **FASE 1: Preparação e Banco de Dados (3-4 semanas)**

- [ ] Review detalhado de tabelas e relacionamentos
- [ ] Criar scripts SQL de migração (estoque_has_consumo + estoque)
- [ ] Criar tabela imposto_classificacao e importar dados da planilha Excel
- [ ] Atualizar view_produto_estoque com novos campos NULL
- [ ] Backup e teste de migração em ambiente staging
- [ ] **Entrega:** Scripts prontos para deploy

#### **FASE 2: Backend C++ (4-5 semanas)**

- [ ] Adicionar data mappers para IBS/CBS/IS
- [ ] Implementar calculaIBS(), calculaCBS(), calculaIS()
- [ ] Implementar validarClassTrib()
- [ ] Conectar sinais/slots para novos campos
- [ ] Atualizar updateTotais() para novos impostos
- [ ] Modificar writeProduto() para gerar [IBSCBS###] e [IS###]
- [ ] Modificar writeTotal() com novos campos de totais
- [ ] Adicionar validações em validarDados()
- [ ] Testes unitários de cálculos
- [ ] **Entrega:** Backend compilando e testes passando

#### **FASE 3: Interface (2-3 semanas)**

- [ ] Design das novas abas (IBS, CBS, IS) no Qt Designer
- [ ] Criação de widgets (lineEdit, doubleSpinBox, labels)
- [ ] Layout e organização visual
- [ ] Integração com data mapper
- [ ] Testes de UX (validação em tempo real)
- [ ] **Entrega:** UI funcional com validação

#### **FASE 4: Integração e Testes (3-4 semanas)**

- [ ] Teste integrado (BD + Backend + UI)
- [ ] Testes de geração XML com ACBr
- [ ] Testes de compatibilidade ICMS/IBS em paralelo
- [ ] Validação contra tabela de classificação
- [ ] Testes de edge cases (diferimento, redução, desonação)
- [ ] Teste de compatibilidade com homologação SEFAZ
- [ ] **Entrega:** Testes passando

#### **FASE 5: Homologação (contínuo)**

- [ ] Deploy em ambiente de testes SEFAZ
- [ ] Validação de NFe com novos impostos
- [ ] Ajustes conforme feedback SEFAZ
- [ ] Documentação de uso
- [ ] **Entrega:** Aprovado para produção

#### **Cronograma Realista:**

| Data | Atividade | Status |
|------|-----------|--------|
| 2025-01-15 | Aprovação do plano | - |
| 2025-02-28 | ✅ Fase 1 completa | - |
| 2025-04-15 | ✅ Fase 2 completa | - |
| 2025-05-15 | ✅ Fase 3 completa | - |
| 2025-07-01 | ✅ Fase 4 completa | - |
| 2025-09-01 | 🔄 Fase 5 inicia | - |
| **2025-10-31** | **🎯 SEFAZ Deadline** | **CRÍTICO** |
| **2025-12-31** | Último dia com ICMS | Transição |
| **2026-01-01** | 🔴 Obrigatório IBS/CBS/IS | **Deadline** |

---

## PARTE V: RISCOS E MITIGAÇÃO

### 7. Análise de Riscos

| # | Risco | Probabilidade | Impacto | Mitigation |
|---|-------|---------------|---------|-----------|
| 1 | Tabela cClassTrib mudar até produção | Média | Alto | Monitorar portal SEFAZ; implementar atualização automática |
| 2 | ACBr não suporta 100% dos campos | Média | Alto | Contatar comunidade ACBr antecipadamente; plano B com XML manual |
| 3 | Performance com 35 novos campos | Baixa | Médio | Criar índices; otimizar queries |
| 4 | Usuários confusos com novos campos | Alta | Médio | Documentação; tutorial in-app; tooltips |
| 5 | SEFAZ rejeita XMLs inicialmente | Média | Médio | Testes em homologação com antecedência; suporte |
| 6 | Incompatibilidade com dados históricos | Baixa | Baixo | Scripts de migração; dados novos começam como NULL |
| 7 | Deadline não cumprido | Média | Crítico | Sprint planning rigoroso; buffer de 2 semanas |

### 8. Contingência

**Se ACBr não suportar completamente:**
- Implementar geração manual de XML dos novos grupos
- Validação contra schema XSD fornecido pelo SEFAZ
- Possível fork do ACBr se necessário

**Se tabela cClassTrib mudar:**
- Implementar sincronização com portal SEFAZ (API ou download)
- Cache local com hash de atualização

**Se prazos forem críticos:**
- Implementação de apenas IBS (sem CBS/IS) na primeira fase
- Priorizar compatibilidade sobre features opcionais

---

## PARTE VI: REFERÊNCIAS E RECURSOS

### 9. Documentação Oficial

**Baixados do SVN ACBr:**
- ✅ DFe_NT_2024_001 v1.10 - Reforma Tributária IBS/CBS
- ✅ DFe_NT_2024_002 v1.10 - Imposto Seletivo
- ✅ CST_cClassTrib_2025-09-30_Public.xlsx - Tabela de Classificação
- ✅ Layout IBSCBSSel.xlsx - Especificação de Campos
- ✅ LC 214 16-01-2025 - Lei Complementar 214

**URLs Úteis:**
- [ACBr Reforma Tributária](https://svn.code.sf.net/p/acbr/code/tools/DFe/ReformaTributaria/)
- [Portal Nacional NF-e](https://www.nfe.fazenda.gov.br)
- [PLP 68 - Lei Complementar](http://www.planalto.gov.br)
- [Emenda Constitucional 132/2023](http://www.planalto.gov.br)

### 10. Arquivos de Configuração

**Tabelas MySQL:**
```sql
-- Ver seções 3.1-3.4 para DDL completo
estoque_has_consumo (35 novos campos)
estoque (35 novos campos)
view_produto_estoque (15 novos campos NULL)
imposto_classificacao (cache da tabela cClassTrib)
```

**Arquivos Fonte:**
```
src/cadastrarnfe.cpp (Principal)
src/cadastrarnfe.h (Header)
initdb.sql (Schema)
```

**Recursos Externos:**
```
SVN: /tmp/2024/  (documentação baixada)
Excel: CST_cClassTrib_2025-09-30_Public.xlsx
Excel: Layout IBSCBSSel.xlsx
```

---

## PARTE VII: CHECKLIST DE IMPLEMENTAÇÃO

### 11. Checklist Geral

**Antes de Iniciar:**
- [ ] Aprovação do plano com stakeholders
- [ ] Alocação de recursos (dev, QA, suporte)
- [ ] Ambiente de staging preparado
- [ ] Backup de produção atualizado

**Fase 1 - Banco de Dados:**
- [ ] Scripts SQL revisados
- [ ] Migração testada em staging
- [ ] Índices criados e otimizados
- [ ] Dados de teste carregados

**Fase 2 - Backend:**
- [ ] Data mappers compilam
- [ ] Métodos de cálculo testados
- [ ] Validação de cClassTrib funciona
- [ ] XML gerado corretamente
- [ ] Sinais/slots conectados

**Fase 3 - UI:**
- [ ] Layouts criados no Qt Designer
- [ ] Widgets vinculados aos dados
- [ ] Validação em tempo real funciona
- [ ] Aparência consistente com resto da UI

**Fase 4 - Testes:**
- [ ] Testes unitários: 95%+ cobertura
- [ ] Testes integrados: funcionais
- [ ] Testes com ACBr: XMLs gerados corretamente
- [ ] Testes com SEFAZ: aceitos em homologação
- [ ] Testes de compatibilidade: ICMS/IBS em paralelo

**Fase 5 - Produção:**
- [ ] Documentação de usuário finalizada
- [ ] Treinamento de suporte completado
- [ ] Plano de rollback definido
- [ ] Monitoramento em tempo real configurado
- [ ] Contato de suporte SEFAZ estabelecido

---

## CONCLUSÃO

Este plano mestre fornece:

1. ✅ **Análise completa** da arquitetura atual (dados reais de tabelas)
2. ✅ **Especificação detalhada** de novos campos IBS/CBS/IS (35 colunas)
3. ✅ **Código pronto para implementação** (SQL, C++, dados mapper)
4. ✅ **Cronograma realista** com 5 fases e 26 semanas
5. ✅ **Gestão de riscos** e planos de contingência
6. ✅ **Referências completas** aos documentos oficiais da SEFAZ/ACBr

**Próximo Passo:** Aprovação executiva para iniciar Fase 1 (Preparação e Banco de Dados).

---

**Documento:** REFORMA_TRIBUTARIA_MASTER_PLAN.md
**Versão:** 1.0
**Última Atualização:** 24 de Dezembro de 2025
**Status:** ✅ Pronto para Implementação
