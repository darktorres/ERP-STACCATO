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
#include "user.h"
#include "xlsxdocument.h"
#include "xml.h"
#include "xml_viewer.h"

#include <QAuthenticator>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFormLayout>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSet>
#include <QSpinBox>
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
  connect(ui->dateEditAte, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->dateEditDe, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::on_dateEditDe_dateChanged, connectionType);
  connect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxMes_toggled, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxStatus_toggled, connectionType);
  connect(ui->groupBoxUtilizada, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxUtilizada_toggled, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetNfeEntrada::montaFiltro, connectionType);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked, connectionType);
  connect(ui->pushButtonExportarExcel, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportarExcel_clicked, connectionType);
  connect(ui->pushButtonExportarMes, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportarMes_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonInutilizarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated, connectionType);
  connect(ui->table->verticalScrollBar(), &QScrollBar::valueChanged, this, &WidgetNfeEntrada::onTableScrolled, connectionType);
}

void WidgetNfeEntrada::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxInutilizada, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->checkBoxUtilizada, &QCheckBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->dateEditAte, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->dateEditDe, &QDateEdit::dateChanged, this, &WidgetNfeEntrada::on_dateEditDe_dateChanged);
  disconnect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxMes_toggled);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxStatus_toggled);
  disconnect(ui->groupBoxUtilizada, &QGroupBox::toggled, this, &WidgetNfeEntrada::on_groupBoxUtilizada_toggled);
  disconnect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetNfeEntrada::montaFiltro);
  disconnect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportar_clicked);
  disconnect(ui->pushButtonExportarExcel, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonExportarExcel_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonFollowup_clicked);
  disconnect(ui->pushButtonInutilizarNFe, &QPushButton::clicked, this, &WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetNfeEntrada::on_table_activated);
  disconnect(ui->table->verticalScrollBar(), &QScrollBar::valueChanged, this, &WidgetNfeEntrada::onTableScrolled);
}

void WidgetNfeEntrada::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEditAte->setDate(qApp->serverDate());
    ui->dateEditDe->setDate(QDate(qApp->serverDate().year(), qApp->serverDate().month(), 1));
    ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
    setupTables();
    setConnections();
    isSet = true;
  }

  montaFiltro();

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

void WidgetNfeEntrada::resetTables() { setupTables(); }

