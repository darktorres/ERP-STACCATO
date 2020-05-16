#include "importaprodutos.h"
#include "ui_importaprodutos.h"

#include "application.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "importaprodutosproxymodel.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "validadedialog.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

ImportaProdutos::ImportaProdutos(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::ImportaProdutos) {
  ui->setupUi(this);

  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &ImportaProdutos::on_checkBoxRepresentacao_toggled);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ImportaProdutos::on_pushButtonSalvar_clicked);

  setWindowFlags(Qt::Window);

  setVariantMap();
  setProgressDialog();
  setupModels();
}

ImportaProdutos::~ImportaProdutos() { delete ui; }

void ImportaProdutos::importarTabela() {
  if (not readFile()) { return; }
  if (not readValidade()) { return; }

  if (not qApp->startTransaction("ImportaProdutos::importaTabela", false)) { return; }

  if (not importar()) {
    db.close();
    qApp->rollbackTransaction();
    close();
  }
}

bool ImportaProdutos::verificaSeRepresentacao() {
  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec() or not queryFornecedor.first()) { return qApp->enqueueError(false, "Erro lendo tabela fornecedor: " + queryFornecedor.lastError().text(), this); }

  ui->checkBoxRepresentacao->setChecked(queryFornecedor.value("representacao").toBool());

  return true;
}

bool ImportaProdutos::atualizaProduto() {
  currentRow = hashTodos.value(variantMap.value("fornecedor").toString() + variantMap.value("codComercial").toString() + variantMap.value("ui").toString() + QString::number(static_cast<int>(tipo)));

  if (hashImportacao.value(currentRow)) {
    variantMap.insert("fornecedor", "PRODUTO REPETIDO NA TABELA");
    return insereEmErro();
  }

  hashImportacao[currentRow] = true;

  if (not atualizaCamposProduto()) { return false; }
  if (not marcaProdutoNaoDescontinuado()) { return false; }

  return true;
}

bool ImportaProdutos::importar() {
  progressDialog->show();

  QXlsx::Document xlsx(file);

  if (not xlsx.selectSheet("BASE")) { return false; }

  if (not verificaTabela(xlsx)) { return false; }
  if (not cadastraFornecedores(xlsx)) { return false; }
  if (not verificaSeRepresentacao()) { return false; }
  if (not marcaTodosProdutosDescontinuados()) { return false; }
  if (not mostraApenasEstesFornecedores()) { return false; }

  itensExpired = modelProduto.rowCount();

  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
    hashTodos[modelProduto.data(row, "fornecedor").toString() + modelProduto.data(row, "codComercial").toString() + modelProduto.data(row, "ui").toString() +
              modelProduto.data(row, "promocao").toString()] = row;
  }

  int current = 0;
  bool canceled = false;

  const int rows = xlsx.dimension().rowCount();

  for (int row = 2; row < rows; ++row) {
    if (progressDialog->wasCanceled()) {
      canceled = true;
      break;
    }

    progressDialog->setValue(current++);

    if (xlsx.read(row, 1).toString().isEmpty()) { continue; }

    leituraProduto(xlsx, row);

    if (camposForaDoPadrao()) {
      insereEmErro();
      continue;
    }

    const bool existeNoModel = verificaSeProdutoJaCadastrado();
    const bool success = existeNoModel ? atualizaProduto() : insereEmOk();

    if (not success) { return false; }
  }

  progressDialog->cancel();

  if (canceled) { return false; }

  setupTables();

  show();

  const QString resultado = "Produtos importados: " + QString::number(itensImported) + "\nProdutos atualizados: " + QString::number(itensUpdated) +
                            "\nNão modificados: " + QString::number(itensNotChanged) + "\nDescontinuados: " + QString::number(itensExpired) + "\nCom erro: " + QString::number(itensError);

  QMessageBox::information(this, "Aviso!", resultado);

  return true;
}

