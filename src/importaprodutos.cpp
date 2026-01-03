#include "importaprodutos.h"
#include "ui_importaprodutos.h"

#include "application.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "importaprodutosproxymodel.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sqlquery.h"
#include "user.h"
#include "validadedialog.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

namespace {
// Optimized string normalization: toUpper + trim + left(maxLen) in single pass
// Avoids creating 3 temporary QStrings
QString normalizeString(const QString &input, int maxLen) {
  if (input.isEmpty()) return input;

  // Find first non-whitespace
  int start = 0;
  while (start < input.size() && input[start].isSpace()) ++start;

  // Find last non-whitespace
  int end = input.size() - 1;
  while (end > start && input[end].isSpace()) --end;

  // Calculate length (trimmed)
  int len = end - start + 1;
  if (len <= 0) return QString();

  // Apply maxLen limit
  if (len > maxLen) len = maxLen;

  // Build result with uppercase in single allocation
  QString result;
  result.reserve(len);
  for (int i = start; i < start + len; ++i) {
    result.append(input[i].toUpper());
  }
  return result;
}
}  // namespace

// Column order for preview model (must match setupTables column visibility)
const QStringList ImportaProdutos::PREVIEW_COLUMNS = {
    "idProduto",   "idFornecedor",  "fornecedor",    "descricao",   "un",           "un2",        "colecao",       "m2cx",     "pccx",
    "kgcx",        "formComercial", "codComercial", "codBarras",   "ncm",        "qtdPallet",     "custo",    "precoVenda",
    "ui",          "minimo",        "mva",         "st",           "sticms",     "quantCaixa",    "markup",   "validade",
    "descontinuado"};

ImportaProdutos::ImportaProdutos(const Tipo tipo_, QWidget *parent) : QDialog(parent), tipo(tipo_), ui(new Ui::ImportaProdutos) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setProgressDialog();
  setupModels();

  setConnections();
}

ImportaProdutos::~ImportaProdutos() { delete ui; }

void ImportaProdutos::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &ImportaProdutos::on_checkBoxRepresentacao_toggled, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ImportaProdutos::on_pushButtonSalvar_clicked, connectionType);
}

void ImportaProdutos::importarTabela() {
  try {
    if (not readFile() or not readValidade()) {
      close();
      return;
    }

    qApp->startTransaction("ImportaProdutos::importaTabela");

    processarArquivoOptimized();
  } catch (std::exception &) {
    close();
    throw;
  }
}

void ImportaProdutos::importarTabelaCLI(const QString &filePath, int validadeDias) {
  file = filePath;
  validade = validadeDias;

  if (validade != -1) { validadeString = qApp->serverDate().addDays(validade).toString("yyyy-MM-dd"); }

  setWindowTitle(file);

  qApp->startTransaction("ImportaProdutos::importarTabelaCLI");

  processarArquivoOptimized();
}

void ImportaProdutos::verificaSeRepresentacao() {
  SqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", m_fornecedor.left(100));

  if (not queryFornecedor.exec()) { throw RuntimeException("Erro lendo tabela fornecedor: " + queryFornecedor.lastError().text()); }

  if (not queryFornecedor.first()) { throw RuntimeException("Dados não encontrados para fornecedor: '" + m_fornecedor + "'"); }

  ui->checkBoxRepresentacao->setChecked(queryFornecedor.value("representacao").toBool());
}

void ImportaProdutos::atualizaProduto() {
  const int row = hashModel.value(produto.fornecedor + produto.codComercial + produto.ui + QString::number(static_cast<int>(tipo)));

  if (vectorProdutosImportados.contains(row)) {
    produto.fornecedor = "PRODUTO REPETIDO NA TABELA";
    return insereEmErro();
  }

  vectorProdutosImportados << row;

  atualizaCamposProduto(row);
  marcaProdutoNaoDescontinuado(row);
}

void ImportaProdutos::processarArquivo() {
  QXlsx::Document xlsx(file, this);

  xlsx.selectSheet("BASE");
  verificaTabela(xlsx);

#ifndef BENCHMARK_BUILD
  progressDialog.show();
#endif

  cadastraFornecedores(xlsx);
  verificaSeRepresentacao();
  marcaTodosProdutosDescontinuados();
  mostraApenasEstesFornecedores();

  // Disable proxy during bulk operations for performance (saves ~20% overhead)
  auto *savedProxy = modelProduto.proxyModel;
  modelProduto.proxyModel = nullptr;

  itensExpired = modelProduto.rowCount();

  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
    hashModel[modelProduto.data(row, "fornecedor").toString() + modelProduto.data(row, "codComercial").toString() + modelProduto.data(row, "ui").toString() +
              modelProduto.data(row, "promocao").toString()] = row;
  }

  int current = 0;
  bool canceled = false;

  const int rowCount = xlsx.dimension().rowCount();

  for (int row = 2; row <= rowCount; ++row) {
    if (progressDialog.wasCanceled()) {
      canceled = true;
      break;
    }

    progressDialog.setValue(current++);

    if (xlsx.readValue(row, 1).toString().isEmpty()) { continue; }

    leituraProduto(xlsx, row);

    if (camposForaDoPadrao()) {
      insereEmErro();
      continue;
    }

    const bool existeNoModel = hashModel.contains(produto.fornecedor + produto.codComercial + produto.ui + QString::number(static_cast<int>(tipo)));
    existeNoModel ? atualizaProduto() : insereEmOk();
  }

  progressDialog.cancel();

  if (canceled) { throw std::exception(); }

  // Restore proxy for UI display
  modelProduto.proxyModel = savedProxy;

#ifdef BENCHMARK_BUILD
  qInfo() << "Produtos importados:" << itensImported << "| Atualizados:" << itensUpdated
          << "| Não modificados:" << itensNotChanged << "| Descontinuados:" << itensExpired << "| Com erro:" << itensError;
#else
  setupTables();

  ui->tableProdutos->sortByColumn("descontinuado", Qt::AscendingOrder);

  showMaximized();

  const QString resultado = "Produtos importados: " + QString::number(itensImported) + "\nProdutos atualizados: " + QString::number(itensUpdated) +
                            "\nNão modificados: " + QString::number(itensNotChanged) + "\nDescontinuados: " + QString::number(itensExpired) + "\nCom erro: " + QString::number(itensError);

  QMessageBox::information(this, "Aviso!", resultado);
#endif
}

void ImportaProdutos::setProgressDialog() {
  progressDialog.reset();
  progressDialog.setCancelButton(nullptr);
  progressDialog.setLabelText("Importando...");
  progressDialog.setWindowTitle("ERP Staccato");
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.setMinimum(0);
  progressDialog.setMaximum(0);
  progressDialog.setCancelButtonText("Cancelar");
}

// ============================================================================
// OPTIMIZED PROCESSING - Bypasses QSqlTableModel for ~10x performance improvement
// ============================================================================

void ImportaProdutos::processarArquivoOptimized() {
  QXlsx::Document xlsx(file, this);

  xlsx.selectSheet("BASE");
  verificaTabela(xlsx);

#ifndef BENCHMARK_BUILD
  progressDialog.show();
#endif

  cadastraFornecedores(xlsx);
  verificaSeRepresentacao();
  marcaTodosProdutosDescontinuados();

  // Phase 1: Load existing products via direct SQL (bypasses QSqlTableModel)
  loadExistingProducts();

  const int rowCount = xlsx.dimension().rowCount();
  QSet<QString> processedKeys;

  int current = 0;
  bool canceled = false;

  // Phase 2: Process Excel rows and build change lists
  for (int row = 2; row <= rowCount; ++row) {
    if (progressDialog.wasCanceled()) {
      canceled = true;
      break;
    }

    progressDialog.setValue(current++);

    if (xlsx.readValue(row, 1).toString().isEmpty()) { continue; }

    Produto p = parseExcelRow(xlsx, row);
    QString key = makeProductKey(p);

    // Check for duplicates
    if (processedKeys.contains(key)) {
      Produto errorProd = p;
      errorProd.fornecedor = "PRODUTO REPETIDO NA TABELA";
      m_errorChanges.append(makeErrorChange(errorProd, "PRODUTO REPETIDO"));
      continue;
    }
    processedKeys.insert(key);

    // Validate fields
    if (camposForaDoPadrao(p)) {
      m_errorChanges.append(makeErrorChange(p, "CAMPOS FORA DO PADRAO"));
      continue;
    }

    // Compare with existing or create new
    if (m_existingProducts.contains(key)) {
      ProductChange change = compareProducts(m_existingProducts[key], p);
      m_productChanges.append(change);

      if (change.type == ProductChange::Type::Updated) {
        itensUpdated++;
      } else {
        itensNotChanged++;
      }
      itensExpired--;  // Not discontinued
    } else {
      m_productChanges.append(makeNewProductChange(p));
      itensImported++;
    }
  }

  progressDialog.cancel();

  if (canceled) { throw std::exception(); }

  // Count errors
  itensError = m_errorChanges.size();

  // Phase 3: Build preview models (uses QStandardItemModel - very fast)
  buildPreviewModels();

#ifdef BENCHMARK_BUILD
  qInfo() << "Produtos importados:" << itensImported << "| Atualizados:" << itensUpdated
          << "| Não modificados:" << itensNotChanged << "| Descontinuados:" << itensExpired << "| Com erro:" << itensError;
#else
  setupTables();

  showMaximized();

  const QString resultado = "Produtos importados: " + QString::number(itensImported) + "\nProdutos atualizados: " + QString::number(itensUpdated) +
                            "\nNão modificados: " + QString::number(itensNotChanged) + "\nDescontinuados: " + QString::number(itensExpired) + "\nCom erro: " + QString::number(itensError);

  QMessageBox::information(this, "Aviso!", resultado);
#endif
}

