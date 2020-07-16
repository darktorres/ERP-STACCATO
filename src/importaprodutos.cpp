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

  if (not queryFornecedor.exec() or not queryFornecedor.first()) { return qApp->enqueueException(false, "Erro lendo tabela fornecedor: " + queryFornecedor.lastError().text(), this); }

  ui->checkBoxRepresentacao->setChecked(queryFornecedor.value("representacao").toBool());

  return true;
}

bool ImportaProdutos::atualizaProduto() {
  const int row = hashModel.value(produto.fornecedor + produto.codComercial + produto.ui + QString::number(static_cast<int>(tipo)));

  if (vectorProdutosImportados.contains(row)) {
    produto.fornecedor = "PRODUTO REPETIDO NA TABELA";
    return insereEmErro();
  }

  vectorProdutosImportados << row;

  if (not atualizaCamposProduto(row)) { return false; }
  if (not marcaProdutoNaoDescontinuado(row)) { return false; }

  return true;
}

bool ImportaProdutos::importar() {
  QXlsx::Document xlsx(file);

  if (not xlsx.selectSheet("BASE")) { return false; }
  if (not verificaTabela(xlsx)) { return false; }

  progressDialog->show();

  if (not cadastraFornecedores(xlsx)) { return false; }
  if (not verificaSeRepresentacao()) { return false; }
  if (not marcaTodosProdutosDescontinuados()) { return false; }
  if (not mostraApenasEstesFornecedores()) { return false; }

  itensExpired = modelProduto.rowCount();

  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
    hashModel[modelProduto.data(row, "fornecedor").toString() + modelProduto.data(row, "codComercial").toString() + modelProduto.data(row, "ui").toString() +
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

    const bool existeNoModel = hashModel.contains(produto.fornecedor + produto.codComercial + produto.ui + QString::number(static_cast<int>(tipo)));
    const bool success = existeNoModel ? atualizaProduto() : insereEmOk();

    if (not success) { return false; }
  }

  progressDialog->cancel();

  if (canceled) { return false; }

  setupTables();

  showMaximized();

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

  auto *doubleDelegate = new DoubleDelegate(4, this);
  auto *reaisDelegate = new ReaisDelegate(4, true, this);
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
    queryFornecedor.bindValue(":validade", (validade == -1) ? QVariant() : qApp->serverDate().addDays(validade));
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) { return qApp->enqueueException(false, "Erro salvando validade: " + queryFornecedor.lastError().text(), this); }
  }

  idsFornecedor = ids.join(",");

  if (fornecedores.isEmpty()) { return qApp->enqueueException(false, "Erro ao cadastrar fornecedores!", this); }

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
    return qApp->enqueueException(false, "Erro marcando produtos descontinuados: " + query.lastError().text(), this);
  }

  return true;
}

void ImportaProdutos::contaProdutos() {
  QSqlQuery queryProdSize("SELECT COUNT(*) FROM [BASE$]", db);
  if (not queryProdSize.first()) { return; }
  progressDialog->setMaximum(queryProdSize.value(0).toInt());
}

