#include "widgetgare.h"
#include "ui_widgetgare.h"

#include "application.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "xml_viewer.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

WidgetGare::WidgetGare(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGare) {
  ui->setupUi(this);

  connect(ui->pushButtonDarBaixaItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonDarBaixaItau_clicked);
  connect(ui->pushButtonDarBaixaSantander, &QPushButton::clicked, this, &WidgetGare::on_pushButtonDarBaixaSantander_clicked);
  connect(ui->pushButtonRetornoItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRetornoItau_clicked);
  connect(ui->pushButtonRetornoSantander, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRetornoSantander_clicked);
  connect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRemessaItau_clicked);
  connect(ui->pushButtonRemessaSantander, &QPushButton::clicked, this, &WidgetGare::on_pushButtonRemessaSantander_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetGare::on_table_activated);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetGare::montaFiltro);
  connect(ui->radioButtonPendente, &QRadioButton::toggled, this, &WidgetGare::montaFiltro);
  connect(ui->radioButtonLiberado, &QRadioButton::toggled, this, &WidgetGare::montaFiltro);
  connect(ui->radioButtonGerado, &QRadioButton::toggled, this, &WidgetGare::montaFiltro);
  connect(ui->radioButtonPago, &QRadioButton::toggled, this, &WidgetGare::montaFiltro);

  ui->dateEdit->setDate(QDate::currentDate());
}

WidgetGare::~WidgetGare() { delete ui; }

void WidgetGare::resetTables() { modelIsSet = false; }

void WidgetGare::updateTables() {
  if (not isSet) { isSet = true; }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not model.select()) { return; }
}

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

  const QString textoBusca = ui->lineEditBusca->text();
  const QString filtroBusca = "(numeroNFe LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  model.setFilter(filtros.join(" AND "));
}

void WidgetGare::on_pushButtonDarBaixaItau_clicked() {
  auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  QStringList ids;

  for (auto &index : selection) { ids << model.data(index.row(), "idNFe").toString(); }

  QSqlQuery query;

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET valorReal = valor, status = 'PAGO GARE', idConta = 33, dataRealizado = '" + ui->dateEdit->date().toString("yyyy-MM-dd") +
                     "' WHERE idNFe IN (" + ids.join(", ") + ")")) {
    return qApp->enqueueError("Erro dando baixa nas gares: " + query.lastError().text(), this);
  }

  if (not model.select()) { qApp->enqueueError("Erro atualizando a tabela: " + model.lastError().text(), this); }

  qApp->enqueueInformation("Baixa salva com sucesso!", this);
}

void WidgetGare::on_pushButtonDarBaixaSantander_clicked() {
  auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  QStringList ids;

  for (auto &index : selection) { ids << model.data(index.row(), "idNFe").toString(); }

  QSqlQuery query;

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET valorReal = valor, status = 'PAGO GARE', idConta = 3, dataRealizado = '" + ui->dateEdit->date().toString("yyyy-MM-dd") +
                     "' WHERE idNFe IN (" + ids.join(", ") + ")")) {
    return qApp->enqueueError("Erro dando baixa nas gares: " + query.lastError().text(), this);
  }

  if (not model.select()) { qApp->enqueueError("Erro atualizando a tabela: " + model.lastError().text(), this); }

  qApp->enqueueInformation("Baixa salva com sucesso!", this);
}

void WidgetGare::setupTables() {
  model.setTable("view_gares");

  model.setHeaderData("numeroNFe", "NFe");
  model.setHeaderData("valor", "R$");
  model.setHeaderData("dataPagamento", "Data Pgt.");
  model.setHeaderData("dataRealizado", "Data Realizado");
  model.setHeaderData("status", "Status");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("referencia");
  ui->table->hideColumn("cnpjOrig");
}

void WidgetGare::on_pushButtonRemessaItau_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  CNAB cnab;
  cnab.remessaGareItau240(montarGare(selection));
}

void WidgetGare::on_pushButtonRemessaSantander_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  CNAB cnab;
  cnab.remessaGareSantander240(montarGare(selection));
}

QVector<CNAB::Gare> WidgetGare::montarGare(const QModelIndexList selection) {
  QVector<CNAB::Gare> gares;

  for (const auto index : selection) {
    CNAB::Gare gare;

    gare.idNFe = model.data(index.row(), "idNFe").toInt();

    QString referencia = model.data(index.row(), "referencia").toString();
    QString referencia2;
    referencia2.append(referencia.at(5));
    referencia2.append(referencia.at(6));
    referencia2.append(referencia.at(0));
    referencia2.append(referencia.at(1));
    referencia2.append(referencia.at(2));
    referencia2.append(referencia.at(3));
    gare.mesAnoReferencia = referencia2.toInt();

    QString dataPagamento = model.data(index.row(), "dataPagamento").toString();
    QString dataPagamento2;
    dataPagamento2.append(dataPagamento.at(8));
    dataPagamento2.append(dataPagamento.at(9));
    dataPagamento2.append(dataPagamento.at(5));
    dataPagamento2.append(dataPagamento.at(6));
    dataPagamento2.append(dataPagamento.at(0));
    dataPagamento2.append(dataPagamento.at(1));
    dataPagamento2.append(dataPagamento.at(2));
    dataPagamento2.append(dataPagamento.at(3));
    gare.dataVencimento = dataPagamento2.toInt();

    gare.valor = model.data(index.row(), "valor").toDouble() * 100;
    gare.numeroNF = model.data(index.row(), "numeroNFe").toString();
    gare.cnpjOrig = model.data(index.row(), "cnpjOrig").toString();

    gares << gare;
  }

  return gares;
}

void WidgetGare::on_pushButtonRetornoItau_clicked() {}

void WidgetGare::on_pushButtonRetornoSantander_clicked() {}

void WidgetGare::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

// TODO: deve salvar gare/gareData em nfe
// TODO: dar baixa automatico nas gares 0