void ImportaProdutos::loadExistingProducts() {
  SqlQuery query;
  // ORDER BY idProduto to match QSqlTableModel's default ordering
  query.prepare(
      "SELECT idProduto, idFornecedor, fornecedor, descricao, un, un2, colecao, m2cx, pccx, kgcx, "
      "formComercial, codComercial, codBarras, ncm, qtdPallet, custo, precoVenda, ui, minimo, "
      "mva, st, sticms, quantCaixa, markup, validade "
      "FROM produto WHERE idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = :promocao "
      "ORDER BY idProduto");
  query.bindValue(":promocao", static_cast<int>(tipo));

  if (not query.exec()) { throw RuntimeException("Erro carregando produtos existentes: " + query.lastError().text()); }

  m_existingProducts.clear();
  m_existingProducts.reserve(query.size() > 0 ? query.size() : 10000);
  m_allExistingProducts.clear();
  m_allExistingProducts.reserve(query.size() > 0 ? query.size() : 10000);

  // Cache field indices for positional access (avoids string lookup per field per row)
  const QSqlRecord rec = query.record();
  const int iIdProduto = rec.indexOf("idProduto");
  const int iIdFornecedor = rec.indexOf("idFornecedor");
  const int iFornecedor = rec.indexOf("fornecedor");
  const int iDescricao = rec.indexOf("descricao");
  const int iUn = rec.indexOf("un");
  const int iUn2 = rec.indexOf("un2");
  const int iColecao = rec.indexOf("colecao");
  const int iM2cx = rec.indexOf("m2cx");
  const int iPccx = rec.indexOf("pccx");
  const int iKgcx = rec.indexOf("kgcx");
  const int iFormComercial = rec.indexOf("formComercial");
  const int iCodComercial = rec.indexOf("codComercial");
  const int iCodBarras = rec.indexOf("codBarras");
  const int iNcm = rec.indexOf("ncm");
  const int iQtdPallet = rec.indexOf("qtdPallet");
  const int iCusto = rec.indexOf("custo");
  const int iPrecoVenda = rec.indexOf("precoVenda");
  const int iUi = rec.indexOf("ui");
  const int iMinimo = rec.indexOf("minimo");
  const int iMva = rec.indexOf("mva");
  const int iSt = rec.indexOf("st");
  const int iSticms = rec.indexOf("sticms");
  const int iQuantCaixa = rec.indexOf("quantCaixa");
  const int iMarkup = rec.indexOf("markup");
  const int iValidade = rec.indexOf("validade");

  while (query.next()) {
    Produto p;
    p.idProduto = query.value(iIdProduto).toInt();
    p.idFornecedor = query.value(iIdFornecedor).toInt();
    // Normalize strings using optimized single-pass function
    p.fornecedor = normalizeString(query.value(iFornecedor).toString(), 100);
    p.descricao = query.value(iDescricao).toString();
    p.un = query.value(iUn).toString();
    p.un2 = query.value(iUn2).toString();
    p.colecao = query.value(iColecao).toString();
    p.m2cx = query.value(iM2cx).toDouble();
    p.pccx = query.value(iPccx).toDouble();
    p.kgcx = query.value(iKgcx).toDouble();
    p.formComercial = query.value(iFormComercial).toString();
    p.codComercial = normalizeString(query.value(iCodComercial).toString(), 100);
    p.codBarras = query.value(iCodBarras).toString();
    p.ncm = query.value(iNcm).toString();
    p.qtdPallet = query.value(iQtdPallet);  // Preserve NULL
    p.custo = query.value(iCusto).toDouble();
    p.precoVenda = query.value(iPrecoVenda).toDouble();
    p.ui = normalizeString(query.value(iUi).toString(), 45);
    if (p.ui.isEmpty()) { p.ui = QStringLiteral("0"); }
    p.minimo = query.value(iMinimo);        // Preserve NULL
    p.mva = query.value(iMva);              // Preserve NULL
    p.st = query.value(iSt);                // Preserve NULL
    p.sticms = query.value(iSticms);        // Preserve NULL
    p.quantCaixa = query.value(iQuantCaixa).toDouble();
    p.markup = query.value(iMarkup).toDouble();
    p.validade = query.value(iValidade).toDate();

    QString key = makeProductKey(p);
    m_existingProducts[key] = p;
    m_allExistingProducts.append(p);
  }

  // Use total product count (including duplicates) for expired count to match legacy behavior
  itensExpired = m_allExistingProducts.size();
}

ImportaProdutos::Produto ImportaProdutos::parseExcelRow(QXlsx::Document &xlsx, int row) {
  Produto p;
  const QLocale locale(QLocale::Portuguese);

  QVariant fornecedor = xlsx.readValue(row, 1);
  QVariant descricao = xlsx.readValue(row, 2);
  QVariant un = xlsx.readValue(row, 3);
  QVariant colecao = xlsx.readValue(row, 4);

  QVariant m2cx = xlsx.readValue(row, 5);
  if (m2cx.userType() == QMetaType::QString) { m2cx = locale.toDouble(m2cx.toString()); }
  m2cx = qApp->roundDouble(m2cx.toDouble());

  QVariant pccx = xlsx.readValue(row, 6);
  if (pccx.userType() == QMetaType::QString) { pccx = locale.toDouble(pccx.toString()); }
  pccx = qApp->roundDouble(pccx.toDouble());

  QVariant kgcx = xlsx.readValue(row, 7);
  if (kgcx.userType() == QMetaType::QString) { kgcx = locale.toDouble(kgcx.toString()); }
  kgcx = qApp->roundDouble(kgcx.toDouble());

  QVariant formComercial = xlsx.readValue(row, 8);
  QVariant codComercial = xlsx.readValue(row, 9);
  QVariant codBarras = xlsx.readValue(row, 10);
  QVariant ncm = xlsx.readValue(row, 11);

  QVariant qtdPallet = xlsx.readValue(row, 12);
  if (qtdPallet.userType() == QMetaType::QString) { qtdPallet = locale.toDouble(qtdPallet.toString()); }
  qtdPallet = qApp->roundDouble(qtdPallet.toDouble());

  QVariant custo = xlsx.readValue(row, 13);
  if (custo.userType() == QMetaType::QString) { custo = locale.toDouble(custo.toString()); }
  custo = qApp->roundDouble(custo.toDouble());

  QVariant precoVenda = xlsx.readValue(row, 14);
  if (precoVenda.userType() == QMetaType::QString) { precoVenda = locale.toDouble(precoVenda.toString()); }
  precoVenda = qApp->roundDouble(precoVenda.toDouble());

  QVariant ui2 = xlsx.readValue(row, 15);
  QVariant un2 = xlsx.readValue(row, 16);

  QVariant minimo = xlsx.readValue(row, 17);
  if (minimo.userType() == QMetaType::QString) { minimo = locale.toDouble(minimo.toString()); }
  minimo = qApp->roundDouble(minimo.toDouble());

  QVariant mva = xlsx.readValue(row, 18);
  if (mva.userType() == QMetaType::QString) { mva = locale.toDouble(mva.toString()); }
  mva = qApp->roundDouble(mva.toDouble());

  QVariant st = xlsx.readValue(row, 19);
  if (st.userType() == QMetaType::QString) { st = locale.toDouble(st.toString()); }
  st = qApp->roundDouble(st.toDouble());

  QVariant sticms = xlsx.readValue(row, 20);
  if (sticms.userType() == QMetaType::QString) { sticms = locale.toDouble(sticms.toString()); }
  sticms = qApp->roundDouble(sticms.toDouble());

  p.idFornecedor = m_fornecedores.value(fornecedor.toString().trimmed());
  p.fornecedor = fornecedor.toString().toUpper().trimmed().left(100);
  p.descricao = descricao.toString().remove("*").remove("()").replace('_', ' ').toUpper().trimmed().left(250);
  p.un = un.toString().remove("*").toUpper().trimmed().left(45);
  p.colecao = colecao.toString().remove("*").toUpper().trimmed().left(200);
  p.m2cx = m2cx.toDouble();
  p.pccx = pccx.toDouble();
  p.kgcx = kgcx.toDouble();
  p.formComercial = formComercial.toString().remove("*").toUpper().trimmed().left(100);
  p.codComercial = codComercial.toString().remove("*").remove(".").remove(",").toUpper().trimmed().left(100);
  p.codBarras = codBarras.toString().remove("*").remove(".").remove(",").toUpper().trimmed().left(100);
  p.ncm = ncm.toString().remove("*").remove(".").remove(",").remove("-").remove(" ").toUpper().trimmed().left(10);
  p.qtdPallet = qtdPallet;  // Keep as QVariant (nullable)
  p.custo = custo.toDouble();
  p.precoVenda = precoVenda.toDouble();
  p.ui = ui2.toString().remove("*").toUpper().trimmed().left(45);
  p.un2 = un2.toString().remove("*").toUpper().trimmed().left(45);
  p.minimo = minimo;        // Keep as QVariant (nullable)
  p.mva = mva;              // Keep as QVariant (nullable)
  p.st = st;                // Keep as QVariant (nullable)
  p.sticms = sticms;        // Keep as QVariant (nullable)
  p.markup = qApp->roundDouble(((p.precoVenda / p.custo) - 1.) * 100);

  // Data consistency
  if (p.ui.isEmpty()) { p.ui = "0"; }
  if (p.codBarras == "0") { p.codBarras.clear(); }
  if (p.ncm == "0") { p.ncm.clear(); }
  if (p.ncm.length() == 6) { p.ncm.append("00"); }
  if (p.un == "M²") { p.un = "M2"; }

  p.quantCaixa = (p.un == "M2" or p.un == "ML") ? p.m2cx : p.pccx;

  if (validade != -1) {
    p.validade = qApp->serverDate().addDays(validade);
  }

  return p;
}

QString ImportaProdutos::makeProductKey(const Produto &p) const {
  return p.fornecedor + p.codComercial + p.ui + QString::number(static_cast<int>(tipo));
}

bool ImportaProdutos::camposForaDoPadrao(const Produto &p) const {
  if ((p.un == "M2" or p.un == "ML") and p.m2cx <= 0.) { return true; }
  if (p.un != "M2" and p.un != "ML" and p.pccx < 1) { return true; }
  if (p.codComercial == "0" or p.codComercial.isEmpty()) { return true; }
  if (p.custo <= 0.) { return true; }
  if (p.precoVenda <= 0.) { return true; }
  if (p.precoVenda < p.custo) { return true; }
  if (not ui->checkBoxRepresentacao->isChecked() and p.ncm.length() != 8) { return true; }

  return false;
}

