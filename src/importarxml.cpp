#include "importarxml.h"
#include "ui_importarxml.h"

#include "application.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "estoqueproxymodel.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

ImportarXML::ImportarXML(const QStringList &idsCompra, const QDate &dataFaturamento, QWidget *parent)
    : QDialog(parent), dataFaturamento(dataFaturamento), idsCompra(idsCompra), ui(new Ui::ImportarXML) {
  ui->setupUi(this);

  setupTables();

  setConnections();

  ui->itemBoxNFe->setSearchDialog(SearchDialog::nfe(this));

  setWindowFlags(Qt::Window);
}

ImportarXML::~ImportarXML() { delete ui; }

void ImportarXML::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxSemLote, &QCheckBox::toggled, this, &ImportarXML::on_checkBoxSemLote_toggled, connectionType);
  connect(ui->itemBoxNFe, &ItemBox::textChanged, this, &ImportarXML::on_itemBoxNFe_textChanged, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonImportar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonImportar_clicked, connectionType);
  connect(ui->pushButtonProcurar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonProcurar_clicked, connectionType);
  connect(ui->tableEstoque->model(), &QAbstractItemModel::dataChanged, this, &ImportarXML::updateTableData, connectionType);
}

void ImportarXML::unsetConnections() {
  disconnect(ui->checkBoxSemLote, &QCheckBox::toggled, this, &ImportarXML::on_checkBoxSemLote_toggled);
  disconnect(ui->itemBoxNFe, &ItemBox::textChanged, this, &ImportarXML::on_itemBoxNFe_textChanged);
  disconnect(ui->pushButtonCancelar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonCancelar_clicked);
  disconnect(ui->pushButtonImportar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonImportar_clicked);
  disconnect(ui->pushButtonProcurar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonProcurar_clicked);
  disconnect(ui->tableEstoque->model(), &QAbstractItemModel::dataChanged, this, &ImportarXML::updateTableData);
}

void ImportarXML::updateTableData(const QModelIndex &topLeft) {
  unsetConnections();

  try {
    [&] {
      const QString header = modelEstoque.headerData(topLeft.column(), Qt::Horizontal).toString();
      const int row = topLeft.row();

      // TODO: se alterar quant. tem que alterar caixas

      if (header == "Quant." or header == "R$ Unid.") {
        const double preco = modelEstoque.data(row, "quant").toDouble() * modelEstoque.data(row, "valorUnid").toDouble();
        modelEstoque.setData(row, "valor", preco);
      }

      if (header == "R$") {
        const double prcUnitario = modelEstoque.data(row, "valor").toDouble() / modelEstoque.data(row, "quant").toDouble();
        modelEstoque.setData(row, "valorUnid", prcUnitario);
      }

      reparear(topLeft);
    }();
  } catch (std::exception &e) { ui->pushButtonImportar->setDisabled(true); }

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
  ui->tableEstoque->setItemDelegateForColumn("quant", new DoubleDelegate(3, this));
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
  ui->tableEstoque->hideColumn("nve");
  ui->tableEstoque->hideColumn("extipi");
  ui->tableEstoque->hideColumn("cest");
  ui->tableEstoque->hideColumn("cfop");
  ui->tableEstoque->hideColumn("codBarrasTrib");
  ui->tableEstoque->hideColumn("unTrib");
  ui->tableEstoque->hideColumn("quantTrib");
  ui->tableEstoque->hideColumn("valorUnidTrib");
  ui->tableEstoque->hideColumn("frete");
  ui->tableEstoque->hideColumn("seguro");
  ui->tableEstoque->hideColumn("desconto");
  ui->tableEstoque->hideColumn("outros");
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
  modelConsumo.setHeaderData("bloco", "Bloco");
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

  ui->tableConsumo->hideColumn("idVendaProduto2");
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
  ui->tableConsumo->hideColumn("valorUnidTrib");
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

  modelCompra.select();

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
  ui->tableCompra->hideColumn("codFornecedor");
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

  modelVenda.select();

  // -------------------------------------------------------------------------

  modelPagamento.setTable("conta_a_pagar_has_pagamento");
}

void ImportarXML::cadastrarProdutoEstoque(const QVector<ProdutoEstoque> &tuples) {
  SqlQuery query;
  query.prepare(
      "INSERT INTO produto SELECT NULL, p.idProdutoUpd, :idEstoque, p.idFornecedor, p.idFornecedorUpd, p.fornecedor, p.fornecedorUpd, CONCAT(p.descricao, ' (ESTOQUE)'), p.descricaoUpd, "
      ":estoqueRestante, p.estoqueRestanteUpd, p.quantCaixa, p.quantCaixaUpd, p.un, p.unUpd, p.un2, p.un2Upd, p.colecao, p.colecaoUpd, p.tipo, p.tipoUpd, p.minimo, p.minimoUpd, p.multiplo, "
      "p.multiploUpd, p.m2cx, p.m2cxUpd, p.pccx, p.pccxUpd, p.kgcx, p.kgcxUpd, p.formComercial, p.formComercialUpd, p.codComercial, p.codComercialUpd, p.codBarras, p.codBarrasUpd, p.ncm, p.ncmUpd, "
      "p.ncmEx, p.ncmExUpd, p.cfop, p.cfopUpd, p.icms, p.icmsUpd, p.cst, p.cstUpd, p.qtdPallet, p.qtdPalletUpd, :custo, p.custoUpd, p.ipi, p.ipiUpd, p.st, p.stUpd, p.sticms, p.sticmsUpd, p.mva, "
      "p.mvaUpd, p.oldPrecoVenda, p.precoVenda, p.precoVendaUpd, p.markup, p.markupUpd, p.comissao, p.comissaoUpd, p.observacoes, p.observacoesUpd, p.origem, p.origemUpd, p.temLote, p.temLoteUpd, "
      "p.ui, p.uiUpd, NULL, p.validadeUpd, :descontinuado, p.descontinuadoUpd, p.atualizarTabelaPreco, p.representacao, 1, 0, p.idProduto, 0, NULL, NULL FROM produto p WHERE p.idProduto = "
      ":idProduto");

  for (const auto &tuple : tuples) {
    const auto [idProduto, idEstoque, estoqueRestante, valorUnid] = tuple;

    // TODO: verificar se precisa alterar outros campos como lote, markup
    query.bindValue(":idProduto", idProduto);
    query.bindValue(":idEstoque", idEstoque);
    query.bindValue(":estoqueRestante", estoqueRestante);
    query.bindValue(":descontinuado", qFuzzyIsNull(estoqueRestante) ? true : false);
    query.bindValue(":custo", valorUnid);

    if (not query.exec()) { throw RuntimeException("Erro criando produto_estoque: " + query.lastError().text()); }
  }
}

