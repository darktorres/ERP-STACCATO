#include "widgetnfeentrada.h"
#include "ui_widgetnfeentrada.h"

#include "acbr.h"
#include "application.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "usersession.h"
#include "xml_viewer.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>

WidgetNfeEntrada::WidgetNfeEntrada(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeEntrada) { ui->setupUi(this); }

WidgetNfeEntrada::~WidgetNfeEntrada() { delete ui; }

void WidgetNfeEntrada::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeEntrada::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked, connectionType);
  connect(ui->pushButtonRemoverNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonRemoverNFe_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated, connectionType);
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

  modelViewNFeEntrada.select();
}

void WidgetNfeEntrada::resetTables() { modelIsSet = false; }

void WidgetNfeEntrada::setupTables() {
  modelViewNFeEntrada.setTable("view_nfe_entrada");

  modelViewNFeEntrada.setHeaderData("created", "Importado em");

  ui->table->setModel(&modelViewNFeEntrada);

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");
  ui->table->hideColumn("nsu");
  ui->table->hideColumn("utilizada");

  ui->table->showColumn("created");

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("GARE", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("GARE Pago Em", new DateFormatDelegate(this));
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetNfeEntrada::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetNfeEntrada::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  modelViewNFeEntrada.setFilter("NFe LIKE '%" + text + "%' OR OC LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%'");
}

void WidgetNfeEntrada::on_pushButtonRemoverNFe_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (list.size() > 1) { throw RuntimeError("Selecione apenas uma linha!", this); }

  const int row = list.first().row();

  //--------------------------------------------------------------

  SqlQuery queryGare;
  queryGare.prepare("SELECT status, valor FROM conta_a_pagar_has_pagamento WHERE contraParte = 'GARE' AND idNFe = :idNFe");
  queryGare.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryGare.exec()) { throw RuntimeError("Erro verificando GARE: " + queryGare.lastError().text(), this); }

  if (queryGare.first()) {
    const QString status = queryGare.value("status").toString();
    const double valor = queryGare.value("valor").toDouble();

    if ((status == "GERADO GARE" or status == "PAGO GARE") and valor > 0) { throw RuntimeError("GARE 'em pagamento/pago'!", this); }
  }

  //--------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT status FROM venda_has_produto2 WHERE status IN ('ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.') AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE "
                "idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe))");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro verificando pedidos: " + query.lastError().text(), this); }

  if (query.size() > 0) { throw RuntimeError("NFe possui itens 'EM ENTREGA/ENTREGUE'!", this); }

  //--------------------------------------------------------------

  if (modelViewNFeEntrada.data(row, "nsu").toInt() > 0 and modelViewNFeEntrada.data(row, "utilizada").toBool() == false) { throw RuntimeError("NFe não utilizada!", this); }

  //--------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Remover?", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  qApp->startTransaction("WidgetNfeEntrada::on_pushButtonRemoverNFe");

  remover(row);

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Removido com sucesso!", this);
}

void WidgetNfeEntrada::remover(const int row) {
  SqlQuery queryPedidoFornecedor;
  queryPedidoFornecedor.prepare(
      "UPDATE `pedido_fornecedor_has_produto2` SET status = 'EM FATURAMENTO', quantUpd = 0, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
      "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque "
      "FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  queryPedidoFornecedor.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryPedidoFornecedor.exec()) { throw RuntimeException("Erro voltando compra para faturamento: " + queryPedidoFornecedor.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryVendaProduto;
  queryVendaProduto.prepare(
      "UPDATE venda_has_produto2 SET status = 'EM FATURAMENTO', dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, "
      "dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` IN (SELECT "
      "`idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  queryVendaProduto.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryVendaProduto.exec()) { throw RuntimeException("Erro voltando venda para faturamento: " + queryVendaProduto.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryEstoque;
  queryEstoque.prepare("SELECT idEstoque FROM estoque WHERE idNFe = :idNFe");
  queryEstoque.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryEstoque.exec()) { throw RuntimeException("Erro buscando consumos: " + queryEstoque.lastError().text()); }

  SqlQuery queryDeleteConsumo;
  queryDeleteConsumo.prepare("DELETE FROM estoque_has_consumo WHERE idEstoque = :idEstoque");

  while (queryEstoque.next()) {
    queryDeleteConsumo.bindValue(":idEstoque", queryEstoque.value("idEstoque"));

    if (not queryDeleteConsumo.exec()) { throw RuntimeException("Erro removendo consumos: " + queryDeleteConsumo.lastError().text()); }
  }

  //-----------------------------------------------------------------------------

  SqlQuery queryDeleteCompra;
  queryDeleteCompra.prepare("DELETE FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)");
  queryDeleteCompra.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryDeleteCompra.exec()) { throw RuntimeException("Erro removendo compras: " + queryDeleteCompra.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryProduto;
  queryProduto.prepare("UPDATE produto SET desativado = TRUE WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  queryProduto.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryProduto.exec()) { throw RuntimeException("Erro removendo produto estoque: " + queryProduto.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryCancelaEstoque;
  queryCancelaEstoque.prepare("UPDATE estoque SET status = 'CANCELADO', idNFe = NULL WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  queryCancelaEstoque.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryCancelaEstoque.exec()) { throw RuntimeException("Erro removendo estoque: " + queryCancelaEstoque.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryGare;
  queryGare.prepare("DELETE FROM conta_a_pagar_has_pagamento WHERE contraParte = 'GARE' AND idNFe = :idNFe");
  queryGare.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not queryGare.exec()) { throw RuntimeException("Erro removendo pagamento da GARE: " + queryGare.lastError().text()); }

  //-----------------------------------------------------------------------------

  if (modelViewNFeEntrada.data(row, "nsu").toInt() > 0) {
    SqlQuery queryUpdateNFe;
    queryUpdateNFe.prepare("UPDATE nfe SET utilizada = FALSE, GARE = NULL WHERE idNFe = :idNFe");
    queryUpdateNFe.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

    if (not queryUpdateNFe.exec()) { throw RuntimeException("Erro marcando NFe como não utilizada: " + queryUpdateNFe.lastError().text()); }
  } else {
    SqlQuery queryDeleteNFe;
    queryDeleteNFe.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
    queryDeleteNFe.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

    if (not queryDeleteNFe.exec()) { throw RuntimeException("Erro removendo NFe: " + queryDeleteNFe.lastError().text()); }
  }
}

void WidgetNfeEntrada::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  ACBr acbrLocal;

  for (const auto &index : list) {
    // TODO: se a conexao com o acbr falhar ou der algum erro pausar o loop e perguntar para o usuario se ele deseja tentar novamente (do ponto que parou)
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    if (modelViewNFeEntrada.data(index.row(), "status").toString() != "AUTORIZADO") { continue; }

    // pegar XML do MySQL e salvar em arquivo

    const QString chaveAcesso = modelViewNFeEntrada.data(index.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando xml: " + query.lastError().text()); }

    QFile fileXml(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita xml: " + fileXml.errorString()); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar XML para ACBr gerar PDF

    const QString pdfOrigem = acbrLocal.gerarDanfe(query.value("xml").toByteArray(), false);

    if (pdfOrigem.isEmpty()) { throw RuntimeException("Resposta vazia!"); }

    // copiar para pasta predefinida

    const QString pdfDestino = QDir::currentPath() + "/arquivos/" + chaveAcesso + ".pdf";

    QFile filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(pdfOrigem, pdfDestino)) { throw RuntimeException("Erro copiando pdf!"); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para " + QDir::currentPath() + "/arquivos/" + "!", this);
}

// TODO: 5copiar filtros do widgetnfesaida
