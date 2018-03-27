#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "ui_widgetlogisticarecebimento.h"
#include "venda.h"
#include "widgetlogisticarecebimento.h"

WidgetLogisticaRecebimento::WidgetLogisticaRecebimento(QWidget *parent) : Widget(parent), ui(new Ui::WidgetLogisticaRecebimento) {
  ui->setupUi(this);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaRecebimento::on_lineEditBusca_textChanged);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonMarcarRecebido, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaRecebimento::on_pushButtonVenda_clicked);
  connect(ui->table, &TableView::entered, this, &WidgetLogisticaRecebimento::on_table_entered);
}

WidgetLogisticaRecebimento::~WidgetLogisticaRecebimento() { delete ui; }

bool WidgetLogisticaRecebimento::updateTables() {
  if (modelViewRecebimento.tableName().isEmpty()) setupTables();

  if (not modelViewRecebimento.select()) {
    emit errorSignal("Erro lendo tabela pedido_fornecedor_has_produto: " + modelViewRecebimento.lastError().text());
    return false;
  }

  for (int row = 0; row < modelViewRecebimento.rowCount(); ++row) ui->table->openPersistentEditor(row, "selecionado");

  ui->table->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaRecebimento::tableFornLogistica_activated(const QString &fornecedor) {
  this->fornecedor = fornecedor;

  ui->lineEditBusca->clear();

  modelViewRecebimento.setFilter("fornecedor = '" + fornecedor + "'");

  if (not modelViewRecebimento.select()) {
    emit errorSignal("Erro lendo tabela pedido_fornecedor_has_produto: " + modelViewRecebimento.lastError().text());
    return;
  }

  ui->checkBoxMarcarTodos->setChecked(false);

  ui->table->resizeColumnsToContents();
}

void WidgetLogisticaRecebimento::setupTables() {
  modelViewRecebimento.setTable("view_recebimento");
  modelViewRecebimento.setEditStrategy(QSqlTableModel::OnManualSubmit);

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

  modelViewRecebimento.setFilter("0");

  ui->table->setModel(new EstoquePrazoProxyModel(&modelViewRecebimento, this));
  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("prazoEntrega");
  ui->table->hideColumn("ordemCompra");
}

bool WidgetLogisticaRecebimento::processRows(const QModelIndexList &list, const QDateTime &dataReceb, const QString &recebidoPor) {
  // TODO: 4aqui e na funcao de cancelar verificar se é possivel trocar 'IN ()' por idVendaProduto
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'ESTOQUE', recebidoPor = :recebidoPor WHERE idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE estoque_has_consumo SET status = 'CONSUMO' WHERE status = 'PRÉ-CONSUMO' AND idEstoque = :idEstoque");

  QSqlQuery query3;
  query3.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE "
                 "idEstoque = :idEstoque) AND codComercial = :codComercial");

  QSqlQuery query4;
  // salvar status na venda
  query4.prepare(
      "UPDATE venda_has_produto SET status = 'ESTOQUE', dataRealReceb = :dataRealReceb WHERE idVendaProduto IN (SELECT idVendaProduto FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  for (const auto &item : list) {
    query1.bindValue(":recebidoPor", recebidoPor);
    query1.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));

    if (not query1.exec()) {
      emit errorSignal("Erro atualizando status do estoque: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));

    if (not query2.exec()) {
      emit errorSignal("Erro atualizando status da venda: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":dataRealReceb", dataReceb);
    query3.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));
    query3.bindValue(":codComercial", modelViewRecebimento.data(item.row(), "codComercial"));

    if (not query3.exec()) {
      emit errorSignal("Erro atualizando status da compra: " + query3.lastError().text());
      return false;
    }

    query4.bindValue(":dataRealReceb", dataReceb);
    query4.bindValue(":idEstoque", modelViewRecebimento.data(item.row(), "idEstoque"));

    if (not query4.exec()) {
      emit errorSignal("Erro atualizando produtos venda: " + query4.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaRecebimento::on_pushButtonMarcarRecebido_clicked() {
  // TODO: 1gerar gare da nota de entrada

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QStringList ids;
  QStringList idVendas;

  for (const auto &item : list) {
    ids << modelViewRecebimento.data(item.row(), "idEstoque").toString();
    idVendas << modelViewRecebimento.data(item.row(), "idVenda").toString();
  }

  InputDialogConfirmacao inputDlg(InputDialogConfirmacao::Tipo::Recebimento);
  inputDlg.setFilterRecebe(ids);

  if (inputDlg.exec() != InputDialogConfirmacao::Accepted) return;

  const QDateTime dataReceb = inputDlg.getDateTime();
  const QString recebidoPor = inputDlg.getRecebeu();

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not processRows(list, dataReceb, recebidoPor)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) return;

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();

  updateTables();
  emit informationSignal("Confirmado recebimento!");
}

void WidgetLogisticaRecebimento::on_checkBoxMarcarTodos_clicked(const bool) { ui->table->selectAll(); }

void WidgetLogisticaRecebimento::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetLogisticaRecebimento::on_lineEditBusca_textChanged(const QString &text) {
  modelViewRecebimento.setFilter("(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')");

  if (not modelViewRecebimento.select()) emit errorSignal("Erro lendo tabela: " + modelViewRecebimento.lastError().text());
}

void WidgetLogisticaRecebimento::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  InputDialog input(InputDialog::Tipo::AgendarRecebimento);

  if (input.exec() != InputDialog::Accepted) return;

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not reagendar(list, input.getNextDate())) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();

  updateTables();
  emit informationSignal("Reagendado com sucesso!");
}

bool WidgetLogisticaRecebimento::reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) {
  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevReceb = :dataPrevReceb WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) AND codComercial = "
                 ":codComercial");

  QSqlQuery query2;
  query2.prepare(
      "UPDATE venda_has_produto SET dataPrevReceb = :dataPrevReceb WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) AND codComercial = :codComercial");

  for (const auto &item : list) {
    const int idEstoque = modelViewRecebimento.data(item.row(), "idEstoque").toInt();
    const QString codComercial = modelViewRecebimento.data(item.row(), "codComercial").toString();

    query1.bindValue(":dataPrevReceb", dataPrevReceb);
    query1.bindValue(":idEstoque", idEstoque);
    query1.bindValue(":codComercial", codComercial);

    if (not query1.exec()) {
      emit errorSignal("Erro salvando status no pedido_fornecedor: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":dataPrevReceb", dataPrevReceb);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) {
      emit errorSignal("Erro salvando status na venda_produto: " + query2.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaRecebimento::on_pushButtonVenda_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  for (const auto &item : list) {
    const QString idVenda = modelViewRecebimento.data(item.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) return;

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
    }
  }
}

void WidgetLogisticaRecebimento::on_pushButtonCancelar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QStringList idVendas;

  for (const auto &index : list) idVendas << modelViewRecebimento.data(index.row(), "idVenda").toString();

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) return;

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();

  updateTables();
  emit informationSignal("Cancelado com sucesso!");
}

bool WidgetLogisticaRecebimento::cancelar(const QModelIndexList &list) {
  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM COLETA' WHERE idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE "
                 "idEstoque = :idEstoque) AND codComercial = :codComercial");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET status = 'EM COLETA', dataRealColeta = NULL, dataPrevReceb = NULL WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = "
                 ":idEstoque) AND codComercial = :codComercial");

  for (const auto &item : list) {
    const int idEstoque = modelViewRecebimento.data(item.row(), "idEstoque").toInt();
    const QString codComercial = modelViewRecebimento.data(item.row(), "codComercial").toString();

    query1.bindValue(":idEstoque", idEstoque);

    if (not query1.exec()) {
      emit errorSignal("Erro salvando status no estoque: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) {
      emit errorSignal("Erro salvando status no pedido_fornecedor: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":idEstoque", idEstoque);
    query3.bindValue(":codComercial", codComercial);

    if (not query3.exec()) {
      emit errorSignal("Erro salvando status na venda_produto: " + query3.lastError().text());
      return false;
    }
  }

  return true;
}

// TODO: 0marcar em qual bloco foi guardado (opcional?)
