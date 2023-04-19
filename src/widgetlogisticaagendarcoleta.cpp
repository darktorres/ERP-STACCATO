#include "widgetlogisticaagendarcoleta.h"
#include "ui_widgetlogisticaagendarcoleta.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoqueprazoproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QSqlError>
#include <QStandardItemModel>
#include <QUrl>

WidgetLogisticaAgendarColeta::WidgetLogisticaAgendarColeta(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarColeta) { ui->setupUi(this); }

WidgetLogisticaAgendarColeta::~WidgetLogisticaAgendarColeta() { delete ui; }

void WidgetLogisticaAgendarColeta::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked, connectionType);
  connect(ui->pushButtonAgendarColeta, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked, connectionType);
  connect(ui->pushButtonCancelarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonLimparFiltro_clicked, connectionType);
  connect(ui->pushButtonMontarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked, connectionType);
  connect(ui->pushButtonRemoverProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked, connectionType);
  connect(ui->tableEstoque, &QTableView::doubleClicked, this, &WidgetLogisticaAgendarColeta::on_tableEstoque_doubleClicked, connectionType);
  connect(ui->tableForn->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarColeta::on_tableForn_selectionChanged, connectionType);
}

void WidgetLogisticaAgendarColeta::setupTables() {
  modelEstoque.setTable("view_agendar_coleta");

  modelEstoque.setSort("prazoEntrega");

  modelEstoque.setHeaderData("prazoEntrega", "Prazo Limite");
  modelEstoque.setHeaderData("dataRealFat", "Data Faturado");
  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");
  modelEstoque.setHeaderData("numeroNFe", "NF-e");
  modelEstoque.setHeaderData("idVenda", "Venda");
  modelEstoque.setHeaderData("ordemCompra", "O.C.");
  modelEstoque.setHeaderData("produto", "Produto");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Cx.");
  modelEstoque.setHeaderData("kgcx", "Kg./Cx.");

  modelEstoque.proxyModel = new EstoquePrazoProxyModel(&modelEstoque, this);

  ui->tableEstoque->setModel(&modelEstoque);

  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));

  ui->tableEstoque->hideColumn("fornecedor");
  ui->tableEstoque->hideColumn("quantCaixa");
  ui->tableEstoque->hideColumn("formComercial");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("idCompra");

  connect(ui->tableEstoque->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarColeta::calcularPeso);

  // -------------------------------------------------------------------------

  modelTranspAtual.setTable("veiculo_has_produto");

  modelTranspAtual.setHeaderData("idEstoque", "Estoque");
  modelTranspAtual.setHeaderData("status", "Status");
  modelTranspAtual.setHeaderData("observacao", "Observação");
  modelTranspAtual.setHeaderData("produto", "Produto");
  modelTranspAtual.setHeaderData("caixas", "Cx.");
  modelTranspAtual.setHeaderData("kg", "Kg");
  modelTranspAtual.setHeaderData("quant", "Quant.");
  modelTranspAtual.setHeaderData("un", "Un.");
  modelTranspAtual.setHeaderData("codComercial", "Cód. Com.");

  ui->tableTranspAtual->setModel(&modelTranspAtual);

  ui->tableTranspAtual->hideColumn("fotoEntrega");
  ui->tableTranspAtual->hideColumn("id");
  ui->tableTranspAtual->hideColumn("idEvento");
  ui->tableTranspAtual->hideColumn("idVeiculo");
  ui->tableTranspAtual->hideColumn("idVendaProduto1");
  ui->tableTranspAtual->hideColumn("idVendaProduto2");
  ui->tableTranspAtual->hideColumn("idCompra");
  ui->tableTranspAtual->hideColumn("idNFeSaida");
  ui->tableTranspAtual->hideColumn("fornecedor");
  ui->tableTranspAtual->hideColumn("idVenda");
  ui->tableTranspAtual->hideColumn("idLoja");
  ui->tableTranspAtual->hideColumn("idProduto");
  ui->tableTranspAtual->hideColumn("obs");
  ui->tableTranspAtual->hideColumn("quantCaixa");
  ui->tableTranspAtual->hideColumn("formComercial");
  ui->tableTranspAtual->hideColumn("data");

  // -------------------------------------------------------------------------

  modelTranspAgend.setTable("veiculo_has_produto");

  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("observacao", "Observação");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("kg", "Kg");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");

  ui->tableTranspAgend->setModel(&modelTranspAgend);

  ui->tableTranspAgend->hideColumn("fotoEntrega");
  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idVendaProduto1");
  ui->tableTranspAgend->hideColumn("idVendaProduto2");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("fornecedor");
  ui->tableTranspAgend->hideColumn("idVenda");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("quantCaixa");
  ui->tableTranspAgend->hideColumn("obs");
  ui->tableTranspAgend->hideColumn("formComercial");

  // -------------------------------------------------------------------------

  modelFornecedor.setTable("view_fornecedor_logistica_agendar_coleta");

  modelFornecedor.setFilter("");

  ui->tableForn->setModel(&modelFornecedor);

  ui->tableForn->sortByColumn("Fornecedor");
}

