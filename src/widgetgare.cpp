#include "widgetgare.h"
#include "ui_widgetgare.h"

#include "acbrlib.h"
#include "application.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>

WidgetGare::WidgetGare(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGare) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 4);
}

WidgetGare::~WidgetGare() { delete ui; }

void WidgetGare::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditFiltro, &QDateEdit::dateChanged, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->groupBoxDia, &QGroupBox::toggled, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetGare::montaFiltro, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetGare::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonDarBaixaItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonDarBaixaItau_clicked, connectionType);
  connect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRemessaItau_clicked, connectionType);
  connect(ui->pushButtonRetornoItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRetornoItau_clicked, connectionType);
  connect(ui->radioButtonCancelado, &QRadioButton::toggled, this, &WidgetGare::on_radioButtonCancelado_toggled, connectionType);
  connect(ui->radioButtonGerado, &QRadioButton::toggled, this, &WidgetGare::on_radioButtonGerado_toggled, connectionType);
  connect(ui->radioButtonLiberado, &QRadioButton::toggled, this, &WidgetGare::on_radioButtonLiberado_toggled, connectionType);
  connect(ui->radioButtonPago, &QRadioButton::toggled, this, &WidgetGare::on_radioButtonPago_toggled, connectionType);
  connect(ui->radioButtonPendente, &QRadioButton::toggled, this, &WidgetGare::on_radioButtonPendente_toggled, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetGare::on_table_activated, connectionType);
}

void WidgetGare::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetGare::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEditBaixa->setDate(qApp->serverDate());
    ui->dateEditFiltro->setDate(qApp->serverDate());
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  model.select();
  modelVencidos.select();
  modelVencer.select();
}

void WidgetGare::habilitarBotoes() {
  const bool desabilitar = ui->radioButtonPago->isChecked();

  ui->dateEditBaixa->setDisabled(desabilitar);
  ui->pushButtonDarBaixaItau->setDisabled(desabilitar);
  ui->pushButtonRemessaItau->setDisabled(desabilitar);
  ui->pushButtonRetornoItau->setDisabled(desabilitar);
}

void WidgetGare::montaFiltro() {
  habilitarBotoes();

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  QString filtroRadio;

  if (ui->radioButtonPendente->isChecked()) { filtroRadio = "status = 'PENDENTE GARE'"; }
  if (ui->radioButtonLiberado->isChecked()) { filtroRadio = "status = 'LIBERADO GARE'"; }
  if (ui->radioButtonGerado->isChecked()) { filtroRadio = "status = 'GERADO GARE'"; }
  if (ui->radioButtonPago->isChecked()) { filtroRadio = "status = 'PAGO GARE'"; }
  if (ui->radioButtonCancelado->isChecked()) { filtroRadio = "status = 'CANCELADO GARE'"; }

  filtros << filtroRadio;

  //-------------------------------------

  const QString filtroDia = ui->groupBoxDia->isChecked() ? "DATE_FORMAT(dataRealizado, '%Y-%m-%d') = '" + ui->dateEditFiltro->date().toString("yyyy-MM-dd") + "'" : "";
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

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET valorReal = valor, status = 'PAGO GARE', idConta = 33, dataRealizado = '" + ui->dateEditBaixa->date().toString("yyyy-MM-dd") +
                     "' WHERE idNFe IN (" + ids.join(", ") + ") AND grupo = 'IMPOSTOS - ICMS;ST;ISS'")) {
    throw RuntimeException("Erro dando baixa nas GAREs: " + query.lastError().text(), this);
  }

  model.select();

  updateTables();

  qApp->enqueueInformation("Baixa salva com sucesso!", this);
}

void WidgetGare::setupTables() {
  modelVencidos.setQuery(Sql::view_gare_vencidos());

  modelVencidos.sort("`Data`");

  ui->tableVencidos->setModel(&modelVencidos);

  ui->tableVencidos->setItemDelegate(new ReaisDelegate(this));

  // -------------------------------------------------------------------------

  modelVencer.setQuery(Sql::view_gare_vencer());

  modelVencer.sort("`Data`");

  ui->tableVencer->setModel(&modelVencer);

  ui->tableVencer->setItemDelegate(new ReaisDelegate(this));

  // -------------------------------------------------------------------------

  model.setTable("view_gares");

  model.setHeaderData("valor", "R$");
  model.setHeaderData("numeroNFe", "NF-e");
  model.setHeaderData("dataPagamento", "Data Pgt.");
  model.setHeaderData("dataRealizado", "Data Realizado");
  model.setHeaderData("banco", "Banco");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("referencia");
  ui->table->hideColumn("status");
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

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("XML não encontrado para a NF-e de id: '" + model.data(index.row(), "idNFe").toString() + "'"); }

  ACBrLib::gerarDanfe(query.value("xml").toString(), true);
}

void WidgetGare::on_tableSelection_changed() {
  double total = 0;

  const auto selection = ui->table->selectionModel()->selectedRows();

  for (const auto &index : selection) { total += QString::number(model.data(index.row(), "valor").toDouble(), 'f', 2).toDouble(); }

  ui->doubleSpinBoxTotal->setValue(total);
}

void WidgetGare::on_pushButtonCancelar_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // -------------------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // -------------------------------------------------------------------------

  qApp->startTransaction("WidgetGare::on_pushButtonCancelar_clicked");

  for (const auto index : selection) {
    SqlQuery query;

    if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET status = 'CANCELADO GARE' WHERE idPagamento = " + model.data(index.row(), "idPagamento").toString())) {
      throw RuntimeException("Erro cancelando GARE: " + query.lastError().text());
    }
  }

  qApp->endTransaction();

  qApp->enqueueInformation("GAREs canceladas!");
}

void WidgetGare::on_radioButtonPendente_toggled(const bool checked) {
  ui->pushButtonCancelar->setEnabled(true);

  if (checked) { montaFiltro(); }
}

void WidgetGare::on_radioButtonLiberado_toggled(const bool checked) {
  ui->pushButtonCancelar->setEnabled(true);

  if (checked) { montaFiltro(); }
}

void WidgetGare::on_radioButtonGerado_toggled(const bool checked) {
  ui->pushButtonCancelar->setEnabled(true);

  if (checked) { montaFiltro(); }
}

void WidgetGare::on_radioButtonPago_toggled(const bool checked) {
  ui->pushButtonCancelar->setDisabled(true);

  if (checked) { montaFiltro(); }
}

void WidgetGare::on_radioButtonCancelado_toggled(const bool checked) {
  ui->pushButtonCancelar->setDisabled(true);

  if (checked) { montaFiltro(); }
}

// TODO: deve salvar gare/gareData em nfe
// TODO: quando importar retorno com status de 'agendado' alterar a linha?
// TODO: renomear classe para WidgetFinanceiroGare
