#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "ui_widgetlogisticarecebimento.h"
#include "venda.h"
#include "widgetlogisticarecebimento.h"

WidgetLogisticaRecebimento::WidgetLogisticaRecebimento(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaRecebimento) { ui->setupUi(this); }

WidgetLogisticaRecebimento::~WidgetLogisticaRecebimento() { delete ui; }

void WidgetLogisticaRecebimento::setConnections() {
  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaRecebimento::on_lineEditBusca_textChanged);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonMarcarRecebido, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonVenda_clicked);
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

  if (not modelViewRecebimento.select()) { return; }
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

  modelViewRecebimento.setHeaderData("idEstoque", "Estoque");
  modelViewRecebimento.setHeaderData("lote", "Lote");
  modelViewRecebimento.setHeaderData("local", "Local");
  modelViewRecebimento.setHeaderData("bloco", "Bloco");
  modelViewRecebimento.setHeaderData("numeroNFe", "NFe");
  modelViewRecebimento.setHeaderData("produto", "Produto");
  modelViewRecebimento.setHeaderData("quant", "Quant.");
  modelViewRecebimento.setHeaderData("un", "Un.");
  modelViewRecebimento.setHeaderData("caixas", "Caixas");
  modelViewRecebimento.setHeaderData("idVenda", "Venda");
  modelViewRecebimento.setHeaderData("codComercial", "Cód. Com.");
  modelViewRecebimento.setHeaderData("ordemCompra", "OC");
  modelViewRecebimento.setHeaderData("local", "Local");
  modelViewRecebimento.setHeaderData("dataPrevReceb", "Data Prev. Rec.");
  modelViewRecebimento.setHeaderData("prazoEntrega", "Prazo Limite");

  modelViewRecebimento.proxyModel = new EstoquePrazoProxyModel(&modelViewRecebimento, this);

  ui->table->setModel(&modelViewRecebimento);

  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("ordemCompra");
}

bool WidgetLogisticaRecebimento::processRows(const QModelIndexList &list, const QDateTime &dataReceb, const QString &recebidoPor) {
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'ESTOQUE', recebidoPor = :recebidoPor WHERE status = 'EM RECEBIMENTO' AND idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE estoque_has_consumo SET status = 'CONSUMO' WHERE status = 'PRÉ-CONSUMO' AND idEstoque = :idEstoque");

  QSqlQuery query3;
  query3.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM RECEBIMENTO' AND "
                 "idPedido IN (SELECT idPedido FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  QSqlQuery query4;
  // salvar status na venda
  query4.prepare("UPDATE venda_has_produto SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE status = 'EM RECEBIMENTO' AND "
                 "idVendaProduto IN (SELECT idVendaProduto FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  for (const auto &item : list) {
    query1.bindValue(":recebidoPor", recebidoPor);
    query1.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro atualizando status do estoque: " + query1.lastError().text(), this); }

    query2.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando status da venda: " + query2.lastError().text(), this); }

    query3.bindValue(":dataRealReceb", dataReceb);
    query3.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));
    query3.bindValue(":codComercial", modelViewRecebimento.data(item.row(), "codComercial"));

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + query3.lastError().text(), this); }

    query4.bindValue(":dataRealReceb", dataReceb);
    query4.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));

    if (not query4.exec()) { return qApp->enqueueError(false, "Erro atualizando produtos venda: " + query4.lastError().text(), this); }
  }

  return true;
}

void WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked() {
  // TODO: 1gerar gare da nota de entrada

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  QStringList ids;
  QStringList idVendas;

  for (const auto &item : list) {
    ids << modelViewRecebimento.data(item.row(), "idEstoque").toString();
    idVendas << modelViewRecebimento.data(item.row(), "idVenda").toString();
  }

  InputDialogConfirmacao inputDlg(InputDialogConfirmacao::Tipo::Recebimento, this);
  inputDlg.setFilterRecebe(ids);

  if (inputDlg.exec() != InputDialogConfirmacao::Accepted) { return; }

  const QDateTime dataReceb = inputDlg.getDateTime();
  const QString recebidoPor = inputDlg.getRecebeu();

  if (not qApp->startTransaction()) { return; }

  if (not processRows(list, dataReceb, recebidoPor)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVendas)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Confirmado recebimento!", this);
}

void WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void WidgetLogisticaRecebimento::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaRecebimento::montaFiltro() {
  const QString text = ui->lineEditBusca->text();

  modelViewRecebimento.setFilter("(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')");
}

void WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarRecebimento, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not reagendar(list, input.getNextDate())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

bool WidgetLogisticaRecebimento::reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) {
  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevReceb = :dataPrevReceb WHERE idPedido IN (SELECT idPedido FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET dataPrevReceb = :dataPrevReceb WHERE idVendaProduto IN (SELECT idVendaProduto FROM estoque_has_consumo WHERE idEstoque = :idEstoque) "
                 "AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  for (const auto &item : list) {
    const int idEstoque = modelViewRecebimento.data(item.row(), "idEstoque").toInt();
    const QString codComercial = modelViewRecebimento.data(item.row(), "codComercial").toString();

    query1.bindValue(":dataPrevReceb", dataPrevReceb);
    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro salvando status no pedido_fornecedor: " + query1.lastError().text(), this); }

    query2.bindValue(":dataPrevReceb", dataPrevReceb);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando status na venda_produto: " + query2.lastError().text(), this); }
  }

  return true;
}

void WidgetLogisticaRecebimento::on_pushButtonVenda_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  for (const auto &item : list) {
    const QString idVenda = modelViewRecebimento.data(item.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) { return; }

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
    }
  }
}

void WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelViewRecebimento.data(index.row(), "idVenda").toString(); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(list)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVendas)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Cancelado com sucesso!", this);
}

bool WidgetLogisticaRecebimento::cancelar(const QModelIndexList &list) {
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM COLETA' WHERE status = 'EM RECEBIMENTO' AND idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE status = 'EM RECEBIMENTO' AND idPedido IN (SELECT idPedido FROM "
                 "estoque_has_compra WHERE idEstoque = :idEstoque)");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE status = 'EM RECEBIMENTO' AND idVendaProduto IN (SELECT idVendaProduto FROM "
                 "estoque_has_consumo WHERE idEstoque = :idEstoque)");

  for (const auto &item : list) {
    const int idEstoque = modelViewRecebimento.data(item.row(), "idEstoque").toInt();
    const QString codComercial = modelViewRecebimento.data(item.row(), "codComercial").toString();

    query1.bindValue(":idEstoque", idEstoque);

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro salvando status no estoque: " + query1.lastError().text(), this); }

    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando status no pedido_fornecedor: " + query2.lastError().text(), this); }

    query3.bindValue(":idEstoque", idEstoque);
    query3.bindValue(":codComercial", codComercial);

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro salvando status na venda_produto: " + query3.lastError().text(), this); }
  }

  return true;
}

// TODO: 0marcar em qual bloco foi guardado (opcional?)