QVector<ImportarXML::ProdutoEstoque> ImportarXML::mapTuples() {
  QVector<ProdutoEstoque> produtos;

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    const int idProduto = modelEstoque.data(row, "idProduto").toInt();
    const int idEstoque = modelEstoque.data(row, "idEstoque").toInt();
    const double estoqueRestante = modelEstoque.data(row, "restante").toDouble();
    const double valorUnid = modelEstoque.data(row, "valorUnid").toDouble();

    produtos << ProdutoEstoque{idProduto, idEstoque, estoqueRestante, valorUnid};
  }

  return produtos;
}

void ImportarXML::salvarDadosVenda() {
  for (int row = 0; row < modelConsumo.rowCount(); ++row) {
    const int idEstoque = modelConsumo.data(row, "idEstoque").toInt();

    const auto list = modelEstoque.multiMatch({{"idEstoque", idEstoque}}, false);

    if (list.isEmpty()) { throw RuntimeException("Erro buscando lote!"); }

    const QString lote = modelEstoque.data(list.first(), "lote").toString();

    const int idVendaProduto2 = modelConsumo.data(row, "idVendaProduto2").toInt();

    const auto list2 = modelVenda.multiMatch({{"idVendaProduto2", idVendaProduto2}}, false);

    if (list2.isEmpty()) { throw RuntimeException("Erro buscando lote!"); }

    modelVenda.setData(list2.first(), "lote", lote);
  }

  modelVenda.submitAll();

  QStringList idVendas;

  for (int row = 0; row < modelVenda.rowCount(); ++row) { idVendas << modelVenda.data(row, "idVenda").toString(); }

  Sql::updateVendaStatus(idVendas);
}

void ImportarXML::atualizarNFes() {
  auto iterator = mapNFes.constBegin();

  while (iterator != mapNFes.constEnd()) {
    SqlQuery query;

    if (not query.exec("UPDATE nfe SET gare = " + QString::number(iterator.value()) + ", utilizada = TRUE WHERE chaveAcesso = '" + iterator.key() + "'")) {
      throw RuntimeException("Erro atualizando dados da NFe: " + query.lastError().text());
    }

    iterator++;
  }
}

void ImportarXML::importar() {
  salvarDadosVenda();

  salvarDadosCompra();

  modelNFe.submitAll();

  atualizarNFes();

  // mapear idProduto/idEstoque para cadastrarProdutoEstoque
  const auto tuples = mapTuples();

  modelEstoque.submitAll();

  cadastrarProdutoEstoque(tuples);

  modelConsumo.submitAll();

  modelEstoque_compra.submitAll();

  modelPagamento.submitAll();
}