ImportaProdutos::ProductChange ImportaProdutos::compareProducts(const Produto &existing, const Produto &imported) {
  ProductChange change;
  change.oldData = existing;
  change.newData = imported;
  change.newData.idProduto = existing.idProduto;  // Preserve ID for update

  bool hasChanges = false;

  // Compare each field and set status
  auto compareField = [&](const QString &field, const QVariant &oldVal, const QVariant &newVal) {
    bool changed = false;
    if (oldVal.userType() == QMetaType::Double || newVal.userType() == QMetaType::Double) {
      changed = not qFuzzyCompare(oldVal.toDouble(), newVal.toDouble());
    } else {
      changed = (oldVal != newVal);
    }

    if (changed) {
      change.fieldStatus[field] = FieldStatus::Changed;
      hasChanges = true;
    } else {
      change.fieldStatus[field] = FieldStatus::NoChange;
    }
  };

  compareField("fornecedor", existing.fornecedor, imported.fornecedor);
  compareField("descricao", existing.descricao, imported.descricao);
  compareField("un", existing.un, imported.un);
  compareField("un2", existing.un2, imported.un2);
  compareField("colecao", existing.colecao, imported.colecao);
  compareField("m2cx", existing.m2cx, imported.m2cx);
  compareField("pccx", existing.pccx, imported.pccx);
  compareField("kgcx", existing.kgcx, imported.kgcx);
  compareField("formComercial", existing.formComercial, imported.formComercial);
  compareField("codComercial", existing.codComercial, imported.codComercial);
  compareField("codBarras", existing.codBarras, imported.codBarras);
  compareField("ncm", existing.ncm, imported.ncm);
  compareField("qtdPallet", existing.qtdPallet, imported.qtdPallet);
  compareField("custo", existing.custo, imported.custo);
  compareField("precoVenda", existing.precoVenda, imported.precoVenda);
  compareField("ui", existing.ui, imported.ui);
  compareField("minimo", existing.minimo, imported.minimo);
  compareField("mva", existing.mva, imported.mva);
  compareField("st", existing.st, imported.st);
  compareField("sticms", existing.sticms, imported.sticms);
  compareField("quantCaixa", existing.quantCaixa, imported.quantCaixa);
  compareField("markup", existing.markup, imported.markup);

  // Validade comparison
  if ((existing.validade.isValid() and existing.validade != imported.validade) or
      (not existing.validade.isValid() and imported.validade.isValid())) {
    change.fieldStatus["validade"] = FieldStatus::Changed;
    hasChanges = true;
  } else {
    change.fieldStatus["validade"] = FieldStatus::NoChange;
  }

  change.type = hasChanges ? ProductChange::Type::Updated : ProductChange::Type::NotChanged;
  return change;
}

ImportaProdutos::ProductChange ImportaProdutos::makeNewProductChange(const Produto &p) {
  ProductChange change;
  change.type = ProductChange::Type::New;
  change.newData = p;

  // All fields are new
  for (const QString &field : PREVIEW_COLUMNS) {
    if (field != "idProduto" && field != "descontinuado") {
      change.fieldStatus[field] = FieldStatus::New;
    }
  }

  return change;
}

ImportaProdutos::ProductChange ImportaProdutos::makeDiscontinuedChange(const Produto &p) {
  ProductChange change;
  change.type = ProductChange::Type::Discontinued;
  change.newData = p;
  change.oldData = p;

  // No field changes for discontinued
  for (const QString &field : PREVIEW_COLUMNS) {
    change.fieldStatus[field] = FieldStatus::NoChange;
  }

  return change;
}

ImportaProdutos::ProductChange ImportaProdutos::makeErrorChange(const Produto &p, const QString & /*reason*/) {
  ProductChange change;
  change.type = ProductChange::Type::Error;
  change.newData = p;

  // Mark invalid fields
  for (const QString &field : PREVIEW_COLUMNS) {
    if (field != "idProduto" && field != "descontinuado") {
      change.fieldStatus[field] = FieldStatus::New;
    }
  }

  // Mark specific error fields
  if (p.fornecedor == "PRODUTO REPETIDO NA TABELA") {
    change.fieldStatus["fornecedor"] = FieldStatus::Invalid;
  }
  if ((p.un == "M2" or p.un == "ML") and p.m2cx <= 0.) {
    change.fieldStatus["m2cx"] = FieldStatus::Invalid;
  }
  if (p.un != "M2" and p.un != "ML" and p.pccx < 1) {
    change.fieldStatus["pccx"] = FieldStatus::Invalid;
  }
  if (p.codComercial == "0" or p.codComercial.isEmpty()) {
    change.fieldStatus["codComercial"] = FieldStatus::Invalid;
  }
  if (p.custo <= 0.) {
    change.fieldStatus["custo"] = FieldStatus::Invalid;
  }
  if (p.precoVenda <= 0. or p.precoVenda < p.custo) {
    change.fieldStatus["precoVenda"] = FieldStatus::Invalid;
  }
  if (p.ncm.length() != 8) {
    change.fieldStatus["ncm"] = (p.ncm.isEmpty() or p.ncm == "00000000") ? FieldStatus::OutOfSpec : FieldStatus::Invalid;
  }
  if (p.codBarras.isEmpty() or p.codBarras == "0") {
    change.fieldStatus["codBarras"] = FieldStatus::OutOfSpec;
  }

  return change;
}

QBrush ImportaProdutos::getFieldColor(FieldStatus status) const {
  switch (status) {
    case FieldStatus::New: return QBrush(Qt::green);
    case FieldStatus::Changed: return QBrush(Qt::yellow);
    case FieldStatus::OutOfSpec: return QBrush(Qt::gray);
    case FieldStatus::Invalid: return QBrush(Qt::red);
    default: return QBrush();
  }
}

