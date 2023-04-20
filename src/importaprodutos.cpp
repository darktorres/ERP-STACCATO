#include "importaprodutos.h"
#include "ui_importaprodutos.h"

#include "application.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "importaprodutosproxymodel.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sqlquery.h"
#include "validadedialog.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

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

    importar();
  } catch (std::exception &) {
    close();
    throw;
  }
}

void ImportaProdutos::verificaSeRepresentacao() {
  SqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", m_fornecedor);

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

void ImportaProdutos::importar() {
  QXlsx::Document xlsx(file, this);

  xlsx.selectSheet("BASE");
  verificaTabela(xlsx);

  progressDialog.show();

  cadastraFornecedores(xlsx);
  verificaSeRepresentacao();
  marcaTodosProdutosDescontinuados();
  mostraApenasEstesFornecedores();

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

  setupTables();

  ui->tableProdutos->sortByColumn("descontinuado", Qt::AscendingOrder);

  showMaximized();

  const QString resultado = "Produtos importados: " + QString::number(itensImported) + "\nProdutos atualizados: " + QString::number(itensUpdated) +
                            "\nNão modificados: " + QString::number(itensNotChanged) + "\nDescontinuados: " + QString::number(itensExpired) + "\nCom erro: " + QString::number(itensError);

  QMessageBox::information(this, "Aviso!", resultado);
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
  ui->tableProdutos->hideColumn("oldPrecoVenda");
  ui->tableProdutos->hideColumn("comissao");
  ui->tableProdutos->hideColumn("observacoes");
  ui->tableProdutos->hideColumn("origem");
  ui->tableProdutos->hideColumn("representacao");
  ui->tableProdutos->hideColumn("icms");
  ui->tableProdutos->hideColumn("cst");
  ui->tableProdutos->hideColumn("ipi");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("compra");
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
  ui->tableProdutos->setItemDelegateForColumn("sticms", new PorcentagemDelegate(true, this));
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
  ui->tableErro->hideColumn("quantCaixa");
  ui->tableErro->hideColumn("idFornecedor");
  ui->tableErro->hideColumn("desativado");
  ui->tableErro->hideColumn("descontinuado");
  ui->tableErro->hideColumn("estoque");
  ui->tableErro->hideColumn("cfop");
  ui->tableErro->hideColumn("atualizarTabelaPreco");
  ui->tableErro->hideColumn("temLote");
  ui->tableErro->hideColumn("tipo");
  ui->tableErro->hideColumn("oldPrecoVenda");
  ui->tableErro->hideColumn("comissao");
  ui->tableErro->hideColumn("observacoes");
  ui->tableErro->hideColumn("origem");
  ui->tableErro->hideColumn("representacao");
  ui->tableErro->hideColumn("icms");
  ui->tableErro->hideColumn("cst");
  ui->tableErro->hideColumn("ipi");
  ui->tableErro->hideColumn("estoque");
  ui->tableErro->hideColumn("promocao");
  ui->tableErro->hideColumn("compra");
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
  ui->tableErro->setItemDelegateForColumn("st", new PorcentagemDelegate(true, this));
  ui->tableErro->setItemDelegateForColumn("sticms", new PorcentagemDelegate(true, this));
  ui->tableErro->setItemDelegateForColumn("mva", porcDelegate);
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
    m_fornecedor = fornecedor;

    const int idFornecedor = buscarCadastrarFornecedor();

    ids << QString::number(idFornecedor);

    m_fornecedores.insert(fornecedor, idFornecedor);

    SqlQuery queryFornecedor;
    queryFornecedor.prepare("UPDATE fornecedor SET validadeProdutos = :validade WHERE razaoSocial = :razaoSocial");
    queryFornecedor.bindValue(":validade", (validade == -1) ? QVariant() : qApp->serverDate().addDays(validade));
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) { throw RuntimeException("Erro salvando validade: " + queryFornecedor.lastError().text()); }
  }

  idsFornecedor = ids.join(",");

  if (m_fornecedores.isEmpty()) { throw RuntimeException("Erro ao cadastrar fornecedores!"); }
}

