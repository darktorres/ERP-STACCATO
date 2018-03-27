#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "acbr.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "ui_widgetcompraoc.h"
#include "widgetcompraoc.h"

WidgetCompraOC::WidgetCompraOC(QWidget *parent) : Widget(parent), ui(new Ui::WidgetCompraOC) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraOC::on_lineEditBusca_textChanged);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetCompraOC::on_pushButtonDanfe_clicked);
  connect(ui->pushButtonDesfazerConsumo, &QPushButton::clicked, this, &WidgetCompraOC::on_pushButtonDesfazerConsumo_clicked);
  connect(ui->tableNFe, &TableView::entered, this, &WidgetCompraOC::on_tableNFe_entered);
  connect(ui->tablePedido, &TableView::clicked, this, &WidgetCompraOC::on_tablePedido_clicked);
  connect(ui->tablePedido, &TableView::entered, this, &WidgetCompraOC::on_tablePedido_entered);
  connect(ui->tableProduto, &TableView::entered, this, &WidgetCompraOC::on_tableProduto_entered);
}

WidgetCompraOC::~WidgetCompraOC() { delete ui; }

bool WidgetCompraOC::updateTables() {
  if (modelPedido.tableName().isEmpty()) setupTables();

  if (not modelPedido.select()) {
    emit errorSignal("Erro lendo tabela pedidos: " + modelPedido.lastError().text());
    return false;
  }

  if (not modelProduto.select()) {
    emit errorSignal("Erro lendo tabela produtos pedido: " + modelProduto.lastError().text());
    return false;
  }

  return true;
}

void WidgetCompraOC::setupTables() {
  // REFAC: refactor this to not select in here

  modelPedido.setTable("view_ordemcompra");
  modelPedido.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (not modelPedido.select()) emit errorSignal("Erro lendo tabela pedidos: " + modelPedido.lastError().text());

  ui->tablePedido->setModel(&modelPedido);

  modelProduto.setTable("pedido_fornecedor_has_produto");
  modelProduto.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelProduto.setHeaderData("status", "Status");
  modelProduto.setHeaderData("idVenda", "Venda");
  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("descricao", "Produto");
  modelProduto.setHeaderData("colecao", "Coleção");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("quant", "Quant.");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("un2", "Un2.");
  modelProduto.setHeaderData("caixas", "Cx.");
  modelProduto.setHeaderData("prcUnitario", "R$ Unit.");
  modelProduto.setHeaderData("preco", "R$");
  modelProduto.setHeaderData("kgcx", "Kg./Cx.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("codBarras", "Cód. Barras");
  modelProduto.setHeaderData("obs", "Obs.");

  modelProduto.setFilter("0");

  if (not modelProduto.select()) emit errorSignal("Erro lendo tabela produtos pedido: " + modelProduto.lastError().text());

  ui->tableProduto->setModel(&modelProduto);
  ui->tableProduto->setItemDelegateForColumn("quant", new DoubleDelegate(this));
  ui->tableProduto->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProduto->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->tableProduto->setItemDelegateForColumn("kgcx", new DoubleDelegate(this));
  ui->tableProduto->hideColumn("idPedido");
  ui->tableProduto->hideColumn("selecionado");
  ui->tableProduto->hideColumn("statusFinanceiro");
  ui->tableProduto->hideColumn("idVendaProduto");
  ui->tableProduto->hideColumn("ordemCompra");
  ui->tableProduto->hideColumn("idCompra");
  ui->tableProduto->hideColumn("idProduto");
  ui->tableProduto->hideColumn("quantUpd");
  ui->tableProduto->hideColumn("quantConsumida");
  ui->tableProduto->hideColumn("aliquotaSt");
  ui->tableProduto->hideColumn("st");
  ui->tableProduto->hideColumn("dataPrevCompra");
  ui->tableProduto->hideColumn("dataRealCompra");
  ui->tableProduto->hideColumn("dataPrevConf");
  ui->tableProduto->hideColumn("dataRealConf");
  ui->tableProduto->hideColumn("dataPrevFat");
  ui->tableProduto->hideColumn("dataRealFat");
  ui->tableProduto->hideColumn("dataPrevColeta");
  ui->tableProduto->hideColumn("dataRealColeta");
  ui->tableProduto->hideColumn("dataPrevReceb");
  ui->tableProduto->hideColumn("dataRealReceb");
  ui->tableProduto->hideColumn("dataPrevEnt");
  ui->tableProduto->hideColumn("dataRealEnt");

  modelNFe.setTable("view_ordemcompra_nfe");
  modelNFe.setEditStrategy(QSqlTableModel::OnManualSubmit);
  modelNFe.setHeaderData("numeroNFe", "NFe");

  modelNFe.setFilter("0");

  if (not modelNFe.select()) emit errorSignal("Erro lendo tabela nfe: " + modelNFe.lastError().text());

  ui->tableNFe->setModel(&modelNFe);
  ui->tableNFe->hideColumn("ordemCompra");
  ui->tableNFe->hideColumn("idNFe");
}

void WidgetCompraOC::on_tablePedido_clicked(const QModelIndex &index) {
  const QString oc = modelPedido.data(index.row(), "OC").toString();

  modelProduto.setFilter("ordemCompra = " + oc);

  modelNFe.setFilter("ordemCompra = " + oc);
}

void WidgetCompraOC::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableNFe->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  if (not ACBr::gerarDanfe(modelNFe.data(list.first().row(), "idNFe").toInt())) return;
}