void WidgetNfeEntrada::setupTables() {
  // TODO: arrumar a coluna n.tipo pois as nfes de tipo 'ENTRADA' de fornecedor são na verdade nfes de saída
  // TODO: mudar view para puxar apenas as NF-es com cnpjDest igual a raiz da staccato

  ui->table->setModel(&model);

  montaFiltro(); // monta e executa a 1a pagina, para popular as colunas do model antes de configurar a tabela

  model.setHeaderLabel("utilizada", "Utilizada");
  model.setHeaderLabel("dataHoraEmissao", "Data");
  model.setHeaderLabel("dataFollowup", "Data Followup");
  model.setHeaderLabel("observacao", "Observação");

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");
  ui->table->hideColumn("Fornecedor");
  ui->table->hideColumn("nsu");

  ui->table->setItemDelegate(new DoubleDelegate(this));

  ui->table->setItemDelegateForColumn("GARE", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("GARE Pago Em", new DateFormatDelegate(this));
  // "Utilizada" (não "utilizada"): o model paginado não é QSqlQueryModel, então TableView resolve a
  // coluna via headerData() (o rótulo renomeado acima), não via record() (nome cru da coluna SQL)
  ui->table->setItemDelegateForColumn("Utilizada", new CheckBoxDelegate(true, this));

  ui->table->setPersistentColumns({"Utilizada"});

  // ----------------------------------------------------

  modelResumo.setQuery("SELECT status AS Status, COUNT(*) AS `` FROM nfe n WHERE tipo = 'ENTRADA' GROUP BY status");

  ui->tableResumo->setModel(&modelResumo);

  ui->tableResumo->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->tableResumo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void WidgetNfeEntrada::onTableScrolled(const int value) {
  if (carregandoPagina) { return; }

  carregandoPagina = true;

  QElapsedTimer timer;
  timer.start();

  try {
    auto *scrollBar = ui->table->verticalScrollBar();
    const int threshold = ui->table->verticalHeader()->defaultSectionSize() * 5;

    qDebug() << "[WidgetNfeEntrada] onTableScrolled: value=" << value << "max=" << scrollBar->maximum() << "threshold=" << threshold;

    if (value >= scrollBar->maximum() - threshold) { model.tryLoadNext(); }
    if (value <= threshold) { model.tryLoadPrevious(); }
  } catch (...) {
    carregandoPagina = false;
    throw;
  }

  carregandoPagina = false;

  qDebug() << "[WidgetNfeEntrada] onTableScrolled: total" << timer.elapsed() << "ms";
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
  // auto *viewer = new XML_Viewer(query.value("xml").toString(), nullptr);
  // viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetNfeEntrada::montaFiltro() {
  // ajustarGroupBoxStatus();
  // ajustarGroupBoxUtilizada();

  //-------------------------------------

  QStringList filtrosNFe;    // tocam só n.* — entram na subquery de corte, antes do JOIN
  QStringList filtrosResumo; // tocam r.* (join com nfe_resumo_compra) — só no WHERE externo, depois do join

  //------------------------------------- filtro texto

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  if (not text.isEmpty()) { filtrosResumo << "(n.emitente LIKE '%" + text + "%' OR n.numeroNFe LIKE '%" + text + "%' OR r.ordemCompra LIKE '%" + text + "%' OR r.idVenda LIKE '%" + text + "%')"; }

  //------------------------------------- filtro data (comparacao direta na coluna, sem funcao, para poder usar indice)

  if (ui->groupBoxMes->isChecked()) {
    filtrosNFe << "n.dataHoraEmissao >= '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + " 00:00:00' AND n.dataHoraEmissao < '" + ui->dateEditAte->date().addDays(1).toString("yyyy-MM-dd") + " 00:00:00'";
  }

  //------------------------------------- filtro status

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtrosNFe << "n.status IN (" + filtroCheck.join(", ") + ")"; }

  //------------------------------------- filtro utilizada

  QStringList filtroutilizada;

  if (ui->checkBoxUtilizada->isChecked()) { filtroutilizada << "1"; }
  if (ui->checkBoxInutilizada->isChecked()) { filtroutilizada << "0"; }

  if (not filtroutilizada.isEmpty()) { filtrosNFe << "n.utilizada IN (" + filtroutilizada.join(", ") + ")"; }

  //------------------------------------- filtro loja

  const QString lojaNome = ui->itemBoxLoja->text();
  const QString idLoja = ui->itemBoxLoja->getId().toString();

  if (not lojaNome.isEmpty()) {
    QSqlQuery queryLoja;

    if (not queryLoja.exec("SELECT cnpj FROM loja WHERE idLoja = " + idLoja)) { throw RuntimeException("Erro buscando CNPJ loja: " + queryLoja.lastError().text()); }

    if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados para loja com id: '" + idLoja + "'"); }

    filtrosNFe << "n.cnpjDest = '" + queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-") + "'";
  }

  //------------------------------------- colunas e expressao SQL de cada uma (pra ORDER BY/keyset da paginacao)
  // -- Entrada nao tem "leque" (resumo materializado colapsou o que precisava de estoque/pedido/GARE), entao a
  // subquery de corte sempre junta n+r direto, nao importa qual coluna esta sendo usada pra ordenar

  static const QStringList fieldNames = {"idNFe",        "chaveAcesso", "CNPJ Dest",     "Emitente", "Fornecedor",   "NFe",       "Status",
                                          "Recebido Por", "Data Receb",  "GARE",          "GARE Pago Em", "OC",   "Venda",     "nsu",
                                          "utilizada",    "dataHoraEmissao", "dataFollowup", "observacao"};

  static const QHash<QString, QString> exprPorCampo = {
      {"idNFe", "n.idNFe"},       {"chaveAcesso", "n.chaveAcesso"},   {"CNPJ Dest", "n.cnpjDest"}, {"Emitente", "n.emitente"},
      {"Fornecedor", "r.fornecedor"}, {"NFe", "n.numeroNFe"},         {"Status", "n.status"},      {"Recebido Por", "r.recebidoPor"},
      {"Data Receb", "r.dataRealReceb"}, {"GARE", "r.gare"},          {"GARE Pago Em", "r.garePagoEm"}, {"OC", "r.ordemCompra"},
      {"Venda", "r.idVenda"},     {"nsu", "n.nsu"},                   {"utilizada", "n.utilizada"}, {"dataHoraEmissao", "n.dataHoraEmissao"},
      {"dataFollowup", "nhf.dataFollowup"}, {"observacao", "nhf.observacao"},
  };

  const QStringList filtrosNFeCopia = filtrosNFe;
  const QStringList filtrosResumoCopia = filtrosResumo;

  const SqlPaginatedModel::QueryBuilderFactory factory = [filtrosNFeCopia, filtrosResumoCopia](const QString &sortColumn, const Qt::SortOrder order) -> SqlPaginatedModel::PageQueryBuilder {
    const QString sortExpr = exprPorCampo.value(sortColumn, "n.dataHoraEmissao");

    return [filtrosNFeCopia, filtrosResumoCopia, sortExpr, order](const SqlPaginatedModel::PageRequest &request) -> QString {
      const bool forward = request.direction != SqlPaginatedModel::Direction::Previous;

      QStringList capFiltros;
      capFiltros << "n.tipo = 'ENTRADA'";
      capFiltros += filtrosNFeCopia;

      if (request.direction != SqlPaginatedModel::Direction::First) {
        capFiltros << SqlPaginatedModel::buildKeysetWhere(sortExpr, "n.idNFe", request.cursorValue, request.cursorId, order, forward);
      }

      const QString capOrderBy = SqlPaginatedModel::buildOrderBy(sortExpr, "n.idNFe", order, forward);

      // FORCE INDEX: sem isso o otimizador as vezes escolhe um indice so de status (nao-covering,
      // bookmark lookup linha a linha) quando ha filtro de status/utilizada sem filtro de data -
      // medido ~2-4s contra ~50-170ms com o indice forcado (ver db/add_index_nfe_tipo_status_utilizada_data.sql)
      const QString capSql = "SELECT n.idNFe FROM nfe n FORCE INDEX (idx_nfe_tipo_status_utilizada_data) LEFT JOIN nfe_resumo_compra r ON r.idNFe = n.idNFe WHERE " + capFiltros.join(" AND ") +
                             " ORDER BY " + capOrderBy + " LIMIT " + QString::number(1000);

      const QString exibicaoOrderBy = SqlPaginatedModel::buildOrderBy(sortExpr, "n.idNFe", order, true); // exibicao sempre na ordem normal

      return "SELECT n.idNFe AS idNFe, n.chaveAcesso AS chaveAcesso, n.cnpjDest AS `CNPJ Dest`, n.emitente AS Emitente, "
             "r.fornecedor AS Fornecedor, n.numeroNFe AS NFe, n.status AS Status, "
             "r.recebidoPor AS `Recebido Por`, r.dataRealReceb AS `Data Receb`, "
             "r.gare AS GARE, r.garePagoEm AS `GARE Pago Em`, "
             "r.ordemCompra AS OC, r.idVenda AS Venda, "
             "n.nsu AS nsu, n.utilizada AS utilizada, n.dataHoraEmissao AS dataHoraEmissao, "
             "nhf.dataFollowup AS dataFollowup, nhf.observacao AS observacao "
             "FROM (" +
             capSql +
             ") lim "
             "JOIN nfe n ON n.idNFe = lim.idNFe "
             "LEFT JOIN nfe_resumo_compra r ON r.idNFe = n.idNFe "
             "LEFT JOIN nfe_has_followup nhf ON (n.idFollowup = nhf.idFollowup)" +
             (filtrosResumoCopia.isEmpty() ? "" : " WHERE " + filtrosResumoCopia.join(" AND ")) + " ORDER BY " + exibicaoOrderBy;
    };
  };

  const QString sortColumnAtual = model.sortColumn().isEmpty() ? "dataHoraEmissao" : model.sortColumn();

  model.reset(fieldNames, sortColumnAtual, model.sortOrder(), factory);
}

void WidgetNfeEntrada::on_pushButtonInutilizarNFe_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (selection.size() > 1) { throw RuntimeError("Selecione apenas uma linha!", this); }

  const int row = selection.first().row();

  //--------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT status FROM venda_has_produto2 WHERE status IN ('ENTREGUE', 'EM ENTREGA', 'SEPARADO', 'ENTREGA AGEND.') AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE "
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

void WidgetNfeEntrada::on_pushButtonExportarMes_clicked() {
  // Envia ao WebDAV o DANFE de cada NF-e de ENTRADA com duplicata (conta_a_pagar, exceto GARE)
  // vencendo no mês escolhido (ou só num dia específico dele), em 'Pagamentos Diarios/<ano>/<MM - Mês_ano>/NOTAS <dd.MM>/'
  // (pasta do dia = dia de vencimento), nome "<valor> - <fornecedor>.pdf". Pula arquivos de mesmo nome já presentes no servidor (HEAD).

  // o Apache exige essa permissão para escrever em /webdav/FINANCEIRO (senão devolve HTTP 401)
  if (not User::temPermissao("webdav_financeiro")) {
    throw RuntimeError("Usuário não possui a permissão 'Rede - Financeiro', necessária para enviar os arquivos ao servidor!\nMarque a permissão no Cadastro de Usuário.", this);
  }

  const QLocale brLocale(QLocale::Portuguese, QLocale::Brazil);

  // ----------------------------------------- diálogo de mês/ano/dia

  QDialog dialog(this);
  dialog.setWindowTitle("Exportar Notas do Mês");

  auto *comboMes = new QComboBox(&dialog);
  for (int m = 1; m <= 12; ++m) { comboMes->addItem(brLocale.monthName(m), m); }

  auto *spinAno = new QSpinBox(&dialog);
  spinAno->setRange(2024, qApp->serverDate().year() + 1);

  auto *comboDia = new QComboBox(&dialog);
  comboDia->addItem("Mês inteiro", 0);
  for (int d = 1; d <= 31; ++d) { comboDia->addItem(QString::number(d), d); }

  comboMes->setCurrentIndex(qApp->serverDate().month() - 1);
  spinAno->setValue(qApp->serverDate().year());

  auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  auto *layout = new QFormLayout(&dialog);
  layout->addRow("Mês:", comboMes);
  layout->addRow("Ano:", spinAno);
  layout->addRow("Dia:", comboDia);
  layout->addRow(buttonBox);

  if (dialog.exec() != QDialog::Accepted) { return; }

  const int mes = comboMes->currentData().toInt();
  const int ano = spinAno->value();
  const int dia = comboDia->currentData().toInt(); // 0 = mês inteiro

  const QString periodo = (dia > 0 ? QString("%1/").arg(dia, 2, 10, QChar('0')) : QString()) + brLocale.monthName(mes) + "/" + QString::number(ano);

  // ----------------------------------------- duplicatas (não-GARE) com vencimento no período, por nota e dia de vencimento

  // Soma das duplicatas por nota e por dia de vencimento. contraParte = nome curto do fornecedor (fallback: emitente).
  SqlQuery query;
  query.prepare("SELECT cp.idNFe, cp.dataPagamento AS venc, SUM(cp.valor) AS valorDia, n.numeroNFe, n.chaveAcesso, "
                "MAX(cp.contraParte) AS contraParte, MAX(n.emitente) AS emitente "
                "FROM conta_a_pagar_has_pagamento cp JOIN nfe n ON n.idNFe = cp.idNFe AND n.tipo = 'ENTRADA' "
                "WHERE COALESCE(cp.contraParte, '') <> 'GARE' AND COALESCE(cp.status, '') NOT LIKE '%GARE%' "
                "AND YEAR(cp.dataPagamento) = :ano AND MONTH(cp.dataPagamento) = :mes " +
                QString(dia > 0 ? "AND DAY(cp.dataPagamento) = :dia " : "") +
                "GROUP BY cp.idNFe, cp.dataPagamento, n.numeroNFe, n.chaveAcesso ORDER BY cp.dataPagamento, cp.idNFe");
  query.bindValue(":ano", ano);
  query.bindValue(":mes", mes);
  if (dia > 0) { query.bindValue(":dia", dia); }

  if (not query.exec()) { throw RuntimeException("Erro buscando duplicatas: " + query.lastError().text(), this); }

  if (query.size() == 0) { return qApp->enqueueInformation("Nenhuma NF-e com vencimento em " + periodo + ".", this); }

  // ----------------------------------------- WebDAV (mesmo servidor/credenciais das fotos de entrega)

  const QString webdavIp = qApp->getWebDavIp();

  if (webdavIp.isEmpty()) { throw RuntimeError("Servidor WebDAV (Locaweb) não configurado!", this); }

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [](QNetworkReply *, QAuthenticator *authenticator) {
    authenticator->setUser(User::usuario);
    authenticator->setPassword(User::senha);
  });

  // Autenticação preemptiva: o re-envio após 401 do authenticationRequired não é confiável para verbos custom (MKCOL via
  // sendCustomRequest), então mandamos as credenciais já na primeira requisição (vale p/ MKCOL/PUT/HEAD).
  const QByteArray basicAuth = "Basic " + (User::usuario + ":" + User::senha).toUtf8().toBase64();

  const auto enc = [](const QString &texto) { return QString::fromUtf8(QUrl::toPercentEncoding(texto)); };

  // Executa um request (PUT/MKCOL) de forma síncrona, seguindo redirecionamentos. Retorna "" em sucesso.
  const auto enviar = [&](const QByteArray &verbo, const QString &urlStr, const QByteArray &corpo) -> QString {
    QUrl url(urlStr);

    for (int tentativa = 0; tentativa < 5; ++tentativa) {
      QNetworkRequest req(url);
      req.setRawHeader("Authorization", basicAuth);
      QEventLoop loop;

      QNetworkReply *reply = (verbo == "PUT") ? manager->put(req, corpo) : manager->sendCustomRequest(req, verbo, corpo);
      connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
      loop.exec();

      const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      const QNetworkReply::NetworkError erro = reply->error();
      const QString erroStr = reply->errorString();
      reply->deleteLater();

      if (redirect.isValid()) {
        url = redirect.isRelative() ? url.resolved(redirect) : redirect;
        continue;
      }

      if (verbo == "MKCOL" and http == 405) { return QString(); } // coleção já existe -> ok

      if (erro != QNetworkReply::NoError) { return erroStr + (http > 0 ? " (HTTP " + QString::number(http) + ")" : QString()); }

      return QString();
    }

    return "muitos redirecionamentos";
  };

  // HEAD read-only: tamanho do arquivo no servidor; -1 se existe mas sem Content-Length; -2 se não encontrado/erro.
  const auto tamanhoRemoto = [&](const QString &urlStr) -> qint64 {
    QUrl url(urlStr);

    for (int tentativa = 0; tentativa < 5; ++tentativa) {
      QNetworkRequest req(url);
      req.setRawHeader("Authorization", basicAuth);
      QEventLoop loop;

      QNetworkReply *reply = manager->head(req);
      connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
      loop.exec();

      const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
      reply->deleteLater();

      if (redirect.isValid()) {
        url = redirect.isRelative() ? url.resolved(redirect) : redirect;
        continue;
      }

      if (http != 200) { return -2; }

      return contentLength.isValid() ? contentLength.toLongLong() : -1;
    }

    return -2;
  };

  // true se o arquivo (mesmo nome) já existe no servidor.
  const auto existe = [&](const QString &urlStr) -> bool { return tamanhoRemoto(urlStr) != -2; };

  const QString baseUrl = "https://" + webdavIp + "/webdav/FINANCEIRO/FINANCEIRO/Contas a Pagar/Pagamentos Diarios/";

  QString mesNome = brLocale.monthName(mes);
  mesNome = mesNome.left(1).toUpper() + mesNome.mid(1); // "maio" -> "Maio", "março" -> "Março"
  const QString mesPasta = QString("%1 - %2_%3").arg(mes, 2, 10, QChar('0')).arg(mesNome).arg(ano);

  const QString anoDirUrl = baseUrl + enc(QString::number(ano)) + "/";
  const QString mesDirUrl = anoDirUrl + enc(mesPasta) + "/";

  QString erroPasta = enviar("MKCOL", anoDirUrl, {});
  if (erroPasta.isEmpty()) { erroPasta = enviar("MKCOL", mesDirUrl, {}); }
  if (not erroPasta.isEmpty()) { throw RuntimeException("Erro criando pasta no servidor: " + erroPasta, this); }

  // ----------------------------------------- envio (uma pasta NOTAS dd.MM por dia de vencimento)

  SqlQuery queryXml;
  queryXml.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");

  QProgressDialog progress("Enviando notas para o servidor...", "Cancelar", 0, query.size(), this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);

  QHash<int, QByteArray> pdfCache;  // DANFE gerado por idNFe (gera uma vez por nota)
  QSet<QString> pastasDiaCriadas;   // pastas NOTAS dd.MM já criadas no servidor
  QSet<QString> nomesUsados;        // dirUrl|nomeArquivo, para evitar colisão dentro da mesma pasta
  QStringList avisos;
  int enviados = 0;
  int ignorados = 0;
  int i = 0;

  while (query.next()) {
    if (progress.wasCanceled()) { break; }
    progress.setValue(i++);

    const int idNFe = query.value("idNFe").toInt();
    const QDate venc = query.value("venc").toDate();
    const double valorDia = query.value("valorDia").toDouble();
    const QString numeroNFe = query.value("numeroNFe").toString();
    const QString chaveAcesso = query.value("chaveAcesso").toString();

    // nome curto do fornecedor: contraParte (mais limpo); fallback p/ emitente (razão social) e, por fim, número da NF
    QString fornecedor = query.value("contraParte").toString().trimmed();
    if (fornecedor.isEmpty()) { fornecedor = query.value("emitente").toString().trimmed(); }
    fornecedor.replace(QRegularExpression(R"([\\/:*?"<>|])"), " "); // remove caracteres inválidos em nome de arquivo
    fornecedor = fornecedor.simplified();
    if (fornecedor.isEmpty()) { fornecedor = "NF " + numeroNFe; }

    // pasta do dia = dia de vencimento da duplicata
    const QString diaPasta = "NOTAS " + venc.toString("dd.MM");
    const QString diaDirUrl = mesDirUrl + enc(diaPasta) + "/";

    // nome do arquivo: "<valor> - <fornecedor>.pdf" (casa com a convenção da pasta)
    const QString base = brLocale.toString(valorDia, 'f', 2) + " - " + fornecedor;
    QString nomeArquivo = base + ".pdf";

    if (nomesUsados.contains(diaDirUrl + "|" + nomeArquivo)) { nomeArquivo = base + " (NF " + numeroNFe + ").pdf"; }
    nomesUsados.insert(diaDirUrl + "|" + nomeArquivo);

    const QString fileUrl = diaDirUrl + enc(nomeArquivo);

    // pula se o arquivo (mesmo nome) já existe no servidor
    if (existe(fileUrl)) {
      ++ignorados;
      continue;
    }

    // cria a pasta do dia só quando há algo novo a enviar
    if (not pastasDiaCriadas.contains(diaDirUrl)) {
      const QString erroDia = enviar("MKCOL", diaDirUrl, {});

      if (not erroDia.isEmpty()) {
        avisos << diaPasta + ": erro criando pasta (" + erroDia + ")";
        continue;
      }

      pastasDiaCriadas.insert(diaDirUrl);
    }

    // gera (uma vez por nota) o DANFE via ACBr em ./pdf/{chaveAcesso}-nfe.pdf
    if (not pdfCache.contains(idNFe)) {
      queryXml.bindValue(":idNFe", idNFe);

      if (not queryXml.exec() or not queryXml.first()) {
        avisos << "NF " + numeroNFe + ": XML não encontrado";
        continue;
      }

      const QString xml = queryXml.value("xml").toString();

      if (xml.isEmpty()) {
        avisos << "NF " + numeroNFe + ": XML vazio";
        continue;
      }

      ACBrLib::gerarDanfe(xml, false);

      File pdf(QDir::currentPath() + "/pdf/" + chaveAcesso + "-nfe.pdf");

      if (not pdf.exists() or not pdf.open(QFile::ReadOnly)) {
        avisos << "NF " + numeroNFe + ": PDF não gerado";
        continue;
      }

      pdfCache.insert(idNFe, pdf.readAll());
      pdf.close();
    }

    const QByteArray &corpo = pdfCache.value(idNFe);

    if (corpo.isEmpty()) {
      avisos << nomeArquivo + ": PDF vazio (0 bytes)";
      continue;
    }

    // envia e confirma via HEAD que o tamanho gravado bate; re-tenta uma vez se a verificação falhar
    QString erroPut;

    for (int tentativa = 0; tentativa < 2; ++tentativa) {
      erroPut = enviar("PUT", fileUrl, corpo);

      if (not erroPut.isEmpty()) { continue; }

      const qint64 remoto = tamanhoRemoto(fileUrl);

      if (remoto == corpo.size() or remoto == -1) { break; }

      erroPut = (remoto == -2) ? "arquivo não encontrado no servidor após o envio"
                               : "servidor gravou " + QString::number(remoto) + " bytes (esperado " + QString::number(corpo.size()) + ")";
    }

    if (not erroPut.isEmpty()) {
      avisos << nomeArquivo + ": " + erroPut;
      continue;
    }

    ++enviados;
  }

  progress.setValue(query.size());

  // ----------------------------------------- resultado

  QString msg = QString("%1 nota(s) enviada(s), %2 já existia(m) no servidor (venc. em %3).").arg(enviados).arg(ignorados).arg(periodo);

  if (not avisos.isEmpty()) { msg += "\n\nAvisos (" + QString::number(avisos.size()) + "):\n" + avisos.join("\n"); }

  qApp->enqueueInformation(msg, this);
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

void WidgetNfeEntrada::on_dateEditDe_dateChanged(const QDate date) { ui->dateEditAte->setDate(date); }

void WidgetNfeEntrada::on_groupBoxMes_toggled(const bool enabled) {
  const auto children = ui->groupBoxMes->findChildren<QDateEdit *>(QRegularExpression("dateEdit"));

  for (const auto &child : children) { child->setEnabled(enabled); }
}

// TODO: colocar opção de buscar por uma palavra-chave para buscar NF-es de um produto especifico, por ex: notebook
// TODO: colocar followup
