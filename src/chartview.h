#pragma once

#include "charttooltip.h"

#include <QGraphicsView>
#include <QtCharts>

QT_CHARTS_USE_NAMESPACE

class ChartView : public QGraphicsView {
public:
  explicit ChartView(QChart *chart, QWidget *parent);

  auto keepTooltip() -> void;
  auto removeTooltips() -> void;
  auto tooltip(const QPointF point, const bool state) -> void;

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void override;
  auto resizeEvent(QResizeEvent *event) -> void override;

private:
  // attributes
  ChartTooltip *m_tooltip = nullptr;
  QChart *m_chart = nullptr;
  QGraphicsSimpleTextItem *m_coordX = nullptr;
  QGraphicsSimpleTextItem *m_coordY = nullptr;
  QList<ChartTooltip *> m_tooltips;
};
