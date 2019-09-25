#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "doubledelegate.h"
#include "ui_widgetnfeentrada.h"
#include "widgetnfeentrada.h"
#include "xml_viewer.h"

WidgetNfeEntrada::WidgetNfeEntrada(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeEntrada) { ui->setupUi(this); }

WidgetNfeEntrada::~WidgetNfeEntrada() { delete ui; }

void WidgetNfeEntrada::setConnections() {
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeEntrada::on_lineEditBusca_textChanged);
  connect(ui->pushButtonRemoverNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonRemoverNFe_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated);
}

void WidgetNfeEntrada::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelViewNFeEntrada.select()) { return; }
}

void WidgetNfeEntrada::resetTables() { modelIsSet = false; }

void WidgetNfeEntrada::setupTables() {
  modelViewNFeEntrada.setTable("view_nfe_entrada");

  ui->table->setModel(&modelViewNFeEntrada);

  ui->table->hideColumn("idNFe");

  ui->table->setItemDelegate(new DoubleDelegate(this));
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetNfeEntrada::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetNfeEntrada::montaFiltro() {
  const QString text = ui->lineEditBusca->text();

  modelViewNFeEntrada.setFilter("NFe LIKE '%" + text + "%' OR OC LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%'");
}

void WidgetNfeEntrada::on_pushButtonRemoverNFe_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  const int row = list.first().row();

  //--------------------------------------------------------------

  QSqlQuery query;
  query.prepare("SELECT status FROM venda_has_produto2 WHERE status IN ('ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.') AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE "
                "idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe))");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query.exec()) { return qApp->enqueueError("Erro verificando pedidos: " + query.lastError().text(), this); }

  if (query.size() > 0) { return qApp->enqueueError("NFe possui itens 'EM ENTREGA/ENTREGUE'!", this); }

  //--------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Remover?", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not remover(row)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Removido com sucesso!", this);
}

bool WidgetNfeEntrada::remover(const int row) {
  QSqlQuery query1;
  query1.prepare(
      "UPDATE `pedido_fornecedor_has_produto2` SET status = 'EM FATURAMENTO', quantUpd = 0, quantConsumida = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
      "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque "
      "FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  query1.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query1.exec()) { return qApp->enqueueError(false, "Erro voltando compra para faturamento: " + query1.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query2;
  query2.prepare(
      "UPDATE venda_has_produto2 SET status = 'EM FATURAMENTO', dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, "
      "dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` IN (SELECT "
      "`idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  query2.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query2.exec()) { return qApp->enqueueError(false, "Erro voltando venda para faturamento: " + query2.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query3;
  query3.prepare("DELETE FROM estoque_has_consumo WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)");
  query3.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query3.exec()) { return qApp->enqueueError(false, "Erro removendo consumos: " + query3.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query4;
  query4.prepare("DELETE FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)");
  query4.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query4.exec()) { return qApp->enqueueError(false, "Erro removendo compras: " + query4.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query5;
  query5.prepare("UPDATE produto SET desativado = TRUE WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  query5.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query5.exec()) { return qApp->enqueueError(false, "Erro removendo produto estoque: " + query5.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query6;
  query6.prepare("UPDATE estoque SET status = 'CANCELADO', idNFe = NULL WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  query6.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query6.exec()) { return qApp->enqueueError(false, "Erro removendo estoque: " + query6.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query7;
  query7.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
  query7.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query7.exec()) { return qApp->enqueueError(false, "Erro cancelando nota: " + query7.lastError().text(), this); }

  return true;
}

// TODO: 5copiar filtros do widgetnfesaida

// TODO: para gerar GARE da nfe de entrada:
// dados ncm, valorMercadoria, ipi, desconto e aliqIcms...

// valorIcms = (valorMercadoria - desconto) * aliqIcms
// mva:
// 		se aliqIcms = 4% procurar mva4% usando ncm
// 		se aliqIcms <> 4% procurar mva12% usando ncm
// IndiceAgregado = (valorMercadoria + Ipi) * MVA
// baseCalculoIcmsST = valorMercadoria + Ipi + IndiceAgregado
// icmsInterno = procurar aliqInterna pelo ncm
// icmsInternoR$ = baseCalculoIcmsST * icmsInterno
// valorIcmsSt = icmsInternoR$ - valorIcms
