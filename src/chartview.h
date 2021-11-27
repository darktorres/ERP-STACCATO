#pragma once

#include "charttooltip.h"

// TODO: why is this not derived from QChartView
class ChartView : public QGraphicsView {
  Q_OBJECT

public:
  explicit ChartView(QChart *chart, QWidget *parent);

  //  auto keepTooltip() -> void;
  auto removeTooltips() -> void;
  auto resetRange(const bool startXZero, const bool startYZero) -> void;
  auto setFormatX(const QString &newFormatX) -> void;
  auto setFormatY(const QString &newFormatY) -> void;
  auto setLabelX(const QString &newLabelX) -> void;
  auto setLabelY(const QString &newLabelY) -> void;
  auto setTheme(const QChart::ChartTheme theme) -> void;
  auto tooltip(const QPointF point, const bool state) -> void;

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;

private:
  // attributes
  ChartTooltip *m_tooltip = nullptr;
  QChart *m_chart = nullptr;
  QGraphicsSimpleTextItem *m_coordX = nullptr;
  QGraphicsSimpleTextItem *m_coordY = nullptr;
  QList<ChartTooltip *> m_tooltips;
  QString formatX;
  QString formatY;
  QString labelX;
  QString labelY;
};
