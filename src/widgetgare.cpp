#include "widgetgare.h"
#include "ui_widgetgare.h"

#include "application.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "sqlquery.h"
#include "xml_viewer.h"

#include <QDebug>
#include <QFileDialog>
#include <QSqlError>

WidgetGare::WidgetGare(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGare) {
  ui->setupUi(this);

  timer.setSingleShot(true);
}

WidgetGare::~WidgetGare() { delete ui; }

void WidgetGare::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->dateEditDia, &QDateEdit::dateChanged, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->groupBoxDia, &QGroupBox::toggled, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetGare::delayFiltro, connectionType);
  connect(ui->pushButtonDarBaixaItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonDarBaixaItau_clicked, connectionType);
  connect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRemessaItau_clicked, connectionType);
  connect(ui->pushButtonRetornoItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRetornoItau_clicked, connectionType);
  connect(ui->radioButtonGerado, &QRadioButton::toggled, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->radioButtonLiberado, &QRadioButton::toggled, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->radioButtonPago, &QRadioButton::toggled, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->radioButtonPendente, &QRadioButton::toggled, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetGare::on_table_activated, connectionType);
}

void WidgetGare::resetTables() { modelIsSet = false; }

void WidgetGare::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(qApp->serverDate());
    ui->dateEditDia->setDate(qApp->serverDate());

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

void WidgetGare::delayFiltro() { timer.start(500); }

void WidgetGare::montaFiltro() {
  QStringList filtros;

  //-------------------------------------

  QString filtroRadio;

  if (ui->radioButtonPendente->isChecked()) { filtroRadio = "status = 'PENDENTE GARE'"; }
  if (ui->radioButtonLiberado->isChecked()) { filtroRadio = "status = 'LIBERADO GARE'"; }
  if (ui->radioButtonGerado->isChecked()) { filtroRadio = "status = 'GERADO GARE'"; }
  if (ui->radioButtonPago->isChecked()) { filtroRadio = "status = 'PAGO GARE'"; }

  filtros << filtroRadio;

  //-------------------------------------

  const QString filtroDia = ui->groupBoxDia->isChecked() ? "DATE_FORMAT(dataRealizado, '%Y-%m-%d') = '" + ui->dateEditDia->date().toString("yyyy-MM-dd") + "'" : "";
  if (not filtroDia.isEmpty()) { filtros << filtroDia; }

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(numeroNFe LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  model.setFilter(filtros.join(" AND "));
}

void WidgetGare::on_pushButtonDarBaixaItau_clicked() {
  auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  QStringList ids;

  for (auto &index : selection) { ids << model.data(index.row(), "idNFe").toString(); }

  SqlQuery query;

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET valorReal = valor, status = 'PAGO GARE', idConta = 33, dataRealizado = '" + ui->dateEdit->date().toString("yyyy-MM-dd") +
                     "' WHERE idNFe IN (" + ids.join(", ") + ")")) {
    throw RuntimeException("Erro dando baixa nas GAREs: " + query.lastError().text(), this);
  }

  model.select();

  updateTables();

  qApp->enqueueInformation("Baixa salva com sucesso!", this);
}

void WidgetGare::setupTables() {
  model.setTable("view_gares");

  model.setHeaderData("status", "Status");
  model.setHeaderData("valor", "R$");
  model.setHeaderData("numeroNFe", "NFe");
  model.setHeaderData("dataPagamento", "Data Pgt.");
  model.setHeaderData("dataRealizado", "Data Realizado");
  model.setHeaderData("banco", "Banco");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("referencia");
  ui->table->hideColumn("cnpjOrig");

  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetGare::on_tableSelection_changed);
}

void WidgetGare::on_pushButtonRemessaItau_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  for (const auto index : selection) {
    if (model.data(index.row(), "status").toString() == "PAGO GARE") { throw RuntimeError("GARE já paga!", this); }
  }

  CNAB cnab(this);
  QString idCnab = cnab.remessaGareItau240(montarGare(selection));

  QStringList ids;

  for (const auto &index : selection) { ids << model.data(index.row(), "idPagamento").toString(); }

  SqlQuery query;

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET status = 'GERADO GARE', idConta = 33, dataRealizado = '" + qApp->serverDate().toString("yyyy-MM-dd") + "', idCnab = " + idCnab +
                     " WHERE idPagamento IN (" + ids.join(",") + ")")) {
    throw RuntimeException("Erro alterando GARE: " + query.lastError().text(), this);
  }

  updateTables();
}

QVector<CNAB::Gare> WidgetGare::montarGare(const QModelIndexList &selection) {
  QVector<CNAB::Gare> gares;

  for (const auto index : selection) {
    CNAB::Gare gare;

    QDate datePgt = model.data(index.row(), "dataPagamento").toDate();
    if (datePgt < qApp->serverDate()) { datePgt = qApp->serverDate(); }
    gare.dataVencimento = datePgt.toString("ddMMyyyy").toInt();

    gare.idNFe = model.data(index.row(), "idNFe").toInt();
    gare.mesAnoReferencia = model.data(index.row(), "referencia").toDate().toString("MMyyyy").toInt();
    gare.valor = QString::number(model.data(index.row(), "valor").toDouble(), 'f', 2).remove('.').toULong();
    gare.numeroNF = model.data(index.row(), "numeroNFe").toString();
    gare.cnpjOrig = model.data(index.row(), "cnpjOrig").toString();

    gares << gare;
  }

  return gares;
}

void WidgetGare::on_pushButtonRetornoItau_clicked() {
  // TODO: delete file after saving in SQL?

  const QString filePath = QFileDialog::getOpenFileName(this, "Arquivo Retorno", "", "RET (*.RET)");

  if (filePath.isEmpty()) { return; }

  CNAB cnab(this);
  cnab.retornoGareItau240(filePath);

  updateTables();
}

void WidgetGare::on_table_activated(const QModelIndex &index) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetGare::on_tableSelection_changed() {
  double total = 0;

  const auto selection = ui->table->selectionModel()->selectedRows();

  for (const auto &index : selection) { total += QString::number(model.data(index.row(), "valor").toDouble(), 'f', 2).toDouble(); }

  ui->doubleSpinBoxTotal->setValue(total);
}

// TODO: deve salvar gare/gareData em nfe
// TODO: quando importar retorno com status de 'agendado' alterar a linha?
// TODO: renomear classe para WidgetFinanceiroGare
