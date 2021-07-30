#include "widgetgalpaopeso.h"
#include "ui_widgetgalpaopeso.h"

#include "application.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

WidgetGalpaoPeso::WidgetGalpaoPeso(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGalpaoPeso) {
  ui->setupUi(this);
  progressDialog.setAutoClose(true);
  progressDialog.reset();
}

WidgetGalpaoPeso::~WidgetGalpaoPeso() { delete ui; }

void WidgetGalpaoPeso::resetTables() { modelIsSet = false; }

void WidgetGalpaoPeso::setChart() {
  axisX.setFormat("dd-MM-yyyy");

  chart.legend()->hide();
  chart.setLocalizeNumbers(true);

  chart.addAxis(&axisX, Qt::AlignBottom);
  chart.addAxis(&axisY, Qt::AlignLeft);

  chart.addSeries(&series);

  series.attachAxis(&axisX);
  series.attachAxis(&axisY);

  chartView = new ChartView(&chart, this);
  chartView->setTheme(QChart::ChartThemeBlueCerulean);
  chartView->setLabelX("Dia");
  chartView->setLabelY("Kg");
  chartView->setFormatX("QDateTime");
  chartView->setFormatY("QString");

  const auto gridLayout = static_cast<QGridLayout *>(ui->frame->layout());

  const auto horizontalLayout = gridLayout->takeAt(0);

  gridLayout->addWidget(chartView, 0, 0);
  gridLayout->addItem(horizontalLayout, 1, 0);
}

void WidgetGalpaoPeso::updateChart() {
  series.clear();

  for (int row = 0; row < model.rowCount(); ++row) {
    const QDateTime xValue = model.data(row, "data").toDateTime();
    const qreal yValue = model.data(row, "kg").toReal();

    series.append(xValue.toMSecsSinceEpoch(), yValue);
  }

  chartView->resetRange(false, true);
}

void WidgetGalpaoPeso::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    setChart();
    modelIsSet = true;
  }

  updateChart();
}

void WidgetGalpaoPeso::on_pushButtonAtualizar_clicked() {
  SqlQuery query;

  if (not query.exec("TRUNCATE estoque_peso")) { throw RuntimeException("Erro limpando dados antigos: " + query.lastError().text()); }

  setProgressDialog();

  for (int dia = 0; dia < ui->spinBoxDias->value(); ++dia) {
    if (progressDialog.wasCanceled()) { break; }

    const int row = model.insertRowAtEnd();

    if (not query.exec("SELECT CURDATE() + INTERVAL - " + QString::number(dia) + " DAY AS data, kg_dia(CURDATE() + INTERVAL - " + QString::number(dia) + " DAY) AS kg")) {
      throw RuntimeException("Erro calculando peso do estoque: " + query.lastError().text());
    }

    if (not query.first()) { throw RuntimeException("Peso não encontrado!"); }

    model.setData(row, "data", query.value("data"));
    model.setData(row, "kg", query.value("kg"));

    progressDialog.setValue(dia + 1);
  }

  model.submitAll();

  updateChart();
}

void WidgetGalpaoPeso::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &WidgetGalpaoPeso::on_pushButtonAtualizar_clicked, connectionType);
}

void WidgetGalpaoPeso::setupTables() {
  model.setTable("estoque_peso");

  model.setFilter("");

  model.setHeaderData("data", "Data");
  model.setHeaderData("kg", "Kg.");

  model.select();
}

void WidgetGalpaoPeso::setProgressDialog() {
  progressDialog.reset();
  progressDialog.setCancelButton(nullptr);
  progressDialog.setLabelText("Processando...");
  progressDialog.setWindowTitle("ERP Staccato");
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.setMinimum(0);
  progressDialog.setMaximum(ui->spinBoxDias->value());
  progressDialog.setCancelButtonText("Cancelar");
}
