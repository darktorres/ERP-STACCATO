#include "widgetnfeentrada.h"
#include "ui_widgetnfeentrada.h"

#include "acbrlib.h"
#include "application.h"
#include "checkboxdelegate.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "file.h"
#include "reaisdelegate.h"
#include "user.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>

WidgetNfeEntrada::WidgetNfeEntrada(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeEntrada) { ui->setupUi(this); }

WidgetNfeEntrada::~WidgetNfeEntrada() { delete ui; }

void WidgetNfeEntrada::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeEntrada::delayFiltro, connectionType);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked, connectionType);
  connect(ui->pushButtonInutilizarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated, connectionType);
}

void WidgetNfeEntrada::unsetConnections() {
  blockingSignals.push(0);

  disconnect(&timer, &QTimer::timeout, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeEntrada::delayFiltro);
  disconnect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked);
  disconnect(ui->pushButtonInutilizarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated);
}

void WidgetNfeEntrada::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    ui->dateEdit->setDate(qApp->serverDate());
    ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  model.select();
}

void WidgetNfeEntrada::delayFiltro() { timer.start(qApp->delayedTimer); }

void WidgetNfeEntrada::resetTables() { modelIsSet = false; }

void WidgetNfeEntrada::setupTables() {
  // TODO: arrumar a coluna n.tipo pois as nfes de tipo 'ENTRADA' de fornecedor são na verdade nfes de saída
  // TODO: mudar view para puxar apenas as NFes com cnpjDest igual a raiz da staccato
  model.setTable("view_nfe_entrada");

  model.setHeaderData("utilizada", "Utilizada");
  model.setHeaderData("dataHoraEmissao", "Data Emissão");

  ui->table->setModel(&model);

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");
  ui->table->hideColumn("Fornecedor");
  ui->table->hideColumn("nsu");

  ui->table->setItemDelegate(new DoubleDelegate(this));

  ui->table->setItemDelegateForColumn("GARE", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("GARE Pago Em", new DateFormatDelegate(this));
  ui->table->setItemDelegateForColumn("utilizada", new CheckBoxDelegate(true, this));

  ui->table->setPersistentColumns({"utilizada"});
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NFe: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrado XML da NFe com id: " + model.data(index.row(), "idNFe").toString(), this); }

  ACBrLib::gerarDanfe(query.value("xml"), true);
}

void WidgetNfeEntrada::montaFiltro() {
  ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtros;

  //------------------------------------- filtro texto

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  const QString filtroBusca = "Emitente LIKE '%" + text + "%' OR NFe LIKE '%" + text + "%' OR OC LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%'";
  if (not text.isEmpty()) { filtros << filtroBusca; }

  //------------------------------------- filtro data

  const QString filtroData = ui->groupBoxMes->isChecked() ? "DATE_FORMAT(`Importado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";
  if (not filtroData.isEmpty()) { filtros << filtroData; }

  //------------------------------------- filtro status

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //------------------------------------- filtro loja

  const QString lojaNome = ui->itemBoxLoja->text();
  const QString idLoja = ui->itemBoxLoja->getId().toString();

  if (not lojaNome.isEmpty()) {
    QSqlQuery queryLoja;

    if (not queryLoja.exec("SELECT cnpj FROM loja WHERE idLoja = " + idLoja)) { throw RuntimeException("Erro buscando CNPJ loja: " + queryLoja.lastError().text()); }

    if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados para loja com id: " + idLoja); }

    filtros << "`CNPJ Dest` = '" + queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-") + "'";
  }

  //-------------------------------------

  model.setFilter(filtros.join(" AND "));
}

void WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (list.size() > 1) { throw RuntimeError("Selecione apenas uma linha!", this); }

  const int row = list.first().row();

  //--------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT status FROM venda_has_produto2 WHERE status IN ('ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.') AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE "
                "idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe))");
  query.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro verificando pedidos: " + query.lastError().text(), this); }

  if (query.size() > 0) { throw RuntimeError("NFe possui itens 'EM ENTREGA/ENTREGUE'!", this); }

  //--------------------------------------------------------------

  if (model.data(row, "nsu").toInt() > 0 and not model.data(row, "utilizada").toBool()) { throw RuntimeError("NFe não utilizada!", this); }

  //--------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Inutilizar?", "Tem certeza que deseja inutilizar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Inutilizar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  qApp->startTransaction("WidgetNfeEntrada::on_pushButtonInutilizarNFe");

  inutilizar(row);

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Inutilizado com sucesso!", this);
}

