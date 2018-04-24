#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "sql.h"
#include "ui_widgetlogisticacoleta.h"
#include "venda.h"
#include "widgetlogisticacoleta.h"

WidgetLogisticaColeta::WidgetLogisticaColeta(QWidget *parent) : Widget(parent), ui(new Ui::WidgetLogisticaColeta) { ui->setupUi(this); }

WidgetLogisticaColeta::~WidgetLogisticaColeta() { delete ui; }

void WidgetLogisticaColeta::setConnections() {
  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaColeta::on_checkBoxMarcarTodos_clicked);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaColeta::on_lineEditBusca_textChanged);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonMarcarColetado, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonMarcarColetado_clicked);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonReagendar_clicked);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaColeta::on_pushButtonVenda_clicked);
  connect(ui->table, &TableView::entered, this, &WidgetLogisticaColeta::on_table_entered);
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

  if (not modelViewColeta.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetLogisticaColeta::tableFornLogistica_activated(const QString &fornecedor) {
  this->fornecedor = fornecedor;

  ui->lineEditBusca->clear();

  modelViewColeta.setFilter("fornecedor = '" + fornecedor + "'");

  if (not modelViewColeta.select()) { return; }

  ui->checkBoxMarcarTodos->setChecked(false);

  ui->table->resizeColumnsToContents();
}

void WidgetLogisticaColeta::resetTables() { modelIsSet = false; }

void WidgetLogisticaColeta::setupTables() {
  modelViewColeta.setTable("view_coleta");
  modelViewColeta.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelViewColeta.setHeaderData("idEstoque", "Estoque");
  modelViewColeta.setHeaderData("lote", "Lote");
  modelViewColeta.setHeaderData("local", "Local");
  modelViewColeta.setHeaderData("bloco", "Bloco");
  modelViewColeta.setHeaderData("numeroNFe", "NFe");
  modelViewColeta.setHeaderData("produto", "Produto");
  modelViewColeta.setHeaderData("codComercial", "Cód. Com.");
  modelViewColeta.setHeaderData("quant", "Quant.");
  modelViewColeta.setHeaderData("un", "Un.");
  modelViewColeta.setHeaderData("caixas", "Caixas");
  modelViewColeta.setHeaderData("kgcx", "Kg./Cx.");
  modelViewColeta.setHeaderData("idVenda", "Venda");
  modelViewColeta.setHeaderData("ordemCompra", "OC");
  modelViewColeta.setHeaderData("local", "Local");
  modelViewColeta.setHeaderData("dataPrevColeta", "Data Prev. Col.");
  modelViewColeta.setHeaderData("prazoEntrega", "Prazo Limite");

  modelViewColeta.setFilter("0");

  ui->table->setModel(new EstoquePrazoProxyModel(&modelViewColeta, this));
  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("prazoEntrega");
  ui->table->hideColumn("ordemCompra");
}

void WidgetLogisticaColeta::on_pushButtonMarcarColetado_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelViewColeta.data(index.row(), "idVenda").toString(); }

  InputDialog input(InputDialog::Tipo::Coleta);

  if (input.exec() != InputDialog::Accepted) { return; }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not cadastrar(list, input.getDate(), input.getNextDate())) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) { return; }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Confirmado coleta!");
}