void ImportaProdutos::buildPreviewModels() {
  // Cache theme setting once (avoid 16000+ lookups)
  const QString tema = User::getSetting("User/tema").toString();
  const QBrush textColor = (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
  const QBrush cyanBrush(Qt::cyan);

  // Setup column headers
  const int colCount = PREVIEW_COLUMNS.size();
  m_previewModel.setColumnCount(colCount);
  m_errorModel.setColumnCount(colCount);

  for (int i = 0; i < colCount; ++i) {
    m_previewModel.setHeaderData(i, Qt::Horizontal, PREVIEW_COLUMNS[i]);
    m_errorModel.setHeaderData(i, Qt::Horizontal, PREVIEW_COLUMNS[i]);
  }

  // Build set of processed product IDs for fast lookup
  QSet<int> processedIds;
  processedIds.reserve(m_productChanges.size());
  for (const ProductChange &change : qAsConst(m_productChanges)) {
    if (change.newData.idProduto > 0) {
      processedIds.insert(change.newData.idProduto);
    }
  }

  // Count discontinued products for pre-allocation
  int discontinuedCount = 0;
  for (const Produto &p : qAsConst(m_allExistingProducts)) {
    if (!processedIds.contains(p.idProduto)) {
      ++discontinuedCount;
    }
  }

  // Pre-allocate rows (avoid incremental resizing)
  const int previewRowCount = m_productChanges.size() + discontinuedCount;
  m_previewModel.setRowCount(previewRowCount);
  m_errorModel.setRowCount(m_errorChanges.size());

  // Block signals during bulk insert (avoid per-cell signal emission)
  m_previewModel.blockSignals(true);
  m_errorModel.blockSignals(true);

  int previewRow = 0;

  // Add product changes (new, updated, not changed)
  for (const ProductChange &change : qAsConst(m_productChanges)) {
    addRowToModelFast(m_previewModel, previewRow++, change, textColor, cyanBrush);
  }

  // Add discontinued products (ALL existing products that weren't processed)
  for (const Produto &p : qAsConst(m_allExistingProducts)) {
    if (!processedIds.contains(p.idProduto)) {
      ProductChange discontinued = makeDiscontinuedChange(p);
      addRowToModelFast(m_previewModel, previewRow++, discontinued, textColor, cyanBrush);
    }
  }

  // Add errors
  int errorRow = 0;
  for (const ProductChange &change : qAsConst(m_errorChanges)) {
    addRowToModelFast(m_errorModel, errorRow++, change, textColor, cyanBrush);
  }

  // Re-enable signals
  m_previewModel.blockSignals(false);
  m_errorModel.blockSignals(false);
}

void ImportaProdutos::addRowToModel(QStandardItemModel &model, const ProductChange &change) {
  // Legacy version - calls the fast version with default brushes
  const QString tema = User::getSetting("User/tema").toString();
  const QBrush textColor = (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
  const QBrush cyanBrush(Qt::cyan);
  int row = model.rowCount();
  model.insertRow(row);
  addRowToModelFast(model, row, change, textColor, cyanBrush);
}

void ImportaProdutos::addRowToModelFast(QStandardItemModel &model, int row, const ProductChange &change,
                                        const QBrush &textColor, const QBrush &cyanBrush) {
  const Produto &p = change.newData;
  const bool isDiscontinued = (change.type == ProductChange::Type::Discontinued);

  static const QBrush blackBrush(Qt::black);

  auto setCell = [&](int col, const QVariant &value, const QString &fieldName) {
    QStandardItem *item = new QStandardItem();
    item->setData(value, Qt::DisplayRole);

    // Set background and foreground colors
    // Rule: colored backgrounds always get black text (like the old proxy model)
    if (isDiscontinued) {
      item->setData(cyanBrush, Qt::BackgroundRole);
      item->setData(blackBrush, Qt::ForegroundRole);
    } else {
      // Use find() instead of contains() + operator[] to avoid double lookup
      auto it = change.fieldStatus.constFind(fieldName);
      if (it != change.fieldStatus.constEnd()) {
        QBrush bg = getFieldColor(it.value());
        if (bg.style() != Qt::NoBrush) {
          item->setData(bg, Qt::BackgroundRole);
          item->setData(blackBrush, Qt::ForegroundRole);
        } else {
          item->setData(textColor, Qt::ForegroundRole);
        }
      } else {
        item->setData(textColor, Qt::ForegroundRole);
      }
    }

    model.setItem(row, col, item);
  };

  int col = 0;
  setCell(col++, p.idProduto, "idProduto");
  setCell(col++, p.idFornecedor, "idFornecedor");
  setCell(col++, p.fornecedor, "fornecedor");
  setCell(col++, p.descricao, "descricao");
  setCell(col++, p.un, "un");
  setCell(col++, p.un2, "un2");
  setCell(col++, p.colecao, "colecao");
  setCell(col++, p.m2cx, "m2cx");
  setCell(col++, p.pccx, "pccx");
  setCell(col++, p.kgcx, "kgcx");
  setCell(col++, p.formComercial, "formComercial");
  setCell(col++, p.codComercial, "codComercial");
  setCell(col++, p.codBarras, "codBarras");
  setCell(col++, p.ncm, "ncm");
  setCell(col++, p.qtdPallet, "qtdPallet");
  setCell(col++, p.custo, "custo");
  setCell(col++, p.precoVenda, "precoVenda");
  setCell(col++, p.ui, "ui");
  setCell(col++, p.minimo, "minimo");
  setCell(col++, p.mva, "mva");
  setCell(col++, p.st, "st");
  setCell(col++, p.sticms, "sticms");
  setCell(col++, p.quantCaixa, "quantCaixa");
  setCell(col++, p.markup, "markup");
  setCell(col++, p.validade, "validade");
  setCell(col++, isDiscontinued ? 1 : 0, "descontinuado");
}

void ImportaProdutos::salvarOptimized() {
  // ==========================================================================
  // BATCH SAVE - Uses multi-row INSERT and INSERT...ON DUPLICATE KEY UPDATE
  // Reduces ~3700 queries to ~40 queries (100 rows per batch)
  // ==========================================================================

  if (m_productChanges.isEmpty()) { goto postProcess; }

  {
    constexpr int BATCH_SIZE = 100;

    // Separate new products from updates
    QVector<const ProductChange *> newProducts;
    QVector<const ProductChange *> updateProducts;

    for (const ProductChange &change : qAsConst(m_productChanges)) {
      if (change.type == ProductChange::Type::New) {
        newProducts.append(&change);
      } else if (change.type == ProductChange::Type::Updated || change.type == ProductChange::Type::NotChanged) {
        updateProducts.append(&change);
      }
    }

    // Helper to escape string for SQL (basic escaping)
    auto escapeStr = [](const QString &s) -> QString {
      QString escaped = s;
      escaped.replace("\\", "\\\\");
      escaped.replace("'", "\\'");
      return escaped;
    };

    // Helper to format QVariant for SQL
    auto formatValue = [&escapeStr](const QVariant &v) -> QString {
      if (v.isNull() || !v.isValid()) return "NULL";
      switch (v.type()) {
        case QVariant::Int:
        case QVariant::LongLong:
          return QString::number(v.toLongLong());
        case QVariant::Double:
          return QString::number(v.toDouble(), 'f', 6);
        case QVariant::Date:
          return "'" + v.toDate().toString("yyyy-MM-dd") + "'";
        case QVariant::String:
        default:
          return "'" + escapeStr(v.toString()) + "'";
      }
    };

    // ========================================================================
    // BATCH INSERT for new products
    // ========================================================================
    if (!newProducts.isEmpty()) {
      const int promocaoInt = static_cast<int>(tipo);

      for (int batchStart = 0; batchStart < newProducts.size(); batchStart += BATCH_SIZE) {
        const int batchEnd = qMin(batchStart + BATCH_SIZE, newProducts.size());

        QString sql = "INSERT INTO produto (atualizarTabelaPreco, promocao, idFornecedor, fornecedor, descricao, "
                      "un, un2, colecao, m2cx, pccx, kgcx, formComercial, codComercial, codBarras, ncm, "
                      "qtdPallet, custo, precoVenda, ui, minimo, mva, st, sticms, quantCaixa, markup, validade) VALUES ";

        QStringList valueRows;
        for (int i = batchStart; i < batchEnd; ++i) {
          const Produto &p = newProducts[i]->newData;
          QStringList vals;
          vals << "TRUE"
               << QString::number(promocaoInt)
               << QString::number(p.idFornecedor)
               << formatValue(p.fornecedor)
               << formatValue(p.descricao)
               << formatValue(p.un)
               << formatValue(p.un2)
               << formatValue(p.colecao)
               << QString::number(p.m2cx, 'f', 6)
               << QString::number(p.pccx, 'f', 6)
               << QString::number(p.kgcx, 'f', 6)
               << formatValue(p.formComercial)
               << formatValue(p.codComercial)
               << formatValue(p.codBarras)
               << formatValue(p.ncm)
               << formatValue(p.qtdPallet)
               << QString::number(p.custo, 'f', 6)
               << QString::number(p.precoVenda, 'f', 6)
               << formatValue(p.ui)
               << formatValue(p.minimo)
               << formatValue(p.mva)
               << formatValue(p.st)
               << formatValue(p.sticms)
               << QString::number(p.quantCaixa, 'f', 6)
               << QString::number(p.markup, 'f', 6)
               << (p.validade.isValid() ? formatValue(p.validade) : "NULL");
          valueRows << "(" + vals.join(",") + ")";
        }

        sql += valueRows.join(",");

        SqlQuery query;
        if (!query.exec(sql)) {
          throw RuntimeException("Erro inserindo produtos em lote: " + query.lastError().text());
        }
      }

      // Link promocao products in bulk (if needed)
      if (tipo == Tipo::Promocao) {
        QString linkSql = "UPDATE produto p1 "
            "SET idProdutoRelacionado = ("
            "  SELECT p2.idProduto FROM produto p2 "
            "  WHERE p2.idFornecedor = p1.idFornecedor "
            "  AND p2.codComercial = p1.codComercial "
            "  AND p2.promocao = FALSE AND p2.estoque = FALSE "
            "  LIMIT 1"
            ") "
            "WHERE p1.promocao = TRUE AND p1.idProdutoRelacionado IS NULL "
            "AND p1.idFornecedor IN (" + idsFornecedor + ")";

        SqlQuery linkQuery;
        linkQuery.exec(linkSql);  // Ignore errors - optional operation
      }
    }

    // ========================================================================
    // BATCH UPDATE for existing products using INSERT...ON DUPLICATE KEY UPDATE
    // ========================================================================
    if (!updateProducts.isEmpty()) {
      for (int batchStart = 0; batchStart < updateProducts.size(); batchStart += BATCH_SIZE) {
        const int batchEnd = qMin(batchStart + BATCH_SIZE, updateProducts.size());

        QString sql = "INSERT INTO produto (idProduto, idFornecedor, atualizarTabelaPreco, descontinuado, fornecedor, descricao, "
                      "un, un2, colecao, m2cx, pccx, kgcx, formComercial, codComercial, codBarras, ncm, "
                      "qtdPallet, custo, precoVenda, ui, minimo, mva, st, sticms, quantCaixa, markup, validade) VALUES ";

        QStringList valueRows;
        for (int i = batchStart; i < batchEnd; ++i) {
          const Produto &p = updateProducts[i]->newData;
          QStringList vals;
          vals << QString::number(p.idProduto)
               << QString::number(p.idFornecedor)
               << "TRUE"
               << "FALSE"
               << formatValue(p.fornecedor)
               << formatValue(p.descricao)
               << formatValue(p.un)
               << formatValue(p.un2)
               << formatValue(p.colecao)
               << QString::number(p.m2cx, 'f', 6)
               << QString::number(p.pccx, 'f', 6)
               << QString::number(p.kgcx, 'f', 6)
               << formatValue(p.formComercial)
               << formatValue(p.codComercial)
               << formatValue(p.codBarras)
               << formatValue(p.ncm)
               << formatValue(p.qtdPallet)
               << QString::number(p.custo, 'f', 6)
               << QString::number(p.precoVenda, 'f', 6)
               << formatValue(p.ui)
               << formatValue(p.minimo)
               << formatValue(p.mva)
               << formatValue(p.st)
               << formatValue(p.sticms)
               << QString::number(p.quantCaixa, 'f', 6)
               << QString::number(p.markup, 'f', 6)
               << (p.validade.isValid() ? formatValue(p.validade) : "NULL");
          valueRows << "(" + vals.join(",") + ")";
        }

        sql += valueRows.join(",");
        sql += " ON DUPLICATE KEY UPDATE "
               "atualizarTabelaPreco = VALUES(atualizarTabelaPreco), "
               "descontinuado = VALUES(descontinuado), "
               "fornecedor = VALUES(fornecedor), "
               "descricao = VALUES(descricao), "
               "un = VALUES(un), "
               "un2 = VALUES(un2), "
               "colecao = VALUES(colecao), "
               "m2cx = VALUES(m2cx), "
               "pccx = VALUES(pccx), "
               "kgcx = VALUES(kgcx), "
               "formComercial = VALUES(formComercial), "
               "codComercial = VALUES(codComercial), "
               "codBarras = VALUES(codBarras), "
               "ncm = VALUES(ncm), "
               "qtdPallet = VALUES(qtdPallet), "
               "custo = VALUES(custo), "
               "precoVenda = VALUES(precoVenda), "
               "ui = VALUES(ui), "
               "minimo = VALUES(minimo), "
               "mva = VALUES(mva), "
               "st = VALUES(st), "
               "sticms = VALUES(sticms), "
               "quantCaixa = VALUES(quantCaixa), "
               "markup = VALUES(markup), "
               "validade = VALUES(validade)";

        SqlQuery query;
        if (!query.exec(sql)) {
          throw RuntimeException("Erro atualizando produtos em lote: " + query.lastError().text());
        }
      }
    }
  }

postProcess:

  // Insert price records
  SqlQuery queryPrecos;
  if (validade != -1) {
    queryPrecos.prepare(
        "INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) "
        "SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim "
        "FROM produto WHERE atualizarTabelaPreco = TRUE");
    queryPrecos.bindValue(":validadeInicio", qApp->serverDate().toString("yyyy-MM-dd"));
    queryPrecos.bindValue(":validadeFim", validadeString);

    if (not queryPrecos.exec()) {
      throw RuntimeException("Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text());
    }
  }

  if (not queryPrecos.exec("UPDATE produto SET atualizarTabelaPreco = FALSE")) {
    throw RuntimeException("Erro comunicando com banco de dados: " + queryPrecos.lastError().text());
  }

  SqlQuery queryExpirar;
  if (not queryExpirar.exec("CALL invalidar_produtos_expirados()")) {
    throw RuntimeException("Erro executando invalidar_produtos_expirados: " + queryExpirar.lastError().text());
  }

  atualizaPrecoEstoque();
}

bool ImportaProdutos::readFile() {
  file = QFileDialog::getOpenFileName(this, "Importar tabela genérica", "", tr("Excel (*.xlsx)"));

  if (file.isEmpty()) { return false; }

  setWindowTitle(file);

  return true;
}

bool ImportaProdutos::readValidade() {
  auto *validadeDlg = new ValidadeDialog(this);

  if (validadeDlg->exec() == QDialog::Rejected) { return false; }

  validade = validadeDlg->getValidade();

  if (validade != -1) { validadeString = qApp->serverDate().addDays(validade).toString("yyyy-MM-dd"); }

  return true;
}

void ImportaProdutos::setupModels() {
  modelProduto.setTable("produto");

  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("descricao", "Descrição");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("un2", "Un.2");
  modelProduto.setHeaderData("colecao", "Coleção");
  modelProduto.setHeaderData("tipo", "Tipo");
  modelProduto.setHeaderData("minimo", "Mínimo");
  modelProduto.setHeaderData("multiplo", "Múltiplo");
  modelProduto.setHeaderData("m2cx", "M./Cx.");
  modelProduto.setHeaderData("pccx", "Pç./Cx.");
  modelProduto.setHeaderData("kgcx", "Kg./Cx.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("codBarras", "Cód. Barras");
  modelProduto.setHeaderData("ncm", "NCM");
  modelProduto.setHeaderData("icms", "ICMS");
  modelProduto.setHeaderData("cst", "CST");
  modelProduto.setHeaderData("qtdPallet", "Qt. Pallet");
  modelProduto.setHeaderData("custo", "Custo");
  modelProduto.setHeaderData("ipi", "IPI");
  modelProduto.setHeaderData("st", "ST");
  modelProduto.setHeaderData("sticms", "ST ICMS");
  modelProduto.setHeaderData("mva", "MVA");
  modelProduto.setHeaderData("precoVenda", "Preço Venda");
  modelProduto.setHeaderData("comissao", "Comissão");
  modelProduto.setHeaderData("observacoes", "Obs.");
  modelProduto.setHeaderData("origem", "Origem");
  modelProduto.setHeaderData("ui", "UI");
  modelProduto.setHeaderData("validade", "Validade");
  modelProduto.setHeaderData("markup", "Markup");

  modelProduto.proxyModel = new ImportaProdutosProxyModel(&modelProduto, this);

  //-------------------------------------------------------------//

  modelErro.setTable("produto");

  modelErro.setHeaderData("fornecedor", "Fornecedor");
  modelErro.setHeaderData("descricao", "Descrição");
  modelErro.setHeaderData("un", "Un.");
  modelErro.setHeaderData("un2", "Un.2");
  modelErro.setHeaderData("colecao", "Coleção");
  modelErro.setHeaderData("tipo", "Tipo");
  modelErro.setHeaderData("m2cx", "M./Cx.");
  modelErro.setHeaderData("pccx", "Pç./Cx.");
  modelErro.setHeaderData("kgcx", "Kg./Cx.");
  modelErro.setHeaderData("minimo", "Mínimo");
  modelErro.setHeaderData("multiplo", "Múltiplo");
  modelErro.setHeaderData("formComercial", "Form. Com.");
  modelErro.setHeaderData("codComercial", "Cód. Com.");
  modelErro.setHeaderData("codBarras", "Cód. Barras");
  modelErro.setHeaderData("ncm", "NCM");
  modelErro.setHeaderData("icms", "ICMS");
  modelErro.setHeaderData("cst", "CST");
  modelErro.setHeaderData("qtdPallet", "Qt. Pallet");
  modelErro.setHeaderData("custo", "Custo");
  modelErro.setHeaderData("ipi", "IPI");
  modelErro.setHeaderData("st", "ST");
  modelErro.setHeaderData("sticms", "ST ICMS");
  modelErro.setHeaderData("mva", "MVA");
  modelErro.setHeaderData("precoVenda", "Preço Venda");
  modelErro.setHeaderData("comissao", "Comissão");
  modelErro.setHeaderData("observacoes", "Obs.");
  modelErro.setHeaderData("origem", "Origem");
  modelErro.setHeaderData("ui", "UI");
  modelErro.setHeaderData("validade", "Validade");
  modelErro.setHeaderData("markup", "Markup");

  modelErro.proxyModel = new ImportaProdutosProxyModel(&modelErro, this);
}

void ImportaProdutos::setupTables() {
  // Helper to get column index from PREVIEW_COLUMNS
  auto colIdx = [](const QString &name) { return PREVIEW_COLUMNS.indexOf(name); };

  // Setup product table
  ui->tableProdutos->setAutoResize(false);
  ui->tableProdutos->setModel(&m_previewModel);

  // Hide columns not needed for display
  ui->tableProdutos->setColumnHidden(colIdx("idProduto"), true);
  ui->tableProdutos->setColumnHidden(colIdx("idFornecedor"), true);
  ui->tableProdutos->setColumnHidden(colIdx("descontinuado"), true);
  ui->tableProdutos->setColumnHidden(colIdx("quantCaixa"), true);

  // Setup delegates for formatted display (use column index directly for QAbstractItemView)
  auto *doubleDelegate = new DoubleDelegate(4, this);
  auto *reaisDelegate = new ReaisDelegate(4, true, this);
  auto *porcDelegate = new PorcentagemDelegate(false, this);

  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("validade"), new DateFormatDelegate(this));
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("m2cx"), doubleDelegate);
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("kgcx"), doubleDelegate);
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("qtdPallet"), doubleDelegate);
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("custo"), reaisDelegate);
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("precoVenda"), reaisDelegate);
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("markup"), porcDelegate);
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("st"), new PorcentagemDelegate(true, this));
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("sticms"), new PorcentagemDelegate(true, this));
  ui->tableProdutos->QAbstractItemView::setItemDelegateForColumn(colIdx("mva"), porcDelegate);

  //-------------------------------------------------------------//

  // Setup error table
  ui->tableErro->setAutoResize(false);
  ui->tableErro->setModel(&m_errorModel);

  // Hide columns not needed for display
  ui->tableErro->setColumnHidden(colIdx("idProduto"), true);
  ui->tableErro->setColumnHidden(colIdx("idFornecedor"), true);
  ui->tableErro->setColumnHidden(colIdx("descontinuado"), true);
  ui->tableErro->setColumnHidden(colIdx("quantCaixa"), true);

  // Reuse delegates for error table
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("validade"), new DateFormatDelegate(this));
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("m2cx"), doubleDelegate);
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("kgcx"), doubleDelegate);
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("qtdPallet"), doubleDelegate);
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("custo"), reaisDelegate);
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("precoVenda"), reaisDelegate);
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("markup"), porcDelegate);
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("st"), new PorcentagemDelegate(true, this));
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("sticms"), new PorcentagemDelegate(true, this));
  ui->tableErro->QAbstractItemView::setItemDelegateForColumn(colIdx("mva"), porcDelegate);
}

