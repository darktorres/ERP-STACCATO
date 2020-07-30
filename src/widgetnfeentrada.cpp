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
#include <QSqlQuery>

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

  if (not modelViewNFeEntrada.select()) { return; }
}

void WidgetNfeEntrada::resetTables() { modelIsSet = false; }

void WidgetNfeEntrada::setupTables() {
  modelViewNFeEntrada.setTable("view_nfe_entrada");

  modelViewNFeEntrada.setHeaderData("created", "Importado em");

  ui->table->setModel(&modelViewNFeEntrada);

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");

  ui->table->showColumn("created");

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("GARE", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("GARE Pago Em", new DateFormatDelegate(this));
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { return qApp->enqueueException("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetNfeEntrada::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetNfeEntrada::montaFiltro() {
  const QString text = ui->lineEditBusca->text().remove("'");

  modelViewNFeEntrada.setFilter("NFe LIKE '%" + text + "%' OR OC LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%'");
}

void WidgetNfeEntrada::on_pushButtonRemoverNFe_clicked() {
  // TODO: remover GARE da tabela de contas_a_pagar

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  if (list.size() > 1) { return qApp->enqueueError("Selecione apenas uma linha!"); }

  const int row = list.first().row();

  //--------------------------------------------------------------

  QSqlQuery query;
  query.prepare("SELECT status FROM venda_has_produto2 WHERE status IN ('ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.') AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE "
                "idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe))");
  query.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query.exec()) { return qApp->enqueueException("Erro verificando pedidos: " + query.lastError().text(), this); }

  if (query.size() > 0) { return qApp->enqueueError("NFe possui itens 'EM ENTREGA/ENTREGUE'!", this); }

  //--------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Remover?", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  if (not qApp->startTransaction("WidgetNfeEntrada::on_pushButtonRemoverNFe")) { return; }

  if (not remover(row)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Removido com sucesso!", this);
}

bool WidgetNfeEntrada::remover(const int row) {
  QSqlQuery query1;
  query1.prepare(
      "UPDATE `pedido_fornecedor_has_produto2` SET status = 'EM FATURAMENTO', quantUpd = 0, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
      "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque "
      "FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  query1.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query1.exec()) { return qApp->enqueueException(false, "Erro voltando compra para faturamento: " + query1.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query2;
  query2.prepare(
      "UPDATE venda_has_produto2 SET status = 'EM FATURAMENTO', dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, "
      "dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` IN (SELECT "
      "`idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  query2.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query2.exec()) { return qApp->enqueueException(false, "Erro voltando venda para faturamento: " + query2.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query03;
  query03.prepare("SELECT idEstoque FROM estoque WHERE idNFe = :idNFe");
  query03.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query03.exec()) { return qApp->enqueueException(false, "Erro buscando consumos: " + query03.lastError().text(), this); }

  QSqlQuery query3;
  query3.prepare("DELETE FROM estoque_has_consumo WHERE idEstoque = :idEstoque");

  while (query03.next()) {
    query3.bindValue(":idEstoque", query03.value("idEstoque"));

    if (not query3.exec()) { return qApp->enqueueException(false, "Erro removendo consumos: " + query3.lastError().text(), this); }
  }

  //-----------------------------------------------------------------------------

  QSqlQuery query4;
  query4.prepare("DELETE FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)");
  query4.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query4.exec()) { return qApp->enqueueException(false, "Erro removendo compras: " + query4.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query5;
  query5.prepare("UPDATE produto SET desativado = TRUE WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  query5.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query5.exec()) { return qApp->enqueueException(false, "Erro removendo produto estoque: " + query5.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query6;
  query6.prepare("UPDATE estoque SET status = 'CANCELADO', idNFe = NULL WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  query6.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query6.exec()) { return qApp->enqueueException(false, "Erro removendo estoque: " + query6.lastError().text(), this); }

  //-----------------------------------------------------------------------------

  QSqlQuery query7;
  query7.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
  query7.bindValue(":idNFe", modelViewNFeEntrada.data(row, "idNFe"));

  if (not query7.exec()) { return qApp->enqueueException(false, "Erro cancelando nota: " + query7.lastError().text(), this); }

  return true;
}

void WidgetNfeEntrada::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  QSqlQuery query;
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

    if (not query.exec() or not query.first()) { return qApp->enqueueException("Erro buscando xml: " + query.lastError().text(), this); }

    QFile fileXml(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) { return qApp->enqueueException("Erro abrindo arquivo para escrita xml: " + fileXml.errorString(), this); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar XML para ACBr gerar PDF

    const auto pdfOrigem = acbrLocal.gerarDanfe(query.value("xml").toByteArray(), false);

    if (not pdfOrigem) { return; }

    if (pdfOrigem->isEmpty()) { return qApp->enqueueException("Resposta vazia!", this); }

    // copiar para pasta predefinida

    const QString pdfDestino = QDir::currentPath() + "/arquivos/" + chaveAcesso + ".pdf";

    QFile filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(*pdfOrigem, pdfDestino)) { return qApp->enqueueException("Erro copiando pdf!", this); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para " + QDir::currentPath() + "/arquivos/" + "!", this);
}

// TODO: 5copiar filtros do widgetnfesaida
