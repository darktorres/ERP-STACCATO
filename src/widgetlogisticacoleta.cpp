#include "widgetlogisticacoleta.h"
#include "ui_widgetlogisticacoleta.h"

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

WidgetLogisticaColeta::WidgetLogisticaColeta(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaColeta) { ui->setupUi(this); }

WidgetLogisticaColeta::~WidgetLogisticaColeta() { delete ui; }

void WidgetLogisticaColeta::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaColeta::on_checkBoxMarcarTodos_clicked, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetLogisticaColeta::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonLimparFiltro_clicked, connectionType);
  connect(ui->pushButtonMarcarColetado, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonMarcarColetado_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->table, &QTableView::doubleClicked, this, &WidgetLogisticaColeta::on_table_doubleClicked, connectionType);
  connect(ui->tableForn->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaColeta::on_tableForn_selectionChanged, connectionType);
}

void WidgetLogisticaColeta::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelColeta.select();
  modelFornecedor.select();
}

void WidgetLogisticaColeta::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetLogisticaColeta::setupTables() {
  modelColeta.setTable("view_coleta");

  modelColeta.setSort("prazoEntrega");

  modelColeta.setHeaderData("prazoEntrega", "Prazo Limite");
  modelColeta.setHeaderData("dataPrevColeta", "Data Prev. Col.");
  modelColeta.setHeaderData("idEstoque", "Estoque");
  modelColeta.setHeaderData("lote", "Lote");
  modelColeta.setHeaderData("local", "Local");
  modelColeta.setHeaderData("bloco", "Bloco");
  modelColeta.setHeaderData("numeroNFe", "NF-e");
  modelColeta.setHeaderData("idVenda", "Venda");
  modelColeta.setHeaderData("ordemCompra", "O.C.");
  modelColeta.setHeaderData("codFornecedor", "Cód. Forn.");
  modelColeta.setHeaderData("produto", "Produto");
  modelColeta.setHeaderData("codComercial", "Cód. Com.");
  modelColeta.setHeaderData("quant", "Quant.");
  modelColeta.setHeaderData("un", "Un.");
  modelColeta.setHeaderData("caixas", "Caixas");
  modelColeta.setHeaderData("kgcx", "Kg./Cx.");

  modelColeta.proxyModel = new EstoquePrazoProxyModel(&modelColeta, this);

  ui->table->setModel(&modelColeta);

  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("idCompra");

  // -------------------------------------------------------------------------

  modelFornecedor.setTable("view_fornecedor_logistica_coleta");

  modelFornecedor.setFilter("");

  ui->tableForn->setModel(&modelFornecedor);

  ui->tableForn->sortByColumn("Fornecedor");
}

void WidgetLogisticaColeta::on_pushButtonMarcarColetado_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList idVendas;

  for (const auto &index : selection) { idVendas << modelColeta.data(index.row(), "idVenda").toString(); }

  InputDialog input(InputDialog::Tipo::Coleta, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaColeta::on_pushButtonMarcarColetado");

  cadastrar(selection, input.getDate(), input.getNextDate());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  ui->table->clearSelection();

  updateTables();
  qApp->enqueueInformation("Confirmado coleta!", this);
}