void ImportaProdutos::cadastraFornecedores(QXlsx::Document &xlsx) {
  const int rows = xlsx.dimension().rowCount();

  QStringList fornecedores;

  int count = 0;

  for (int row = 2; row < rows; ++row) {
    const QString fornec = xlsx.readValue(row, 1).toString();

    if (not fornec.isEmpty()) { ++count; }
    if (fornec.isEmpty() or fornecedores.contains(fornec)) { continue; }

    fornecedores << xlsx.readValue(row, 1).toString();
  }

  progressDialog.setMaximum(count);

  QStringList ids;

  for (auto const &fornecedor : qAsConst(fornecedores)) {
    m_fornecedor = fornecedor.left(100);

    const int idFornecedor = buscarCadastrarFornecedor();

    ids << QString::number(idFornecedor);

    m_fornecedores.insert(fornecedor.left(100), idFornecedor);

    SqlQuery queryFornecedor;
    queryFornecedor.prepare("UPDATE fornecedor SET validadeProdutos = :validade WHERE razaoSocial = :razaoSocial");
    queryFornecedor.bindValue(":validade", (validade == -1) ? QVariant() : qApp->serverDate().addDays(validade));
    queryFornecedor.bindValue(":razaoSocial", fornecedor.left(100));

    if (not queryFornecedor.exec()) { throw RuntimeException("Erro salvando validade: " + queryFornecedor.lastError().text()); }
  }

  idsFornecedor = ids.join(",");

  if (m_fornecedores.isEmpty()) { throw RuntimeException("Erro ao cadastrar fornecedores!"); }
}