void ImportarXML::salvarDadosCompra() {
  const auto list = modelCompra.multiMatch({{"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green)}});

  if (list.isEmpty()) { throw RuntimeException("Erro buscando produtos da compra!"); }

  for (const auto &row : list) {
    modelCompra.setData(row, "status", "EM COLETA");
    modelCompra.setData(row, "dataRealFat", dataFaturamento);
  }

  modelCompra.submitAll();

  // TODO: ainda precisa disso considerando que agora tem o trigger no BD?
  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    SqlQuery query;

    if (not query.exec("CALL update_pedido_fornecedor_status(" + modelCompra.data(row, "idPedidoFK").toString() + ")")) {
      throw RuntimeException("Erro atualizando status compra: " + query.lastError().text());
    }
  }
}

void ImportarXML::verifyFields() {
  const auto list = modelCompra.multiMatch({{"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green)}}, false);

  if (list.isEmpty()) { throw RuntimeError("Nenhuma compra pareada!", this); }

  //----------------------------------------------------------------

  const auto list2 = modelEstoque.multiMatch({{"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

  if (not list2.isEmpty()) { throw RuntimeError("Nem todos os estoques estão ok!", this); }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (modelEstoque.data(row, "lote").toString().isEmpty()) { throw RuntimeError("Lote vazio! Se não há coloque 'N/D'!", this); }
  }
}

void ImportarXML::on_pushButtonImportar_clicked() {
  try {
    verifyFields();
  } catch (std::exception &e) { return; }

  unsetConnections();

  try {
    qApp->startTransaction("ImportarXML::on_pushButtonImportar");

    importar();

    qApp->endTransaction();
  } catch (std::exception &e) {
    qApp->rollbackTransaction();
    ui->pushButtonImportar->setDisabled(true);
    return;
  }

  setConnections();

  QDialog::accept();
  close();
}

void ImportarXML::limparAssociacoes() {
  const auto list = modelCompra.multiMatch({{"status", "EM FATURAMENTO"}});

  for (const auto row : list) { modelCompra.setData(row, "quantUpd", static_cast<int>(FieldColors::None)); }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    modelEstoque.setData(row, "quantUpd", static_cast<int>(FieldColors::Red));
    modelEstoque.setData(row, "restante", modelEstoque.data(row, "quant"));
  }

  for (int row = 0; row < modelVenda.rowCount(); ++row) {
    modelVenda.setData(row, "status", "EM FATURAMENTO");
    modelVenda.setData(row, "dataRealFat", QVariant());
  }

  modelConsumo.revertAll();
  modelEstoque_compra.revertAll();
}

void ImportarXML::on_itemBoxNFe_textChanged(const QString &text) {
  if (text.isEmpty()) { return; }

  unsetConnections();

  try {
    [&] {
      usarXMLBaixado();
      parear();

      const auto list = modelCompra.multiMatch({{"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

      if (list.isEmpty()) {
        ui->pushButtonProcurar->setDisabled(true);
        ui->itemBoxNFe->setReadOnly(true);
      }
    }();
  } catch (RuntimeError &e) {
  } catch (std::exception &e) { ui->pushButtonImportar->setDisabled(true); }

  setConnections();
}

void ImportarXML::on_pushButtonProcurar_clicked() {
  unsetConnections();

  try {
    [&] {
      if (not lerXML()) {
        ui->lineEdit->clear();
        return;
      }

      parear();

      const auto list = modelCompra.multiMatch({{"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

      if (list.isEmpty()) {
        ui->pushButtonProcurar->setDisabled(true);
        ui->itemBoxNFe->setReadOnly(true);
      }
    }();
  } catch (RuntimeError &e) {
  } catch (std::exception &e) { ui->pushButtonImportar->setDisabled(true); }

  setConnections();
}

double ImportarXML::buscarCaixas(const int rowEstoque) {
  SqlQuery query;
  query.prepare("SELECT quantCaixa FROM produto WHERE codComercial = :codComercial");
  query.bindValue(":codComercial", modelEstoque.data(rowEstoque, "codComercial"));

  if (not query.exec()) { throw RuntimeException("Erro buscando produto: " + query.lastError().text()); }

  if (not query.first()) { return 0; }

  const double quantCaixa = query.value("quantCaixa").toDouble();
  const double quant = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double caixas = quant / quantCaixa;

  return caixas;
}

void ImportarXML::associarIgual(const int rowCompra, const int rowEstoque) {
  modelCompra.setData(rowCompra, "descricao", modelEstoque.data(rowEstoque, "descricao").toString());
  modelCompra.setData(rowCompra, "quantUpd", static_cast<int>(FieldColors::Green));

  const double caixas = buscarCaixas(rowEstoque);

  if (not qFuzzyIsNull(caixas)) { modelEstoque.setData(rowEstoque, "caixas", caixas); }

  modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(FieldColors::Green));
  modelEstoque.setData(rowEstoque, "idProduto", modelCompra.data(rowCompra, "idProduto"));

  const int rowEstoque_compra = modelEstoque_compra.insertRowAtEnd();

  modelEstoque_compra.setData(rowEstoque_compra, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"));
  modelEstoque_compra.setData(rowEstoque_compra, "idCompra", modelCompra.data(rowCompra, "idCompra"));
  modelEstoque_compra.setData(rowEstoque_compra, "idPedido2", modelCompra.data(rowCompra, "idPedido2"));

  criarConsumo(rowCompra, rowEstoque);
}

void ImportarXML::associarDiferente(const int rowCompra, const int rowEstoque, double &estoquePareado, bool &repareado) {
  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double estoqueDisponivel = quantEstoque - estoquePareado;

  const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();
  const double quantAdicionar = qMin(estoqueDisponivel, quantCompra);
  estoquePareado += quantAdicionar;

  const bool compraCompleta = qFuzzyCompare(quantAdicionar, quantCompra);

  // refazer pareamento do começo porque a linha nova não vai estar no match()
  if (not compraCompleta) {
    dividirCompra(rowCompra, quantAdicionar);

    repareado = true;

    parear();

    return;
  }

  modelCompra.setData(rowCompra, "descricao", modelEstoque.data(rowEstoque, "descricao").toString());
  modelCompra.setData(rowCompra, "quantUpd", static_cast<int>(FieldColors::Green));

  const double caixas = buscarCaixas(rowEstoque);

  if (not qFuzzyIsNull(caixas)) { modelEstoque.setData(rowEstoque, "caixas", caixas); }

  modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(qFuzzyCompare(estoquePareado, quantEstoque) ? FieldColors::Green : FieldColors::Yellow));
  modelEstoque.setData(rowEstoque, "idProduto", modelCompra.data(rowCompra, "idProduto"));

  const int rowEstoque_compra = modelEstoque_compra.insertRowAtEnd();

  modelEstoque_compra.setData(rowEstoque_compra, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"));
  modelEstoque_compra.setData(rowEstoque_compra, "idCompra", modelCompra.data(rowCompra, "idCompra"));
  modelEstoque_compra.setData(rowEstoque_compra, "idPedido2", modelCompra.data(rowCompra, "idPedido2"));

  criarConsumo(rowCompra, rowEstoque);
}

bool ImportarXML::verificaExiste(const XML &xml) {
  const auto list = modelNFe.multiMatch({{"chaveAcesso", xml.chaveAcesso}}, false);

  if (not list.isEmpty()) { throw RuntimeError("Nota já cadastrada!", this); }

  // ----------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT status, utilizada FROM nfe WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", xml.chaveAcesso);

  if (not query.exec()) { throw RuntimeException("Erro verificando se nota já cadastrada: " + query.lastError().text()); }

  if (query.first()) {
    if (query.value("utilizada").toBool()) { throw RuntimeError("Nota já utilizada!", this); }

    if (query.value("status") == "CANCELADA") { throw RuntimeError("Nota cancelada!", this); }

    if (query.value("status") == "RESUMO") {
      SqlQuery queryAtualiza;
      queryAtualiza.prepare("UPDATE nfe SET status = 'AUTORIZADO', xml = :xml, transportadora = :transportadora, infCpl = :infCpl WHERE chaveAcesso = :chaveAcesso");
      queryAtualiza.bindValue(":xml", xml.fileContent);
      queryAtualiza.bindValue(":transportadora", xml.xNomeTransp);
      queryAtualiza.bindValue(":infCpl", encontraInfCpl(xml.fileContent));
      queryAtualiza.bindValue(":chaveAcesso", xml.chaveAcesso);

      if (not queryAtualiza.exec()) { throw RuntimeException("Erro cadastrando XML: " + queryAtualiza.lastError().text()); }
    }

    // nota já autorizada ou recém atualizada para autorizada
    ui->itemBoxNFe->setText(xml.chaveAcesso);
    on_itemBoxNFe_textChanged(xml.chaveAcesso);

    return true;
  }

  return false;
}

QString ImportarXML::encontraInfCpl(const QString &xml) {
  const int indexInfCpl1 = xml.indexOf("<infCpl>");
  const int indexInfCpl2 = xml.indexOf("</infCpl>");

  if (indexInfCpl1 != -1 and indexInfCpl2 != -1) { return xml.mid(indexInfCpl1, indexInfCpl2 - indexInfCpl1).remove("<infCpl>"); }

  return "";
}

void ImportarXML::cadastrarNFe(XML &xml, const double gare) {
  const int row = modelNFe.insertRowAtEnd();

  modelNFe.setData(row, "idNFe", xml.idNFe);
  modelNFe.setData(row, "tipo", "ENTRADA");
  modelNFe.setData(row, "cnpjDest", xml.cnpjDest);
  modelNFe.setData(row, "cnpjOrig", xml.cnpjOrig);
  modelNFe.setData(row, "chaveAcesso", xml.chaveAcesso);
  modelNFe.setData(row, "numeroNFe", xml.nNF);
  modelNFe.setData(row, "xml", xml.fileContent);
  modelNFe.setData(row, "transportadora", xml.xNomeTransp);
  modelNFe.setData(row, "valor", xml.vNF_Total);
  modelNFe.setData(row, "gare", gare);
  modelNFe.setData(row, "utilizada", 1);
}

void ImportarXML::usarXMLBaixado() {
  SqlQuery query;

  if (not query.exec("SELECT idNFe, xml, status, utilizada FROM nfe WHERE chaveAcesso = '" + ui->itemBoxNFe->text() + "'") or not query.first()) {
    throw RuntimeException("Erro buscando XML: " + query.lastError().text(), this);
  }

  if (query.value("status") != "AUTORIZADO") { throw RuntimeError("NFe não está autorizada!", this); }

  const auto fileContent = query.value("xml").toByteArray();

  // ----------------------------------------------------------------

  XML xml(fileContent, XML::Tipo::Entrada, this);

  // verifica se já cadastrado dentre as notas utilizadas nessa importacao
  if (mapNFes.contains(xml.chaveAcesso)) { throw RuntimeError("Nota já cadastrada!", this); }

  xml.verificaNCMs();

  perguntarLocal(xml);

  // ----------------------------------------------------------------

  xml.idNFe = query.value("idNFe").toInt();

  // ----------------------------------------------------------------

  const double gare = calculaGare(xml);

  criarPagamentoGare(gare, xml);

  // ----------------------------------------------------------------

  percorrerXml(xml);

  mapNFes.insert(xml.chaveAcesso, gare);
}

bool ImportarXML::lerXML() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Arquivo XML", QDir::currentPath(), "XML (*.xml)");

  if (filePath.isEmpty()) { return false; }

  // ----------------------------------------------------------------

  ui->lineEdit->setText(filePath);

  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo arquivo: " + file.errorString()); }

  auto fileContent = file.readAll();

  file.close();

  if (fileContent.left(3) == "o;?") { fileContent.remove(0, 3); }

  // ----------------------------------------------------------------

  XML xml(fileContent, XML::Tipo::Entrada, this);

  xml.validar();

  if (verificaExiste(xml)) { return false; }

  xml.verificaNCMs();

  perguntarLocal(xml);

  // ----------------------------------------------------------------

  const int id = qApp->reservarIdNFe();

  xml.idNFe = id;

  // ----------------------------------------------------------------

  const double gare = calculaGare(xml);

  criarPagamentoGare(gare, xml);

  // ----------------------------------------------------------------

  percorrerXml(xml);

  cadastrarNFe(xml, gare);

  return true;
}

void ImportarXML::perguntarLocal(XML &xml) {
  SqlQuery query;

  if (not query.exec("SELECT descricao FROM loja WHERE descricao NOT IN ('', 'CD') ORDER BY descricao")) { throw RuntimeException("Erro buscando lojas: " + query.lastError().text(), this); }

  QStringList lojas{"CD"};

  while (query.next()) { lojas << query.value("descricao").toString(); }

  QInputDialog input;
  input.setInputMode(QInputDialog::TextInput);
  input.setCancelButtonText("Cancelar");
  input.setWindowTitle("Local");
  input.setLabelText("Local do depósito:");
  input.setComboBoxItems(lojas);
  input.setComboBoxEditable(false);

  if (input.exec() != QInputDialog::Accepted) { throw RuntimeError("Local inválido!"); }

  const QString local = input.textValue();

  xml.local = local;
}

void ImportarXML::percorrerXml(XML &xml) {
  for (const auto &produto : xml.produtos) {
    const int idEstoque = qApp->reservarIdEstoque();

    const int newRow = modelEstoque.insertRowAtEnd();

    double desconto = produto.desconto;
    QString codigo = produto.codProd;

    if (xml.xNome == "CECRISA REVEST. CERAMICOS S.A.") {
      if (codigo.endsWith("A")) { codigo = codigo.left(codigo.size() - 1); }
      desconto = 0;
    }

    modelEstoque.setData(newRow, "idEstoque", idEstoque);
    modelEstoque.setData(newRow, "idNFe", xml.idNFe);
    modelEstoque.setData(newRow, "fornecedor", xml.xNome);
    modelEstoque.setData(newRow, "local", xml.local);
    modelEstoque.setData(newRow, "descricao", produto.descricao);
    modelEstoque.setData(newRow, "quant", produto.quant);
    modelEstoque.setData(newRow, "restante", produto.quant);
    modelEstoque.setData(newRow, "un", produto.un);
    modelEstoque.setData(newRow, "codBarras", produto.codBarras);
    modelEstoque.setData(newRow, "codComercial", codigo);
    modelEstoque.setData(newRow, "ncm", produto.ncm);
    modelEstoque.setData(newRow, "nve", produto.nve);
    modelEstoque.setData(newRow, "extipi", produto.extipi);
    modelEstoque.setData(newRow, "cest", produto.cest);
    modelEstoque.setData(newRow, "cfop", produto.cfop);
    modelEstoque.setData(newRow, "valorUnid", (produto.valor + produto.vIPI - desconto) / produto.quant);
    modelEstoque.setData(newRow, "valor", produto.valor + produto.vIPI - desconto);
    modelEstoque.setData(newRow, "codBarrasTrib", produto.codBarrasTrib);
    modelEstoque.setData(newRow, "unTrib", produto.unTrib);
    modelEstoque.setData(newRow, "quantTrib", produto.quantTrib);
    modelEstoque.setData(newRow, "valorUnidTrib", produto.valorUnidTrib);
    modelEstoque.setData(newRow, "frete", produto.frete);
    modelEstoque.setData(newRow, "seguro", produto.seguro);
    modelEstoque.setData(newRow, "desconto", desconto);
    modelEstoque.setData(newRow, "outros", produto.outros);
    modelEstoque.setData(newRow, "compoeTotal", produto.compoeTotal);
    modelEstoque.setData(newRow, "numeroPedido", produto.numeroPedido);
    modelEstoque.setData(newRow, "itemPedido", produto.itemPedido);
    modelEstoque.setData(newRow, "tipoICMS", produto.tipoICMS);
    modelEstoque.setData(newRow, "orig", produto.orig);
    modelEstoque.setData(newRow, "cstICMS", produto.cstICMS);
    modelEstoque.setData(newRow, "modBC", produto.modBC);
    modelEstoque.setData(newRow, "vBC", produto.vBC);
    modelEstoque.setData(newRow, "pICMS", produto.pICMS);
    modelEstoque.setData(newRow, "vICMS", produto.vICMS);
    modelEstoque.setData(newRow, "modBCST", produto.modBCST);
    modelEstoque.setData(newRow, "pMVAST", produto.pMVAST);
    modelEstoque.setData(newRow, "vBCST", produto.vBCST);
    modelEstoque.setData(newRow, "pICMSST", produto.pICMSST);
    modelEstoque.setData(newRow, "vICMSST", produto.vICMSST);
    modelEstoque.setData(newRow, "cEnq", produto.cEnq);
    modelEstoque.setData(newRow, "cstIPI", produto.cstIPI);
    modelEstoque.setData(newRow, "vBCIPI", produto.vBCIPI);
    modelEstoque.setData(newRow, "pIPI", produto.pIPI);
    modelEstoque.setData(newRow, "vIPI", produto.vIPI);
    modelEstoque.setData(newRow, "cstPIS", produto.cstPIS);
    modelEstoque.setData(newRow, "vBCPIS", produto.vBCPIS);
    modelEstoque.setData(newRow, "pPIS", produto.pPIS);
    modelEstoque.setData(newRow, "vPIS", produto.vPIS);
    modelEstoque.setData(newRow, "cstCOFINS", produto.cstCOFINS);
    modelEstoque.setData(newRow, "vBCCOFINS", produto.vBCCOFINS);
    modelEstoque.setData(newRow, "pCOFINS", produto.pCOFINS);
    modelEstoque.setData(newRow, "vCOFINS", produto.vCOFINS);
    modelEstoque.setData(newRow, "status", "EM COLETA");
    modelEstoque.setData(newRow, "valorGare", produto.valorGare);
  }
}

void ImportarXML::criarConsumo(const int rowCompra, const int rowEstoque) {
  const int idEstoque = modelEstoque.data(rowEstoque, "idEstoque").toInt();
  const int idVendaProduto2 = modelCompra.data(rowCompra, "idVendaProduto2").toInt();

  if (idVendaProduto2 == 0) { return; }

  // -------------------------------------

  const auto list = modelVenda.multiMatch({{"idVendaProduto2", idVendaProduto2}}, false);

  if (list.isEmpty()) { throw RuntimeException("Erro procurando produto da venda: " + QString::number(idVendaProduto2), this); }

  const int rowVenda = list.first();

  const double quantVenda = qApp->roundDouble(modelVenda.data(rowVenda, "quant").toDouble());
  const double restanteEstoque = qApp->roundDouble(modelEstoque.data(rowEstoque, "restante").toDouble());
  const double quantConsumo = qApp->roundDouble(qMin(quantVenda, restanteEstoque));

  if (qFuzzyIsNull(quantConsumo)) { throw RuntimeException("quantConsumo = 0!", this); }
  if (quantConsumo < quantVenda) { throw RuntimeException("quantConsumo < quantVenda", this); }

  modelVenda.setData(rowVenda, "status", "EM COLETA");
  modelVenda.setData(rowVenda, "dataRealFat", dataFaturamento);

  // -------------------------------------

  modelEstoque.setData(rowEstoque, "restante", restanteEstoque - quantConsumo);

  // -------------------------------------

  const int rowConsumo = modelConsumo.insertRowAtEnd();

  for (int column = 0, columnCount = modelEstoque.columnCount(); column < columnCount; ++column) {
    const QString field = modelEstoque.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field, true);

    if (index == -1) { continue; }
    if (column == modelEstoque.fieldIndex("valor")) { break; }

    const QVariant value = modelEstoque.data(rowEstoque, column);

    if (value.isNull()) { continue; }

    modelConsumo.setData(rowConsumo, index, value);
  }

  // -------------------------------------

  SqlQuery queryProduto;
  queryProduto.prepare("SELECT quantCaixa FROM produto WHERE idProduto = :idProduto");
  queryProduto.bindValue(":idProduto", modelCompra.data(rowCompra, "idProduto"));

  if (not queryProduto.exec() or not queryProduto.first()) { throw RuntimeException("Erro buscando dados do produto: " + queryProduto.lastError().text(), this); }

  const double quantCaixa = queryProduto.value("quantCaixa").toDouble();
  const double caixas = quantConsumo / quantCaixa;
  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double proporcao = quantConsumo / quantEstoque;

  modelConsumo.setData(rowConsumo, "idEstoque", idEstoque);
  modelConsumo.setData(rowConsumo, "idVendaProduto2", idVendaProduto2);
  modelConsumo.setData(rowConsumo, "status", "PRÉ-CONSUMO");
  modelConsumo.setData(rowConsumo, "quant", quantConsumo * -1);
  modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen));
  modelConsumo.setData(rowConsumo, "caixas", caixas);
  const double valorUnid = modelConsumo.data(rowConsumo, "valorUnid").toDouble();
  modelConsumo.setData(rowConsumo, "valor", quantConsumo * valorUnid);

  modelConsumo.setData(rowConsumo, "codBarrasTrib", modelEstoque.data(rowEstoque, "codBarrasTrib"));
  modelConsumo.setData(rowConsumo, "unTrib", modelEstoque.data(rowEstoque, "unTrib"));
  modelConsumo.setData(rowConsumo, "quantTrib", modelEstoque.data(rowEstoque, "quantTrib").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "valorUnidTrib", modelEstoque.data(rowEstoque, "valorUnidTrib"));
  modelConsumo.setData(rowConsumo, "desconto", modelEstoque.data(rowEstoque, "desconto").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "compoeTotal", modelEstoque.data(rowEstoque, "compoeTotal"));
  modelConsumo.setData(rowConsumo, "numeroPedido", modelEstoque.data(rowEstoque, "numeroPedido"));
  modelConsumo.setData(rowConsumo, "itemPedido", modelEstoque.data(rowEstoque, "itemPedido"));
  modelConsumo.setData(rowConsumo, "tipoICMS", modelEstoque.data(rowEstoque, "tipoICMS"));
  modelConsumo.setData(rowConsumo, "orig", modelEstoque.data(rowEstoque, "orig"));
  modelConsumo.setData(rowConsumo, "cstICMS", modelEstoque.data(rowEstoque, "cstICMS"));
  modelConsumo.setData(rowConsumo, "modBC", modelEstoque.data(rowEstoque, "modBC"));
  modelConsumo.setData(rowConsumo, "vBC", modelEstoque.data(rowEstoque, "vBC").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pICMS", modelEstoque.data(rowEstoque, "pICMS"));
  modelConsumo.setData(rowConsumo, "vICMS", modelEstoque.data(rowEstoque, "vICMS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "modBCST", modelEstoque.data(rowEstoque, "modBCST"));
  modelConsumo.setData(rowConsumo, "pMVAST", modelEstoque.data(rowEstoque, "pMVAST"));
  modelConsumo.setData(rowConsumo, "vBCST", modelEstoque.data(rowEstoque, "vBCST").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pICMSST", modelEstoque.data(rowEstoque, "pICMSST"));
  modelConsumo.setData(rowConsumo, "vICMSST", modelEstoque.data(rowEstoque, "vICMSST").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "cEnq", modelEstoque.data(rowEstoque, "cEnq"));
  modelConsumo.setData(rowConsumo, "cstIPI", modelEstoque.data(rowEstoque, "cstIPI"));
  modelConsumo.setData(rowConsumo, "cstPIS", modelEstoque.data(rowEstoque, "cstPIS"));
  modelConsumo.setData(rowConsumo, "vBCPIS", modelEstoque.data(rowEstoque, "vBCPIS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pPIS", modelEstoque.data(rowEstoque, "pPIS"));
  modelConsumo.setData(rowConsumo, "vPIS", modelEstoque.data(rowEstoque, "vPIS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "cstCOFINS", modelEstoque.data(rowEstoque, "cstCOFINS"));
  modelConsumo.setData(rowConsumo, "vBCCOFINS", modelEstoque.data(rowEstoque, "vBCCOFINS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pCOFINS", modelEstoque.data(rowEstoque, "pCOFINS"));
  modelConsumo.setData(rowConsumo, "vCOFINS", modelEstoque.data(rowEstoque, "vCOFINS").toDouble() * proporcao);
}

int ImportarXML::dividirVenda(const int rowVenda, const double quantAdicionar) {
  // NOTE: *quebralinha venda_produto2
  const int novoIdVendaProduto2 = qApp->reservarIdVendaProduto2();

  // -------------------------------------

  const double quantVenda = modelVenda.data(rowVenda, "quant").toDouble();
  const double quantCaixa = modelVenda.data(rowVenda, "quantCaixa").toDouble();
  const double proporcao = quantAdicionar / quantVenda;
  const double parcial = modelVenda.data(rowVenda, "parcial").toDouble();
  const double parcialDesc = modelVenda.data(rowVenda, "parcialDesc").toDouble();
  const double total = modelVenda.data(rowVenda, "total").toDouble();

  modelVenda.setData(rowVenda, "quant", quantAdicionar);
  modelVenda.setData(rowVenda, "caixas", quantAdicionar / quantCaixa);
  modelVenda.setData(rowVenda, "parcial", parcial * proporcao);
  modelVenda.setData(rowVenda, "parcialDesc", parcialDesc * proporcao);
  modelVenda.setData(rowVenda, "total", total * proporcao);

  // -------------------------------------

  const int newRowVenda = modelVenda.insertRowAtEnd();

  for (int column = 0, columnCount = modelVenda.columnCount(); column < columnCount; ++column) {
    if (column == modelVenda.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelVenda.fieldIndex("created")) { continue; }
    if (column == modelVenda.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelVenda.data(rowVenda, column);

    if (value.isNull()) { continue; }

    modelVenda.setData(newRowVenda, column, value);
  }

  const double quantNovo = quantVenda - quantAdicionar;
  const double proporcaoNovo = quantNovo / quantVenda;

  modelVenda.setData(newRowVenda, "idVendaProduto2", novoIdVendaProduto2);
  modelVenda.setData(newRowVenda, "idRelacionado", modelVenda.data(rowVenda, "idVendaProduto2"));
  modelVenda.setData(newRowVenda, "quant", quantNovo);
  modelVenda.setData(newRowVenda, "caixas", quantNovo / quantCaixa);
  modelVenda.setData(newRowVenda, "parcial", parcial * proporcaoNovo);
  modelVenda.setData(newRowVenda, "parcialDesc", parcialDesc * proporcaoNovo);
  modelVenda.setData(newRowVenda, "total", total * proporcaoNovo);

  // -------------------------------------

  return novoIdVendaProduto2;
}

void ImportarXML::dividirCompra(const int rowCompra, const double quantAdicionar) {
  // NOTE: *quebralinha pedido_fornecedor2
  const int novoIdPedido2 = qApp->reservarIdPedido2();

  // -------------------------------------

  const double quantOriginal = modelCompra.data(rowCompra, "quant").toDouble();
  const double proporcaoAntigo = quantAdicionar / quantOriginal;
  const double caixas = modelCompra.data(rowCompra, "caixas").toDouble();
  const double prcUnitario = modelCompra.data(rowCompra, "prcUnitario").toDouble();

  modelCompra.setData(rowCompra, "quant", quantAdicionar);
  modelCompra.setData(rowCompra, "caixas", caixas * proporcaoAntigo);
  modelCompra.setData(rowCompra, "preco", prcUnitario * quantAdicionar);

  // -------------------------------------

  const int newRow = modelCompra.insertRowAtEnd();

  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(rowCompra, column);

    if (value.isNull()) { continue; }

    modelCompra.setData(newRow, column, value);
  }

  const double quantNovo = quantOriginal - quantAdicionar;
  const double proporcaoNovo = quantNovo / quantOriginal;

  modelCompra.setData(newRow, "idPedido2", novoIdPedido2);
  modelCompra.setData(newRow, "idRelacionado", modelCompra.data(rowCompra, "idPedido2"));
  modelCompra.setData(newRow, "quant", quantNovo);
  modelCompra.setData(newRow, "caixas", caixas * proporcaoNovo);
  modelCompra.setData(newRow, "preco", prcUnitario * quantNovo);

  // -------------------------------------

  const int idVendaProduto2 = modelCompra.data(rowCompra, "idVendaProduto2").toInt();

  if (idVendaProduto2 != 0) {
    const auto list = modelVenda.multiMatch({{"idVendaProduto2", idVendaProduto2}}, false);

    if (list.isEmpty()) { throw RuntimeException("Erro procurando produto da venda!"); }

    const int rowVenda = list.first();

    const int novoIdVenda = dividirVenda(rowVenda, quantAdicionar);

    modelCompra.setData(newRow, "idVendaProduto2", novoIdVenda);
  }
}

void ImportarXML::on_pushButtonCancelar_clicked() { close(); }

void ImportarXML::reparear(const QModelIndex &index) {
  if (index.column() != modelEstoque.fieldIndex("codComercial") and index.column() != modelEstoque.fieldIndex("quant")) { return; }

  parear();
}

void ImportarXML::parear() {
  unsetConnections();

  try {
    [&] {
      limparAssociacoes();

      for (int rowEstoque = 0, totalEstoque = modelEstoque.rowCount(); rowEstoque < totalEstoque; ++rowEstoque) {
        const QString codComercialEstoque = modelEstoque.data(rowEstoque, "codComercial").toString();
        const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();

        // fazer busca por quantidades iguais
        const auto iguais =
            modelCompra.multiMatch({{"codComercial", codComercialEstoque}, {"quant", quantEstoque}, {"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green), false}}, false);

        if (not iguais.isEmpty()) {
          associarIgual(iguais.first(), rowEstoque);
          continue;
        }

        // fazer busca por quantidades diferentes
        const auto diferentes =
            modelCompra.multiMatch({{"codComercial", codComercialEstoque}, {"quant", quantEstoque, false}, {"status", "EM FATURAMENTO"}, {"quantUpd", static_cast<int>(FieldColors::Green), false}});

        bool repareado = false;
        double estoquePareado = 0;

        for (const auto rowCompra : diferentes) {
          associarDiferente(rowCompra, rowEstoque, estoquePareado, repareado);
          if (repareado) { return; }
          if (qFuzzyCompare(quantEstoque, estoquePareado)) { break; }
        }
      }

      ui->tableCompra->resort();

      ui->tableEstoque->clearSelection();
      ui->tableConsumo->clearSelection();
      ui->tableCompra->clearSelection();

      ui->tableCompra->setFocus(); // for updating proxyModel
    }();
  } catch (std::exception &e) { ui->pushButtonImportar->setDisabled(true); }

  setConnections();
}

void ImportarXML::on_checkBoxSemLote_toggled(const bool checked) {
  for (int row = 0; row < modelEstoque.rowCount(); ++row) { modelEstoque.setData(row, "lote", checked ? "N/D" : ""); }
}

double ImportarXML::calculaGare(XML &xml) {
  const QString dataEmissao = xml.dataHoraEmissao.left(10);

  double total = 0;

  if (xml.xNome == "DOCOL METAIS SANITARIOS LTDA") { return 0; }

  for (auto &produto : xml.produtos) {
    qDebug() << "tipoICMS: " << produto.tipoICMS;
    if (produto.tipoICMS != "ICMS00") { continue; }
    // TODO: tratar quando for simples nacional

    const ImportarXML::NCM ncm = buscaNCM(produto.ncm);

    if (qFuzzyIsNull(ncm.aliq)) { continue; }

    const double icmsIntra = ncm.aliq;
    const double mva = (qFuzzyCompare(produto.pICMS, 4)) ? ncm.mva4 : ncm.mva12;
    const double baseCalculo = produto.valor + produto.vIPI + produto.outros + produto.frete + produto.seguro - produto.desconto;
    const double icmsProprio = produto.vICMS;
    const double baseST = (baseCalculo) * (1 + mva);
    const double icmsST = (baseST * icmsIntra) - icmsProprio;

    total += icmsST;

    produto.valorGare = icmsST;

    qDebug() << "baseCalculo: " << baseCalculo;
    qDebug() << "mvaAjustado: " << mva;
    qDebug() << "icmsIntra: " << icmsIntra;
    qDebug() << "icmsProprio: " << icmsProprio;
    qDebug() << "baseST: " << baseST;
    qDebug() << "icmsST: " << icmsST;
  }

  qDebug() << "gare: " << total;

  return total;
}

ImportarXML::NCM ImportarXML::buscaNCM(const QString &ncm) {
  SqlQuery query;

  if (not query.exec("SELECT * FROM ncm WHERE ncm = '" + ncm + "'") or not query.first()) { throw RuntimeException("Erro buscando ncm: " + query.lastError().text()); }

  return NCM{query.value("mva4").toDouble() / 100, query.value("mva12").toDouble() / 100, query.value("aliq").toDouble() / 100};
}

void ImportarXML::criarPagamentoGare(const double valor, const XML &xml) {
  const int row = modelPagamento.insertRowAtEnd();

  const int lojaGeral = 1;

  modelPagamento.setData(row, "dataEmissao", qApp->serverDate());
  modelPagamento.setData(row, "dataPagamento", qApp->serverDate().addDays(15));
  modelPagamento.setData(row, "idLoja", lojaGeral);
  modelPagamento.setData(row, "contraParte", "GARE");
  modelPagamento.setData(row, "idNFe", xml.idNFe);
  modelPagamento.setData(row, "nfe", xml.nNF);
  modelPagamento.setData(row, "valor", valor);
  modelPagamento.setData(row, "tipo", "Boleto");
  modelPagamento.setData(row, "observacao", "GARE ICMS ST " + xml.nNF);
  modelPagamento.setData(row, "status", qFuzzyIsNull(valor) ? "PAGO GARE" : "PENDENTE GARE");
  modelPagamento.setData(row, "centroCusto", lojaGeral);
  modelPagamento.setData(row, "grupo", "Impostos - ICMS;ST;ISS");
}

// NOTE: 5utilizar tabela em arvore (qtreeview) para agrupar consumos com seu estoque
//          (para cada linha do model inserir items na arvore?)
// TODO: 5quando as unidades vierem diferente pedir para usuario converter
// TODO: 5avisar se R$ da nota for diferente do R$ da compra
// TODO: verificar se o valor total da nota bate com o valor total da compra (bater impostos/st)