void WidgetLogisticaAgendarColeta::calcularPeso() {
  double peso = 0;
  double caixas = 0;

  const auto selection = ui->tableEstoque->selectionModel()->selectedRows();

  for (const auto &index : selection) {
    const double kg = modelEstoque.data(index.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(index.row(), "caixas").toDouble();
    peso += kg * caixa;
    caixas += caixa;
  }

  ui->doubleSpinBoxPeso->setValue(peso);
  ui->doubleSpinBoxPeso->setSuffix(" Kg (" + QString::number(caixas) + " Cx.)");
}

void WidgetLogisticaAgendarColeta::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->frameCaminhao->hide();
    ui->pushButtonCancelarCarga->hide();
    ui->dateTimeEdit->setDate(qApp->serverDate());
    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelEstoque.select();
  modelTranspAgend.select();
  modelFornecedor.select();
}

void WidgetLogisticaAgendarColeta::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked() {
  if (not ui->frameCaminhao->isVisible()) {
    ui->frameCaminhao->show();
    ui->pushButtonAgendarColeta->hide();
    ui->pushButtonCancelarCarga->show();
    return;
  }

  if (modelTranspAtual.rowCount() == 0) { throw RuntimeError("Nenhum item no veículo!", this); }

  QModelIndexList list;

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) { list << modelTranspAtual.index(row, 0); }

  qApp->startTransaction("WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga");

  processRows(list, ui->dateTimeEdit->date());

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Agendado com sucesso!", this);

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();
}

void WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked() {
  // NOTE: quando não é montado carga as linhas selecionadas pulam a etapa de recebimento e vão direto para estoque
  const auto selection = ui->tableEstoque->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList ids;
  QStringList idVendas;

  for (const auto &index : selection) {
    ids << modelEstoque.data(index.row(), "idEstoque").toString();
    idVendas << modelEstoque.data(index.row(), "idVenda").toString();
  }

  InputDialogConfirmacao inputDlg(InputDialogConfirmacao::Tipo::Recebimento, this);
  inputDlg.setFilterRecebe(ids);

  if (inputDlg.exec() != InputDialogConfirmacao::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta");

  processRows(selection, inputDlg.getDate(), inputDlg.getRecebeu());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  ui->tableEstoque->clearSelection();

  updateTables();
  qApp->enqueueInformation("Confirmado recebimento!", this);
}

void WidgetLogisticaAgendarColeta::processRows(const QModelIndexList &list, const QDate dataReceb, const QString &recebidoPor) {
  SqlQuery queryEstoque;
  queryEstoque.prepare("UPDATE estoque SET status = 'ESTOQUE', idBloco = :idBloco, recebidoPor = :recebidoPor WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  SqlQuery queryConsumo;
  queryConsumo.prepare("UPDATE estoque_has_consumo SET status = 'CONSUMO', idBloco = :idBloco WHERE idEstoque = :idEstoque AND status = 'PRÉ-CONSUMO'");

  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM COLETA' AND "
                      "idPedido2 IN (SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM COLETA' AND "
                     "idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  SqlQuery queryGare;
  queryGare.prepare("UPDATE conta_a_pagar_has_pagamento SET status = 'LIBERADO GARE', dataPagamento = :dataRealReceb WHERE status = 'PENDENTE GARE' AND idNFe IN (SELECT idNFe FROM estoque WHERE "
                    "idEstoque = :idEstoque)");

  SqlQuery queryNFe;
  queryNFe.prepare("UPDATE nfe SET confirmar = TRUE WHERE idNFe = :idNFe AND nsu IS NOT NULL AND statusDistribuicao = 'CIÊNCIA'");

  SqlQuery queryBloco;

  if (not queryBloco.exec("SELECT idBloco FROM galpao WHERE label = 'ENTRADA'")) { throw RuntimeException("Erro buscando id do bloco de entrada: " + queryBloco.lastError().text()); }

  if (not queryBloco.first()) { throw RuntimeException("Bloco de entrada não encontrado!"); }

  const int idBloco = queryBloco.value("idBloco").toInt();

  for (const auto &index : list) {
    const bool isCD = (modelEstoque.data(index.row(), "local").toString() == "CD");

    queryEstoque.bindValue(":idBloco", (isCD) ? idBloco : QVariant());
    queryEstoque.bindValue(":recebidoPor", recebidoPor);
    queryEstoque.bindValue(":idEstoque", modelEstoque.data(index.row(), "idEstoque"));

    if (not queryEstoque.exec()) { throw RuntimeException("Erro atualizando status do estoque: " + queryEstoque.lastError().text()); }

    //-----------------------------------------------------------------

    queryConsumo.bindValue(":idEstoque", modelEstoque.data(index.row(), "idEstoque"));
    queryConsumo.bindValue(":idBloco", (isCD) ? idBloco : QVariant());

    if (not queryConsumo.exec()) { throw RuntimeException("Erro atualizando status da venda: " + queryConsumo.lastError().text()); }

    //-----------------------------------------------------------------

    queryCompra.bindValue(":dataRealReceb", dataReceb);
    queryCompra.bindValue(":idEstoque", modelEstoque.data(index.row(), "idEstoque"));
    queryCompra.bindValue(":codComercial", modelEstoque.data(index.row(), "codComercial"));

    if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando status da compra: " + queryCompra.lastError().text()); }

    //-----------------------------------------------------------------

    queryVenda.bindValue(":dataRealReceb", dataReceb);
    queryVenda.bindValue(":idEstoque", modelEstoque.data(index.row(), "idEstoque"));

    if (not queryVenda.exec()) { throw RuntimeException("Erro atualizando produtos venda: " + queryVenda.lastError().text()); }

    //-----------------------------------------------------------------

    queryGare.bindValue(":dataRealReceb", qApp->ajustarDiaUtil(dataReceb.addDays(1)));
    queryGare.bindValue(":idEstoque", modelEstoque.data(index.row(), "idEstoque"));

    if (not queryGare.exec()) { throw RuntimeException("Erro atualizando pagamento gare: " + queryGare.lastError().text()); }

    //-----------------------------------------------------------------

    queryNFe.bindValue(":idNFe", modelEstoque.data(index.row(), "idNFe"));

    if (not queryNFe.exec()) { throw RuntimeException("Erro marcando NF-e para confirmar: " + queryNFe.lastError().text()); }
  }
}

void WidgetLogisticaAgendarColeta::processRows(const QModelIndexList &list, const QDate dataPrevColeta) {
  SqlQuery queryTemp;
  queryTemp.prepare("SELECT codComercial FROM estoque WHERE idEstoque = :idEstoque");

  SqlQuery query2;
  query2.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE status = 'EM COLETA' AND idPedido2 IN (SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  SqlQuery query3;
  query3.prepare("UPDATE venda_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE status = 'EM COLETA' AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = "
                 ":idEstoque)");

  for (const auto &index : list) {
    int idEstoque;

    SqlQuery query;
    if (not query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto")) { throw RuntimeException("Erro buscando próximo idEvento: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("Erro buscando próximo idEvento!"); }

    const int idEvento = query.value(0).toInt();

    //-------------------------------------------------

    modelTranspAtual.setData(index.row(), "data", dataPrevColeta);
    modelTranspAtual.setData(index.row(), "idEvento", idEvento);

    //-------------------------------------------------

    idEstoque = modelTranspAtual.data(index.row(), "idEstoque").toInt();

    queryTemp.bindValue(":idEstoque", idEstoque);

    if (not queryTemp.exec()) { throw RuntimeException("Erro buscando codComercial do estoque: " + queryTemp.lastError().text()); }

    if (not queryTemp.first()) { throw RuntimeException("Dados não encontrados para estoque de id: '" + QString::number(idEstoque) + "'"); }
    // TODO: codComercial é selecionado mas não é usado, essa query é apenas para verificar se existe codComercial?

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query2.lastError().text()); }

    //-------------------------------------------------

    query3.bindValue(":dataPrevColeta", dataPrevColeta);
    query3.bindValue(":idEstoque", idEstoque);

    if (not query3.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query3.lastError().text()); }
  }

  modelTranspAtual.submitAll();
}

void WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged() {
  SqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getId());

  if (not query.exec()) { throw RuntimeException("Erro buscando dados do veículo: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para veículo de id: '" + ui->itemBoxVeiculo->getId().toString() + "'", this); }

  modelTranspAtual.select();

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  modelTranspAgend.select();

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());
}