void ImportaProdutos::mostraApenasEstesFornecedores() {
  modelProduto.setFilter("idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)));

  modelProduto.select();

  cacheFieldIndices();
}

void ImportaProdutos::cacheFieldIndices() {
  // Cache all field indices used in atualizaCamposProduto, insereEmOk, insereEmErro
  const QStringList fields = {
      "atualizarTabelaPreco", "fornecedor",    "fornecedorUpd",    "descricao",       "descricaoUpd",    "un",
      "unUpd",                "colecao",       "colecaoUpd",       "m2cx",            "m2cxUpd",         "pccx",
      "pccxUpd",              "kgcx",          "kgcxUpd",          "formComercial",   "formComercialUpd", "codComercial",
      "codComercialUpd",      "codBarras",     "codBarrasUpd",     "ncm",             "ncmUpd",          "qtdPallet",
      "qtdPalletUpd",         "custo",         "custoUpd",         "precoVenda",      "precoVendaUpd",   "ui",
      "uiUpd",                "un2",           "un2Upd",           "minimo",          "minimoUpd",       "mva",
      "mvaUpd",               "st",            "stUpd",            "sticms",          "sticmsUpd",       "quantCaixa",
      "quantCaixaUpd",        "markup",        "markupUpd",        "validade",        "validadeUpd",     "descontinuado",
      "idFornecedor",         "promocao",      "idProdutoRelacionado"};

  for (const QString &field : fields) { fieldIdx[field] = modelProduto.fieldIndex(field, true); }
}

void ImportaProdutos::marcaTodosProdutosDescontinuados() {
  SqlQuery query;

  if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)))) {
    throw RuntimeException("Erro marcando produtos descontinuados: " + query.lastError().text());
  }
}

void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
  produto = {};

  const QLocale locale(QLocale::Portuguese);

  QVariant fornecedor = xlsx.readValue(row, 1);
  QVariant descricao = xlsx.readValue(row, 2);
  QVariant un = xlsx.readValue(row, 3);
  QVariant colecao = xlsx.readValue(row, 4);

  QVariant m2cx = xlsx.readValue(row, 5);
  if (m2cx.userType() == QMetaType::QString) { m2cx = locale.toDouble(m2cx.toString()); }
  m2cx = qApp->roundDouble(m2cx.toDouble());

  QVariant pccx = xlsx.readValue(row, 6);
  if (pccx.userType() == QMetaType::QString) { pccx = locale.toDouble(pccx.toString()); }
  pccx = qApp->roundDouble(pccx.toDouble());

  QVariant kgcx = xlsx.readValue(row, 7);
  if (kgcx.userType() == QMetaType::QString) { kgcx = locale.toDouble(kgcx.toString()); }
  kgcx = qApp->roundDouble(kgcx.toDouble());

  QVariant formComercial = xlsx.readValue(row, 8);
  QVariant codComercial = xlsx.readValue(row, 9);
  QVariant codBarras = xlsx.readValue(row, 10);
  QVariant ncm = xlsx.readValue(row, 11);

  QVariant qtdPallet = xlsx.readValue(row, 12);
  if (qtdPallet.userType() == QMetaType::QString) { qtdPallet = locale.toDouble(qtdPallet.toString()); }
  qtdPallet = qApp->roundDouble(qtdPallet.toDouble());

  QVariant custo = xlsx.readValue(row, 13);
  if (custo.userType() == QMetaType::QString) { custo = locale.toDouble(custo.toString()); }
  custo = qApp->roundDouble(custo.toDouble());

  QVariant precoVenda = xlsx.readValue(row, 14);
  if (precoVenda.userType() == QMetaType::QString) { precoVenda = locale.toDouble(precoVenda.toString()); }
  precoVenda = qApp->roundDouble(precoVenda.toDouble());

  QVariant ui2 = xlsx.readValue(row, 15);
  QVariant un2 = xlsx.readValue(row, 16);

  QVariant minimo = xlsx.readValue(row, 17);
  if (minimo.userType() == QMetaType::QString) { minimo = locale.toDouble(minimo.toString()); }
  minimo = qApp->roundDouble(minimo.toDouble());

  QVariant mva = xlsx.readValue(row, 18);
  if (mva.userType() == QMetaType::QString) { mva = locale.toDouble(mva.toString()); }
  mva = qApp->roundDouble(mva.toDouble());

  QVariant st = xlsx.readValue(row, 19);
  if (st.userType() == QMetaType::QString) { st = locale.toDouble(st.toString()); }
  st = qApp->roundDouble(st.toDouble());

  QVariant sticms = xlsx.readValue(row, 20);
  if (sticms.userType() == QMetaType::QString) { sticms = locale.toDouble(sticms.toString()); }
  sticms = qApp->roundDouble(sticms.toDouble());

  produto.idFornecedor = m_fornecedores.value(fornecedor.toString().trimmed());
  produto.fornecedor = fornecedor.toString().toUpper().trimmed().left(100);
  produto.descricao = descricao.toString().remove("*").remove("()").replace('_', ' ').toUpper().trimmed().left(250);
  produto.un = un.toString().remove("*").toUpper().trimmed().left(45);
  produto.colecao = colecao.toString().remove("*").toUpper().trimmed().left(200);
  produto.m2cx = m2cx.toDouble();
  produto.pccx = pccx.toDouble();
  produto.kgcx = kgcx.toDouble();
  produto.formComercial = formComercial.toString().remove("*").toUpper().trimmed().left(100);
  produto.codComercial = codComercial.toString().remove("*").remove(".").remove(",").toUpper().trimmed().left(100);
  produto.codBarras = codBarras.toString().remove("*").remove(".").remove(",").toUpper().trimmed().left(100);
  produto.ncm = ncm.toString().remove("*").remove(".").remove(",").remove("-").remove(" ").toUpper().trimmed().left(10);
  produto.qtdPallet = qtdPallet.toDouble();
  produto.custo = custo.toDouble();
  produto.precoVenda = precoVenda.toDouble();
  produto.ui = ui2.toString().remove("*").toUpper().trimmed().left(45);
  produto.un2 = un2.toString().remove("*").toUpper().trimmed().left(45);
  produto.minimo = minimo.toDouble();
  produto.mva = mva.toDouble();
  produto.st = st.toDouble();
  produto.sticms = sticms.toDouble();
  produto.markup = qApp->roundDouble(((produto.precoVenda / produto.custo) - 1.) * 100);

  // consistencia dados

  if (produto.ui.isEmpty()) { produto.ui = "0"; }

  if (produto.codBarras == "0") { produto.codBarras.clear(); }

  if (produto.ncm == "0") { produto.ncm.clear(); }

  if (produto.ncm.length() == 6) { produto.ncm.append("00"); }

  if (produto.un == "M²") { produto.un = "M2"; }

  const double quantCaixa = (produto.un == "M2" or produto.un == "ML") ? produto.m2cx : produto.pccx;

  produto.quantCaixa = quantCaixa;
}

void ImportaProdutos::atualizaPrecoEstoque() {
  SqlQuery query;

  if (not query.exec("UPDATE produto p1, produto p2 "
                     "SET p2.precoVenda = p1.precoVenda "
                     "WHERE p1.idFornecedor = p2.idFornecedor AND p1.codComercial = p2.codComercial AND p1.idProduto <> p2.idProduto "
                     "AND p1.descontinuado = FALSE AND p1.estoque = FALSE AND p1.promocao = FALSE "
                     "AND p2.estoque = TRUE AND p2.promocao = FALSE")) {
    throw RuntimeException("Erro atualizando preço dos estoques: " + query.lastError().text());
  }

  if (not query.exec("UPDATE produto p1, produto p2 "
                     "SET p2.oldPrecoVenda = p1.precoVenda "
                     "WHERE p1.idFornecedor = p2.idFornecedor AND p1.codComercial = p2.codComercial AND p1.idProduto <> p2.idProduto "
                     "AND p1.descontinuado = FALSE AND p1.estoque = FALSE AND p1.promocao = FALSE "
                     "AND p2.estoque = TRUE AND p2.promocao = 2")) {
    throw RuntimeException("Erro atualizando preço dos estoques: " + query.lastError().text());
  }
}

