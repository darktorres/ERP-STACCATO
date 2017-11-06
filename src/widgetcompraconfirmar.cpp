#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "ui_widgetcompraconfirmar.h"
#include "widgetcompraconfirmar.h"

WidgetCompraConfirmar::WidgetCompraConfirmar(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraConfirmar) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetCompraConfirmar::~WidgetCompraConfirmar() { delete ui; }

void WidgetCompraConfirmar::setupTables() {
  modelResumo.setTable("view_fornecedor_compra_confirmar");

  modelResumo.setFilter("(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  modelResumo.setHeaderData("fornecedor", "Forn.");

  ui->tableResumo->setModel(&modelResumo);
  ui->tableResumo->hideColumn("idVenda");

  model.setTable("view_compras");

  model.setFilter("(Venda NOT LIKE '%CAMB%' OR Venda IS NULL)");

  model.setHeaderData("dataPrevConf", "Prev. Conf.");

  ui->table->setModel(&model);
  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
  ui->table->hideColumn("Compra");
}

bool WidgetCompraConfirmar::updateTables() {
  if (model.tableName().isEmpty()) setupTables();

  if (not modelResumo.select()) {
    emit errorSignal("Erro lendo tabela resumo: " + modelResumo.lastError().text());
    return false;
  }

  ui->tableResumo->resizeColumnsToContents();

  if (not model.select()) {
    emit errorSignal("Erro lendo tabela compras: " + model.lastError().text());
    return false;
  }

  ui->table->resizeColumnsToContents();

  return true;
}

void WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked() {
  if (ui->table->selectionModel()->selectedRows().isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  const int row = ui->table->selectionModel()->selectedRows().first().row();
  const QString idCompra = model.data(row, "Compra").toString();

  InputDialogFinanceiro inputDlg(InputDialogFinanceiro::Tipo::ConfirmarCompra);
  if (not inputDlg.setFilter(idCompra)) return;

  if (inputDlg.exec() != InputDialogFinanceiro::Accepted) return;

  const QDateTime dataPrevista = inputDlg.getDate();
  const QDateTime dataConf = inputDlg.getNextDate();

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not confirmarCompra(idCompra, dataPrevista, dataConf)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  QSqlQuery query;

  if (not query.exec("CALL update_venda_status()")) {
    QMessageBox::critical(this, "Erro!", "Erro atualizando status das vendas: " + query.lastError().text());
    return;
  }

  updateTables();
  QMessageBox::information(this, "Aviso!", "Compra confirmada!");
}

bool WidgetCompraConfirmar::confirmarCompra(const QString &idCompra, const QDateTime &dataPrevista, const QDateTime &dataConf) {
  QSqlQuery query;
  query.prepare("SELECT idPedido, idVendaProduto FROM pedido_fornecedor_has_produto WHERE idCompra = :idCompra AND selecionado = TRUE");
  query.bindValue(":idCompra", idCompra);

  if (not query.exec()) {
    emit errorSignal("Erro buscando produtos: " + query.lastError().text());
    return false;
  }

  while (query.next()) {
    QSqlQuery queryUpdate;
    queryUpdate.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, "
                        "dataPrevFat = :dataPrevFat, selecionado = FALSE WHERE idPedido = :idPedido");
    queryUpdate.bindValue(":dataRealConf", dataConf);
    queryUpdate.bindValue(":dataPrevFat", dataPrevista);
    queryUpdate.bindValue(":idPedido", query.value("idPedido"));

    if (not queryUpdate.exec()) {
      emit errorSignal("Erro atualizando status da compra: " + queryUpdate.lastError().text());
      return false;
    }

    if (query.value("idVendaProduto").toInt() != 0) {
      queryUpdate.prepare("UPDATE venda_has_produto SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat "
                          "= :dataPrevFat WHERE idVendaProduto = :idVendaProduto");
      queryUpdate.bindValue(":dataRealConf", dataConf);
      queryUpdate.bindValue(":dataPrevFat", dataPrevista);
      queryUpdate.bindValue(":idVendaProduto", query.value("idVendaProduto"));

      if (not queryUpdate.exec()) {
        emit errorSignal("Erro salvando status da venda: " + queryUpdate.lastError().text());
        return false;
      }
    }
  }

  return true;
}

void WidgetCompraConfirmar::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

bool WidgetCompraConfirmar::cancelar(const QModelIndexList &list) {
  const int row = list.first().row();

  QSqlQuery query;

  query.prepare("SELECT idVendaProduto FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra AND status = 'EM COMPRA'");
  query.bindValue(":ordemCompra", model.data(row, "OC"));

  if (not query.exec()) {
    emit errorSignal("Erro buscando dados: " + query.lastError().text());
    return false;
  }

  while (query.next()) {
    QSqlQuery query2;
    query2.prepare("UPDATE venda_has_produto SET status = 'PENDENTE', idCompra = NULL WHERE idVendaProduto = :idVendaProduto AND status = 'EM COMPRA'");
    query2.bindValue(":idVendaProduto", query.value("idVendaProduto"));

    if (not query2.exec()) {
      emit errorSignal("Erro voltando status do produto: " + query2.lastError().text());
      return false;
    }
  }

  query.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'CANCELADO' WHERE ordemCompra = :ordemCompra AND status = 'EM COMPRA'");
  query.bindValue(":ordemCompra", model.data(row, "OC"));

  if (not query.exec()) {
    emit errorSignal("Erro salvando dados: " + query.lastError().text());
    return false;
  }

  return true;
}

void WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked() {
  // TODO: 5cancelar itens individuais no lugar da compra toda?

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  updateTables();
  QMessageBox::information(this, "Aviso!", "Itens cancelados!");
}

void WidgetCompraConfirmar::on_checkBoxMostrarSul_toggled(bool checked) {
  model.setFilter(checked ? "(Venda LIKE '%CAMB%')" : "(Venda NOT LIKE '%CAMB%' OR Venda IS NULL)");

  if (not model.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + model.lastError().text());
}

// TODO: 1poder confirmar dois pedidos juntos (quando vem um espelho só) (cancelar os pedidos e fazer um pedido só?)
// TODO: 1permitir na tela de compras alterar uma venda para quebrar um produto em dois para os casos de lotes diferentes: 50 -> 40+10
// TODO: 0colocar data para frete/st e se elas são inclusas nas parcelas ou separadas
// TODO: 0mesmo bug do gerarcompra/produtospendentes em que o prcUnitario é multiplicado pela quantidade total e nao a da linha
// TODO: 0cancelar nesta tela nao altera status para pendente
