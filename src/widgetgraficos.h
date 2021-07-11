#pragma once

#include "chartview.h"

namespace Ui {
class WidgetGraficos;
}

class WidgetGraficos : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGraficos(QWidget *parent);
  ~WidgetGraficos();

  auto resetTables() -> void;
  auto setComboBoxLojas() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  ChartView *chartView;
  QChart chart;
  QValueAxis *axisX;
  QValueAxis *axisY;
  QVector<QLineSeries *> series;
  Ui::WidgetGraficos *ui;
  // methods
  auto handleMarkerClicked() -> void;
  auto on_checkBox_toggled() -> void;
  auto on_comboBox_currentIndexChanged(const int index) -> void;
  auto on_pushButtonCleanTooltips_clicked() -> void;
  auto setChart() -> void;
  auto setConnections() -> void;
  auto toggleMarker(QLegendMarker *marker) const -> void;
  auto updateChart() -> void;
};