void WidgetNfeEntrada::inutilizar(const int row) {
  // TODO: em vez de deletar linhas apenas marcar como cancelado?

  SqlQuery queryPedidoFornecedor;
  queryPedidoFornecedor.prepare(
      "UPDATE `pedido_fornecedor_has_produto2` SET status = 'EM FATURAMENTO', quantUpd = 0, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
      "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idPedido2` IN (SELECT `idPedido2` FROM estoque_has_compra WHERE idEstoque IN (SELECT idEstoque "
      "FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  queryPedidoFornecedor.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryPedidoFornecedor.exec()) { throw RuntimeException("Erro voltando compra para faturamento: " + queryPedidoFornecedor.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryVendaProduto;
  queryVendaProduto.prepare(
      "UPDATE venda_has_produto2 SET status = 'EM FATURAMENTO', dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, "
      "dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` IN (SELECT "
      "`idVendaProduto2` FROM estoque_has_consumo WHERE idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe)) AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  queryVendaProduto.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryVendaProduto.exec()) { throw RuntimeException("Erro voltando venda para faturamento: " + queryVendaProduto.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryEstoque;
  queryEstoque.prepare("SELECT idEstoque FROM estoque WHERE idNFe = :idNFe");
  queryEstoque.bindValue(":idNFe", model.data(row, "idNFe"));

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
  queryDeleteCompra.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryDeleteCompra.exec()) { throw RuntimeException("Erro removendo compras: " + queryDeleteCompra.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryProduto;
  queryProduto.prepare("UPDATE produto SET desativado = TRUE WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  queryProduto.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryProduto.exec()) { throw RuntimeException("Erro removendo produto estoque: " + queryProduto.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryCancelaEstoque;
  queryCancelaEstoque.prepare("UPDATE estoque SET status = 'CANCELADO', idNFe = NULL WHERE idEstoque IN (SELECT idEstoque FROM (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe) temp)");
  queryCancelaEstoque.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryCancelaEstoque.exec()) { throw RuntimeException("Erro removendo estoque: " + queryCancelaEstoque.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryCancelaGare;
  queryCancelaGare.prepare("DELETE FROM conta_a_pagar_has_pagamento WHERE idNFe = :idNFe AND status IN ('PENDENTE GARE', 'LIBERADO GARE')");
  queryCancelaGare.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryCancelaGare.exec()) { throw RuntimeException("Erro removendo GARE: " + queryCancelaGare.lastError().text()); }

  //-----------------------------------------------------------------------------

  SqlQuery queryUpdateNFe;
  queryUpdateNFe.prepare("UPDATE nfe SET utilizada = FALSE WHERE idNFe = :idNFe");
  queryUpdateNFe.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not queryUpdateNFe.exec()) { throw RuntimeException("Erro marcando NFe como não utilizada: " + queryUpdateNFe.lastError().text()); }
}

void WidgetNfeEntrada::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  for (const auto &index : list) {
    // TODO: se a conexao com o acbr falhar ou der algum erro pausar o loop e perguntar para o usuario se ele deseja tentar novamente (do ponto que parou)
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    if (model.data(index.row(), "status").toString() == "RESUMO") { continue; }

    // pegar XML do MySQL e salvar em arquivo

    const QString chaveAcesso = model.data(index.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec()) { throw RuntimeException("Erro buscando XML da NFe: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("Não encontrou XML da NFe com chave: " + chaveAcesso); }

    File fileXml(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita xml: " + fileXml.errorString()); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar XML para ACBr gerar PDF

    ACBrLib::gerarDanfe(query.value("xml"), false);

    // copiar para pasta predefinida

    const QString pdfOrigem = QDir::currentPath() + "/pdf/" + chaveAcesso + "-nfe.pdf";
    const QString pdfDestino = QDir::currentPath() + "/arquivos/" + chaveAcesso + ".pdf";

    File filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(pdfOrigem, pdfDestino)) { throw RuntimeException("Erro copiando pdf!"); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para:\n" + QDir::currentPath() + "/arquivos/", this);
}

void WidgetNfeEntrada::ajustarGroupBoxStatus() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (auto *checkBox : filtrosStatus) {
    if (checkBox->isChecked()) { empty = false; }
  }

  unsetConnections();

  ui->groupBoxStatus->setChecked(not empty);

  for (auto *checkBox : filtrosStatus) { checkBox->setEnabled(true); }

  setConnections();
}

// TODO: 5copiar filtros do widgetnfesaida
// TODO: colocar opção de buscar por uma palavra-chave para buscar NFes de um produto especifico, por ex: notebook