void WidgetLogisticaColeta::cadastrar(const QModelIndexList &list, const QDate dataColeta, const QDate dataPrevReceb) {
  SqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM RECEBIMENTO' WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  SqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM RECEBIMENTO', dataRealColeta = :dataRealColeta, dataPrevReceb = :dataPrevReceb WHERE status = 'EM COLETA' AND idPedido2 IN "
                 "(SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  SqlQuery query3;
  // salvar status na venda
  query3.prepare("UPDATE venda_has_produto2 SET status = 'EM RECEBIMENTO', dataRealColeta = :dataRealColeta, dataPrevReceb = :dataPrevReceb WHERE status = 'EM COLETA' AND idVendaProduto2 IN (SELECT "
                 "idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  SqlQuery query4;
  query4.prepare("UPDATE veiculo_has_produto SET status = 'COLETADO' WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  for (const auto &index : list) {
    query1.bindValue(":idEstoque", modelColeta.data(index.row(), "idEstoque"));

    if (not query1.exec()) { throw RuntimeException("Erro salvando status do estoque: " + query1.lastError().text()); }

    query2.bindValue(":dataRealColeta", dataColeta);
    query2.bindValue(":dataPrevReceb", dataPrevReceb);
    query2.bindValue(":idEstoque", modelColeta.data(index.row(), "idEstoque"));

    if (not query2.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query2.lastError().text()); }

    query3.bindValue(":dataRealColeta", dataColeta);
    query3.bindValue(":dataPrevReceb", dataPrevReceb);
    query3.bindValue(":idEstoque", modelColeta.data(index.row(), "idEstoque"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando status da compra: " + query3.lastError().text()); }

    // -------------------------------------------------------------------------

    query4.bindValue(":idEstoque", modelColeta.data(index.row(), "idEstoque"));

    if (not query4.exec()) { throw RuntimeException("Erro atualizando veiculo_has_produto: " + query4.lastError().text()); }
  }
}

void WidgetLogisticaColeta::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void WidgetLogisticaColeta::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetLogisticaColeta::montaFiltro() {
  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());

  modelColeta.setFilter("(numeroNFe LIKE '%" + textoBusca + "%' OR produto LIKE '%" + textoBusca + "%' OR idVenda LIKE '%" + textoBusca + "%' OR ordemCompra LIKE '%" + textoBusca + "%')");
}

void WidgetLogisticaColeta::on_pushButtonReagendar_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarColeta, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaColeta::on_pushButtonReagendar");

  reagendar(selection, input.getNextDate());

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

void WidgetLogisticaColeta::reagendar(const QModelIndexList &list, const QDate dataPrevColeta) {
  SqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM COLETA'");

  SqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque = :idEstoque) "
                 "AND status = 'EM COLETA'");

  SqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = :data WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  for (const auto &index : list) {
    const int idEstoque = modelColeta.data(index.row(), "idEstoque").toInt();
    const QString codComercial = modelColeta.data(index.row(), "codComercial").toString();

    query1.bindValue(":dataPrevColeta", dataPrevColeta);
    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }

    query3.bindValue(":data", dataPrevColeta);
    query3.bindValue(":idEstoque", modelColeta.data(index.row(), "idEstoque"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando data no veiculo: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaColeta::cancelar(const QModelIndexList &list) {
  SqlQuery query1;
  query1.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = NULL WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque = :idEstoque) AND status = 'EM COLETA'");

  SqlQuery query2;
  query2.prepare(
      "UPDATE venda_has_produto2 SET dataPrevColeta = NULL WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque = :idEstoque) AND status = 'EM COLETA'");

  SqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = NULL WHERE status = 'EM COLETA' AND idEstoque = :idEstoque");

  for (const auto &index : list) {
    const int idEstoque = modelColeta.data(index.row(), "idEstoque").toInt();
    const QString codComercial = modelColeta.data(index.row(), "codComercial").toString();

    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }

    query3.bindValue(":idEstoque", modelColeta.data(index.row(), "idEstoque"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando data no veiculo: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaColeta::on_pushButtonCancelar_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  qApp->startTransaction("WidgetLogisticaColeta::on_pushButtonCancelar");

  cancelar(selection);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Cancelado com sucesso!", this);
}

void WidgetLogisticaColeta::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const QString idEstoque = modelColeta.data(selection.first().row(), "idEstoque").toString();

  auto *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetLogisticaColeta::on_table_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelColeta.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Estoque") { return qApp->abrirEstoque(modelColeta.data(index.row(), "idEstoque")); }

  if (header == "NF-e") { return qApp->abrirNFe(modelColeta.data(index.row(), "idNFe")); }

  if (header == "Venda") {
    QStringList ids = modelColeta.data(index.row(), "idVenda").toString().split(", ");

    for (const auto &id : ids) { qApp->abrirVenda(id); }

    return;
  }

  if (header == "O.C.") { return qApp->abrirCompra(modelColeta.data(index.row(), "ordemCompra")); }
}

void WidgetLogisticaColeta::on_pushButtonLimparFiltro_clicked() {
  ui->table->clearSelection();
  ui->tableForn->clearSelection();
}

void WidgetLogisticaColeta::on_tableForn_selectionChanged() {
  const auto selection = ui->tableForn->selectionModel()->selectedRows();

  ui->lineEditBusca->clear();

  const QString fornecedor = (selection.isEmpty()) ? "" : modelFornecedor.data(selection.first().row(), "fornecedor").toString();

  const QString filtro = (fornecedor.isEmpty()) ? "" : "fornecedor = '" + fornecedor + "'";

  modelColeta.setFilter(filtro);

  ui->checkBoxMarcarTodos->setChecked(false);
}