void ImportaProdutos::leituraProduto(QXlsx::Document &xlsx, const int row) {
  produto = {};

  produto.idFornecedor = fornecedores.value(xlsx.read(row, 1).toString());
  produto.fornecedor = xlsx.read(row, 1).toString().toUpper();
  produto.descricao = xlsx.read(row, 2).toString().remove("*").toUpper();
  produto.un = xlsx.read(row, 3).toString().remove("*").toUpper().toUpper();
  produto.colecao = xlsx.read(row, 4).toString().remove("*").toUpper();
  produto.m2cx = qApp->roundDouble(xlsx.read(row, 5).toDouble());
  produto.pccx = qApp->roundDouble(xlsx.read(row, 6).toDouble());
  produto.kgcx = qApp->roundDouble(xlsx.read(row, 7).toDouble());
  produto.formComercial = xlsx.read(row, 8).toString().remove("*").toUpper();
  produto.codComercial = xlsx.read(row, 9).toString().remove("*").remove(".").remove(",").toUpper();
  produto.codBarras = xlsx.read(row, 10).toString().remove("*").remove(".").remove(",").toUpper();
  produto.ncm = xlsx.read(row, 11).toString().remove("*").remove(".").remove(",").remove("-").remove(" ").toUpper();
  produto.qtdPallet = qApp->roundDouble(xlsx.read(row, 12).toDouble());
  produto.custo = qApp->roundDouble(xlsx.read(row, 13).toDouble());
  produto.precoVenda = qApp->roundDouble(xlsx.read(row, 14).toDouble());
  produto.ui = xlsx.read(row, 15).toString().remove("*").toUpper();
  produto.un2 = xlsx.read(row, 16).toString().remove("*").toUpper();
  produto.minimo = qApp->roundDouble(xlsx.read(row, 17).toDouble());
  produto.mva = qApp->roundDouble(xlsx.read(row, 18).toDouble());
  produto.st = qApp->roundDouble(xlsx.read(row, 19).toDouble());
  produto.sticms = qApp->roundDouble(xlsx.read(row, 20).toDouble());
  produto.markup = qApp->roundDouble(((produto.precoVenda / produto.custo) - 1.) * 100);

  // consistencia dados

  if (produto.ui.isEmpty()) { produto.ui = "0"; }

  QString m_ncmEx;

  if (produto.ncm.length() == 10) {
    produto.ncmEx = produto.ncm.right(2);
    produto.ncm = produto.ncm.left(8);
  }

  if (produto.un == "M²") { produto.un = "M2"; }

  const double quantCaixa = (produto.un == "M2" or produto.un == "ML") ? produto.m2cx : produto.pccx;

  produto.quantCaixa = quantCaixa;
}

