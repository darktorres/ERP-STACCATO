#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "estoqueproxymodel.h"
#include "importarxml.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "ui_importarxml.h"

ImportarXML::ImportarXML(const QStringList &idsCompra, const QDate &dataReal, QWidget *parent) : QDialog(parent), dataReal(dataReal), idsCompra(idsCompra), ui(new Ui::ImportarXML) {
  ui->setupUi(this);

  setupTables();

  setConnections();

  setWindowFlags(Qt::Window);
}

ImportarXML::~ImportarXML() { delete ui; }

void ImportarXML::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonImportar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonImportar_clicked, connectionType);
  connect(ui->pushButtonProcurar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonProcurar_clicked, connectionType);
  connect(ui->checkBoxSemLote, &QCheckBox::toggled, this, &ImportarXML::on_checkBoxSemLote_toggled, connectionType);
  connect(ui->tableEstoque->model(), &QAbstractItemModel::dataChanged, this, &ImportarXML::updateTableData, connectionType);
}

void ImportarXML::unsetConnections() {
  disconnect(ui->pushButtonCancelar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonCancelar_clicked);
  disconnect(ui->pushButtonImportar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonImportar_clicked);
  disconnect(ui->pushButtonProcurar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonProcurar_clicked);
  disconnect(ui->checkBoxSemLote, &QCheckBox::toggled, this, &ImportarXML::on_checkBoxSemLote_toggled);
  disconnect(ui->tableEstoque->model(), &QAbstractItemModel::dataChanged, this, &ImportarXML::updateTableData);
}

void ImportarXML::updateTableData(const QModelIndex &topLeft) {
  unsetConnections();

  [&] {
    const QString header = modelEstoque.headerData(topLeft.column(), Qt::Horizontal).toString();
    const int row = topLeft.row();

    // TODO: se alterar quant. tem que alterar caixas

    if (header == "Quant." or header == "R$ Unid.") {
      const double preco = modelEstoque.data(row, "quant").toDouble() * modelEstoque.data(row, "valorUnid").toDouble();
      if (not modelEstoque.setData(row, "valor", preco)) { return; }
    }

    if (header == "R$") {
      const double prcUnitario = modelEstoque.data(row, "valor").toDouble() / modelEstoque.data(row, "quant").toDouble();
      if (not modelEstoque.setData(row, "valorUnid", prcUnitario)) { return; }
    }
  }();

  reparear(topLeft);

  setConnections();
}

