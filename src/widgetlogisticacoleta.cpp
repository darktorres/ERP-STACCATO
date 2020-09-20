#include "widgetlogisticacoleta.h"
#include "ui_widgetlogisticacoleta.h"

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "sql.h"
#include "venda.h"

#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

WidgetLogisticaColeta::WidgetLogisticaColeta(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaColeta) { ui->setupUi(this); }

WidgetLogisticaColeta::~WidgetLogisticaColeta() { delete ui; }

void WidgetLogisticaColeta::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaColeta::on_checkBoxMarcarTodos_clicked, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaColeta::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonMarcarColetado, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonMarcarColetado_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonVenda_clicked, connectionType);
}

void WidgetLogisticaColeta::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewColeta.select();
}

void WidgetLogisticaColeta::tableFornLogistica_clicked(const QString &fornecedor) {
  ui->lineEditBusca->clear();

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelViewColeta.setFilter(filtro);

  ui->checkBoxMarcarTodos->setChecked(false);
}

void WidgetLogisticaColeta::resetTables() { modelIsSet = false; }

void WidgetLogisticaColeta::setupTables() {
  modelViewColeta.setTable("view_coleta");

  modelViewColeta.setSort("prazoEntrega");

  modelViewColeta.setHeaderData("prazoEntrega", "Prazo Limite");
  modelViewColeta.setHeaderData("dataPrevColeta", "Data Prev. Col.");
  modelViewColeta.setHeaderData("idEstoque", "Estoque");
  modelViewColeta.setHeaderData("lote", "Lote");
  modelViewColeta.setHeaderData("local", "Local");
  modelViewColeta.setHeaderData("bloco", "Bloco");
  modelViewColeta.setHeaderData("numeroNFe", "NFe");
  modelViewColeta.setHeaderData("idVenda", "Venda");
  modelViewColeta.setHeaderData("ordemCompra", "OC");
  modelViewColeta.setHeaderData("produto", "Produto");
  modelViewColeta.setHeaderData("codComercial", "Cód. Com.");
  modelViewColeta.setHeaderData("quant", "Quant.");
  modelViewColeta.setHeaderData("un", "Un.");
  modelViewColeta.setHeaderData("caixas", "Caixas");
  modelViewColeta.setHeaderData("kgcx", "Kg./Cx.");

  modelViewColeta.proxyModel = new EstoquePrazoProxyModel(&modelViewColeta, this);

  ui->table->setModel(&modelViewColeta);

  ui->table->hideColumn("fornecedor");
}

void WidgetLogisticaColeta::on_pushButtonMarcarColetado_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelViewColeta.data(index.row(), "idVenda").toString(); }

  InputDialog input(InputDialog::Tipo::Coleta, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaColeta::on_pushButtonMarcarColetado");

  cadastrar(list, input.getDate(), input.getNextDate());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Confirmado coleta!", this);
}

void WidgetLogisticaColeta::cadastrar(const QModelIndexList &list, const QDate &dataColeta, const QDate &dataPrevReceb) {
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM RECEBIMENTO' WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM RECEBIMENTO', dataRealColeta = :dataRealColeta, dataPrevReceb = :dataPrevReceb WHERE status = 'EM COLETA' AND idPedido2 IN "
                 "(SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  QSqlQuery query3;
  // salvar status na venda
  query3.prepare("UPDATE venda_has_produto2 SET status = 'EM RECEBIMENTO', dataRealColeta = :dataRealColeta, dataPrevReceb = :dataPrevReceb WHERE status = 'EM COLETA' AND idVendaProduto2 IN (SELECT "
                 "idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  QSqlQuery query4;
  query4.prepare("UPDATE veiculo_has_produto SET status = 'COLETADO' WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  for (const auto &index : list) {
    query1.bindValue(":idEstoque", modelViewColeta.data(index.row(), "idEstoque"));

    if (not query1.exec()) { throw RuntimeException("Erro salvando status do estoque: " + query1.lastError().text()); }

    query2.bindValue(":dataRealColeta", dataColeta);
    query2.bindValue(":dataPrevReceb", dataPrevReceb);
    query2.bindValue(":idEstoque", modelViewColeta.data(index.row(), "idEstoque"));

    if (not query2.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query2.lastError().text()); }

    query3.bindValue(":dataRealColeta", dataColeta);
    query3.bindValue(":dataPrevReceb", dataPrevReceb);
    query3.bindValue(":idEstoque", modelViewColeta.data(index.row(), "idEstoque"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando status da compra: " + query3.lastError().text()); }

    // -------------------------------------------------------------------------

    query4.bindValue(":idEstoque", modelViewColeta.data(index.row(), "idEstoque"));

    if (not query4.exec()) { throw RuntimeException("Erro atualizando veiculo_has_produto: " + query4.lastError().text()); }
  }
}

void WidgetLogisticaColeta::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void WidgetLogisticaColeta::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaColeta::montaFiltro() {
  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());

  modelViewColeta.setFilter("(numeroNFe LIKE '%" + textoBusca + "%' OR produto LIKE '%" + textoBusca + "%' OR idVenda LIKE '%" + textoBusca + "%' OR ordemCompra LIKE '%" + textoBusca + "%')");
}

void WidgetLogisticaColeta::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarColeta, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaColeta::on_pushButtonReagendar");

  reagendar(list, input.getNextDate());

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

void WidgetLogisticaColeta::reagendar(const QModelIndexList &list, const QDate &dataPrevColeta) {
  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM COLETA'");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM COLETA'");

  QSqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = :data WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  for (const auto &index : list) {
    const int idEstoque = modelViewColeta.data(index.row(), "idEstoque").toInt();
    const QString codComercial = modelViewColeta.data(index.row(), "codComercial").toString();

    query1.bindValue(":dataPrevColeta", dataPrevColeta);
    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }

    query3.bindValue(":data", dataPrevColeta);
    query3.bindValue(":idEstoque", modelViewColeta.data(index.row(), "idEstoque"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando data no veiculo: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaColeta::on_pushButtonVenda_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  for (const auto &index : list) {
    const QString idVenda = modelViewColeta.data(index.row(), "idVenda").toString();
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

void WidgetLogisticaColeta::cancelar(const QModelIndexList &list) {
  QSqlQuery query1;
  query1.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = NULL WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque = :idEstoque) AND status = 'EM COLETA'");

  QSqlQuery query2;
  query2.prepare(
      "UPDATE venda_has_produto2 SET dataPrevColeta = NULL WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque = :idEstoque) AND status = 'EM COLETA'");

  QSqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = NULL WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  for (const auto &index : list) {
    const int idEstoque = modelViewColeta.data(index.row(), "idEstoque").toInt();
    const QString codComercial = modelViewColeta.data(index.row(), "codComercial").toString();

    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }

    query3.bindValue(":idEstoque", modelViewColeta.data(index.row(), "idEstoque"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando data no veiculo: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaColeta::on_pushButtonCancelar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  qApp->startTransaction("WidgetLogisticaColeta::on_pushButtonCancelar");

  cancelar(list);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Cancelado com sucesso!", this);
}