bool ImportaProdutos::atualizaCamposProduto(const int row) {
  bool changed = false;

  if (not modelProduto.setData(row, "atualizarTabelaPreco", true)) { return false; }

  const int yellow = static_cast<int>(FieldColors::Yellow);
  const int white = static_cast<int>(FieldColors::White);

  if (modelProduto.data(row, "fornecedor") != produto.fornecedor) {
    if (not modelProduto.setData(row, "fornecedor", produto.fornecedor)) { return false; }
    if (not modelProduto.setData(row, "fornecedorUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "fornecedorUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "descricao") != produto.descricao) {
    if (not modelProduto.setData(row, "descricao", produto.descricao)) { return false; }
    if (not modelProduto.setData(row, "descricaoUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "descricaoUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "un") != produto.un) {
    if (not modelProduto.setData(row, "un", produto.un)) { return false; }
    if (not modelProduto.setData(row, "unUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "unUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "colecao") != produto.colecao) {
    if (not modelProduto.setData(row, "colecao", produto.colecao)) { return false; }
    if (not modelProduto.setData(row, "colecaoUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "colecaoUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "m2cx") != produto.m2cx) {
    if (not modelProduto.setData(row, "m2cx", produto.m2cx)) { return false; }
    if (not modelProduto.setData(row, "m2cxUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "m2cxUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "pccx") != produto.pccx) {
    if (not modelProduto.setData(row, "pccx", produto.pccx)) { return false; }
    if (not modelProduto.setData(row, "pccxUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "pccxUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "kgcx") != produto.kgcx) {
    if (not modelProduto.setData(row, "kgcx", produto.kgcx)) { return false; }
    if (not modelProduto.setData(row, "kgcxUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "kgcxUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "formComercial") != produto.formComercial) {
    if (not modelProduto.setData(row, "formComercial", produto.formComercial)) { return false; }
    if (not modelProduto.setData(row, "formComercialUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "formComercialUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "codComercial") != produto.codComercial) {
    if (not modelProduto.setData(row, "codComercial", produto.codComercial)) { return false; }
    if (not modelProduto.setData(row, "codComercialUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "codComercialUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "codBarras") != produto.codBarras) {
    if (not modelProduto.setData(row, "codBarras", produto.codBarras)) { return false; }
    if (not modelProduto.setData(row, "codBarrasUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "codBarrasUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "ncm") != produto.ncm) {
    if (not modelProduto.setData(row, "ncm", produto.ncm)) { return false; }
    if (not modelProduto.setData(row, "ncmUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "ncmUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "ncmEx") != produto.ncmEx) {
    if (not modelProduto.setData(row, "ncmEx", produto.ncmEx)) { return false; }
    if (not modelProduto.setData(row, "ncmExUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "ncmExUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "qtdPallet") != produto.qtdPallet) {
    if (not modelProduto.setData(row, "qtdPallet", produto.qtdPallet)) { return false; }
    if (not modelProduto.setData(row, "qtdPalletUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "qtdPalletUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "custo") != produto.custo) {
    if (not modelProduto.setData(row, "custo", produto.custo)) { return false; }
    if (not modelProduto.setData(row, "custoUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "custoUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "precoVenda") != produto.precoVenda) {
    if (not modelProduto.setData(row, "precoVenda", produto.precoVenda)) { return false; }
    if (not modelProduto.setData(row, "precoVendaUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "precoVendaUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "ui") != produto.ui) {
    if (not modelProduto.setData(row, "ui", produto.ui)) { return false; }
    if (not modelProduto.setData(row, "uiUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "uiUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "un2") != produto.un2) {
    if (not modelProduto.setData(row, "un2", produto.un2)) { return false; }
    if (not modelProduto.setData(row, "un2Upd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "un2Upd", white)) { return false; }
  }

  if (modelProduto.data(row, "minimo") != produto.minimo) {
    if (not modelProduto.setData(row, "minimo", produto.minimo)) { return false; }
    if (not modelProduto.setData(row, "minimoUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "minimoUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "mva") != produto.mva) {
    if (not modelProduto.setData(row, "mva", produto.mva)) { return false; }
    if (not modelProduto.setData(row, "mvaUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "mvaUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "st") != produto.st) {
    if (not modelProduto.setData(row, "st", produto.st)) { return false; }
    if (not modelProduto.setData(row, "stUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "stUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "sticms") != produto.sticms) {
    if (not modelProduto.setData(row, "sticms", produto.sticms)) { return false; }
    if (not modelProduto.setData(row, "sticmsUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "sticmsUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "quantCaixa") != produto.quantCaixa) {
    if (not modelProduto.setData(row, "quantCaixa", produto.quantCaixa)) { return false; }
    if (not modelProduto.setData(row, "quantCaixaUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "quantCaixaUpd", white)) { return false; }
  }

  if (modelProduto.data(row, "markup") != produto.markup) {
    if (not modelProduto.setData(row, "markup", produto.markup)) { return false; }
    if (not modelProduto.setData(row, "markupUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "markupUpd", white)) { return false; }
  }

  const QDate dataSalva = modelProduto.data(row, "validade").toDate();

  if ((dataSalva.isValid() and dataSalva.toString("yyyy-MM-dd") != validadeString) or (not dataSalva.isValid() and not validadeString.isEmpty())) {
    if (not modelProduto.setData(row, "validade", (validade == -1) ? QVariant() : validadeString)) { return false; }
    if (not modelProduto.setData(row, "validadeUpd", yellow)) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "validadeUpd", white)) { return false; }
  }

  changed ? itensUpdated++ : itensNotChanged++;

  return true;
}

bool ImportaProdutos::marcaProdutoNaoDescontinuado(const int row) {
  if (not modelProduto.setData(row, "descontinuado", false)) { return false; }

  itensExpired--;

  return true;
}

bool ImportaProdutos::pintarCamposForaDoPadrao(const int row) {
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

  if (ncm == "0" or ncm.isEmpty() or (ncm.length() != 8 and ncm.length() != 10)) {
    if (not modelErro.setData(row, "ncmUpd", gray)) { return false; }
  }

  if (codBarras == "0" or codBarras.isEmpty()) {
    if (not modelErro.setData(row, "codBarrasUpd", gray)) { return false; }
  }

  // Errados

  if (fornecedor == "PRODUTO REPETIDO NA TABELA") {
    if (not modelErro.setData(row, "fornecedorUpd", red)) { return false; }
  }

  if ((un == "M2" or un == "ML") and m2cx <= 0.) {
    if (not modelErro.setData(row, "m2cxUpd", red)) { return false; }
  }

  if (un != "M2" and un != "ML" and pccx < 1) {
    if (not modelErro.setData(row, "pccxUpd", red)) { return false; }
  }

  if (codComercial == "0" or codComercial.isEmpty()) {
    if (not modelErro.setData(row, "codComercialUpd", red)) { return false; }
  }

  if (custo <= 0.) {
    if (not modelErro.setData(row, "custoUpd", red)) { return false; }
  }

  if (precoVenda <= 0.) {
    if (not modelErro.setData(row, "precoVendaUpd", red)) { return false; }
  }

  if (precoVenda < custo) {
    if (not modelErro.setData(row, "precoVendaUpd", red)) { return false; }
  }

  return true;
}

bool ImportaProdutos::camposForaDoPadrao() {
  if ((produto.un == "M2" or produto.un == "ML") and produto.m2cx <= 0.) { return true; }
  if (produto.un != "M2" and produto.un != "ML" and produto.pccx < 1) { return true; }
  if (produto.codComercial == "0" or produto.codComercial.isEmpty()) { return true; }
  if (produto.custo <= 0.) { return true; }
  if (produto.precoVenda <= 0.) { return true; }
  if (produto.precoVenda < produto.custo) { return true; }

  return false;
}

bool ImportaProdutos::insereEmErro() {
  const int row = modelErro.insertRowAtEnd();

  if (not modelErro.setData(row, "atualizarTabelaPreco", true)) { return false; }

  if (not modelErro.setData(row, "idFornecedor", produto.idFornecedor)) { return false; }
  if (not modelErro.setData(row, "fornecedor", produto.fornecedor)) { return false; }
  if (not modelErro.setData(row, "descricao", produto.descricao)) { return false; }
  if (not modelErro.setData(row, "un", produto.un)) { return false; }
  if (not modelErro.setData(row, "colecao", produto.colecao)) { return false; }
  if (not modelErro.setData(row, "m2cx", produto.m2cx)) { return false; }
  if (not modelErro.setData(row, "pccx", produto.pccx)) { return false; }
  if (not modelErro.setData(row, "kgcx", produto.kgcx)) { return false; }
  if (not modelErro.setData(row, "formComercial", produto.formComercial)) { return false; }
  if (not modelErro.setData(row, "codComercial", produto.codComercial)) { return false; }
  if (not modelErro.setData(row, "codBarras", produto.codBarras)) { return false; }
  if (not modelErro.setData(row, "ncm", produto.ncm)) { return false; }
  if (not modelErro.setData(row, "ncmEx", produto.ncmEx)) { return false; }
  if (not modelErro.setData(row, "qtdPallet", produto.qtdPallet)) { return false; }
  if (not modelErro.setData(row, "custo", produto.custo)) { return false; }
  if (not modelErro.setData(row, "precoVenda", produto.precoVenda)) { return false; }
  if (not modelErro.setData(row, "ui", produto.ui)) { return false; }
  if (not modelErro.setData(row, "un2", produto.un2)) { return false; }
  if (not modelErro.setData(row, "minimo", produto.minimo)) { return false; }
  if (not modelErro.setData(row, "mva", produto.mva)) { return false; }
  if (not modelErro.setData(row, "st", produto.st)) { return false; }
  if (not modelErro.setData(row, "sticms", produto.sticms)) { return false; }
  if (not modelErro.setData(row, "quantCaixa", produto.quantCaixa)) { return false; }
  if (not modelErro.setData(row, "markup", produto.markup)) { return false; }
  if (not modelErro.setData(row, "validade", (validade == -1) ? QVariant() : validadeString)) { return false; }

  // paint cells
  const int green = static_cast<int>(FieldColors::Green);

  if (not modelErro.setData(row, "fornecedorUpd", green)) { return false; }
  if (not modelErro.setData(row, "descricaoUpd", green)) { return false; }
  if (not modelErro.setData(row, "unUpd", green)) { return false; }
  if (not modelErro.setData(row, "colecaoUpd", green)) { return false; }
  if (not modelErro.setData(row, "m2cxUpd", green)) { return false; }
  if (not modelErro.setData(row, "pccxUpd", green)) { return false; }
  if (not modelErro.setData(row, "kgcxUpd", green)) { return false; }
  if (not modelErro.setData(row, "formComercialUpd", green)) { return false; }
  if (not modelErro.setData(row, "codComercialUpd", green)) { return false; }
  if (not modelErro.setData(row, "codBarrasUpd", green)) { return false; }
  if (not modelErro.setData(row, "ncmUpd", green)) { return false; }
  if (not modelErro.setData(row, "ncmExUpd", green)) { return false; }
  if (not modelErro.setData(row, "qtdPalletUpd", green)) { return false; }
  if (not modelErro.setData(row, "custoUpd", green)) { return false; }
  if (not modelErro.setData(row, "precoVendaUpd", green)) { return false; }
  if (not modelErro.setData(row, "uiUpd", green)) { return false; }
  if (not modelErro.setData(row, "un2Upd", green)) { return false; }
  if (not modelErro.setData(row, "minimoUpd", green)) { return false; }
  if (not modelErro.setData(row, "mvaUpd", green)) { return false; }
  if (not modelErro.setData(row, "stUpd", green)) { return false; }
  if (not modelErro.setData(row, "sticmsUpd", green)) { return false; }
  if (not modelErro.setData(row, "quantCaixaUpd", green)) { return false; }
  if (not modelErro.setData(row, "markupUpd", green)) { return false; }
  if (not modelErro.setData(row, "validadeUpd", green)) { return false; }

  // -------------------------------------------------

  if (not pintarCamposForaDoPadrao(row)) { return false; }

  itensError++;

  return true;
}

bool ImportaProdutos::insereEmOk() {
  const int row = modelProduto.insertRowAtEnd();

  if (not modelProduto.setData(row, "atualizarTabelaPreco", true)) { return false; }
  if (not modelProduto.setData(row, "promocao", static_cast<int>(tipo))) { return false; }

  if (not modelProduto.setData(row, "idFornecedor", produto.idFornecedor)) { return false; }
  if (not modelProduto.setData(row, "fornecedor", produto.fornecedor)) { return false; }
  if (not modelProduto.setData(row, "descricao", produto.descricao)) { return false; }
  if (not modelProduto.setData(row, "un", produto.un)) { return false; }
  if (not modelProduto.setData(row, "colecao", produto.colecao)) { return false; }
  if (not modelProduto.setData(row, "m2cx", produto.m2cx)) { return false; }
  if (not modelProduto.setData(row, "pccx", produto.pccx)) { return false; }
  if (not modelProduto.setData(row, "kgcx", produto.kgcx)) { return false; }
  if (not modelProduto.setData(row, "formComercial", produto.formComercial)) { return false; }
  if (not modelProduto.setData(row, "codComercial", produto.codComercial)) { return false; }
  if (not modelProduto.setData(row, "codBarras", produto.codBarras)) { return false; }
  if (not modelProduto.setData(row, "ncm", produto.ncm)) { return false; }
  if (not modelProduto.setData(row, "ncmEx", produto.ncmEx)) { return false; }
  if (not modelProduto.setData(row, "qtdPallet", produto.qtdPallet)) { return false; }
  if (not modelProduto.setData(row, "custo", produto.custo)) { return false; }
  if (not modelProduto.setData(row, "precoVenda", produto.precoVenda)) { return false; }
  if (not modelProduto.setData(row, "ui", produto.ui)) { return false; }
  if (not modelProduto.setData(row, "un2", produto.un2)) { return false; }
  if (not modelProduto.setData(row, "minimo", produto.minimo)) { return false; }
  if (not modelProduto.setData(row, "mva", produto.mva)) { return false; }
  if (not modelProduto.setData(row, "st", produto.st)) { return false; }
  if (not modelProduto.setData(row, "sticms", produto.sticms)) { return false; }
  if (not modelProduto.setData(row, "quantCaixa", produto.quantCaixa)) { return false; }
  if (not modelProduto.setData(row, "markup", produto.markup)) { return false; }
  if (not modelProduto.setData(row, "validade", (validade == -1) ? QVariant() : validadeString)) { return false; }

  // paint cells
  const int green = static_cast<int>(FieldColors::Green);

  if (not modelProduto.setData(row, "fornecedorUpd", green)) { return false; }
  if (not modelProduto.setData(row, "descricaoUpd", green)) { return false; }
  if (not modelProduto.setData(row, "unUpd", green)) { return false; }
  if (not modelProduto.setData(row, "colecaoUpd", green)) { return false; }
  if (not modelProduto.setData(row, "m2cxUpd", green)) { return false; }
  if (not modelProduto.setData(row, "pccxUpd", green)) { return false; }
  if (not modelProduto.setData(row, "kgcxUpd", green)) { return false; }
  if (not modelProduto.setData(row, "formComercialUpd", green)) { return false; }
  if (not modelProduto.setData(row, "codComercialUpd", green)) { return false; }
  if (not modelProduto.setData(row, "codBarrasUpd", green)) { return false; }
  if (not modelProduto.setData(row, "ncmUpd", green)) { return false; }
  if (not modelProduto.setData(row, "ncmExUpd", green)) { return false; }
  if (not modelProduto.setData(row, "qtdPalletUpd", green)) { return false; }
  if (not modelProduto.setData(row, "custoUpd", green)) { return false; }
  if (not modelProduto.setData(row, "precoVendaUpd", green)) { return false; }
  if (not modelProduto.setData(row, "uiUpd", green)) { return false; }
  if (not modelProduto.setData(row, "un2Upd", green)) { return false; }
  if (not modelProduto.setData(row, "minimoUpd", green)) { return false; }
  if (not modelProduto.setData(row, "mvaUpd", green)) { return false; }
  if (not modelProduto.setData(row, "stUpd", green)) { return false; }
  if (not modelProduto.setData(row, "sticmsUpd", green)) { return false; }
  if (not modelProduto.setData(row, "quantCaixaUpd", green)) { return false; }
  if (not modelProduto.setData(row, "markupUpd", green)) { return false; }
  if (not modelProduto.setData(row, "validadeUpd", green)) { return false; }

  if (tipo == Tipo::Promocao) {
    QSqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE idFornecedor = :idFornecedor AND codComercial = :codComercial AND promocao = FALSE AND estoque = FALSE");
    query.bindValue(":idFornecedor", produto.idFornecedor);
    query.bindValue(":codComercial", modelProduto.data(row, "codComercial"));

    if (not query.exec()) { return qApp->enqueueException(false, "Erro buscando produto relacionado: " + query.lastError().text(), this); }

    if (query.first() and not modelProduto.setData(row, "idProdutoRelacionado", query.value("idProduto"))) { return false; }
  }

  hashModel[produto.fornecedor + produto.codComercial + produto.ui + QString::number(static_cast<int>(tipo))] = row;

  vectorProdutosImportados << row;

  itensImported++;

  return true;
}

std::optional<int> ImportaProdutos::buscarCadastrarFornecedor() {
  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    qApp->enqueueException("Erro buscando fornecedor: " + queryFornecedor.lastError().text(), this);
    return {};
  }

  if (not queryFornecedor.first()) {
    queryFornecedor.prepare("INSERT INTO fornecedor (razaoSocial) VALUES (:razaoSocial)");
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) {
      qApp->enqueueException("Erro cadastrando fornecedor: " + queryFornecedor.lastError().text(), this);
      return {};
    }

    if (queryFornecedor.lastInsertId().isNull()) {
      qApp->enqueueException("Erro lastInsertId", this);
      return {};
    }

    return queryFornecedor.lastInsertId().toInt();
  }

  return queryFornecedor.value("idFornecedor").toInt();
}

bool ImportaProdutos::salvar() {
  if (not modelProduto.submitAll()) { return false; }

  QSqlQuery queryPrecos;

  if (validade != -1) {
    queryPrecos.prepare(
        "INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim FROM "
        "produto WHERE atualizarTabelaPreco = TRUE");
    queryPrecos.bindValue(":validadeInicio", qApp->serverDate().toString("yyyy-MM-dd"));
    queryPrecos.bindValue(":validadeFim", validadeString);

    if (not queryPrecos.exec()) { return qApp->enqueueException(false, "Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text(), this); }
  }

  if (not queryPrecos.exec("UPDATE produto SET atualizarTabelaPreco = FALSE")) { return qApp->enqueueException(false, "Erro comunicando com banco de dados: " + queryPrecos.lastError().text(), this); }

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
  if (xlsx.read(1, 1).toString() != "fornecedor") { return qApp->enqueueError(false, "Faltou a coluna 'fornecedor' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 2).toString() != "descricao") { return qApp->enqueueError(false, "Faltou a coluna 'descricao' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 3).toString() != "un") { return qApp->enqueueError(false, "Faltou a coluna 'un' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 4).toString() != "colecao") { return qApp->enqueueError(false, "Faltou a coluna 'colecao' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 5).toString() != "m2cx") { return qApp->enqueueError(false, "Faltou a coluna 'm2cx' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 6).toString() != "pccx") { return qApp->enqueueError(false, "Faltou a coluna 'pccx' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 7).toString() != "kgcx") { return qApp->enqueueError(false, "Faltou a coluna 'kgcx' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 8).toString() != "formComercial") { return qApp->enqueueError(false, "Faltou a coluna 'formComercial' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 9).toString() != "codComercial") { return qApp->enqueueError(false, "Faltou a coluna 'codComercial' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 10).toString() != "codBarras") { return qApp->enqueueError(false, "Faltou a coluna 'codBarras' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 11).toString() != "ncm") { return qApp->enqueueError(false, "Faltou a coluna 'ncm' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 12).toString() != "qtdPallet") { return qApp->enqueueError(false, "Faltou a coluna 'qtdPallet' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 13).toString() != "custo") { return qApp->enqueueError(false, "Faltou a coluna 'custo' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 14).toString() != "precoVenda") { return qApp->enqueueError(false, "Faltou a coluna 'precoVenda' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 15).toString() != "ui") { return qApp->enqueueError(false, "Faltou a coluna 'ui' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 16).toString() != "un2") { return qApp->enqueueError(false, "Faltou a coluna 'un2' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 17).toString() != "minimo") { return qApp->enqueueError(false, "Faltou a coluna 'minimo' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 18).toString() != "mva") { return qApp->enqueueError(false, "Faltou a coluna 'mva' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 19).toString() != "st") { return qApp->enqueueError(false, "Faltou a coluna 'st' no cabeçalho da tabela!"); }
  if (xlsx.read(1, 20).toString() != "sticms") { return qApp->enqueueError(false, "Faltou a coluna 'sticms' no cabeçalho da tabela!"); }

  if (xlsx.read(2, 1).toString().contains("=IF")) { return qApp->enqueueError(false, "Células estão com fórmula! Trocar por valores no Excel!", this); }

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
    return qApp->enqueueException("Erro guardando 'Representacao' em Fornecedor: " + query.lastError().text(), this);
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
