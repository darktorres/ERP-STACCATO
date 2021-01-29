#pragma once

#include "chartview.h"

#include <QWidget>
#include <QtCharts>

namespace Ui {
class WidgetGraficos;
}

QT_CHARTS_USE_NAMESPACE

class WidgetGraficos : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGraficos(QWidget *parent);
  ~WidgetGraficos();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  Ui::WidgetGraficos *ui;
  QChart chart;
  ChartView *chartView;
  QVector<QLineSeries *> series;
  // methods
  auto handleMarkerClicked() -> void;
  auto on_checkBox_toggled() -> void;
  auto on_comboBox_currentIndexChanged(int index) -> void;
  auto on_pushButtonCleanTooltips_clicked() -> void;
  auto setConnections() -> void;
  auto toggleMarker(QLegendMarker *marker) -> void;
};