void ImportarXML::setupTables() {
  modelEstoque.setTable("estoque");

  modelEstoque.setHeaderData("status", "Status");
  modelEstoque.setHeaderData("fornecedor", "Fornecedor");
  modelEstoque.setHeaderData("descricao", "Produto");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("restante", "Restante");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Cx.");
  modelEstoque.setHeaderData("codBarras", "Cód. Bar.");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("valorUnid", "R$ Unid.");
  modelEstoque.setHeaderData("valor", "R$");
  modelEstoque.setHeaderData("vICMSST", "ST");

  modelEstoque.proxyModel = new EstoqueProxyModel(&modelEstoque, this);

  ui->tableEstoque->setModel(&modelEstoque);

  ui->tableEstoque->setItemDelegate(new NoEditDelegate(this));

  ui->tableEstoque->setItemDelegateForColumn("codComercial", new EditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("lote", new EditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("bloco", new EditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("quant", new DoubleDelegate(this, 3));
  ui->tableEstoque->setItemDelegateForColumn("un", new EditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("descricao", new EditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("valorUnid", new ReaisDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("vICMSST", new ReaisDelegate(this));

  ui->tableEstoque->hideColumn("idEstoque");
  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("recebidoPor");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("observacao");
  ui->tableEstoque->hideColumn("quantUpd");
  ui->tableEstoque->hideColumn("ajuste");
  ui->tableEstoque->hideColumn("ncm");
  ui->tableEstoque->hideColumn("cfop");
  ui->tableEstoque->hideColumn("codBarrasTrib");
  ui->tableEstoque->hideColumn("unTrib");
  ui->tableEstoque->hideColumn("quantTrib");
  ui->tableEstoque->hideColumn("valorTrib");
  ui->tableEstoque->hideColumn("desconto");
  ui->tableEstoque->hideColumn("compoeTotal");
  ui->tableEstoque->hideColumn("numeroPedido");
  ui->tableEstoque->hideColumn("itemPedido");
  ui->tableEstoque->hideColumn("tipoICMS");
  ui->tableEstoque->hideColumn("orig");
  ui->tableEstoque->hideColumn("cstICMS");
  ui->tableEstoque->hideColumn("modBC");
  ui->tableEstoque->hideColumn("vBC");
  ui->tableEstoque->hideColumn("pICMS");
  ui->tableEstoque->hideColumn("vICMS");
  ui->tableEstoque->hideColumn("modBCST");
  ui->tableEstoque->hideColumn("pMVAST");
  ui->tableEstoque->hideColumn("vBCST");
  ui->tableEstoque->hideColumn("pICMSST");
  ui->tableEstoque->hideColumn("cEnq");
  ui->tableEstoque->hideColumn("cstIPI");
  ui->tableEstoque->hideColumn("vBCIPI");
  ui->tableEstoque->hideColumn("pIPI");
  ui->tableEstoque->hideColumn("vIPI");
  ui->tableEstoque->hideColumn("cstPIS");
  ui->tableEstoque->hideColumn("vBCPIS");
  ui->tableEstoque->hideColumn("pPIS");
  ui->tableEstoque->hideColumn("vPIS");
  ui->tableEstoque->hideColumn("cstCOFINS");
  ui->tableEstoque->hideColumn("vBCCOFINS");
  ui->tableEstoque->hideColumn("pCOFINS");
  ui->tableEstoque->hideColumn("vCOFINS");
  ui->tableEstoque->hideColumn("valorGare");

  // -------------------------------------------------------------------------

  modelConsumo.setTable("estoque_has_consumo");

  modelConsumo.setHeaderData("status", "Status");
  modelConsumo.setHeaderData("local", "Local");
  modelConsumo.setHeaderData("fornecedor", "Fornecedor");
  modelConsumo.setHeaderData("descricao", "Produto");
  modelConsumo.setHeaderData("quant", "Quant.");
  modelConsumo.setHeaderData("un", "Un.");
  modelConsumo.setHeaderData("caixas", "Cx.");
  modelConsumo.setHeaderData("codBarras", "Cód. Bar.");
  modelConsumo.setHeaderData("codComercial", "Cód. Com.");

  modelConsumo.proxyModel = new EstoqueProxyModel(&modelConsumo, this);

  ui->tableConsumo->setModel(&modelConsumo);

  ui->tableConsumo->setItemDelegate(new NoEditDelegate(this));

  ui->tableConsumo->setItemDelegateForColumn("valorUnid", new ReaisDelegate(this));
  ui->tableConsumo->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  ui->tableConsumo->hideColumn("idVendaProduto1");
  ui->tableConsumo->hideColumn("idVendaProduto2");
  ui->tableConsumo->hideColumn("idPedido1");
  ui->tableConsumo->hideColumn("idPedido2");
  ui->tableConsumo->hideColumn("idConsumo");
  ui->tableConsumo->hideColumn("quantUpd");
  ui->tableConsumo->hideColumn("idEstoque");
  ui->tableConsumo->hideColumn("idProduto");
  ui->tableConsumo->hideColumn("ncm");
  ui->tableConsumo->hideColumn("cfop");
  ui->tableConsumo->hideColumn("valorUnid");
  ui->tableConsumo->hideColumn("valor");
  ui->tableConsumo->hideColumn("codBarrasTrib");
  ui->tableConsumo->hideColumn("unTrib");
  ui->tableConsumo->hideColumn("quantTrib");
  ui->tableConsumo->hideColumn("valorTrib");
  ui->tableConsumo->hideColumn("desconto");
  ui->tableConsumo->hideColumn("compoeTotal");
  ui->tableConsumo->hideColumn("numeroPedido");
  ui->tableConsumo->hideColumn("itemPedido");
  ui->tableConsumo->hideColumn("tipoICMS");
  ui->tableConsumo->hideColumn("orig");
  ui->tableConsumo->hideColumn("cstICMS");
  ui->tableConsumo->hideColumn("modBC");
  ui->tableConsumo->hideColumn("vBC");
  ui->tableConsumo->hideColumn("pICMS");
  ui->tableConsumo->hideColumn("vICMS");
  ui->tableConsumo->hideColumn("modBCST");
  ui->tableConsumo->hideColumn("pMVAST");
  ui->tableConsumo->hideColumn("vBCST");
  ui->tableConsumo->hideColumn("pICMSST");
  ui->tableConsumo->hideColumn("vICMSST");
  ui->tableConsumo->hideColumn("cEnq");
  ui->tableConsumo->hideColumn("cstIPI");
  ui->tableConsumo->hideColumn("cstPIS");
  ui->tableConsumo->hideColumn("vBCPIS");
  ui->tableConsumo->hideColumn("pPIS");
  ui->tableConsumo->hideColumn("vPIS");
  ui->tableConsumo->hideColumn("cstCOFINS");
  ui->tableConsumo->hideColumn("vBCCOFINS");
  ui->tableConsumo->hideColumn("pCOFINS");
  ui->tableConsumo->hideColumn("vCOFINS");

  // -------------------------------------------------------------------------

  modelCompra.setTable("pedido_fornecedor_has_produto2");

  modelCompra.setHeaderData("status", "Status");
  modelCompra.setHeaderData("ordemCompra", "OC");
  modelCompra.setHeaderData("idVenda", "Venda");
  modelCompra.setHeaderData("fornecedor", "Fornecedor");
  modelCompra.setHeaderData("descricao", "Produto");
  modelCompra.setHeaderData("colecao", "Coleção");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("un2", "Un. 2");
  modelCompra.setHeaderData("caixas", "Cx.");
  modelCompra.setHeaderData("prcUnitario", "R$ Unid.");
  modelCompra.setHeaderData("preco", "R$");
  modelCompra.setHeaderData("kgcx", "Kg./Cx.");
  modelCompra.setHeaderData("formComercial", "Form. Com.");
  modelCompra.setHeaderData("codComercial", "Cód. Com.");
  modelCompra.setHeaderData("codBarras", "Cód. Bar.");
  modelCompra.setHeaderData("obs", "Obs.");

  modelCompra.setFilter("idCompra = " + idsCompra.join(" OR idCompra = ") + " AND status NOT IN ('CANCELADO')");

  if (not modelCompra.select()) { return; }

  modelCompra.proxyModel = new EstoqueProxyModel(&modelCompra, this);

  ui->tableCompra->setModel(&modelCompra);

  ui->tableCompra->setItemDelegate(new NoEditDelegate(this));

  ui->tableCompra->setItemDelegateForColumn("codComercial", new EditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("descricao", new EditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  ui->tableCompra->hideColumn("idPedido2");
  ui->tableCompra->hideColumn("idPedidoFK");
  ui->tableCompra->hideColumn("idVendaProduto1");
  ui->tableCompra->hideColumn("idVendaProduto2");
  ui->tableCompra->hideColumn("idRelacionado");
  ui->tableCompra->hideColumn("ordemRepresentacao");
  ui->tableCompra->hideColumn("selecionado");
  ui->tableCompra->hideColumn("statusFinanceiro");
  ui->tableCompra->hideColumn("quantUpd");
  ui->tableCompra->hideColumn("idCompra");
  ui->tableCompra->hideColumn("idProduto");
  ui->tableCompra->hideColumn("dataPrevCompra");
  ui->tableCompra->hideColumn("dataRealCompra");
  ui->tableCompra->hideColumn("dataPrevConf");
  ui->tableCompra->hideColumn("dataRealConf");
  ui->tableCompra->hideColumn("dataPrevFat");
  ui->tableCompra->hideColumn("dataRealFat");
  ui->tableCompra->hideColumn("dataPrevColeta");
  ui->tableCompra->hideColumn("dataRealColeta");
  ui->tableCompra->hideColumn("dataPrevReceb");
  ui->tableCompra->hideColumn("dataRealReceb");
  ui->tableCompra->hideColumn("dataPrevEnt");
  ui->tableCompra->hideColumn("dataRealEnt");
  ui->tableCompra->hideColumn("aliquotaSt");
  ui->tableCompra->hideColumn("st");

  // -------------------------------------------------------------------------

  modelNFe.setTable("nfe");

  // -------------------------------------------------------------------------

  modelEstoque_compra.setTable("estoque_has_compra");

  // -------------------------------------------------------------------------

  QStringList idVendas;

  for (int row = 0; row < modelCompra.rowCount(); ++row) { idVendas << modelCompra.data(row, "idVendaProduto2").toString(); }

  modelVenda.setTable("venda_has_produto2");

  modelVenda.setFilter("idVendaProduto2 IN (" + idVendas.join(", ") + ") AND status = 'EM FATURAMENTO'");

  if (not modelVenda.select()) { return; }
}

bool ImportarXML::cadastrarProdutoEstoque(const QVector<ProdutoEstoque> &tuples) {
  QSqlQuery query;
  query.prepare(
      "INSERT INTO produto SELECT NULL, p.idProdutoUpd, :idEstoque, p.idFornecedor, p.idFornecedorUpd, p.fornecedor, p.fornecedorUpd, CONCAT(p.descricao, ' (ESTOQUE)'), p.descricaoUpd, "
      ":estoqueRestante, p.estoqueRestanteUpd, p.un, p.unUpd, p.un2, p.un2Upd, p.colecao, p.colecaoUpd, p.tipo, p.tipoUpd, p.minimo, p.minimoUpd, p.multiplo, p.multiploUpd, p.m2cx, p.m2cxUpd, "
      "p.pccx, p.pccxUpd, p.kgcx, p.kgcxUpd, p.formComercial, p.formComercialUpd, p.codComercial, p.codComercialUpd, p.codBarras, p.codBarrasUpd, p.ncm, p.ncmUpd, p.ncmEx, p.ncmExUpd, p.cfop, "
      "p.cfopUpd, p.icms, p.icmsUpd, p.cst, p.cstUpd, p.qtdPallet, p.qtdPalletUpd, p.custo, p.custoUpd, p.ipi, p.ipiUpd, p.st, p.stUpd, p.sticms, p.sticmsUpd, p.mva, p.mvaUpd, p.precoVenda, "
      "p.precoVendaUpd, p.markup, p.markupUpd, p.comissao, p.comissaoUpd, p.observacoes, p.observacoesUpd, p.origem, p.origemUpd, p.temLote, p.temLoteUpd, p.ui, p.uiUpd, NULL, p.validadeUpd, "
      ":descontinuado, p.descontinuadoUpd, p.atualizarTabelaPreco, p.representacao, 1, 0, p.idProduto, 0, NULL, NULL FROM produto p WHERE p.idProduto = :idProduto");

  for (const auto &tuple : tuples) {
    const auto [idProduto, idEstoque, estoqueRestante] = tuple;

    query.bindValue(":idProduto", idProduto);
    query.bindValue(":idEstoque", idEstoque);
    query.bindValue(":estoqueRestante", estoqueRestante);
    query.bindValue(":descontinuado", qFuzzyIsNull(estoqueRestante) ? true : false);

    if (not query.exec()) { return qApp->enqueueError(false, "Erro criando produto_estoque: " + query.lastError().text(), this); }
  }

  return true;
}

QVector<ImportarXML::ProdutoEstoque> ImportarXML::mapTuples() {
  QVector<ProdutoEstoque> produtos;

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    const int idProduto = modelEstoque.data(row, "idProduto").toInt();
    const int idEstoque = modelEstoque.data(row, "idEstoque").toInt();
    const double estoqueRestante = modelEstoque.data(row, "restante").toDouble();

    produtos << ProdutoEstoque{idProduto, idEstoque, estoqueRestante};
  }

  return produtos;
}

bool ImportarXML::salvarDadosVenda() {
  for (int row = 0; row < modelConsumo.rowCount(); ++row) {
    const int idEstoque = modelConsumo.data(row, "idEstoque").toInt();

    const auto list = modelEstoque.multiMatch({{"idEstoque", idEstoque}}, false);

    if (list.isEmpty()) { return qApp->enqueueError(false, "Erro buscando lote!", this); }

    const QString lote = modelEstoque.data(list.first(), "lote").toString();

    const int idVendaProduto2 = modelConsumo.data(row, "idVendaProduto2").toInt();

    const auto list2 = modelVenda.multiMatch({{"idVendaProduto2", idVendaProduto2}}, false);

    if (list2.isEmpty()) { return qApp->enqueueError(false, "Erro buscando lote!", this); }

    if (not modelVenda.setData(list2.first(), "lote", lote)) { return false; }
  }

  if (not modelVenda.submitAll()) { return false; }

  QStringList idVendas;

  for (int row = 0; row < modelVenda.rowCount(); ++row) { idVendas << modelVenda.data(row, "idVenda").toString(); }

  if (not Sql::updateVendaStatus(idVendas)) { return false; }

  return true;
}

bool ImportarXML::importar() {
  if (not salvarDadosVenda()) { return false; }

  if (not salvarDadosCompra()) { return false; }

  if (not modelNFe.submitAll()) { return false; }

  // mapear idProduto/idEstoque para cadastrarProdutoEstoque
  const auto tuples = mapTuples();

  if (not modelEstoque.submitAll()) { return false; }

  if (not cadastrarProdutoEstoque(tuples)) { return false; }

  if (not modelConsumo.submitAll()) { return false; }

  if (not modelEstoque_compra.submitAll()) { return false; }

  return true;
}

bool ImportarXML::salvarDadosCompra() {
  const auto list = modelCompra.multiMatch({{"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green)}});

  if (list.isEmpty()) { return qApp->enqueueError(false, "Erro buscando produtos da compra!", this); }

  for (const auto &row : list) {
    if (not modelCompra.setData(row, "status", "EM COLETA")) { return false; }
    if (not modelCompra.setData(row, "dataRealFat", dataReal)) { return false; }
  }

  if (not modelCompra.submitAll()) { return false; }

  // TODO: ainda precisa disso considerando que agora tem o trigger no BD?
  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    QSqlQuery query;

    if (not query.exec("CALL update_pedido_fornecedor_status(" + modelCompra.data(row, "idPedidoFK").toString() + ")")) {
      return qApp->enqueueError(false, "Erro atualizando status compra: " + query.lastError().text(), this);
    }
  }

  return true;
}

bool ImportarXML::verifyFields() {
  const auto list = modelCompra.multiMatch({{"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green)}}, false);

  if (list.isEmpty()) { return qApp->enqueueError(false, "Nenhuma compra pareada!", this); }

  //----------------------------------------------------------------

  const auto list2 = modelEstoque.multiMatch({{"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

  if (not list2.isEmpty()) { return qApp->enqueueError(false, "Nem todos os estoques estão ok!", this); }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (modelEstoque.data(row, "lote").toString().isEmpty()) { return qApp->enqueueError(false, "Lote vazio! Se não há coloque 'N/D'!", this); }
  }

  return true;
}

void ImportarXML::on_pushButtonImportar_clicked() {
  if (not verifyFields()) { return; }

  unsetConnections();

  const auto ok = [&] {
    if (not qApp->startTransaction()) { return false; }

    if (not importar()) {
      qApp->rollbackTransaction();
      close(); // TODO: is this still needed?
      return false;
    }

    if (not qApp->endTransaction()) { return false; }

    return true;
  }();

  setConnections();

  if (not ok) { return; }

  QDialog::accept();
  close();
}

bool ImportarXML::limparAssociacoes() {
  const auto list = modelCompra.multiMatch({{"status", "EM FATURAMENTO"}});

  for (const auto row : list) {
    if (not modelCompra.setData(row, "quantUpd", static_cast<int>(FieldColors::None))) { return false; }
  }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (not modelEstoque.setData(row, "quantUpd", static_cast<int>(FieldColors::Red))) { return false; }
    if (not modelEstoque.setData(row, "restante", modelEstoque.data(row, "quant"))) { return false; }
  }

  for (int row = 0; row < modelVenda.rowCount(); ++row) {
    if (not modelVenda.setData(row, "status", "EM FATURAMENTO")) { return false; }
    if (not modelVenda.setData(row, "dataRealFat", QVariant())) { return false; }
  }

  modelConsumo.revertAll();
  modelEstoque_compra.revertAll();

  return true;
}

void ImportarXML::on_pushButtonProcurar_clicked() {
  unsetConnections();

  [&] {
    if (not lerXML()) { return; }

    if (not parear()) { return; }

    const auto list = modelCompra.multiMatch({{"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

    if (list.isEmpty()) { ui->pushButtonProcurar->setDisabled(true); }
  }();

  setConnections();
}

std::optional<double> ImportarXML::buscarCaixas(const int rowEstoque) {
  QSqlQuery query;
  query.prepare("SELECT m2cx, pccx FROM produto WHERE codComercial = :codComercial");
  query.bindValue(":codComercial", modelEstoque.data(rowEstoque, "codComercial"));

  if (not query.exec()) {
    qApp->enqueueError("Erro lendo tabela produto: " + query.lastError().text(), this);
    return {};
  }

  if (not query.first()) { return {}; }

  const QString un = modelEstoque.data(rowEstoque, "un").toString();

  const double quantCaixa = un == "M2" or un == "M²" or un == "ML" ? query.value("m2cx").toDouble() : query.value("pccx").toDouble();

  const double quant = modelEstoque.data(rowEstoque, "quant").toDouble();

  const double caixas = qRound(quant / quantCaixa * 100) / 100.;

  return caixas;
}

bool ImportarXML::associarIgual(const int rowCompra, const int rowEstoque) {
  if (not modelCompra.setData(rowCompra, "descricao", modelEstoque.data(rowEstoque, "descricao").toString())) { return false; }
  if (not modelCompra.setData(rowCompra, "quantUpd", static_cast<int>(FieldColors::Green))) { return false; }

  const auto caixas = buscarCaixas(rowEstoque);

  if (caixas and not modelEstoque.setData(rowEstoque, "caixas", caixas.value())) { return false; }
  if (not modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(FieldColors::Green))) { return false; }
  if (not modelEstoque.setData(rowEstoque, "idProduto", modelCompra.data(rowCompra, "idProduto"))) { return false; }

  const int rowEstoque_compra = modelEstoque_compra.insertRowAtEnd();

  if (not modelEstoque_compra.setData(rowEstoque_compra, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"))) { return false; }
  if (not modelEstoque_compra.setData(rowEstoque_compra, "idCompra", modelCompra.data(rowCompra, "idCompra"))) { return false; }
  if (not modelEstoque_compra.setData(rowEstoque_compra, "idPedido2", modelCompra.data(rowCompra, "idPedido2"))) { return false; }

  return criarConsumo(rowCompra, rowEstoque);
}

bool ImportarXML::associarDiferente(const int rowCompra, const int rowEstoque, double &estoquePareado, bool &repareado) {
  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double estoqueDisponivel = quantEstoque - estoquePareado;

  const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();
  const double quantAdicionar = qMin(estoqueDisponivel, quantCompra);
  estoquePareado += quantAdicionar;

  const bool compraCompleta = qFuzzyCompare(quantAdicionar, quantCompra);

  // refazer pareamento do começo porque a linha nova não vai estar no match()
  if (not compraCompleta) {
    if (not dividirCompra(rowCompra, quantAdicionar)) { return false; }

    repareado = true;

    return parear();
  }

  if (not modelCompra.setData(rowCompra, "descricao", modelEstoque.data(rowEstoque, "descricao").toString())) { return false; }
  if (not modelCompra.setData(rowCompra, "quantUpd", static_cast<int>(FieldColors::Green))) { return false; }

  const auto caixas = buscarCaixas(rowEstoque);

  if (caixas and not modelEstoque.setData(rowEstoque, "caixas", caixas.value())) { return false; }
  if (not modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(qFuzzyCompare(estoquePareado, quantEstoque) ? FieldColors::Green : FieldColors::Yellow))) { return false; }
  if (not modelEstoque.setData(rowEstoque, "idProduto", modelCompra.data(rowCompra, "idProduto"))) { return false; }

  const int rowEstoque_compra = modelEstoque_compra.insertRowAtEnd();

  if (not modelEstoque_compra.setData(rowEstoque_compra, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"))) { return false; }
  if (not modelEstoque_compra.setData(rowEstoque_compra, "idCompra", modelCompra.data(rowCompra, "idCompra"))) { return false; }
  if (not modelEstoque_compra.setData(rowEstoque_compra, "idPedido2", modelCompra.data(rowCompra, "idPedido2"))) { return false; }

  return criarConsumo(rowCompra, rowEstoque);
}

bool ImportarXML::verificaCNPJ(const QString &cnpj) {
  QSqlQuery queryLoja;

  // TODO: 5make this not hardcoded but still it shouldnt need the user to set a UserSession flag
  if (not queryLoja.exec("SELECT cnpj FROM loja WHERE descricao = 'CD'") or not queryLoja.first()) { return qApp->enqueueError(false, "Erro na query CNPJ: " + queryLoja.lastError().text(), this); }

  if (queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-") != cnpj) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "CNPJ da nota não é do galpão. Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    return (msgBox.exec() == QMessageBox::Yes);
  }

  return true;
}

bool ImportarXML::verificaExiste(const QString &chaveAcesso) {
  QSqlQuery query;
  query.prepare("SELECT idNFe FROM nfe WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", chaveAcesso);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se nota já cadastrada: " + query.lastError().text(), this); }

  const auto list = modelNFe.multiMatch({{"chaveAcesso", chaveAcesso}}, false);

  if (query.first() or not list.isEmpty()) { return qApp->enqueueError(true, "Nota já cadastrada!", this); }

  return false;
}

bool ImportarXML::verificaValido(const XML &xml) {
  if (not xml.fileContent.contains("Autorizado o uso da NF-e")) { return qApp->enqueueError(false, "NFe não está autorizada pela SEFAZ!", this); }

  return true;
}

bool ImportarXML::cadastrarNFe(XML &xml) {
  if (not verificaCNPJ(xml.cnpjDest) or verificaExiste(xml.chaveAcesso) or not verificaValido(xml)) { return false; }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'nfe'") or not query.first()) { return false; }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE nfe auto_increment = " + QString::number(id + 1))) { return false; }

  xml.idNFe = id;

  const int row = modelNFe.insertRowAtEnd();

  if (not modelNFe.setData(row, "idNFe", xml.idNFe)) { return false; }
  if (not modelNFe.setData(row, "tipo", "ENTRADA")) { return false; }
  if (not modelNFe.setData(row, "cnpjDest", xml.cnpjDest)) { return false; }
  if (not modelNFe.setData(row, "chaveAcesso", xml.chaveAcesso)) { return false; }
  if (not modelNFe.setData(row, "numeroNFe", xml.nNF)) { return false; }
  if (not modelNFe.setData(row, "xml", xml.fileContent)) { return false; }
  if (not modelNFe.setData(row, "transportadora", xml.xNomeTransp)) { return false; }
  if (not modelNFe.setData(row, "valor", xml.vNF_Total)) { return false; }

  return true;
}

bool ImportarXML::lerXML() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Arquivo XML", QDir::currentPath(), "XML (*.xml)");

  if (filePath.isEmpty()) { return false; }

  ui->lineEdit->setText(filePath);

  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueError(false, "Erro lendo arquivo: " + file.errorString(), this); }

  const auto local = perguntarLocal();

  if (not local) { return false; }

  XML xml(file.readAll(), file.fileName());
  xml.local = local.value();

  file.close();

  if (not cadastrarNFe(xml)) { return false; }

  if (not percorrerXml(xml, xml.model.item(0, 0))) { return false; }

  return true;
}

std::optional<QString> ImportarXML::perguntarLocal() {
  QSqlQuery query;

  if (not query.exec("SELECT descricao FROM loja WHERE descricao NOT IN ('', 'CD') ORDER BY descricao")) {
    qApp->enqueueError("Erro buscando lojas: " + query.lastError().text(), this);
    return {};
  }

  QStringList lojas{"CD"};

  while (query.next()) { lojas << query.value("descricao").toString(); }

  QInputDialog input;
  input.setInputMode(QInputDialog::TextInput);
  input.setCancelButtonText("Cancelar");
  input.setWindowTitle("Local");
  input.setLabelText("Local do depósito:");
  input.setComboBoxItems(lojas);
  input.setComboBoxEditable(false);

  if (input.exec() != QInputDialog::Accepted) { return {}; }

  const QString local = input.textValue();

  return local;
}

bool ImportarXML::inserirItemModel(const XML &xml) {
  const auto idEstoque = reservarIdEstoque();

  if (not idEstoque) { return false; }

  const int newRow = modelEstoque.insertRowAtEnd();

  if (not modelEstoque.setData(newRow, "idEstoque", idEstoque.value())) { return false; }
  if (not modelEstoque.setData(newRow, "idNFe", xml.idNFe)) { return false; }
  if (not modelEstoque.setData(newRow, "fornecedor", xml.xNome)) { return false; }
  if (not modelEstoque.setData(newRow, "local", xml.local)) { return false; }
  if (not modelEstoque.setData(newRow, "descricao", xml.descricao)) { return false; }
  if (not modelEstoque.setData(newRow, "quant", xml.quant)) { return false; }
  if (not modelEstoque.setData(newRow, "restante", xml.quant)) { return false; }
  if (not modelEstoque.setData(newRow, "un", xml.un)) { return false; }
  if (not modelEstoque.setData(newRow, "codBarras", xml.codBarras)) { return false; }
  if (not modelEstoque.setData(newRow, "codComercial", xml.codProd)) { return false; }
  if (not modelEstoque.setData(newRow, "ncm", xml.ncm)) { return false; }
  if (not modelEstoque.setData(newRow, "cfop", xml.cfop)) { return false; }
  if (not modelEstoque.setData(newRow, "valorUnid", (xml.valor + xml.vIPI) / xml.quant)) { return false; }
  if (not modelEstoque.setData(newRow, "valor", xml.valor + xml.vIPI)) { return false; }
  if (not modelEstoque.setData(newRow, "codBarrasTrib", xml.codBarrasTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "unTrib", xml.unTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "quantTrib", xml.quantTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "valorTrib", xml.valorTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "desconto", xml.desconto)) { return false; }
  if (not modelEstoque.setData(newRow, "compoeTotal", xml.compoeTotal)) { return false; }
  if (not modelEstoque.setData(newRow, "numeroPedido", xml.numeroPedido)) { return false; }
  if (not modelEstoque.setData(newRow, "itemPedido", xml.itemPedido)) { return false; }
  if (not modelEstoque.setData(newRow, "tipoICMS", xml.tipoICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "orig", xml.orig)) { return false; }
  if (not modelEstoque.setData(newRow, "cstICMS", xml.cstICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "modBC", xml.modBC)) { return false; }
  if (not modelEstoque.setData(newRow, "vBC", xml.vBC)) { return false; }
  if (not modelEstoque.setData(newRow, "pICMS", xml.pICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "vICMS", xml.vICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "modBCST", xml.modBCST)) { return false; }
  if (not modelEstoque.setData(newRow, "pMVAST", xml.pMVAST)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCST", xml.vBCST)) { return false; }
  if (not modelEstoque.setData(newRow, "pICMSST", xml.pICMSST)) { return false; }
  if (not modelEstoque.setData(newRow, "vICMSST", xml.vICMSST)) { return false; }
  if (not modelEstoque.setData(newRow, "cEnq", xml.cEnq)) { return false; }
  if (not modelEstoque.setData(newRow, "cstIPI", xml.cstIPI)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCIPI", xml.vBCIPI)) { return false; }
  if (not modelEstoque.setData(newRow, "pIPI", xml.pIPI)) { return false; }
  if (not modelEstoque.setData(newRow, "vIPI", xml.vIPI)) { return false; }
  if (not modelEstoque.setData(newRow, "cstPIS", xml.cstPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCPIS", xml.vBCPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "pPIS", xml.pPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "vPIS", xml.vPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "cstCOFINS", xml.cstCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCCOFINS", xml.vBCCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "pCOFINS", xml.pCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "vCOFINS", xml.vCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "status", "EM COLETA")) { return false; }

  return true;
}

bool ImportarXML::percorrerXml(XML &xml, const QStandardItem *item) {
  for (int row = 0; row < item->rowCount(); ++row) {
    for (int col = 0; col < item->columnCount(); ++col) {
      const QStandardItem *child = item->child(row, col);
      const QString text = child->text();

      if (text.mid(0, 10) == "det nItem=") {
        xml.limparValores();
        xml.lerValores(child);
        if (not inserirItemModel(xml)) { return false; }
      }

      if (child->hasChildren()) { percorrerXml(xml, child); }
    }
  }

  return true;
}

bool ImportarXML::criarConsumo(const int rowCompra, const int rowEstoque) {
  const int idEstoque = modelEstoque.data(rowEstoque, "idEstoque").toInt();
  const int idVendaProduto2 = modelCompra.data(rowCompra, "idVendaProduto2").toInt();

  if (idVendaProduto2 == 0) { return true; }

  // -------------------------------------

  const auto list = modelVenda.multiMatch({{"idVendaProduto2", idVendaProduto2}}, false);

  if (list.isEmpty()) { return qApp->enqueueError(false, "Erro procurando produto da venda!", this); }

  const int rowVenda = list.first();

  const double quantVenda = qRound(modelVenda.data(rowVenda, "quant").toDouble() * 100) / 100.;
  const double restanteEstoque = qRound(modelEstoque.data(rowEstoque, "restante").toDouble() * 100) / 100.;
  const double quantConsumo = qRound(qMin(quantVenda, restanteEstoque) * 100) / 100.;

  if (qFuzzyIsNull(quantConsumo)) { return qApp->enqueueError(false, "quantConsumo = 0!", this); }
  if (quantConsumo < quantVenda) { return qApp->enqueueError(false, "quantConsumo < quantVenda", this); }

  if (not modelVenda.setData(rowVenda, "status", "EM COLETA")) { return false; }
  if (not modelVenda.setData(rowVenda, "dataRealFat", dataReal)) { return false; }

  // -------------------------------------

  if (not modelEstoque.setData(rowEstoque, "restante", restanteEstoque - quantConsumo)) { return false; }

  // -------------------------------------

  const int rowConsumo = modelConsumo.insertRowAtEnd();

  for (int column = 0, columnCount = modelEstoque.columnCount(); column < columnCount; ++column) {
    const QString field = modelEstoque.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field, true);

    if (index == -1) { continue; }
    if (column == modelEstoque.fieldIndex("cfop")) { break; }

    const QVariant value = modelEstoque.data(rowEstoque, column);

    if (not modelConsumo.setData(rowConsumo, index, value)) { return false; }
  }

  // -------------------------------------

  QSqlQuery queryProduto;
  queryProduto.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  queryProduto.bindValue(":idProduto", modelCompra.data(rowCompra, "idProduto"));

  if (not queryProduto.exec() or not queryProduto.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + queryProduto.lastError().text(), this); }

  const QString un = queryProduto.value("un").toString();
  const double m2cx = queryProduto.value("m2cx").toDouble();
  const double pccx = queryProduto.value("pccx").toDouble();

  const double unCaixa = (un == "M2" or un == "M²" or un == "ML") ? m2cx : pccx;

  const double caixas = qRound(quantConsumo / unCaixa * 100) / 100.;

  if (not modelConsumo.setData(rowConsumo, "status", "PRÉ-CONSUMO")) { return false; }
  if (not modelConsumo.setData(rowConsumo, "quant", quantConsumo * -1)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "caixas", caixas)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen))) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idVendaProduto2", idVendaProduto2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idEstoque", idEstoque)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idPedido2", modelCompra.data(rowCompra, "idPedido2"))) { return false; }

  // -------------------------------------

  return true;
}

std::optional<int> ImportarXML::dividirVenda(const int rowVenda, const double quantAdicionar) {
  // NOTE: *quebralinha venda_has_produto2
  const auto novoIdVendaProduto2 = reservarIdVendaProduto2();

  if (not novoIdVendaProduto2) { return {}; }

  const int newRowVenda = modelVenda.insertRowAtEnd();

  for (int column = 0, columnCount = modelVenda.columnCount(); column < columnCount; ++column) {
    if (column == modelVenda.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelVenda.fieldIndex("created")) { continue; }
    if (column == modelVenda.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelVenda.data(rowVenda, column);

    if (not modelVenda.setData(newRowVenda, column, value)) { return {}; }
  }

  const double quantVenda = modelVenda.data(rowVenda, "quant").toDouble();
  const double unCaixa = modelVenda.data(rowVenda, "unCaixa").toDouble();

  const double proporcao = quantAdicionar / quantVenda;
  const double parcial = modelVenda.data(rowVenda, "parcial").toDouble() * proporcao;
  const double parcialDesc = modelVenda.data(rowVenda, "parcialDesc").toDouble() * proporcao;
  const double total = modelVenda.data(rowVenda, "total").toDouble() * proporcao;

  if (not modelVenda.setData(rowVenda, "quant", quantAdicionar)) { return {}; }
  if (not modelVenda.setData(rowVenda, "caixas", quantAdicionar / unCaixa)) { return {}; }
  if (not modelVenda.setData(rowVenda, "parcial", parcial)) { return {}; }
  if (not modelVenda.setData(rowVenda, "parcialDesc", parcialDesc)) { return {}; }
  if (not modelVenda.setData(rowVenda, "total", total)) { return {}; }

  const double quantNovo = quantVenda - quantAdicionar;
  const double proporcaoNovo = quantNovo / quantVenda;
  const double parcialNovo = modelVenda.data(newRowVenda, "parcial").toDouble() * proporcaoNovo;
  const double parcialDescNovo = modelVenda.data(newRowVenda, "parcialDesc").toDouble() * proporcaoNovo;
  const double totalNovo = modelVenda.data(newRowVenda, "total").toDouble() * proporcaoNovo;

  if (not modelVenda.setData(newRowVenda, "idVendaProduto2", novoIdVendaProduto2.value())) { return {}; }
  if (not modelVenda.setData(newRowVenda, "idRelacionado", modelVenda.data(rowVenda, "idVendaProduto2"))) { return {}; }
  if (not modelVenda.setData(newRowVenda, "quant", quantNovo)) { return {}; }
  if (not modelVenda.setData(newRowVenda, "caixas", quantNovo / unCaixa)) { return {}; }
  if (not modelVenda.setData(newRowVenda, "parcial", parcialNovo)) { return {}; }
  if (not modelVenda.setData(newRowVenda, "parcialDesc", parcialDescNovo)) { return {}; }
  if (not modelVenda.setData(newRowVenda, "total", totalNovo)) { return {}; }

  // -------------------------------------

  return novoIdVendaProduto2.value();
}

bool ImportarXML::dividirCompra(const int rowCompra, const double quantAdicionar) {
  // NOTE: *quebralinha pedido_fornecedor2
  const auto novoIdPedido2 = reservarIdPedido2();

  if (not novoIdPedido2) { return false; }

  const int newRow = modelCompra.insertRowAtEnd();

  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(rowCompra, column);

    if (not modelCompra.setData(newRow, column, value)) { return false; }
  }

  const double caixas = modelCompra.data(rowCompra, "caixas").toDouble();
  const double prcUnitario = modelCompra.data(rowCompra, "prcUnitario").toDouble();
  const double quantOriginal = modelCompra.data(rowCompra, "quant").toDouble();
  const double quantNovo = quantOriginal - quantAdicionar;
  const double proporcaoAntigo = quantAdicionar / quantOriginal;
  const double proporcaoNovo = quantNovo / quantOriginal;

  if (not modelCompra.setData(rowCompra, "quant", quantAdicionar)) { return false; }
  if (not modelCompra.setData(rowCompra, "caixas", caixas * proporcaoAntigo)) { return false; }
  if (not modelCompra.setData(rowCompra, "preco", prcUnitario * quantAdicionar)) { return false; }

  if (not modelCompra.setData(newRow, "idPedido2", novoIdPedido2.value())) { return false; }
  if (not modelCompra.setData(newRow, "idRelacionado", modelCompra.data(rowCompra, "idPedido2"))) { return false; }
  if (not modelCompra.setData(newRow, "quant", quantNovo)) { return false; }
  if (not modelCompra.setData(newRow, "caixas", caixas * proporcaoNovo)) { return false; }
  if (not modelCompra.setData(newRow, "preco", prcUnitario * quantNovo)) { return false; }

  // -------------------------------------

  const int idVendaProduto2 = modelCompra.data(rowCompra, "idVendaProduto2").toInt();

  if (idVendaProduto2 != 0) {
    const auto list = modelVenda.multiMatch({{"idVendaProduto2", idVendaProduto2}}, false);

    if (list.isEmpty()) { return qApp->enqueueError(false, "Erro procurando produto da venda!", this); }

    const int rowVenda = list.first();

    const auto novoIdVenda = dividirVenda(rowVenda, quantAdicionar);

    if (not novoIdVenda) { return false; }

    if (not modelCompra.setData(newRow, "idVendaProduto2", novoIdVenda.value())) { return false; }
  }

  return true;
}

void ImportarXML::on_pushButtonCancelar_clicked() { close(); }

void ImportarXML::reparear(const QModelIndex &index) {
  if (index.column() != modelEstoque.fieldIndex("codComercial") and index.column() != modelEstoque.fieldIndex("quant")) { return; }

  parear();
}

bool ImportarXML::parear() {
  unsetConnections();

  const auto ok = [&] {
    if (not limparAssociacoes()) { return false; }

    for (int rowEstoque = 0, totalEstoque = modelEstoque.rowCount(); rowEstoque < totalEstoque; ++rowEstoque) {
      const QString codComercialEstoque = modelEstoque.data(rowEstoque, "codComercial").toString();
      const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();

      // fazer busca por quantidades iguais
      const auto iguais =
          modelCompra.multiMatch({{"codComercial", codComercialEstoque}, {"quant", quantEstoque}, {"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

      if (not iguais.isEmpty()) {
        if (not associarIgual(iguais.first(), rowEstoque)) { return false; }
        continue;
      }

      // fazer busca por quantidades diferentes
      const auto diferentes =
          modelCompra.multiMatch({{"codComercial", codComercialEstoque}, {"quant", quantEstoque, false}, {"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green), false}});

      bool repareado = false;
      double estoquePareado = 0;

      for (const auto &row : diferentes) {
        if (not associarDiferente(row, rowEstoque, estoquePareado, repareado)) { return false; }
        if (repareado) { return true; }
        if (qFuzzyCompare(quantEstoque, estoquePareado)) { break; }
      }
    }

    ui->tableEstoque->clearSelection();
    ui->tableConsumo->clearSelection();
    ui->tableCompra->clearSelection();

    ui->tableCompra->setFocus(); // for updating proxyModel

    return true;
  }();

  setConnections();

  return ok;
}

std::optional<int> ImportarXML::reservarIdEstoque() {
  if (qApp->getInTransaction()) {
    qApp->enqueueError("Erro ALTER TABLE durante transação!", this);
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'estoque'") or not query.first()) {
    qApp->enqueueError("Erro reservar id estoque: " + query.lastError().text(), this);
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE estoque auto_increment = " + QString::number(id + 1))) {
    qApp->enqueueError("Erro reservar id estoque: " + query.lastError().text(), this);
    return {};
  }

  return id;
}

void ImportarXML::on_checkBoxSemLote_toggled(const bool checked) {
  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (not modelEstoque.setData(row, "lote", checked ? "N/D" : "")) { return; }
  }
}

std::optional<int> ImportarXML::reservarIdVendaProduto2() {
  if (qApp->getInTransaction()) {
    qApp->enqueueError("Erro ALTER TABLE durante transação!", this);
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'venda_has_produto2'") or not query.first()) {
    qApp->enqueueError("Erro reservar id venda: " + query.lastError().text(), this);
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE venda_has_produto2 auto_increment = " + QString::number(id + 1))) {
    qApp->enqueueError("Erro reservar id venda: " + query.lastError().text(), this);
    return {};
  }

  return id;
}

std::optional<int> ImportarXML::reservarIdPedido2() {
  if (qApp->getInTransaction()) {
    qApp->enqueueError("Erro ALTER TABLE durante transação!", this);
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'pedido_fornecedor_has_produto2'") or not query.first()) {
    qApp->enqueueError("Erro reservar id compra: " + query.lastError().text(), this);
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE pedido_fornecedor_has_produto2 auto_increment = " + QString::number(id + 1))) {
    qApp->enqueueError("Erro reservar id compra: " + query.lastError().text(), this);
    return {};
  }

  return id;
}

// NOTE: 5utilizar tabela em arvore (qtreeview) para agrupar consumos com seu estoque
//          (para cada linha do model inserir items na arvore?)
// TODO: 5quando as unidades vierem diferente pedir para usuario converter
// TODO: 5avisar se R$ da nota for diferente do R$ da compra
// TODO: verificar se o valor total da nota bate com o valor total da compra (bater impostos/st)