void WidgetCompraOC::on_tablePedido_entered(const QModelIndex &) { ui->tablePedido->resizeColumnsToContents(); }

void WidgetCompraOC::on_tableProduto_entered(const QModelIndex &) { ui->tableProduto->resizeColumnsToContents(); }

void WidgetCompraOC::on_tableNFe_entered(const QModelIndex &) { ui->tableNFe->resizeColumnsToContents(); }

void WidgetCompraOC::on_pushButtonDesfazerConsumo_clicked() {
  // TODO: mostrar erro/aviso se a linha selecionada noa possuir consumo (linhas de estoque)

  const auto list = ui->tableProduto->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  const int row = list.first().row();

  QMessageBox msgBox(QMessageBox::Question, "Desfazer consumo?", "Tem certeza?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Continuar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  const QString idVenda = modelProduto.data(row, "idVenda").toString();

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not desfazerConsumo(row)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVenda)) return;

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();

  updateTables();

  emit informationSignal("Operação realizada com sucesso!");
}

bool WidgetCompraOC::desfazerConsumo(const int row) {
  // REFAC: pass this responsability to Estoque class

  const int idVendaProduto = modelProduto.data(row, "idVendaProduto").toInt();

  QSqlQuery query1;
  query1.prepare("SELECT status FROM estoque_has_consumo WHERE idVendaProduto = :idVendaProduto");
  query1.bindValue(":idVendaProduto", idVendaProduto);

  if (not query1.exec()) {
    emit errorSignal("Erro buscando status do consumo estoque: " + query1.lastError().text());
    return false;
  }

  if (query1.first()) {
    const QString status = query1.value("status").toString();

    if (status == "ENTREGA AGEND." or status == "EM ENTREGA" or status == "ENTREGUE") {
      emit errorSignal("Consumo do estoque está em entrega/entregue!");
      return false;
    }

    QSqlQuery query2;
    query2.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto = :idVendaProduto");
    query2.bindValue(":idVendaProduto", idVendaProduto);

    if (not query2.exec()) {
      emit errorSignal("Erro removendo consumo estoque: " + query2.lastError().text());
      return false;
    }
  }

  // TODO: juntar linhas sem consumo do mesmo tipo?

  QSqlQuery query3;
  query3.prepare("UPDATE pedido_fornecedor_has_produto SET idVenda = NULL, idVendaProduto = NULL WHERE idVendaProduto = :idVendaProduto");
  query3.bindValue(":idVendaProduto", idVendaProduto);

  if (not query3.exec()) {
    emit errorSignal("Erro atualizando pedido compra: " + query3.lastError().text());
    return false;
  }

  QSqlQuery query4;
  query4.prepare("UPDATE venda_has_produto SET status = 'PENDENTE', idCompra = NULL, dataPrevEnt = NULL WHERE idVendaProduto = :idVendaProduto");
  query4.bindValue(":idVendaProduto", idVendaProduto);

  if (not query4.exec()) {
    emit errorSignal("Erro atualizando pedido venda: " + query4.lastError().text());
    return false;
  }

  return true;
}

void WidgetCompraOC::on_lineEditBusca_textChanged(const QString &text) {
  modelPedido.setFilter("Venda LIKE '%" + text + "%' OR OC LIKE '%" + text + "%'");

  if (not modelPedido.select()) emit errorSignal("Erro lendo tabela pedidos: " + modelPedido.lastError().text());
}
