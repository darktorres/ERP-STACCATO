#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "doubledelegate.h"
#include "ui_widgetnfeentrada.h"
#include "widgetnfeentrada.h"
#include "xml_viewer.h"

WidgetNfeEntrada::WidgetNfeEntrada(QWidget *parent) : Widget(parent), ui(new Ui::WidgetNfeEntrada) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeEntrada::on_lineEditBusca_textChanged);
  connect(ui->pushButtonCancelarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonCancelarNFe_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated);
  connect(ui->table, &TableView::entered, this, &WidgetNfeEntrada::on_table_entered);
}

WidgetNfeEntrada::~WidgetNfeEntrada() { delete ui; }

bool WidgetNfeEntrada::updateTables() {
  if (modelViewNFeEntrada.tableName().isEmpty()) setupTables();

  if (not modelViewNFeEntrada.select()) {
    emit errorSignal("Erro lendo tabela NFe: " + modelViewNFeEntrada.lastError().text());
    return false;
  }

  ui->table->resizeColumnsToContents();

  return true;
}

void WidgetNfeEntrada::setupTables() {
  // REFAC: refactor this to not select in here

  modelViewNFeEntrada.setTable("view_nfe_entrada");
  modelViewNFeEntrada.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (not modelViewNFeEntrada.select()) emit errorSignal("Erro lendo tabela: " + modelViewNFeEntrada.lastError().text());

  ui->table->setModel(&modelViewNFeEntrada);
  ui->table->hideColumn("idNFe");
  ui->table->setItemDelegate(new DoubleDelegate(this));
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando xml da nota: " + query.lastError().text());
    return;
  }

  auto *viewer = new XML_Viewer(this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
  viewer->exibirXML(query.value("xml").toByteArray());
}

void WidgetNfeEntrada::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetNfeEntrada::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetNfeEntrada::montaFiltro() {
  const QString text = ui->lineEditBusca->text();

  modelViewNFeEntrada.setFilter("NFe LIKE '%" + text + "%' OR OC LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%'");

  if (not modelViewNFeEntrada.select()) { return; }
}

void WidgetNfeEntrada::on_pushButtonCancelarNFe_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhuma linha selecionada!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  const int row = list.first().row();

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not cancelar(row)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Cancelado com sucesso!");
}

bool WidgetNfeEntrada::cancelar(const int row) {
  // FIXME: ao cancelar nota nao voltar o pedido de compra inteiro, apenas o item da nota cancelada

  // marcar nota como cancelada

  QSqlQuery query1;
  query1.prepare("UPDATE nfe SET status = 'CANCELADO' WHERE idNFe = :idNFe");
  query1.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query1.exec()) {
    emit errorSignal("Erro cancelando nota: " + query1.lastError().text());
    return false;
  }

  QSqlQuery query2;
  query2.prepare("UPDATE estoque SET status = 'CANCELADO' WHERE idEstoque IN (SELECT idEstoque FROM estoque_has_nfe WHERE idNFe = :idNFe)");
  query2.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query2.exec()) {
    emit errorSignal("Erro marcando estoque cancelado: " + query2.lastError().text());
    return false;
  }

  // voltar compra para faturamento
  QSqlQuery query3;
  query3.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM FATURAMENTO', quantUpd = 0, quantConsumida = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
                 "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque "
                 "FROM estoque_has_nfe WHERE idNFe = :idNFe))");
  query3.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query3.exec()) {
    emit errorSignal("Erro voltando compra para faturamento: " + query3.lastError().text());
    return false;
  }

  // desvincular produtos associados (se houver)
  QSqlQuery query4;
  query4.prepare("SELECT idVendaProduto FROM estoque_has_consumo WHERE idEstoque IN (SELECT idEstoque FROM estoque_has_nfe WHERE idNFe = :idNFe)");
  query4.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query4.exec()) {
    emit errorSignal("Erro buscando consumos: " + query4.lastError().text());
    return false;
  }

  QSqlQuery query5;
  query5.prepare("UPDATE estoque_has_consumo SET status = 'CANCELADO' WHERE idVendaProduto = :idVendaProduto");

  // voltar status para pendente
  QSqlQuery query6;
  query6.prepare("UPDATE venda_has_produto SET status = 'EM FATURAMENTO', dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, dataRealFat = "
                 "NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE idVendaProduto = :idVendaProduto");

  while (query4.next()) {
    // TODO: 1quando cancelar nota pegar os estoques e cancelar/remover da logistica (exceto quando estiverem entregues?)
    // se existir linhas de consumo pode ser que existam linhas de entrega, tratar
    // se houver consumos na nota mostrar para o usuario e pedir que ele faça os 'desconsumir'

    query5.bindValue(":idVendaProduto", query4.value("idVendaProduto"));

    if (not query5.exec()) {
      emit errorSignal("Erro cancelando consumos: " + query5.lastError().text());
      return false;
    }

    query6.bindValue(":idVendaProduto", query4.value("idVendaProduto"));

    if (not query6.exec()) {
      emit errorSignal("Erro voltando produtos para pendente: " + query6.lastError().text());
      return false;
    }
  }

  return true;
}

// TODO: 5copiar filtros do widgetnfesaida
