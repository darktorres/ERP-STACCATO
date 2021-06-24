#pragma once

#include "chartview.h"
#include "sqltablemodel.h"

#include <QWidget>
#include <QtCharts>

namespace Ui {
class WidgetGalpaoPeso;
}

QT_CHARTS_USE_NAMESPACE

class WidgetGalpaoPeso : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGalpaoPeso(QWidget *parent = nullptr);
  ~WidgetGalpaoPeso();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  ChartView *chartView;
  QChart *chart;
  QDateTimeAxis *axisX;
  QLineSeries *series;
  QValueAxis *axisY;
  SqlTableModel model;
  Ui::WidgetGalpaoPeso *ui;
  // methods
  auto on_pushButtonAtualizar_clicked() -> void;
  auto setChart() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto updateChart() -> void;
};