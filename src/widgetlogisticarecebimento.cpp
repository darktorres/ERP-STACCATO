#include "widgetlogisticarecebimento.h"
#include "ui_widgetlogisticarecebimento.h"

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "venda.h"

#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

WidgetLogisticaRecebimento::WidgetLogisticaRecebimento(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaRecebimento) { ui->setupUi(this); }

WidgetLogisticaRecebimento::~WidgetLogisticaRecebimento() { delete ui; }

void WidgetLogisticaRecebimento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaRecebimento::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonMarcarRecebido, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonVenda_clicked, connectionType);
}

void WidgetLogisticaRecebimento::updateTables() {
  if (not isSet) {
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

void WidgetLogisticaRecebimento::processRows(const QModelIndexList &list, const QDate &dataReceb, const QString &recebidoPor) {
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'ESTOQUE', bloco = :bloco, recebidoPor = :recebidoPor WHERE status = 'EM RECEBIMENTO' AND idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE estoque_has_consumo SET status = 'CONSUMO', bloco = :bloco WHERE idEstoque = :idEstoque AND status = 'PRÉ-CONSUMO'");

  QSqlQuery query3;
  query3.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM RECEBIMENTO' AND "
                 "idPedido2 IN (SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  QSqlQuery query4;
  query4.prepare("UPDATE venda_has_produto2 SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM RECEBIMENTO' AND "
                 "idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  QSqlQuery query5;
  query5.prepare("UPDATE conta_a_pagar_has_pagamento SET status = 'LIBERADO GARE', dataPagamento = :dataRealReceb WHERE status = 'PENDENTE GARE' AND idNFe IN (SELECT idNFe FROM estoque WHERE "
                 "idEstoque = :idEstoque)");

  QSqlQuery query6;
  query6.prepare("UPDATE nfe SET confirmar = TRUE WHERE idNFe = :idNFe AND nsu IS NOT NULL AND statusDistribuicao = 'CIÊNCIA'");

  for (const auto &index : list) {
    const bool isCD = (modelViewRecebimento.data(index.row(), "local").toString() == "CD");

    query1.bindValue(":bloco", (isCD) ? "ENTRADA" : "");
    query1.bindValue(":recebidoPor", recebidoPor);
    query1.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));

    if (not query1.exec()) { throw RuntimeException("Erro atualizando status do estoque: " + query1.lastError().text()); }

    //-----------------------------------------------------------------

    query2.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));
    query2.bindValue(":bloco", (isCD) ? "ENTRADA" : "");

    if (not query2.exec()) { throw RuntimeException("Erro atualizando status da venda: " + query2.lastError().text()); }

    //-----------------------------------------------------------------

    query3.bindValue(":dataRealReceb", dataReceb);
    query3.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));
    query3.bindValue(":codComercial", modelViewRecebimento.data(index.row(), "codComercial"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando status da compra: " + query3.lastError().text()); }

    //-----------------------------------------------------------------

    query4.bindValue(":dataRealReceb", dataReceb);
    query4.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));

    if (not query4.exec()) { throw RuntimeException("Erro atualizando produtos venda: " + query4.lastError().text()); }

    //-----------------------------------------------------------------

    query5.bindValue(":dataRealReceb", qApp->ajustarDiaUtil(dataReceb.addDays(1)));
    query5.bindValue(":idEstoque", modelViewRecebimento.data(index.row(), "idEstoque"));

    if (not query5.exec()) { throw RuntimeException("Erro atualizando pagamento gare: " + query5.lastError().text()); }

    //-----------------------------------------------------------------

    query6.bindValue(":idNFe", modelViewRecebimento.data(index.row(), "idNFe"));

    if (not query6.exec()) { throw RuntimeException("Erro marcando NFe para confirmar: " + query6.lastError().text()); }
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

void WidgetLogisticaRecebimento::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaRecebimento::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  modelViewRecebimento.setFilter("(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')");
}

void WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarRecebimento, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaRecebimento::on_pushButtonReagendar");

  reagendar(list, input.getNextDate());

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

void WidgetLogisticaRecebimento::reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) {
  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevReceb = :dataPrevReceb WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM RECEBIMENTO'");

  QSqlQuery query2;
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
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM COLETA' WHERE status = 'EM RECEBIMENTO' AND idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE status = 'EM RECEBIMENTO' AND idPedido2 IN (SELECT idPedido2 FROM "
                 "estoque_has_compra WHERE idEstoque = :idEstoque)");

  QSqlQuery query3;
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

// TODO: 0marcar em qual bloco foi guardado (opcional?)