void WidgetLogisticaAgendarColeta::adicionarProduto(const QModelIndexList &list) {
  for (const auto &index : list) {
    const double kg = modelEstoque.data(index.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(index.row(), "caixas").toDouble();
    const double peso = kg * caixa;

    const int row = modelTranspAtual.insertRowAtEnd();

    modelTranspAtual.setData(row, "fornecedor", modelEstoque.data(index.row(), "fornecedor"));
    modelTranspAtual.setData(row, "quantCaixa", modelEstoque.data(index.row(), "quantCaixa"));
    modelTranspAtual.setData(row, "formComercial", modelEstoque.data(index.row(), "formComercial"));
    modelTranspAtual.setData(row, "idVeiculo", ui->itemBoxVeiculo->getId());
    modelTranspAtual.setData(row, "idEstoque", modelEstoque.data(index.row(), "idEstoque"));
    modelTranspAtual.setData(row, "idProduto", modelEstoque.data(index.row(), "idProduto"));
    modelTranspAtual.setData(row, "produto", modelEstoque.data(index.row(), "produto"));
    modelTranspAtual.setData(row, "codComercial", modelEstoque.data(index.row(), "codComercial"));
    modelTranspAtual.setData(row, "un", modelEstoque.data(index.row(), "un"));
    modelTranspAtual.setData(row, "caixas", modelEstoque.data(index.row(), "caixas"));
    modelTranspAtual.setData(row, "kg", peso);
    modelTranspAtual.setData(row, "quant", modelEstoque.data(index.row(), "quant"));
    modelTranspAtual.setData(row, "status", "EM COLETA");
  }
}

void WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { throw RuntimeError("Deve escolher uma transportadora antes!", this); }

  const auto selection = ui->tableEstoque->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxCapacidade->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!", this); }

  for (const auto &index : selection) {
    const auto match = modelTranspAtual.match("idEstoque", modelEstoque.data(index.row(), "idEstoque"), 1, Qt::MatchExactly);

    if (not match.isEmpty()) { throw RuntimeError("Item '" + modelEstoque.data(index.row(), "produto").toString() + "' já inserido!", this); }
  }

  adicionarProduto(selection);
}

void WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked() {
  const auto selection = ui->tableTranspAtual->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  modelTranspAtual.removeSelection(selection);
}

void WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();

  modelTranspAtual.select();
}

void WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged(const QDate date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");
}

void WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled() { montaFiltro(); }

void WidgetLogisticaAgendarColeta::montaFiltro() {
  QStringList filtros;

  //-------------------------------------

  const auto selection = ui->tableForn->selectionModel()->selectedRows();

  if (not selection.isEmpty()) {
    const QString fornecedor = modelFornecedor.data(selection.first().row(), "fornecedor").toString();

    const QString filtroFornecedor = "fornecedor = '" + fornecedor + "'";

    if (not fornecedor.isEmpty()) { filtros << filtroFornecedor; }
  }

  //-------------------------------------

  const QString filtroEstoque = "idVenda " + QString(ui->checkBoxEstoque->isChecked() ? "IS NULL" : "IS NOT NULL");

  if (not filtroEstoque.isEmpty()) { filtros << filtroEstoque; }

  //-------------------------------------

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')";

  if (not text.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelEstoque.setFilter(filtros.join(" AND "));
}

void WidgetLogisticaAgendarColeta::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tableEstoque->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const QString idEstoque = modelEstoque.data(selection.first().row(), "idEstoque").toString();

  auto *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetLogisticaAgendarColeta::on_tableEstoque_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelEstoque.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Estoque") { return qApp->abrirEstoque(modelEstoque.data(index.row(), "idEstoque")); }

  if (header == "NF-e") { return qApp->abrirNFe(modelEstoque.data(index.row(), "idNFe")); }

  if (header == "Venda") {
    const QStringList ids = modelEstoque.data(index.row(), "idVenda").toString().split(", ");

    for (const auto &id : ids) { qApp->abrirVenda(id); }

    return;
  }

  if (header == "O.C.") { return qApp->abrirCompra(modelEstoque.data(index.row(), "ordemCompra")); }
}

void WidgetLogisticaAgendarColeta::on_pushButtonLimparFiltro_clicked() {
  ui->tableEstoque->clearSelection();
  ui->tableForn->clearSelection();
}

void WidgetLogisticaAgendarColeta::on_tableForn_selectionChanged() {
  const auto selection = ui->tableForn->selectionModel()->selectedRows();

  ui->lineEditBusca->clear();

  montaFiltro();
}

// TODO: 5importar nota de amostra nesta tela dizendo para qual loja ela vai e no final do fluxo gerar nota de tranferencia
// TODO: colocar um botao nessa tela para quando o produto ja chegou e poder marcar direto como recebido
// TODO: remover o filtro 'estoque' nessa tela de modo de quando buscar uma NF-e apareca todas as linhas, seja para um pedido ou estoqueLoja