void ImportaProdutos::setProgressDialog() {
  progressDialog = new QProgressDialog(this);
  progressDialog->reset();
  progressDialog->setCancelButton(nullptr);
  progressDialog->setLabelText("Importando...");
  progressDialog->setWindowTitle("ERP Staccato");
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setMinimum(0);
  progressDialog->setMaximum(0);
  progressDialog->setCancelButtonText("Cancelar");
}

bool ImportaProdutos::readFile() {
  file = QFileDialog::getOpenFileName(this, "Importar tabela genérica", QDir::currentPath(), tr("Excel (*.xlsx)"));

  if (file.isEmpty()) { return false; }

  setWindowTitle(file);

  return true;
}

bool ImportaProdutos::readValidade() {
  auto *validadeDlg = new ValidadeDialog(this);

  if (not validadeDlg->exec()) { return false; }

  validade = validadeDlg->getValidade();

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
  modelProduto.setHeaderData("ncmEx", "NCM EX");
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
  modelErro.setHeaderData("ncmEx", "NCM EX");
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
  ui->tableProdutos->setAutoResize(false);

  ui->tableProdutos->setModel(&modelProduto);

  for (int column = 0; column < modelProduto.columnCount(); ++column) {
    if (modelProduto.record().fieldName(column).endsWith("Upd")) { ui->tableProdutos->setColumnHidden(column, true); }
  }

  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idEstoque");
  ui->tableProdutos->hideColumn("estoqueRestante");
  ui->tableProdutos->hideColumn("quantCaixa");
  ui->tableProdutos->hideColumn("idFornecedor");
  ui->tableProdutos->hideColumn("desativado");
  ui->tableProdutos->hideColumn("descontinuado");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("cfop");
  ui->tableProdutos->hideColumn("atualizarTabelaPreco");
  ui->tableProdutos->hideColumn("temLote");
  ui->tableProdutos->hideColumn("tipo");
  ui->tableProdutos->hideColumn("comissao");
  ui->tableProdutos->hideColumn("observacoes");
  ui->tableProdutos->hideColumn("origem");
  ui->tableProdutos->hideColumn("representacao");
  ui->tableProdutos->hideColumn("icms");
  ui->tableProdutos->hideColumn("cst");
  ui->tableProdutos->hideColumn("ipi");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("idProdutoRelacionado");

  ui->tableProdutos->setItemDelegateForColumn("validade", new DateFormatDelegate(this));

  auto *doubleDelegate = new DoubleDelegate(this, 4);
  auto *reaisDelegate = new ReaisDelegate(this, 4);
  ui->tableProdutos->setItemDelegateForColumn("m2cx", doubleDelegate);
  ui->tableProdutos->setItemDelegateForColumn("kgcx", doubleDelegate);
  ui->tableProdutos->setItemDelegateForColumn("qtdPallet", doubleDelegate);
  ui->tableProdutos->setItemDelegateForColumn("custo", reaisDelegate);
  ui->tableProdutos->setItemDelegateForColumn("precoVenda", reaisDelegate);

  auto *porcDelegate = new PorcentagemDelegate(false, this);
  ui->tableProdutos->setItemDelegateForColumn("icms", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("ipi", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("markup", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("st", new PorcentagemDelegate(true, this));
  ui->tableProdutos->setItemDelegateForColumn("sticms", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("mva", porcDelegate);

  //-------------------------------------------------------------//

  ui->tableErro->setAutoResize(false);

  ui->tableErro->setModel(&modelErro);

  for (int column = 0; column < modelErro.columnCount(); ++column) {
    if (modelErro.record().fieldName(column).endsWith("Upd")) { ui->tableErro->setColumnHidden(column, true); }
  }

  ui->tableErro->hideColumn("idProduto");
  ui->tableErro->hideColumn("idEstoque");
  ui->tableErro->hideColumn("estoqueRestante");
  ui->tableErro->hideColumn("idFornecedor");
  ui->tableErro->hideColumn("desativado");
  ui->tableErro->hideColumn("descontinuado");
  ui->tableErro->hideColumn("estoque");
  ui->tableErro->hideColumn("cfop");
  ui->tableErro->hideColumn("atualizarTabelaPreco");
  ui->tableErro->hideColumn("temLote");
  ui->tableErro->hideColumn("tipo");
  ui->tableErro->hideColumn("comissao");
  ui->tableErro->hideColumn("observacoes");
  ui->tableErro->hideColumn("origem");
  ui->tableErro->hideColumn("representacao");
  ui->tableErro->hideColumn("icms");
  ui->tableErro->hideColumn("cst");
  ui->tableErro->hideColumn("ipi");
  ui->tableErro->hideColumn("estoque");
  ui->tableErro->hideColumn("promocao");
  ui->tableErro->hideColumn("idProdutoRelacionado");

  ui->tableErro->setItemDelegateForColumn("validade", new DateFormatDelegate(this));

  ui->tableErro->setItemDelegateForColumn("m2cx", doubleDelegate);
  ui->tableErro->setItemDelegateForColumn("kgcx", doubleDelegate);
  ui->tableErro->setItemDelegateForColumn("qtdPallet", doubleDelegate);
  ui->tableErro->setItemDelegateForColumn("custo", reaisDelegate);
  ui->tableErro->setItemDelegateForColumn("precoVenda", reaisDelegate);

  ui->tableErro->setItemDelegateForColumn("icms", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("ipi", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("markup", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("st", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("sticms", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("mva", porcDelegate);
}

void ImportaProdutos::setVariantMap() {
  variantMap.insert("fornecedor", QVariant(QVariant::String));
  variantMap.insert("descricao", QVariant(QVariant::String));
  variantMap.insert("un", QVariant(QVariant::String));
  variantMap.insert("colecao", QVariant(QVariant::String));
  variantMap.insert("m2cx", QVariant(QVariant::Double));
  variantMap.insert("pccx", QVariant(QVariant::Double));
  variantMap.insert("kgcx", QVariant(QVariant::Double));
  variantMap.insert("formComercial", QVariant(QVariant::String));
  variantMap.insert("codComercial", QVariant(QVariant::String));
  variantMap.insert("codBarras", QVariant(QVariant::String));
  variantMap.insert("ncm", QVariant(QVariant::String));
  variantMap.insert("qtdPallet", QVariant(QVariant::Double));
  variantMap.insert("custo", QVariant(QVariant::Double));
  variantMap.insert("precoVenda", QVariant(QVariant::Double));
  variantMap.insert("ui", QVariant(QVariant::String));
  variantMap.insert("un2", QVariant(QVariant::String));
  variantMap.insert("minimo", QVariant(QVariant::Double));
  variantMap.insert("st", QVariant(QVariant::Double));
  variantMap.insert("sticms", QVariant(QVariant::Double));
}

bool ImportaProdutos::cadastraFornecedores(QXlsx::Document &xlsx) {
  const int rows = xlsx.dimension().rowCount();

  QStringList m_fornecedores;

  int count = 0;

  for (int row = 2; row < rows; ++row) {
    const QString fornec = xlsx.read(row, 1).toString();

    if (not fornec.isEmpty()) { ++count; }
    if (fornec.isEmpty() or m_fornecedores.contains(fornec)) { continue; }

    m_fornecedores << xlsx.read(row, 1).toString();
  }

  progressDialog->setMaximum(count);

  QStringList ids;

  for (auto const &m_fornecedor : m_fornecedores) {
    fornecedor = m_fornecedor;

    const auto idFornecedor = buscarCadastrarFornecedor();

    if (not idFornecedor) { return false; }

    ids << QString::number(idFornecedor.value());

    fornecedores.insert(fornecedor, idFornecedor.value());

    QSqlQuery queryFornecedor;
    queryFornecedor.prepare("UPDATE fornecedor SET validadeProdutos = :validade WHERE razaoSocial = :razaoSocial");
    queryFornecedor.bindValue(":validade", qApp->serverDate().addDays(validade));
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) { return qApp->enqueueError(false, "Erro salvando validade: " + queryFornecedor.lastError().text(), this); }
  }

  idsFornecedor = ids.join(",");

  if (fornecedores.isEmpty()) { return qApp->enqueueError(false, "Erro ao cadastrar fornecedores!", this); }

  return true;
}

bool ImportaProdutos::mostraApenasEstesFornecedores() {
  modelProduto.setFilter("idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)));

  if (not modelProduto.select()) { return false; }

  return true;
}

bool ImportaProdutos::marcaTodosProdutosDescontinuados() {
  QSqlQuery query;

  if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)))) {
    return qApp->enqueueError(false, "Erro marcando produtos descontinuados: " + query.lastError().text(), this);
  }

  return true;
}

void ImportaProdutos::contaProdutos() {
  QSqlQuery queryProdSize("SELECT COUNT(*) FROM [BASE$]", db);
  if (not queryProdSize.first()) { return; }
  progressDialog->setMaximum(queryProdSize.value(0).toInt());
}

void ImportaProdutos::consistenciaDados() {
  const auto keys = variantMap.keys();

  for (const auto &key : keys) {
    if (variantMap.value(key).toString().contains("*")) { variantMap.insert(key, variantMap.value(key).toString().remove("*")); }
  }

  variantMap.insert("m2cx", variantMap.value("m2cx").toString().replace(",", ".").toDouble());
  variantMap.insert("pccx", variantMap.value("pccx").toString().replace(",", ".").toDouble());
  variantMap.insert("kgcx", variantMap.value("kgcx").toString().replace(",", ".").toDouble());
  variantMap.insert("minimo", variantMap.value("minimo").toString().replace(",", ".").toDouble());
  variantMap.insert("qtdPallet", variantMap.value("qtdPallet").toString().replace(",", ".").toDouble());
  variantMap.insert("custo", variantMap.value("custo").toString().replace(",", ".").toDouble());
  variantMap.insert("precoVenda", variantMap.value("precoVenda").toString().replace(",", ".").toDouble());

  if (variantMap.value("ui").isNull()) { variantMap.insert("ui", 0); }

  if (variantMap.value("ncm").toString().length() == 10) {
    variantMap.insert("ncmEx", variantMap.value("ncm").toString().right(2));
    variantMap.insert("ncm", variantMap.value("ncm").toString().left(8));
  }

  const QString un = variantMap.value("un").toString().toUpper();

  variantMap.insert("un", (un == "M2" or un == "M²") ? "M²" : un);

  const double m2cx = variantMap.value("m2cx").toDouble();
  const double pccx = variantMap.value("pccx").toDouble();

  variantMap.insert("quantCaixa", (un == "M2" or un == "M²" or un == "ML") ? m2cx : pccx);

  variantMap.insert("ncm", variantMap.value("ncm").toString().remove(".").remove(",").remove("-").remove(" "));
  variantMap.insert("codBarras", variantMap.value("codBarras").toString().remove(".").remove(","));
  variantMap.insert("codComercial", variantMap.value("codComercial").toString().remove(".").remove(","));

  variantMap.insert("minimo", variantMap.value("minimo").toDouble());

  if (qFuzzyIsNull(variantMap.value("minimo").toDouble())) { variantMap.insert("minimo", QVariant()); }
  if (qFuzzyIsNull(variantMap.value("multiplo").toDouble())) { variantMap.insert("multiplo", QVariant()); }
}

void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
  QString m_fornecedor = xlsx.read(row, 1).toString();
  QString m_descricao = xlsx.read(row, 2).toString().remove("*");
  QString m_un = xlsx.read(row, 3).toString().remove("*");
  QString m_colecao = xlsx.read(row, 4).toString().remove("*");
  double m_m2cx = xlsx.read(row, 5).toDouble();
  double m_pccx = xlsx.read(row, 6).toDouble();
  double m_kgcx = xlsx.read(row, 7).toDouble();
  QString m_formComercial = xlsx.read(row, 8).toString().remove("*");
  QString m_codComercial = xlsx.read(row, 9).toString().remove("*").remove(".").remove(",");
  QString m_codBarras = xlsx.read(row, 10).toString().remove("*").remove(".").remove(",");
  QString m_ncm = xlsx.read(row, 11).toString().remove("*").remove(".").remove(",").remove("-").remove(" ");
  double m_qtdPallet = xlsx.read(row, 12).toDouble();
  double m_custo = xlsx.read(row, 13).toDouble();
  double m_precoVenda = xlsx.read(row, 14).toDouble();
  QString m_ui = xlsx.read(row, 15).toString().remove("*");
  QString m_un2 = xlsx.read(row, 16).toString().remove("*");
  double m_minimo = xlsx.read(row, 17).toDouble();
  double m_mva = xlsx.read(row, 18).toDouble();
  double m_st = xlsx.read(row, 19).toDouble();
  double m_sticms = xlsx.read(row, 20).toDouble();

  // consistencia dados

  if (m_ui.isEmpty()) { m_ui = "0"; }

  QString m_ncmEx;

  if (m_ncm.length() == 10) {
    m_ncmEx = m_ncm.right(2);
    m_ncm = m_ncm.left(8);
  }

  if (m_un == "M²") { m_un = "M2"; }

  double m_quantCaixa = (m_un == "M2" or m_un == "ML") ? m_m2cx : m_pccx;

  // insere no variantMap

  variantMap.insert("fornecedor", m_fornecedor);
  variantMap.insert("descricao", m_descricao);
  variantMap.insert("un", m_un);
  variantMap.insert("colecao", m_colecao);
  variantMap.insert("m2cx", m_m2cx);
  variantMap.insert("pccx", m_pccx);
  variantMap.insert("kgcx", m_kgcx);
  variantMap.insert("formComercial", m_formComercial);
  variantMap.insert("codComercial", m_codComercial);
  variantMap.insert("codBarras", m_codBarras);
  variantMap.insert("ncm", m_ncm);
  variantMap.insert("ncmEx", m_ncmEx);
  variantMap.insert("qtdPallet", m_qtdPallet);
  variantMap.insert("custo", m_custo);
  variantMap.insert("precoVenda", m_precoVenda);
  variantMap.insert("ui", m_ui);
  variantMap.insert("un2", m_un2);
  variantMap.insert("minimo", m_minimo);
  variantMap.insert("mva", m_mva);
  variantMap.insert("st", m_st);
  variantMap.insert("sticms", m_sticms);
  variantMap.insert("quantCaixa", m_quantCaixa);
}

bool ImportaProdutos::atualizaCamposProduto() {
  bool changed = false;

  const auto keys = variantMap.keys();

  for (const auto &key : keys) {
    if (not variantMap.value(key).isNull() and modelProduto.data(currentRow, key) != variantMap.value(key)) {
      if (not modelProduto.setData(currentRow, key, variantMap.value(key))) { return false; }
      if (not modelProduto.setData(currentRow, key + "Upd", static_cast<int>(FieldColors::Yellow))) { return false; }
    } else {
      if (not modelProduto.setData(currentRow, key + "Upd", static_cast<int>(FieldColors::White))) { return false; }
    }
  }

  if (not modelProduto.setData(currentRow, "atualizarTabelaPreco", true)) { return false; }

  if (modelProduto.data(currentRow, "ncmEx").toString().isEmpty()) {
    if (not modelProduto.setData(currentRow, "ncmExUpd", static_cast<int>(FieldColors::White))) { return false; }
  }

  const QString validadeStr = qApp->serverDate().addDays(validade).toString("yyyy-MM-dd");

  if (modelProduto.data(currentRow, "validade") != validadeStr) {
    if (not modelProduto.setData(currentRow, "validade", validadeStr)) { return false; }
    if (not modelProduto.setData(currentRow, "validadeUpd", static_cast<int>(FieldColors::Yellow))) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(currentRow, "validadeUpd", static_cast<int>(FieldColors::White))) { return false; }
  }

  double markup = 100 * ((variantMap.value("precoVenda").toDouble() / variantMap.value("custo").toDouble()) - 1.);
  const QString markupRound = QString::number(markup, 'f', 4);
  markup = markupRound.toDouble();

  if (not qFuzzyCompare(modelProduto.data(currentRow, "markup").toDouble(), markup)) {
    if (not modelProduto.setData(currentRow, "markup", markup)) { return false; }
    if (not modelProduto.setData(currentRow, "markupUpd", static_cast<int>(FieldColors::Yellow))) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(currentRow, "markupUpd", static_cast<int>(FieldColors::White))) { return false; }
  }

  changed ? itensUpdated++ : itensNotChanged++;

  variantMap.clear();

  return true;
}

bool ImportaProdutos::marcaProdutoNaoDescontinuado() {
  if (not modelProduto.setData(currentRow, "descontinuado", false)) { return false; }

  itensExpired--;

  return true;
}

bool ImportaProdutos::verificaSeProdutoJaCadastrado() {
  return hashTodos.contains(variantMap.value("fornecedor").toString() + variantMap.value("codComercial").toString() + variantMap.value("ui").toString() + QString::number(static_cast<int>(tipo)));
}

bool ImportaProdutos::pintarCamposForaDoPadrao(const int row) {
  // Fora do padrão
  if (variantMap.value("ncm").toString() == "0" or variantMap.value("ncm").toString().isEmpty() or
      (variantMap.value("ncm").toString().length() != 8 and variantMap.value("ncm").toString().length() != 10)) {
    if (not modelErro.setData(row, "ncmUpd", static_cast<int>(FieldColors::Gray))) { return false; }
  }

  if (variantMap.value("codBarras").toString() == "0" or variantMap.value("codBarras").toString().isEmpty()) {
    if (not modelErro.setData(row, "codBarrasUpd", static_cast<int>(FieldColors::Gray))) { return false; }
  }

  // Errados
  if (variantMap.value("fornecedor").toString() == "PRODUTO REPETIDO NA TABELA") {
    if (not modelErro.setData(row, "fornecedorUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if ((variantMap.value("un").toString() == "M2" or variantMap.value("un").toString() == "M²" or variantMap.value("un").toString() == "ML") and variantMap.value("m2cx").toDouble() <= 0.) {
    if (not modelErro.setData(row, "m2cxUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("un").toString() != "M2" and variantMap.value("un").toString() != "M²" and variantMap.value("un").toString() != "ML" and variantMap.value("pccx").toDouble() < 1) {
    if (not modelErro.setData(row, "pccxUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("codComercial").toString() == "0" or variantMap.value("codComercial").toString().isEmpty()) {
    if (not modelErro.setData(row, "codComercialUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("custo").toDouble() <= 0.) {
    if (not modelErro.setData(row, "custoUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("precoVenda").toDouble() <= 0.) {
    if (not modelErro.setData(row, "precoVendaUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("precoVenda").toDouble() < variantMap.value("custo").toDouble()) {
    if (not modelErro.setData(row, "precoVendaUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  return true;
}

bool ImportaProdutos::camposForaDoPadrao() {
  // Errados
  const QString un = variantMap.value("un").toString();
  const double m2cx = variantMap.value("m2cx").toDouble();
  const double pccx = variantMap.value("pccx").toDouble();
  const QString codComercial = variantMap.value("codComercial").toString();
  const double custo = variantMap.value("custo").toDouble();
  const double precoVenda = variantMap.value("precoVenda").toDouble();

  if ((un == "M2" or un == "ML") and m2cx <= 0.) { return true; }
  if (un != "M2" and un != "ML" and pccx < 1) { return true; }
  if (codComercial == "0" or codComercial.isEmpty()) { return true; }
  if (custo <= 0.) { return true; }
  if (precoVenda <= 0.) { return true; }
  if (precoVenda < custo) { return true; }

  return false;
}

bool ImportaProdutos::insereEmErro() {
  const int row = modelErro.insertRowAtEnd();

  const auto keys = variantMap.keys();

  for (const auto &key : keys) {
    if (not modelErro.setData(row, key, variantMap.value(key))) { return false; }
    if (not modelErro.setData(row, key + "Upd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  if (not modelErro.setData(row, "idFornecedor", fornecedores.value(variantMap.value("fornecedor").toString()))) { return false; }

  if (not modelErro.setData(row, "atualizarTabelaPreco", true)) { return false; }
  const QString data = qApp->serverDate().addDays(validade).toString("yyyy-MM-dd");
  if (not modelErro.setData(row, "validade", data)) { return false; }
  if (not modelErro.setData(row, "validadeUpd", static_cast<int>(FieldColors::Green))) { return false; }

  const double markup = 100 * ((variantMap.value("precoVenda").toDouble() / variantMap.value("custo").toDouble()) - 1.);
  if (not modelErro.setData(row, "markup", markup)) { return false; }
  if (not modelErro.setData(row, "markupUpd", static_cast<int>(FieldColors::Green))) { return false; }

  if (variantMap.value("ncm").toString().length() == 10) {
    if (not modelErro.setData(row, "ncmEx", variantMap.value("ncm").toString().right(2))) { return false; }
    if (not modelErro.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
    variantMap.insert("ncm", variantMap.value("ncm").toString().left(8));
  } else {
    if (not modelErro.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  if (not pintarCamposForaDoPadrao(row)) { return false; }

  itensError++;

  variantMap.clear();

  return true;
}

bool ImportaProdutos::insereEmOk() {
  const int row = modelProduto.insertRowAtEnd();

  const auto keys = variantMap.keys();

  for (const auto &key : keys) {
    if (not modelProduto.setData(row, key, variantMap.value(key))) { return false; }
    if (not modelProduto.setData(row, key + "Upd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  if (not modelProduto.setData(row, "promocao", static_cast<int>(tipo))) { return false; }

  if (tipo == Tipo::Promocao) {
    QSqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE idFornecedor = :idFornecedor AND codComercial = :codComercial AND promocao = FALSE AND estoque = FALSE");
    query.bindValue(":idFornecedor", fornecedores.value(variantMap.value("fornecedor").toString()));
    query.bindValue(":codComercial", modelProduto.data(row, "codComercial"));

    if (not query.exec()) { return qApp->enqueueError(false, "Erro buscando produto relacionado: " + query.lastError().text(), this); }

    if (query.first() and not modelProduto.setData(row, "idProdutoRelacionado", query.value("idProduto"))) { return false; }
  }

  const int idFornecedor = fornecedores.value(variantMap.value("fornecedor").toString());
  if (not modelProduto.setData(row, "idFornecedor", idFornecedor)) { return false; }

  if (not modelProduto.setData(row, "atualizarTabelaPreco", true)) { return false; }
  if (not modelProduto.setData(row, "validade", qApp->serverDate().addDays(validade).toString("yyyy-MM-dd"))) { return false; }
  if (not modelProduto.setData(row, "validadeUpd", static_cast<int>(FieldColors::Green))) { return false; }

  const double markup = 100 * ((variantMap.value("precoVenda").toDouble() / variantMap.value("custo").toDouble()) - 1.);
  if (not modelProduto.setData(row, "markup", markup)) { return false; }
  if (not modelProduto.setData(row, "markupUpd", static_cast<int>(FieldColors::Green))) { return false; }

  if (variantMap.value("ncm").toString().length() == 10) {
    if (not modelProduto.setData(row, "ncmEx", variantMap.value("ncm").toString().right(2))) { return false; }
    if (not modelProduto.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
    variantMap.insert("ncm", variantMap.value("ncm").toString().left(8));
  } else {
    if (not modelProduto.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  hashTodos[modelProduto.data(row, "fornecedor").toString() + modelProduto.data(row, "codComercial").toString() + modelProduto.data(row, "ui").toString() +
            modelProduto.data(row, "promocao").toString()] = row;

  hashImportacao[row] = true;

  itensImported++;

  variantMap.clear();

  return true;
}

bool ImportaProdutos::cadastraProduto() { return insereEmOk(); }

std::optional<int> ImportaProdutos::buscarCadastrarFornecedor() {
  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    qApp->enqueueError("Erro buscando fornecedor: " + queryFornecedor.lastError().text(), this);
    return {};
  }

  if (not queryFornecedor.first()) {
    queryFornecedor.prepare("INSERT INTO fornecedor (razaoSocial) VALUES (:razaoSocial)");
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) {
      qApp->enqueueError("Erro cadastrando fornecedor: " + queryFornecedor.lastError().text(), this);
      return {};
    }

    if (queryFornecedor.lastInsertId().isNull()) {
      qApp->enqueueError("Erro lastInsertId", this);
      return {};
    }

    return queryFornecedor.lastInsertId().toInt();
  }

  return queryFornecedor.value("idFornecedor").toInt();
}

bool ImportaProdutos::salvar() {
  if (not modelProduto.submitAll()) { return false; }

  QSqlQuery queryPrecos;
  queryPrecos.prepare("INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim FROM "
                      "produto WHERE atualizarTabelaPreco = TRUE");
  queryPrecos.bindValue(":validadeInicio", qApp->serverDate().toString("yyyy-MM-dd"));
  queryPrecos.bindValue(":validadeFim", qApp->serverDate().addDays(validade).toString("yyyy-MM-dd"));

  if (not queryPrecos.exec()) { return qApp->enqueueError(false, "Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text(), this); }

  if (not queryPrecos.exec("UPDATE produto SET atualizarTabelaPreco = FALSE")) { return qApp->enqueueError(false, "Erro comunicando com banco de dados: " + queryPrecos.lastError().text(), this); }

  return true;
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
  if (modelErro.rowCount() > 0) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Produtos com erro não serão salvos. Deseja continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  if (not salvar()) { return; }

  qApp->endTransaction();

  qApp->enqueueInformation("Tabela salva com sucesso!", this);

  close();
}

bool ImportaProdutos::verificaTabela(QXlsx::Document &xlsx) {
  if (xlsx.read(1, 1).toString() != "fornecedor") { return false; }
  if (xlsx.read(1, 2).toString() != "descricao") { return false; }
  if (xlsx.read(1, 3).toString() != "un") { return false; }
  if (xlsx.read(1, 4).toString() != "colecao") { return false; }
  if (xlsx.read(1, 5).toString() != "m2cx") { return false; }
  if (xlsx.read(1, 6).toString() != "pccx") { return false; }
  if (xlsx.read(1, 7).toString() != "kgcx") { return false; }
  if (xlsx.read(1, 8).toString() != "formComercial") { return false; }
  if (xlsx.read(1, 9).toString() != "codComercial") { return false; }
  if (xlsx.read(1, 10).toString() != "codBarras") { return false; }
  if (xlsx.read(1, 11).toString() != "ncm") { return false; }
  if (xlsx.read(1, 12).toString() != "qtdPallet") { return false; }
  if (xlsx.read(1, 13).toString() != "custo") { return false; }
  if (xlsx.read(1, 14).toString() != "precoVenda") { return false; }
  if (xlsx.read(1, 15).toString() != "ui") { return false; }
  if (xlsx.read(1, 16).toString() != "un2") { return false; }
  if (xlsx.read(1, 17).toString() != "minimo") { return false; }
  if (xlsx.read(1, 18).toString() != "mva") { return false; }
  if (xlsx.read(1, 19).toString() != "st") { return false; }
  if (xlsx.read(1, 20).toString() != "sticms") { return false; }

  if (xlsx.read(2, 1).toString().contains("=IF(")) { return qApp->enqueueError(false, "Células estão com fórmula! Trocar por valores no Excel!", this); }

  return true;
}

void ImportaProdutos::closeEvent(QCloseEvent *event) {
  if (qApp->getInTransaction()) { qApp->rollbackTransaction(); }

  QDialog::closeEvent(event);
}

void ImportaProdutos::on_checkBoxRepresentacao_toggled(const bool checked) {
  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
    if (not modelProduto.setData(row, "representacao", checked)) { return; }
  }

  QSqlQuery query;
  if (not query.exec("UPDATE fornecedor SET representacao = " + QString(checked ? "TRUE" : "FALSE") + " WHERE idFornecedor IN (" + idsFornecedor + ")")) {
    return qApp->enqueueError("Erro guardando 'Representacao' em Fornecedor: " + query.lastError().text(), this);
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
