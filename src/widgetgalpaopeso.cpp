#include "widgetgalpaopeso.h"
#include "ui_widgetgalpaopeso.h"

#include "application.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

WidgetGalpaoPeso::WidgetGalpaoPeso(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGalpaoPeso) { ui->setupUi(this); }

WidgetGalpaoPeso::~WidgetGalpaoPeso() { delete ui; }

void WidgetGalpaoPeso::resetTables() { modelIsSet = false; }

void WidgetGalpaoPeso::setChart() {
  axisX = new QDateTimeAxis;
  axisX->setFormat("dd-MM-yyyy");

  axisY = new QValueAxis;

  chart = new QChart;
  chart->legend()->hide();
  chart->setLocalizeNumbers(true);

  chart->addAxis(axisX, Qt::AlignBottom);
  chart->addAxis(axisY, Qt::AlignLeft);

  series = new QLineSeries;
  chart->addSeries(series);

  series->attachAxis(axisX);
  series->attachAxis(axisY);

  chartView = new ChartView(chart, this);
  chartView->setTheme(QChart::ChartThemeBlueCerulean);
  chartView->setLabelX("Dia");
  chartView->setLabelY("Kg");
  chartView->setFormatX("QDateTime");
  chartView->setFormatY("QString");

  auto gridLayout = static_cast<QGridLayout *>(ui->frame->layout());
  auto widgetPushButton = gridLayout->takeAt(0)->widget();
  gridLayout->removeWidget(widgetPushButton);
  gridLayout->addWidget(chartView, 0, 0);
  gridLayout->addWidget(widgetPushButton, 1, 0);
}

void WidgetGalpaoPeso::updateChart() {
  series->clear();

  for (int row = 0; row < model.rowCount(); ++row) {
    const QDateTime xValue = model.data(row, "data").toDateTime();
    const qreal yValue = model.data(row, "kg").toReal();

    series->append(xValue.toMSecsSinceEpoch(), yValue);
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
  // TODO: colocar uma confirmacao avisando que a atualizacao leva alguns minutos

  SqlQuery query;

  if (not query.exec("TRUNCATE estoque_peso")) { throw RuntimeException("Erro limpando dados antigos: " + query.lastError().text()); }

  if (not query.exec("CALL estoque_peso(365)")) { throw RuntimeException("Erro calculando peso do estoque: " + query.lastError().text()); }

  while (query.next()) {
    const int row = model.insertRowAtEnd();

    model.setData(row, "data", query.value("n"));
    model.setData(row, "kg", query.value("kg"));

    model.submitAll();
  }
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
