#pragma once

#include <QGraphicsView>
#include <QtCharts>

#include "charttooltip.h"

QT_CHARTS_USE_NAMESPACE

class ChartView : public QGraphicsView {
public:
  explicit ChartView(QChart *chart, QWidget *parent = nullptr);
  auto keepTooltip() -> void;
  auto removeTooltips() -> void;
  auto tooltip(const QPointF point, const bool state) -> void;

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void override;
  auto resizeEvent(QResizeEvent *event) -> void override;

private:
  QChart *m_chart;
  QGraphicsSimpleTextItem *m_coordX = nullptr;
  QGraphicsSimpleTextItem *m_coordY = nullptr;
  ChartTooltip *m_tooltip = nullptr;
  QList<ChartTooltip *> m_tooltips;
};
