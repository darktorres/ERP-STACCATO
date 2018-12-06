#ifndef WIDGETGRAFICOS_H
#define WIDGETGRAFICOS_H

#include <QSqlQuery>
#include <QWidget>
#include <QtCharts>

#include "chartview.h"

namespace Ui {
class WidgetGraficos;
}

QT_CHARTS_USE_NAMESPACE

class WidgetGraficos : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGraficos(QWidget *parent = nullptr);
  ~WidgetGraficos();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  Ui::WidgetGraficos *ui;
  QChart chart;
  ChartView *chartView;
  QSqlQuery queryChart;
  QLineSeries series12;
  QLineSeries series11;
  QLineSeries series10;
  QLineSeries series9;
  QLineSeries series8;
  QLineSeries series7;
  QLineSeries series6;
  QLineSeries series5;
  QLineSeries series4;
  QLineSeries series3;
  QLineSeries series2;
  QLineSeries series1;
  QLineSeries series0;
  // methods
  auto handleMarkerClicked() -> void;
  auto on_checkBox_toggled() -> void;
  auto on_comboBox_currentIndexChanged(int index) -> void;
  auto on_pushButtonCleanTooltips_clicked() -> void;
  auto toggleMarker(QLegendMarker *marker) -> void;
};

#endif // WIDGETGRAFICOS_H