void ImportaProdutos::atualizaCamposProduto(const int row) {
  modelProduto.setData(row, fieldIdx["atualizarTabelaPreco"], true);

  const int yellow = static_cast<int>(FieldColors::Yellow);
  const int white = static_cast<int>(FieldColors::White);

  bool changed = false;

  // Use cached field indices for performance (avoids repeated string lookups)

  if (modelProduto.data(row, fieldIdx["fornecedor"]).toString() != produto.fornecedor) {
    modelProduto.setData(row, fieldIdx["fornecedor"], produto.fornecedor);
    modelProduto.setData(row, fieldIdx["fornecedorUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["fornecedorUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["descricao"]).toString() != produto.descricao) {
    modelProduto.setData(row, fieldIdx["descricao"], produto.descricao);
    modelProduto.setData(row, fieldIdx["descricaoUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["descricaoUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["un"]).toString() != produto.un) {
    modelProduto.setData(row, fieldIdx["un"], produto.un);
    modelProduto.setData(row, fieldIdx["unUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["unUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["colecao"]).toString() != produto.colecao) {
    modelProduto.setData(row, fieldIdx["colecao"], produto.colecao);
    modelProduto.setData(row, fieldIdx["colecaoUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["colecaoUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["m2cx"]).toDouble(), produto.m2cx)) {
    modelProduto.setData(row, fieldIdx["m2cx"], produto.m2cx);
    modelProduto.setData(row, fieldIdx["m2cxUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["m2cxUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["pccx"]).toDouble(), produto.pccx)) {
    modelProduto.setData(row, fieldIdx["pccx"], produto.pccx);
    modelProduto.setData(row, fieldIdx["pccxUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["pccxUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["kgcx"]).toDouble(), produto.kgcx)) {
    modelProduto.setData(row, fieldIdx["kgcx"], produto.kgcx);
    modelProduto.setData(row, fieldIdx["kgcxUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["kgcxUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["formComercial"]).toString() != produto.formComercial) {
    modelProduto.setData(row, fieldIdx["formComercial"], produto.formComercial);
    modelProduto.setData(row, fieldIdx["formComercialUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["formComercialUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["codComercial"]).toString() != produto.codComercial) {
    modelProduto.setData(row, fieldIdx["codComercial"], produto.codComercial);
    modelProduto.setData(row, fieldIdx["codComercialUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["codComercialUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["codBarras"]).toString() != produto.codBarras) {
    modelProduto.setData(row, fieldIdx["codBarras"], produto.codBarras);
    modelProduto.setData(row, fieldIdx["codBarrasUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["codBarrasUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["ncm"]).toString() != produto.ncm) {
    modelProduto.setData(row, fieldIdx["ncm"], produto.ncm);
    modelProduto.setData(row, fieldIdx["ncmUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["ncmUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["qtdPallet"]).toDouble(), produto.qtdPallet.toDouble())) {
    modelProduto.setData(row, fieldIdx["qtdPallet"], produto.qtdPallet);
    modelProduto.setData(row, fieldIdx["qtdPalletUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["qtdPalletUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["custo"]).toDouble(), produto.custo)) {
    modelProduto.setData(row, fieldIdx["custo"], produto.custo);
    modelProduto.setData(row, fieldIdx["custoUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["custoUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["precoVenda"]).toDouble(), produto.precoVenda)) {
    modelProduto.setData(row, fieldIdx["precoVenda"], produto.precoVenda);
    modelProduto.setData(row, fieldIdx["precoVendaUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["precoVendaUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["ui"]).toString() != produto.ui) {
    modelProduto.setData(row, fieldIdx["ui"], produto.ui);
    modelProduto.setData(row, fieldIdx["uiUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["uiUpd"], white);
  }

  if (modelProduto.data(row, fieldIdx["un2"]).toString() != produto.un2) {
    modelProduto.setData(row, fieldIdx["un2"], produto.un2);
    modelProduto.setData(row, fieldIdx["un2Upd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["un2Upd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["minimo"]).toDouble(), produto.minimo.toDouble())) {
    modelProduto.setData(row, fieldIdx["minimo"], produto.minimo);
    modelProduto.setData(row, fieldIdx["minimoUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["minimoUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["mva"]).toDouble(), produto.mva.toDouble())) {
    modelProduto.setData(row, fieldIdx["mva"], produto.mva);
    modelProduto.setData(row, fieldIdx["mvaUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["mvaUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["st"]).toDouble(), produto.st.toDouble())) {
    modelProduto.setData(row, fieldIdx["st"], produto.st);
    modelProduto.setData(row, fieldIdx["stUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["stUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["sticms"]).toDouble(), produto.sticms.toDouble())) {
    modelProduto.setData(row, fieldIdx["sticms"], produto.sticms);
    modelProduto.setData(row, fieldIdx["sticmsUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["sticmsUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["quantCaixa"]).toDouble(), produto.quantCaixa)) {
    modelProduto.setData(row, fieldIdx["quantCaixa"], produto.quantCaixa);
    modelProduto.setData(row, fieldIdx["quantCaixaUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["quantCaixaUpd"], white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, fieldIdx["markup"]).toDouble(), produto.markup)) {
    modelProduto.setData(row, fieldIdx["markup"], produto.markup);
    modelProduto.setData(row, fieldIdx["markupUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["markupUpd"], white);
  }

  const QDate dataSalva = modelProduto.data(row, fieldIdx["validade"]).toDate();

  if ((dataSalva.isValid() and dataSalva.toString("yyyy-MM-dd") != validadeString) or (not dataSalva.isValid() and not validadeString.isEmpty())) {
    modelProduto.setData(row, fieldIdx["validade"], (validade == -1) ? QVariant() : validadeString);
    modelProduto.setData(row, fieldIdx["validadeUpd"], yellow);
    changed = true;
  } else {
    modelProduto.setData(row, fieldIdx["validadeUpd"], white);
  }

  changed ? itensUpdated++ : itensNotChanged++;
}

void ImportaProdutos::marcaProdutoNaoDescontinuado(const int row) {
  modelProduto.setData(row, fieldIdx["descontinuado"], 0);

  itensExpired--;
}

void ImportaProdutos::pintarCamposForaDoPadrao(const int row) {
  const QString ncm = produto.ncm;
  const QString codBarras = produto.codBarras;
  const QString fornecedor = produto.fornecedor;
  const QString un = produto.un;
  const QString codComercial = produto.codComercial;
  const double m2cx = produto.m2cx;
  const double pccx = produto.pccx;
  const double custo = produto.custo;
  const double precoVenda = produto.precoVenda;

  const int gray = static_cast<int>(FieldColors::Gray);
  const int red = static_cast<int>(FieldColors::Red);

  // Fora do padrão

  if (ncm == "0" or ncm == "00000000" or ncm.isEmpty()) { modelErro.setData(row, "ncmUpd", gray); }

  if (codBarras == "0" or codBarras.isEmpty()) { modelErro.setData(row, "codBarrasUpd", gray); }

  // Errados

  if (fornecedor == "PRODUTO REPETIDO NA TABELA") { modelErro.setData(row, "fornecedorUpd", red); }

  if ((un == "M2" or un == "ML") and m2cx <= 0.) { modelErro.setData(row, "m2cxUpd", red); }

  if (un != "M2" and un != "ML" and pccx < 1) { modelErro.setData(row, "pccxUpd", red); }

  if (codComercial == "0" or codComercial.isEmpty()) { modelErro.setData(row, "codComercialUpd", red); }

  if (custo <= 0.) { modelErro.setData(row, "custoUpd", red); }

  if (precoVenda <= 0.) { modelErro.setData(row, "precoVendaUpd", red); }

  if (precoVenda < custo) { modelErro.setData(row, "precoVendaUpd", red); }

  if (not ui->checkBoxRepresentacao->isChecked() and ncm.length() != 8) { modelErro.setData(row, "ncmUpd", red); }
}

bool ImportaProdutos::camposForaDoPadrao() {
  if ((produto.un == "M2" or produto.un == "ML") and produto.m2cx <= 0.) { return true; }
  if (produto.un != "M2" and produto.un != "ML" and produto.pccx < 1) { return true; }
  if (produto.codComercial == "0" or produto.codComercial.isEmpty()) { return true; }
  if (produto.custo <= 0.) { return true; }
  if (produto.precoVenda <= 0.) { return true; }
  if (produto.precoVenda < produto.custo) { return true; }
  if (not ui->checkBoxRepresentacao->isChecked() and produto.ncm.length() != 8) { return true; }

  return false;
}

void ImportaProdutos::insereEmErro() {
  const int row = modelErro.insertRowAtEnd();

  modelErro.setData(row, "atualizarTabelaPreco", true);

  modelErro.setData(row, "idFornecedor", produto.idFornecedor);
  modelErro.setData(row, "fornecedor", produto.fornecedor);
  modelErro.setData(row, "descricao", produto.descricao);
  modelErro.setData(row, "un", produto.un);
  modelErro.setData(row, "colecao", produto.colecao);
  modelErro.setData(row, "m2cx", produto.m2cx);
  modelErro.setData(row, "pccx", produto.pccx);
  modelErro.setData(row, "kgcx", produto.kgcx);
  modelErro.setData(row, "formComercial", produto.formComercial);
  modelErro.setData(row, "codComercial", produto.codComercial);
  modelErro.setData(row, "codBarras", produto.codBarras);
  modelErro.setData(row, "ncm", produto.ncm);
  modelErro.setData(row, "qtdPallet", produto.qtdPallet);
  modelErro.setData(row, "custo", produto.custo);
  modelErro.setData(row, "precoVenda", produto.precoVenda);
  modelErro.setData(row, "ui", produto.ui);
  modelErro.setData(row, "un2", produto.un2);
  modelErro.setData(row, "minimo", produto.minimo);
  modelErro.setData(row, "mva", produto.mva);
  modelErro.setData(row, "st", produto.st);
  modelErro.setData(row, "sticms", produto.sticms);
  modelErro.setData(row, "quantCaixa", produto.quantCaixa);
  modelErro.setData(row, "markup", produto.markup);
  modelErro.setData(row, "validade", (validade == -1) ? QVariant() : validadeString);

  // paint cells
  const int green = static_cast<int>(FieldColors::Green);

  modelErro.setData(row, "fornecedorUpd", green);
  modelErro.setData(row, "descricaoUpd", green);
  modelErro.setData(row, "unUpd", green);
  modelErro.setData(row, "colecaoUpd", green);
  modelErro.setData(row, "m2cxUpd", green);
  modelErro.setData(row, "pccxUpd", green);
  modelErro.setData(row, "kgcxUpd", green);
  modelErro.setData(row, "formComercialUpd", green);
  modelErro.setData(row, "codComercialUpd", green);
  modelErro.setData(row, "codBarrasUpd", green);
  modelErro.setData(row, "ncmUpd", green);
  modelErro.setData(row, "qtdPalletUpd", green);
  modelErro.setData(row, "custoUpd", green);
  modelErro.setData(row, "precoVendaUpd", green);
  modelErro.setData(row, "uiUpd", green);
  modelErro.setData(row, "un2Upd", green);
  modelErro.setData(row, "minimoUpd", green);
  modelErro.setData(row, "mvaUpd", green);
  modelErro.setData(row, "stUpd", green);
  modelErro.setData(row, "sticmsUpd", green);
  modelErro.setData(row, "quantCaixaUpd", green);
  modelErro.setData(row, "markupUpd", green);
  modelErro.setData(row, "validadeUpd", green);

  // -------------------------------------------------

  pintarCamposForaDoPadrao(row);

  itensError++;
}

void ImportaProdutos::insereEmOk() {
  const int row = modelProduto.insertRowAtEnd();

  modelProduto.setData(row, "atualizarTabelaPreco", true);
  modelProduto.setData(row, "promocao", static_cast<int>(tipo));

  modelProduto.setData(row, "idFornecedor", produto.idFornecedor);
  modelProduto.setData(row, "fornecedor", produto.fornecedor);
  modelProduto.setData(row, "descricao", produto.descricao);
  modelProduto.setData(row, "un", produto.un);
  modelProduto.setData(row, "colecao", produto.colecao);
  modelProduto.setData(row, "m2cx", produto.m2cx);
  modelProduto.setData(row, "pccx", produto.pccx);
  modelProduto.setData(row, "kgcx", produto.kgcx);
  modelProduto.setData(row, "formComercial", produto.formComercial);
  modelProduto.setData(row, "codComercial", produto.codComercial);
  modelProduto.setData(row, "codBarras", produto.codBarras);
  modelProduto.setData(row, "ncm", produto.ncm);
  modelProduto.setData(row, "qtdPallet", produto.qtdPallet);
  modelProduto.setData(row, "custo", produto.custo);
  modelProduto.setData(row, "precoVenda", produto.precoVenda);
  modelProduto.setData(row, "ui", produto.ui);
  modelProduto.setData(row, "un2", produto.un2);
  modelProduto.setData(row, "minimo", produto.minimo);
  modelProduto.setData(row, "mva", produto.mva);
  modelProduto.setData(row, "st", produto.st);
  modelProduto.setData(row, "sticms", produto.sticms);
  modelProduto.setData(row, "quantCaixa", produto.quantCaixa);
  modelProduto.setData(row, "markup", produto.markup);
  modelProduto.setData(row, "validade", (validade == -1) ? QVariant() : validadeString);

  // paint cells
  const int green = static_cast<int>(FieldColors::Green);

  modelProduto.setData(row, "fornecedorUpd", green);
  modelProduto.setData(row, "descricaoUpd", green);
  modelProduto.setData(row, "unUpd", green);
  modelProduto.setData(row, "colecaoUpd", green);
  modelProduto.setData(row, "m2cxUpd", green);
  modelProduto.setData(row, "pccxUpd", green);
  modelProduto.setData(row, "kgcxUpd", green);
  modelProduto.setData(row, "formComercialUpd", green);
  modelProduto.setData(row, "codComercialUpd", green);
  modelProduto.setData(row, "codBarrasUpd", green);
  modelProduto.setData(row, "ncmUpd", green);
  modelProduto.setData(row, "qtdPalletUpd", green);
  modelProduto.setData(row, "custoUpd", green);
  modelProduto.setData(row, "precoVendaUpd", green);
  modelProduto.setData(row, "uiUpd", green);
  modelProduto.setData(row, "un2Upd", green);
  modelProduto.setData(row, "minimoUpd", green);
  modelProduto.setData(row, "mvaUpd", green);
  modelProduto.setData(row, "stUpd", green);
  modelProduto.setData(row, "sticmsUpd", green);
  modelProduto.setData(row, "quantCaixaUpd", green);
  modelProduto.setData(row, "markupUpd", green);
  modelProduto.setData(row, "validadeUpd", green);

  if (tipo == Tipo::Promocao) {
    SqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE idFornecedor = :idFornecedor AND codComercial = :codComercial AND promocao = FALSE AND estoque = FALSE");
    query.bindValue(":idFornecedor", produto.idFornecedor);
    query.bindValue(":codComercial", modelProduto.data(row, "codComercial"));

    if (not query.exec()) { throw RuntimeException("Erro buscando produto relacionado: " + query.lastError().text()); }

    if (query.first()) { modelProduto.setData(row, "idProdutoRelacionado", query.value("idProduto")); }
  }

  hashModel[produto.fornecedor + produto.codComercial + produto.ui + QString::number(static_cast<int>(tipo))] = row;

  vectorProdutosImportados << row;

  itensImported++;
}

int ImportaProdutos::buscarCadastrarFornecedor() {
  SqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", m_fornecedor.left(100));

  if (not queryFornecedor.exec()) { throw RuntimeException("Erro buscando fornecedor: " + queryFornecedor.lastError().text()); }

  if (not queryFornecedor.first()) {
    queryFornecedor.prepare("INSERT INTO fornecedor (razaoSocial) VALUES (:razaoSocial)");
    queryFornecedor.bindValue(":razaoSocial", m_fornecedor.left(100));

    if (not queryFornecedor.exec()) { throw RuntimeException("Erro cadastrando fornecedor: " + queryFornecedor.lastError().text()); }

    if (queryFornecedor.lastInsertId().isNull()) { throw RuntimeException("Erro lastInsertId"); }

    return queryFornecedor.lastInsertId().toInt();
  }

  return queryFornecedor.value("idFornecedor").toInt();
}

void ImportaProdutos::salvar() {
  modelProduto.submitAll();

  SqlQuery queryPrecos;

  if (validade != -1) {
    queryPrecos.prepare(
        "INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim FROM "
        "produto WHERE atualizarTabelaPreco = TRUE");
    queryPrecos.bindValue(":validadeInicio", qApp->serverDate().toString("yyyy-MM-dd"));
    queryPrecos.bindValue(":validadeFim", validadeString);

    if (not queryPrecos.exec()) { throw RuntimeException("Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text()); }
  }

  if (not queryPrecos.exec("UPDATE produto SET atualizarTabelaPreco = FALSE")) { throw RuntimeException("Erro comunicando com banco de dados: " + queryPrecos.lastError().text()); }

  SqlQuery queryExpirar;

  if (not queryExpirar.exec("CALL invalidar_produtos_expirados()")) { throw RuntimeException("Erro executando invalidar_produtos_expirados: " + queryExpirar.lastError().text()); }

  atualizaPrecoEstoque();
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
  if (m_errorModel.rowCount() > 0) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Produtos com erro não serão salvos. Deseja continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.button(QMessageBox::Yes)->setText("Continuar");
    msgBox.button(QMessageBox::No)->setText("Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  try {
    salvarOptimized();
  } catch (std::exception &) {
    close();
    throw;
  }

  qApp->endTransaction();

  qApp->enqueueInformation("Tabela salva com sucesso!", this);

  close();
}

void ImportaProdutos::verificaTabela(QXlsx::Document &xlsx) {
  if (xlsx.readValue(1, 1).toString() != "fornecedor") { throw RuntimeError("Faltou a coluna 'fornecedor' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 2).toString() != "descricao") { throw RuntimeError("Faltou a coluna 'descricao' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 3).toString() != "un") { throw RuntimeError("Faltou a coluna 'un' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 4).toString() != "colecao") { throw RuntimeError("Faltou a coluna 'colecao' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 5).toString() != "m2cx") { throw RuntimeError("Faltou a coluna 'm2cx' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 6).toString() != "pccx") { throw RuntimeError("Faltou a coluna 'pccx' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 7).toString() != "kgcx") { throw RuntimeError("Faltou a coluna 'kgcx' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 8).toString() != "formComercial") { throw RuntimeError("Faltou a coluna 'formComercial' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 9).toString() != "codComercial") { throw RuntimeError("Faltou a coluna 'codComercial' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 10).toString() != "codBarras") { throw RuntimeError("Faltou a coluna 'codBarras' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 11).toString() != "ncm") { throw RuntimeError("Faltou a coluna 'ncm' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 12).toString() != "qtdPallet") { throw RuntimeError("Faltou a coluna 'qtdPallet' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 13).toString() != "custo") { throw RuntimeError("Faltou a coluna 'custo' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 14).toString() != "precoVenda") { throw RuntimeError("Faltou a coluna 'precoVenda' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 15).toString() != "ui") { throw RuntimeError("Faltou a coluna 'ui' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 16).toString() != "un2") { throw RuntimeError("Faltou a coluna 'un2' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 17).toString() != "minimo") { throw RuntimeError("Faltou a coluna 'minimo' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 18).toString() != "mva") { throw RuntimeError("Faltou a coluna 'mva' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 19).toString() != "st") { throw RuntimeError("Faltou a coluna 'st' no cabeçalho da tabela!"); }
  if (xlsx.readValue(1, 20).toString() != "sticms") { throw RuntimeError("Faltou a coluna 'sticms' no cabeçalho da tabela!"); }
}

void ImportaProdutos::closeEvent(QCloseEvent *event) {
  if (qApp->getInTransaction()) { qApp->rollbackTransaction(""); }

  QDialog::closeEvent(event);
}

void ImportaProdutos::on_checkBoxRepresentacao_toggled(const bool checked) {
  // In optimized mode, preview is read-only so we only update the DB
  // The representacao flag is stored in the fornecedor table, not produto
  SqlQuery query;

  if (not query.exec("UPDATE fornecedor SET representacao = " + QString(checked ? "TRUE" : "FALSE") + " WHERE idFornecedor IN (" + idsFornecedor + ")")) {
    throw RuntimeException("Erro guardando 'Representacao' em Fornecedor: " + query.lastError().text());
  }
}

// NOTE: 3colocar tabela relacao para precos diferenciados por loja (associar produto_has_preco <->
// produto_has_preco_has_loja ou guardar idLoja em produto_has_preco)
// NOTE: remover idProdutoRelacionado?

// TODO: 4markup esta exibindo errado ou salvando errado
// TODO: 4nao mostrar promocao descontinuado
// TODO: 0se der erro durante a leitura o arquivo nao é fechado
// TODO: 0nao marcou produtos representacao com flag 1
// TODO: 0ler 'multiplo' na importacao (para produtos que usam minimo)
// TODO: mostrar os totais na tela e nao apenas na caixa de dialogo

// NOTE: para arrumar o problema da ambiguidade m2cx/pccx:
//       -usar uma segunda coluna 'pccx' tambem
//       -no caso dos produtos por metro é usado ambas as colunas m2cx/pccx mas nos outros produtos apenas o 'pccx'
//       -para minimo/multiplo usar a relacao 'quantCaixa' de forma que se o minimo for uma caixa, entao o minimo é 1,
//        e o multiplo sendo 1/4 de caixa será 0,25. esses numeros serão portanto os valores de minimo e singlestep respectivamente
//        do spinbox.

// NOTE: ao inves de cadastrar uma tabela de estoque, quando o estoque for gerado (importacao de xml) criar uma linha
// correspondente na tabela produto com flag estoque, esse produto vai ser listado junto dos outros porem com cor
// diferente

// estoques gerados por tabela nao terao dados de impostos enquanto os de xml sim

// obs1: o orcamento nao gera pré-consumo mas ao fechar pedido o estoque pode não estar mais disponivel
// obs2: quando fechar pedido gerar pré-consumo
// obs3: quando fechar pedido mudar status de 'pendente' para 'estoque' para nao aparecer na tela de compras
// obs4: colocar na tabela produto um campo para indicar qual o estoque relacionado