bool WidgetLogisticaColeta::cadastrar(const QModelIndexList &list, const QDate &dataColeta, const QDate &dataPrevReceb) {
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM RECEBIMENTO' WHERE idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM RECEBIMENTO', dataRealColeta = :dataRealColeta, dataPrevReceb = :dataPrevReceb WHERE idCompra IN (SELECT idCompra FROM "
                 "estoque_has_compra WHERE idEstoque = :idEstoque) AND codComercial = :codComercial");

  QSqlQuery query3;
  // salvar status na venda
  query3.prepare("UPDATE venda_has_produto SET status = 'EM RECEBIMENTO', dataRealColeta = :dataRealColeta, dataPrevReceb = :dataPrevReceb WHERE idVendaProduto IN (SELECT idVendaProduto FROM "
                 "estoque_has_consumo WHERE idEstoque = :idEstoque) AND status = 'EM COLETA' AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  QSqlQuery query4;
  query4.prepare("UPDATE veiculo_has_produto SET status = 'COLETADO' WHERE idEstoque = :idEstoque AND status = 'EM COLETA'");

  for (const auto &item : list) {
    query1.bindValue(":idEstoque", modelViewColeta.data(item.row(), "idEstoque"));

    if (not query1.exec()) {
      emit errorSignal("Erro salvando status do estoque: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":dataRealColeta", dataColeta);
    query2.bindValue(":dataPrevReceb", dataPrevReceb);
    query2.bindValue(":idEstoque", modelViewColeta.data(item.row(), "idEstoque"));
    query2.bindValue(":codComercial", modelViewColeta.data(item.row(), "codComercial"));

    if (not query2.exec()) {
      emit errorSignal("Erro salvando status no pedido_fornecedor: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":dataRealColeta", dataColeta);
    query3.bindValue(":dataPrevReceb", dataPrevReceb);
    query3.bindValue(":idEstoque", modelViewColeta.data(item.row(), "idEstoque"));

    if (not query3.exec()) {
      emit errorSignal("Erro atualizando status da compra: " + query3.lastError().text());
      return false;
    }

    // -------------------------------------------------------------------------

    query4.bindValue(":idEstoque", modelViewColeta.data(item.row(), "idEstoque"));

    if (not query4.exec()) {
      emit errorSignal("Erro atualizando veiculo_has_produto: " + query4.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaColeta::on_checkBoxMarcarTodos_clicked(const bool) { ui->table->selectAll(); }

void WidgetLogisticaColeta::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetLogisticaColeta::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaColeta::montaFiltro() {
  const QString textoBusca = ui->lineEditBusca->text();

  modelViewColeta.setFilter("(numeroNFe LIKE '%" + textoBusca + "%' OR produto LIKE '%" + textoBusca + "%' OR idVenda LIKE '%" + textoBusca + "%' OR ordemCompra LIKE '%" + textoBusca + "%')");

  if (not modelViewColeta.select()) { return; }
}

void WidgetLogisticaColeta::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  InputDialog input(InputDialog::Tipo::AgendarColeta);

  if (input.exec() != InputDialog::Accepted) { return; }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not reagendar(list, input.getNextDate())) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Reagendado com sucesso!");
}

bool WidgetLogisticaColeta::reagendar(const QModelIndexList &list, const QDate &dataPrevColeta) {
  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevColeta = :dataPrevColeta WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque)"
                 " AND codComercial = :codComercial");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET dataPrevColeta = :dataPrevColeta WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND codComercial = :codComercial AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  QSqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = :data WHERE idEstoque = :idEstoque AND status = 'EM COLETA'");

  for (const auto &item : list) {
    const int idEstoque = modelViewColeta.data(item.row(), "idEstoque").toInt();
    const QString codComercial = modelViewColeta.data(item.row(), "codComercial").toString();

    query1.bindValue(":dataPrevColeta", dataPrevColeta);
    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) {
      emit errorSignal("Erro salvando status no pedido_fornecedor: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) {
      emit errorSignal("Erro salvando status na venda_produto: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":data", dataPrevColeta);
    query3.bindValue(":idEstoque", modelViewColeta.data(item.row(), "idEstoque"));

    if (not query3.exec()) {
      emit errorSignal("Erro atualizando data no veiculo: " + query3.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaColeta::on_pushButtonVenda_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  for (const auto &item : list) {
    const QString idVenda = modelViewColeta.data(item.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) { return; }

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
    }
  }
}

bool WidgetLogisticaColeta::cancelar(const QModelIndexList &list) {
  QSqlQuery query1;
  query1.prepare(
      "UPDATE pedido_fornecedor_has_produto SET dataPrevColeta = NULL WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) AND codComercial = :codComercial");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET dataPrevColeta = NULL WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) AND codComercial = :codComercial "
                 "AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  QSqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = NULL WHERE idEstoque = :idEstoque AND status = 'EM COLETA'");

  for (const auto &item : list) {
    const int idEstoque = modelViewColeta.data(item.row(), "idEstoque").toInt();
    const QString codComercial = modelViewColeta.data(item.row(), "codComercial").toString();

    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) {
      emit errorSignal("Erro salvando status no pedido_fornecedor: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) {
      emit errorSignal("Erro salvando status na venda_produto: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":idEstoque", modelViewColeta.data(item.row(), "idEstoque"));

    if (not query3.exec()) {
      emit errorSignal("Erro atualizando data no veiculo: " + query3.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaColeta::on_pushButtonCancelar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Cancelado com sucesso!");
}
