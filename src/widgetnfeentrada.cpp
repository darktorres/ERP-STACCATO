#include "widgetnfeentrada.h"
#include "ui_widgetnfeentrada.h"

#include "acbrlib.h"
#include "application.h"
#include "checkboxdelegate.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "file.h"
#include "followup.h"
#include "reaisdelegate.h"
#include "sqlquery.h"
#include "xlsxdocument.h"
#include "xml.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>
#include <QUrl>

WidgetNfeEntrada::WidgetNfeEntrada(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeEntrada) { ui->setupUi(this); }

WidgetNfeEntrada::~WidgetNfeEntrada() { delete ui; }

void WidgetNfeEntrada::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxInutilizada, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->checkBoxUtilizada, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxStatus_toggled, connectionType);
  connect(ui->groupBoxUtilizada, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxUtilizada_toggled, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked, connectionType);
  connect(ui->pushButtonExportarExcel, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportarExcel_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonInutilizarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated, connectionType);
}

void WidgetNfeEntrada::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxInutilizada, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxUtilizada, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxStatus_toggled);
  disconnect(ui->groupBoxUtilizada, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxUtilizada_toggled);
  disconnect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked);
  disconnect(ui->pushButtonExportarExcel, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportarExcel_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonFollowup_clicked);
  disconnect(ui->pushButtonInutilizarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated);
}

void WidgetNfeEntrada::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEdit->setDate(qApp->serverDate());
    ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  model.select();

  // ---------------------------------------------------

  modelResumo.select();

  const int fWidth = ui->tableResumo->frameWidth() * 2;

  const int vWidth = ui->tableResumo->verticalHeader()->width();
  const int hWidth = ui->tableResumo->horizontalHeader()->length();

  ui->tableResumo->setFixedWidth(vWidth + hWidth + fWidth);

  const int hHeight = ui->tableResumo->horizontalHeader()->height();
  const int vHeight = ui->tableResumo->verticalHeader()->length();

  ui->tableResumo->setFixedHeight(hHeight + vHeight + fWidth);
}

void WidgetNfeEntrada::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetNfeEntrada::setupTables() {
  // TODO: arrumar a coluna n.tipo pois as nfes de tipo 'ENTRADA' de fornecedor são na verdade nfes de saída
  // TODO: mudar view para puxar apenas as NF-es com cnpjDest igual a raiz da staccato
  model.setTable("view_nfe_entrada");

  model.setHeaderData("utilizada", "Utilizada");
  model.setHeaderData("dataHoraEmissao", "Data");
  model.setHeaderData("dataFollowup", "Data Followup");
  model.setHeaderData("observacao", "Observação");

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

  // ----------------------------------------------------

  modelResumo.setQuery("SELECT status AS Status, COUNT(*) AS `` FROM nfe n WHERE tipo = 'ENTRADA' GROUP BY status");

  ui->tableResumo->setModel(&modelResumo);

  ui->tableResumo->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->tableResumo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  const QString header = model.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(model.data(index.row(), "Venda")); }

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrado XML da NF-e com id: '" + model.data(index.row(), "idNFe").toString() + "'", this); }

  ACBrLib::gerarDanfe(query.value("xml").toString(), true);
}

void WidgetNfeEntrada::montaFiltro() {
  ajustarGroupBoxStatus();
  ajustarGroupBoxUtilizada();

  //-------------------------------------

  QStringList filtros;

  //------------------------------------- filtro texto

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  const QString filtroBusca = "(Emitente LIKE '%" + text + "%' OR NFe LIKE '%" + text + "%' OR OC LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%')";
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

  //------------------------------------- filtro utilizada

  QStringList filtroutilizada;

  if (ui->checkBoxUtilizada->isChecked()) { filtroutilizada << "1"; }
  if (ui->checkBoxInutilizada->isChecked()) { filtroutilizada << "0"; }

  if (not filtroutilizada.isEmpty()) { filtros << "utilizada IN (" + filtroutilizada.join(", ") + ")"; }

  //------------------------------------- filtro loja

  const QString lojaNome = ui->itemBoxLoja->text();
  const QString idLoja = ui->itemBoxLoja->getId().toString();

  if (not lojaNome.isEmpty()) {
    QSqlQuery queryLoja;

    if (not queryLoja.exec("SELECT cnpj FROM loja WHERE idLoja = " + idLoja)) { throw RuntimeException("Erro buscando CNPJ loja: " + queryLoja.lastError().text()); }

    if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados para loja com id: '" + idLoja + "'"); }

    filtros << "`CNPJ Dest` = '" + queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-") + "'";
  }

  //-------------------------------------

  model.setFilter(filtros.join(" AND "));
}

void WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (selection.size() > 1) { throw RuntimeError("Selecione apenas uma linha!", this); }

  const int row = selection.first().row();

  //--------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT status FROM venda_has_produto2 WHERE status IN ('ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.') AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE "
                "idEstoque IN (SELECT idEstoque FROM estoque WHERE idNFe = :idNFe))");
  query.bindValue(":idNFe", model.data(row, "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro verificando pedidos: " + query.lastError().text(), this); }

  if (query.size() > 0) { throw RuntimeError("NF-e possui itens 'EM ENTREGA/ENTREGUE'!", this); }

  //--------------------------------------------------------------

  if (model.data(row, "nsu").toInt() > 0 and not model.data(row, "utilizada").toBool()) { throw RuntimeError("NF-e não utilizada!", this); }

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

  if (not queryUpdateNFe.exec()) { throw RuntimeException("Erro marcando NF-e como não utilizada: " + queryUpdateNFe.lastError().text()); }
}

void WidgetNfeEntrada::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  for (const auto &index : selection) {
    // TODO: se a conexao com o acbr falhar ou der algum erro pausar o loop e perguntar para o usuario se ele deseja tentar novamente (do ponto que parou)
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    if (model.data(index.row(), "status").toString() == "RESUMO") { continue; }

    // pegar XML do MySQL e salvar em arquivo

    const QString chaveAcesso = model.data(index.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("Não encontrou XML da NF-e com chave de acesso: '" + chaveAcesso + "'"); }

    File fileXml(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita XML: " + fileXml.errorString()); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar XML para ACBr gerar PDF

    ACBrLib::gerarDanfe(query.value("xml").toString(), false);

    // copiar para pasta predefinida

    const QString pdfOrigem = QDir::currentPath() + "/pdf/" + chaveAcesso + "-nfe.pdf";
    const QString pdfDestino = QDir::currentPath() + "/arquivos/" + chaveAcesso + ".pdf";

    File filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(pdfOrigem, pdfDestino)) { throw RuntimeException("Erro copiando PDF!"); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para:\n" + QDir::currentPath() + "/arquivos/", this);
}

void WidgetNfeEntrada::on_pushButtonExportarExcel_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QString fileName = "dados_nfe.xlsx";

  QXlsx::Document xlsx(fileName, this);
  xlsx.write("A1", "Fornecedor");
  xlsx.write("B1", "CNPJ");
  xlsx.write("C1", "UF");
  xlsx.write("D1", "Produto");
  xlsx.write("E1", "NCM");
  xlsx.write("F1", "CST");

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  int row = 2;

  for (const auto &index : selection) {
    if (model.data(index.row(), "status").toString() == "RESUMO") { continue; }

    const QString chaveAcesso = model.data(index.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("Não encontrou XML da NF-e com chave de acesso: '" + chaveAcesso + "'"); }

    XML xml(query.value("xml").toString());
    xml.exportarDados(xlsx, row);
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Erro ao salvar arquivo!"); }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, this);
}

void WidgetNfeEntrada::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  try {
    const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

    for (const auto &child : children) {
      child->setEnabled(true);
      child->setChecked(enabled);
    }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  montaFiltro();
}

void WidgetNfeEntrada::on_groupBoxUtilizada_toggled(const bool enabled) {
  unsetConnections();

  try {
    const auto children = ui->groupBoxUtilizada->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

    for (const auto &child : children) {
      child->setEnabled(true);
      child->setChecked(enabled);
    }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  montaFiltro();
}

void WidgetNfeEntrada::ajustarGroupBoxStatus() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (auto *checkBox : filtrosStatus) {
    if (checkBox->isChecked()) { empty = false; }
  }

  unsetConnections();

  ui->groupBoxStatus->setChecked(not empty);

  for (auto *checkBox : filtrosStatus) { checkBox->setEnabled(true); }

  setConnections();
}

void WidgetNfeEntrada::ajustarGroupBoxUtilizada() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxUtilizada->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (auto *checkBox : filtrosStatus) {
    if (checkBox->isChecked()) { empty = false; }
  }

  unsetConnections();

  ui->groupBoxUtilizada->setChecked(not empty);

  for (auto *checkBox : filtrosStatus) { checkBox->setEnabled(true); }

  setConnections();
}

void WidgetNfeEntrada::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = model.data(selection.first().row(), "idNFe").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::NFe, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: colocar opção de buscar por uma palavra-chave para buscar NF-es de um produto especifico, por ex: notebook
// TODO: colocar followup