void ImportaProdutos::mostraApenasEstesFornecedores() {
  modelProduto.setFilter("idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)));

  modelProduto.select();
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
  produto.fornecedor = fornecedor.toString().toUpper().trimmed();
  produto.descricao = descricao.toString().remove("*").remove("()").replace('_', ' ').toUpper().trimmed();
  produto.un = un.toString().remove("*").toUpper().trimmed();
  produto.colecao = colecao.toString().remove("*").toUpper().trimmed();
  produto.m2cx = m2cx.toDouble();
  produto.pccx = pccx.toDouble();
  produto.kgcx = kgcx.toDouble();
  produto.formComercial = formComercial.toString().remove("*").toUpper().trimmed();
  produto.codComercial = codComercial.toString().remove("*").remove(".").remove(",").toUpper().trimmed();
  produto.codBarras = codBarras.toString().remove("*").remove(".").remove(",").toUpper().trimmed();
  produto.ncm = ncm.toString().remove("*").remove(".").remove(",").remove("-").remove(" ").toUpper().trimmed();
  produto.qtdPallet = qtdPallet.toDouble();
  produto.custo = custo.toDouble();
  produto.precoVenda = precoVenda.toDouble();
  produto.ui = ui2.toString().remove("*").toUpper().trimmed();
  produto.un2 = un2.toString().remove("*").toUpper().trimmed();
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
  modelProduto.setData(row, "atualizarTabelaPreco", true);

  const int yellow = static_cast<int>(FieldColors::Yellow);
  const int white = static_cast<int>(FieldColors::White);

  bool changed = false;

  // TODO: trocar esses códigos repetidos por um for() dos membros de 'produto'

  if (modelProduto.data(row, "fornecedor").toString() != produto.fornecedor) {
    modelProduto.setData(row, "fornecedor", produto.fornecedor);
    modelProduto.setData(row, "fornecedorUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "fornecedorUpd", white);
  }

  if (modelProduto.data(row, "descricao").toString() != produto.descricao) {
    modelProduto.setData(row, "descricao", produto.descricao);
    modelProduto.setData(row, "descricaoUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "descricaoUpd", white);
  }

  if (modelProduto.data(row, "un").toString() != produto.un) {
    modelProduto.setData(row, "un", produto.un);
    modelProduto.setData(row, "unUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "unUpd", white);
  }

  if (modelProduto.data(row, "colecao").toString() != produto.colecao) {
    modelProduto.setData(row, "colecao", produto.colecao);
    modelProduto.setData(row, "colecaoUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "colecaoUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "m2cx").toDouble(), produto.m2cx)) {
    modelProduto.setData(row, "m2cx", produto.m2cx);
    modelProduto.setData(row, "m2cxUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "m2cxUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "pccx").toDouble(), produto.pccx)) {
    modelProduto.setData(row, "pccx", produto.pccx);
    modelProduto.setData(row, "pccxUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "pccxUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "kgcx").toDouble(), produto.kgcx)) {
    modelProduto.setData(row, "kgcx", produto.kgcx);
    modelProduto.setData(row, "kgcxUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "kgcxUpd", white);
  }

  if (modelProduto.data(row, "formComercial").toString() != produto.formComercial) {
    modelProduto.setData(row, "formComercial", produto.formComercial);
    modelProduto.setData(row, "formComercialUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "formComercialUpd", white);
  }

  if (modelProduto.data(row, "codComercial").toString() != produto.codComercial) {
    modelProduto.setData(row, "codComercial", produto.codComercial);
    modelProduto.setData(row, "codComercialUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "codComercialUpd", white);
  }

  if (modelProduto.data(row, "codBarras").toString() != produto.codBarras) {
    modelProduto.setData(row, "codBarras", produto.codBarras);
    modelProduto.setData(row, "codBarrasUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "codBarrasUpd", white);
  }

  if (modelProduto.data(row, "ncm").toString() != produto.ncm) {
    modelProduto.setData(row, "ncm", produto.ncm);
    modelProduto.setData(row, "ncmUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "ncmUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "qtdPallet").toDouble(), produto.qtdPallet)) {
    modelProduto.setData(row, "qtdPallet", produto.qtdPallet);
    modelProduto.setData(row, "qtdPalletUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "qtdPalletUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "custo").toDouble(), produto.custo)) {
    modelProduto.setData(row, "custo", produto.custo);
    modelProduto.setData(row, "custoUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "custoUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "precoVenda").toDouble(), produto.precoVenda)) {
    modelProduto.setData(row, "precoVenda", produto.precoVenda);
    modelProduto.setData(row, "precoVendaUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "precoVendaUpd", white);
  }

  if (modelProduto.data(row, "ui").toString() != produto.ui) {
    modelProduto.setData(row, "ui", produto.ui);
    modelProduto.setData(row, "uiUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "uiUpd", white);
  }

  if (modelProduto.data(row, "un2").toString() != produto.un2) {
    modelProduto.setData(row, "un2", produto.un2);
    modelProduto.setData(row, "un2Upd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "un2Upd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "minimo").toDouble(), produto.minimo)) {
    modelProduto.setData(row, "minimo", produto.minimo);
    modelProduto.setData(row, "minimoUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "minimoUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "mva").toDouble(), produto.mva)) {
    modelProduto.setData(row, "mva", produto.mva);
    modelProduto.setData(row, "mvaUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "mvaUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "st").toDouble(), produto.st)) {
    modelProduto.setData(row, "st", produto.st);
    modelProduto.setData(row, "stUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "stUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "sticms").toDouble(), produto.sticms)) {
    modelProduto.setData(row, "sticms", produto.sticms);
    modelProduto.setData(row, "sticmsUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "sticmsUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "quantCaixa").toDouble(), produto.quantCaixa)) {
    modelProduto.setData(row, "quantCaixa", produto.quantCaixa);
    modelProduto.setData(row, "quantCaixaUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "quantCaixaUpd", white);
  }

  if (not qFuzzyCompare(modelProduto.data(row, "markup").toDouble(), produto.markup)) {
    modelProduto.setData(row, "markup", produto.markup);
    modelProduto.setData(row, "markupUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "markupUpd", white);
  }

  const QDate dataSalva = modelProduto.data(row, "validade").toDate();

  if ((dataSalva.isValid() and dataSalva.toString("yyyy-MM-dd") != validadeString) or (not dataSalva.isValid() and not validadeString.isEmpty())) {
    modelProduto.setData(row, "validade", (validade == -1) ? QVariant() : validadeString);
    modelProduto.setData(row, "validadeUpd", yellow);
    changed = true;
  } else {
    modelProduto.setData(row, "validadeUpd", white);
  }

  changed ? itensUpdated++ : itensNotChanged++;
}

void ImportaProdutos::marcaProdutoNaoDescontinuado(const int row) {
  modelProduto.setData(row, "descontinuado", 0);

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

  if (ncm == "0" or ncm.isEmpty()) { modelErro.setData(row, "ncmUpd", gray); }

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
  queryFornecedor.bindValue(":razaoSocial", m_fornecedor);

  if (not queryFornecedor.exec()) { throw RuntimeException("Erro buscando fornecedor: " + queryFornecedor.lastError().text()); }

  if (not queryFornecedor.first()) {
    queryFornecedor.prepare("INSERT INTO fornecedor (razaoSocial) VALUES (:razaoSocial)");
    queryFornecedor.bindValue(":razaoSocial", m_fornecedor);

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
  if (modelErro.rowCount() > 0) {
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
  if (qApp->getInTransaction()) { qApp->rollbackTransaction(); }

  QDialog::closeEvent(event);
}

void ImportaProdutos::on_checkBoxRepresentacao_toggled(const bool checked) {
  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) { modelProduto.setData(row, "representacao", checked); }

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
