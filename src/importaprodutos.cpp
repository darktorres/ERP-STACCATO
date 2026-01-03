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

    processarArquivo();
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

  processarArquivo();
}

void ImportaProdutos::verificaSeRepresentacao() {
  SqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", m_fornecedor.left(100));

  if (not queryFornecedor.exec()) { throw RuntimeException("Erro lendo tabela fornecedor: " + queryFornecedor.lastError().text()); }

  if (not queryFornecedor.first()) { throw RuntimeException("Dados não encontrados para fornecedor: '" + m_fornecedor + "'"); }

  ui->checkBoxRepresentacao->setChecked(queryFornecedor.value("representacao").toBool());
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

  // Sort by descontinuado column (use index since header name is now "Desc.")
  ui->tableProdutos->QTableView::sortByColumn(PREVIEW_COLUMNS.indexOf("descontinuado"), Qt::AscendingOrder);

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

  // Display names for column headers (matching legacy setupModels)
  static const QHash<QString, QString> headerNames = {
      {"idProduto", "ID"},
      {"idFornecedor", "ID Forn."},
      {"fornecedor", "Fornecedor"},
      {"descricao", "Descrição"},
      {"un", "Un."},
      {"un2", "Un.2"},
      {"colecao", "Coleção"},
      {"m2cx", "M./Cx."},
      {"pccx", "Pç./Cx."},
      {"kgcx", "Kg./Cx."},
      {"formComercial", "Form. Com."},
      {"codComercial", "Cód. Com."},
      {"codBarras", "Cód. Barras"},
      {"ncm", "NCM"},
      {"qtdPallet", "Qt. Pallet"},
      {"custo", "Custo"},
      {"precoVenda", "Preço Venda"},
      {"ui", "UI"},
      {"minimo", "Mínimo"},
      {"mva", "MVA"},
      {"st", "ST"},
      {"sticms", "ST ICMS"},
      {"quantCaixa", "Qt. Cx."},
      {"markup", "Markup"},
      {"validade", "Validade"},
      {"descontinuado", "Desc."},
  };

  // Setup column headers
  const int colCount = PREVIEW_COLUMNS.size();
  m_previewModel.setColumnCount(colCount);
  m_errorModel.setColumnCount(colCount);

  for (int i = 0; i < colCount; ++i) {
    const QString &field = PREVIEW_COLUMNS[i];
    const QString displayName = headerNames.value(field, field);
    m_previewModel.setHeaderData(i, Qt::Horizontal, displayName);
    m_errorModel.setHeaderData(i, Qt::Horizontal, displayName);
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

void ImportaProdutos::salvar() {
  // ==========================================================================
  // BATCH SAVE - Uses prepared statements with multi-row INSERT
  // Reduces ~3700 queries to ~40 queries (100 rows per batch)
  // Uses bound parameters for SQL injection protection
  // ==========================================================================

  if (m_productChanges.isEmpty()) { goto postProcess; }

  {
    // Query server for max_allowed_packet and calculate optimal batch size
    int batchSize = 100;  // Conservative default
    {
      SqlQuery packetQuery;
      if (packetQuery.exec("SHOW VARIABLES LIKE 'max_allowed_packet'") && packetQuery.next()) {
        const qint64 maxPacket = packetQuery.value(1).toLongLong();
        // Estimate ~2KB per row (conservative), use 70% of max_allowed_packet for safety margin
        constexpr int ESTIMATED_ROW_SIZE = 2048;
        const int maxRows = static_cast<int>((maxPacket * 0.7) / ESTIMATED_ROW_SIZE);
        // Clamp between 100 and 10000 rows
        batchSize = qBound(100, maxRows, 10000);
      }
    }

    constexpr int INSERT_COLS = 26;  // Number of columns in INSERT for new products
    constexpr int UPDATE_COLS = 27;  // Number of columns in UPDATE (includes idProduto)

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

    // Helper to build placeholder string for N rows: (?,?,...),(?,?,...),...
    auto buildPlaceholders = [](int numRows, int numCols) -> QString {
      QString rowPlaceholder = "(" + QString("?,").repeated(numCols);
      rowPlaceholder.chop(1);  // Remove trailing comma
      rowPlaceholder += ")";
      return QString((rowPlaceholder + ",").repeated(numRows)).chopped(1);
    };

    // ========================================================================
    // BATCH INSERT for new products (using prepared statement)
    // ========================================================================
    if (!newProducts.isEmpty()) {
      const int promocaoInt = static_cast<int>(tipo);

      for (int batchStart = 0; batchStart < newProducts.size(); batchStart += batchSize) {
        const int batchEnd = qMin(batchStart + batchSize, newProducts.size());
        const int batchCount = batchEnd - batchStart;

        QString sql = "INSERT INTO produto (atualizarTabelaPreco, promocao, idFornecedor, fornecedor, descricao, "
                      "un, un2, colecao, m2cx, pccx, kgcx, formComercial, codComercial, codBarras, ncm, "
                      "qtdPallet, custo, precoVenda, ui, minimo, mva, st, sticms, quantCaixa, markup, validade) VALUES "
                    + buildPlaceholders(batchCount, INSERT_COLS);

        SqlQuery query;
        query.prepare(sql);

        // Bind values for each row
        for (int i = batchStart; i < batchEnd; ++i) {
          const Produto &p = newProducts[i]->newData;
          query.addBindValue(true);                                              // atualizarTabelaPreco
          query.addBindValue(promocaoInt);                                       // promocao
          query.addBindValue(p.idFornecedor);                                    // idFornecedor
          query.addBindValue(p.fornecedor);                                      // fornecedor
          query.addBindValue(p.descricao);                                       // descricao
          query.addBindValue(p.un);                                              // un
          query.addBindValue(p.un2.isEmpty() ? QVariant() : p.un2);              // un2
          query.addBindValue(p.colecao);                                         // colecao
          query.addBindValue(p.m2cx);                                            // m2cx
          query.addBindValue(p.pccx);                                            // pccx
          query.addBindValue(p.kgcx);                                            // kgcx
          query.addBindValue(p.formComercial);                                   // formComercial
          query.addBindValue(p.codComercial);                                    // codComercial
          query.addBindValue(p.codBarras.isEmpty() ? QVariant() : p.codBarras);  // codBarras
          query.addBindValue(p.ncm.isEmpty() ? QVariant() : p.ncm);              // ncm
          query.addBindValue(p.qtdPallet);                                       // qtdPallet
          query.addBindValue(p.custo);                                           // custo
          query.addBindValue(p.precoVenda);                                      // precoVenda
          query.addBindValue(p.ui);                                              // ui
          query.addBindValue(p.minimo);                                          // minimo
          query.addBindValue(p.mva);                                             // mva
          query.addBindValue(p.st);                                              // st
          query.addBindValue(p.sticms);                                          // sticms
          query.addBindValue(p.quantCaixa);                                      // quantCaixa
          query.addBindValue(p.markup);                                          // markup
          query.addBindValue(p.validade.isValid() ? p.validade : QVariant());    // validade
        }

        if (!query.exec()) {
          throw RuntimeException("Erro inserindo produtos em lote: " + query.lastError().text());
        }
      }

      // Link promocao products in bulk (if needed)
      if (tipo == Tipo::Promocao) {
        SqlQuery linkQuery;
        linkQuery.prepare(
            "UPDATE produto p1 "
            "SET idProdutoRelacionado = ("
            "  SELECT p2.idProduto FROM produto p2 "
            "  WHERE p2.idFornecedor = p1.idFornecedor "
            "  AND p2.codComercial = p1.codComercial "
            "  AND p2.promocao = FALSE AND p2.estoque = FALSE "
            "  LIMIT 1"
            ") "
            "WHERE p1.promocao = TRUE AND p1.idProdutoRelacionado IS NULL "
            "AND p1.idFornecedor IN (" + idsFornecedor + ")");
        linkQuery.exec();  // Ignore errors - optional operation
      }
    }

    // ========================================================================
    // BATCH UPDATE for existing products using INSERT...ON DUPLICATE KEY UPDATE
    // ========================================================================
    if (!updateProducts.isEmpty()) {
      for (int batchStart = 0; batchStart < updateProducts.size(); batchStart += batchSize) {
        const int batchEnd = qMin(batchStart + batchSize, updateProducts.size());
        const int batchCount = batchEnd - batchStart;

        QString sql = "INSERT INTO produto (idProduto, idFornecedor, atualizarTabelaPreco, descontinuado, fornecedor, descricao, "
                      "un, un2, colecao, m2cx, pccx, kgcx, formComercial, codComercial, codBarras, ncm, "
                      "qtdPallet, custo, precoVenda, ui, minimo, mva, st, sticms, quantCaixa, markup, validade) VALUES "
                    + buildPlaceholders(batchCount, UPDATE_COLS)
                    + " ON DUPLICATE KEY UPDATE "
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
        query.prepare(sql);

        // Bind values for each row
        for (int i = batchStart; i < batchEnd; ++i) {
          const Produto &p = updateProducts[i]->newData;
          query.addBindValue(p.idProduto);                                       // idProduto
          query.addBindValue(p.idFornecedor);                                    // idFornecedor
          query.addBindValue(true);                                              // atualizarTabelaPreco
          query.addBindValue(false);                                             // descontinuado
          query.addBindValue(p.fornecedor);                                      // fornecedor
          query.addBindValue(p.descricao);                                       // descricao
          query.addBindValue(p.un);                                              // un
          query.addBindValue(p.un2.isEmpty() ? QVariant() : p.un2);              // un2
          query.addBindValue(p.colecao);                                         // colecao
          query.addBindValue(p.m2cx);                                            // m2cx
          query.addBindValue(p.pccx);                                            // pccx
          query.addBindValue(p.kgcx);                                            // kgcx
          query.addBindValue(p.formComercial);                                   // formComercial
          query.addBindValue(p.codComercial);                                    // codComercial
          query.addBindValue(p.codBarras.isEmpty() ? QVariant() : p.codBarras);  // codBarras
          query.addBindValue(p.ncm.isEmpty() ? QVariant() : p.ncm);              // ncm
          query.addBindValue(p.qtdPallet);                                       // qtdPallet
          query.addBindValue(p.custo);                                           // custo
          query.addBindValue(p.precoVenda);                                      // precoVenda
          query.addBindValue(p.ui);                                              // ui
          query.addBindValue(p.minimo);                                          // minimo
          query.addBindValue(p.mva);                                             // mva
          query.addBindValue(p.st);                                              // st
          query.addBindValue(p.sticms);                                          // sticms
          query.addBindValue(p.quantCaixa);                                      // quantCaixa
          query.addBindValue(p.markup);                                          // markup
          query.addBindValue(p.validade.isValid() ? p.validade : QVariant());    // validade
        }

        if (!query.exec()) {
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

void ImportaProdutos::marcaTodosProdutosDescontinuados() {
  SqlQuery query;

  if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)))) {
    throw RuntimeException("Erro marcando produtos descontinuados: " + query.lastError().text());
  }
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

void ImportaProdutos::on_pushButtonSalvar_clicked() {
  if (m_errorModel.rowCount() > 0) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Produtos com erro não serão salvos. Deseja continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.button(QMessageBox::Yes)->setText("Continuar");
    msgBox.button(QMessageBox::No)->setText("Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  try {
    salvar();
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
