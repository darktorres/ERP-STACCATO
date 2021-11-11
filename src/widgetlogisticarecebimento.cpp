#include "widgetlogisticarecebimento.h"
#include "ui_widgetlogisticarecebimento.h"

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "sqlquery.h"
#include "venda.h"

#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

WidgetLogisticaRecebimento::WidgetLogisticaRecebimento(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaRecebimento) { ui->setupUi(this); }

WidgetLogisticaRecebimento::~WidgetLogisticaRecebimento() { delete ui; }

void WidgetLogisticaRecebimento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetLogisticaRecebimento::on_lineEditBusca_textChanged, connectionType);
  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaRecebimento::delayFiltro, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonMarcarRecebido, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonVenda_clicked, connectionType);
}

void WidgetLogisticaRecebimento::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewRecebimento.select();
}

void WidgetLogisticaRecebimento::delayFiltro() { timer.start(qApp->delayedTimer); }

void WidgetLogisticaRecebimento::resetTables() { modelIsSet = false; }

void WidgetLogisticaRecebimento::tableFornLogistica_clicked(const QString &fornecedor) {
  ui->lineEditBusca->clear();

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelViewRecebimento.setFilter(filtro);

  ui->checkBoxMarcarTodos->setChecked(false);
}

void WidgetLogisticaRecebimento::setupTables() {
  modelViewRecebimento.setTable("view_recebimento");

  modelViewRecebimento.setSort("prazoEntrega");

  modelViewRecebimento.setHeaderData("prazoEntrega", "Prazo Limite");
  modelViewRecebimento.setHeaderData("dataPrevReceb", "Data Prev. Rec.");
  modelViewRecebimento.setHeaderData("idEstoque", "Estoque");
  modelViewRecebimento.setHeaderData("lote", "Lote");
  modelViewRecebimento.setHeaderData("local", "Local");
  modelViewRecebimento.setHeaderData("bloco", "Bloco");
  modelViewRecebimento.setHeaderData("numeroNFe", "NFe");
  modelViewRecebimento.setHeaderData("idVenda", "Venda");
  modelViewRecebimento.setHeaderData("ordemCompra", "OC");
  modelViewRecebimento.setHeaderData("produto", "Produto");
  modelViewRecebimento.setHeaderData("codComercial", "Cód. Com.");
  modelViewRecebimento.setHeaderData("quant", "Quant.");
  modelViewRecebimento.setHeaderData("un", "Un.");
  modelViewRecebimento.setHeaderData("caixas", "Caixas");

  modelViewRecebimento.proxyModel = new EstoquePrazoProxyModel(&modelViewRecebimento, this);

  ui->table->setModel(&modelViewRecebimento);

  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("idNFe");
}

void WidgetLogisticaRecebimento::processRows(const QModelIndexList &list, const QDate dataReceb, const QString &recebidoPor) {
  SqlQuery queryEstoque;
  queryEstoque.prepare("UPDATE estoque SET status = 'ESTOQUE', idBloco = :idBloco, recebidoPor = :recebidoPor WHERE status = 'EM RECEBIMENTO' AND idEstoque = :idEstoque");

  SqlQuery queryConsumo;
  queryConsumo.prepare("UPDATE estoque_has_consumo SET status = 'CONSUMO', idBloco = :idBloco WHERE idEstoque = :idEstoque AND status = 'PRÉ-CONSUMO'");

  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM RECEBIMENTO' AND "
                      "idPedido2 IN (SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM RECEBIMENTO' AND "
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
    const bool isCD = (modelViewRecebimento.data(index.row(), "local").toString() == "CD");

    queryEstoque.bindValue(":idBloco", (isCD) ? idBloco : QVariant());
    queryEstoque.bindValue(":recebidoPor", recebidoPor);
    queryEstoque.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));

    if (not queryEstoque.exec()) { throw RuntimeException("Erro atualizando status do estoque: " + queryEstoque.lastError().text()); }

    //-----------------------------------------------------------------

    queryConsumo.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));
    queryConsumo.bindValue(":idBloco", (isCD) ? idBloco : QVariant());

    if (not queryConsumo.exec()) { throw RuntimeException("Erro atualizando status da venda: " + queryConsumo.lastError().text()); }

    //-----------------------------------------------------------------

    queryCompra.bindValue(":dataRealReceb", dataReceb);
    queryCompra.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));
    queryCompra.bindValue(":codComercial", modelViewRecebimento.data(index.row(), "codComercial"));

    if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando status da compra: " + queryCompra.lastError().text()); }

    //-----------------------------------------------------------------

    queryVenda.bindValue(":dataRealReceb", dataReceb);
    queryVenda.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));

    if (not queryVenda.exec()) { throw RuntimeException("Erro atualizando produtos venda: " + queryVenda.lastError().text()); }

    //-----------------------------------------------------------------

    queryGare.bindValue(":dataRealReceb", qApp->ajustarDiaUtil(dataReceb.addDays(1)));
    queryGare.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));

    if (not queryGare.exec()) { throw RuntimeException("Erro atualizando pagamento gare: " + queryGare.lastError().text()); }

    //-----------------------------------------------------------------

    queryNFe.bindValue(":idNFe", modelViewRecebimento.data(index.row(), "idNFe"));

    if (not queryNFe.exec()) { throw RuntimeException("Erro marcando NFe para confirmar: " + queryNFe.lastError().text()); }
  }
}

void WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList ids;
  QStringList idVendas;

  for (const auto &index : list) {
    ids << modelViewRecebimento.data(index.row(), "idEstoque").toString();
    idVendas << modelViewRecebimento.data(index.row(), "idVenda").toString();
  }

  InputDialogConfirmacao inputDlg(InputDialogConfirmacao::Tipo::Recebimento, this);
  inputDlg.setFilterRecebe(ids);

  if (inputDlg.exec() != InputDialogConfirmacao::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido");

  processRows(list, inputDlg.getDate(), inputDlg.getRecebeu());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Confirmado recebimento!", this);
}

void WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void WidgetLogisticaRecebimento::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetLogisticaRecebimento::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  modelViewRecebimento.setFilter("(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')");
}

void WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::ReagendarRecebimento, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaRecebimento::on_pushButtonReagendar");

  reagendar(list, input.getNextDate());

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

void WidgetLogisticaRecebimento::reagendar(const QModelIndexList &list, const QDate dataPrevReceb) {
  SqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevReceb = :dataPrevReceb WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM RECEBIMENTO'");

  SqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET dataPrevReceb = :dataPrevReceb WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM RECEBIMENTO'");

  for (const auto &index : list) {
    const int idEstoque = modelViewRecebimento.data(index.row(), "idEstoque").toInt();
    const QString codComercial = modelViewRecebimento.data(index.row(), "codComercial").toString();

    query1.bindValue(":dataPrevReceb", dataPrevReceb);
    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":dataPrevReceb", dataPrevReceb);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }
  }
}

void WidgetLogisticaRecebimento::on_pushButtonVenda_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  for (const auto &index : list) {
    const QString idVenda = modelViewRecebimento.data(index.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) { return; }

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
      venda->show();
    }
  }
}

void WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelViewRecebimento.data(index.row(), "idVenda").toString(); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  qApp->startTransaction("WidgetLogisticaRecebimento::on_pushButtonCancelar");

  cancelar(list);

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Cancelado com sucesso!", this);
}

void WidgetLogisticaRecebimento::cancelar(const QModelIndexList &list) {
  SqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM COLETA' WHERE status = 'EM RECEBIMENTO' AND idEstoque = :idEstoque");

  SqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE status = 'EM RECEBIMENTO' AND idPedido2 IN (SELECT idPedido2 FROM "
                 "estoque_has_compra WHERE idEstoque = :idEstoque)");

  SqlQuery query3;
  query3.prepare("UPDATE venda_has_produto2 SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE status = 'EM RECEBIMENTO' AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM "
                 "estoque_has_consumo WHERE idEstoque = :idEstoque)");

  for (const auto &index : list) {
    const int idEstoque = modelViewRecebimento.data(index.row(), "idEstoque").toInt();
    const QString codComercial = modelViewRecebimento.data(index.row(), "codComercial").toString();

    query1.bindValue(":idEstoque", idEstoque);

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no estoque: " + query1.lastError().text()); }

    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query2.lastError().text()); }

    query3.bindValue(":idEstoque", idEstoque);
    query3.bindValue(":codComercial", codComercial);

    if (not query3.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaRecebimento::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeException("Nenhuma linha selecionada!"); }

  const QString idEstoque = modelViewRecebimento.data(selection.first().row(), "idEstoque").toString();

  auto *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: 0marcar em qual bloco foi guardado (opcional?)
