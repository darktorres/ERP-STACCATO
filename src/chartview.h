#ifndef CHARTVIEW_H
#define CHARTVIEW_H

#include <QGraphicsView>
#include <QtCharts>

#include "charttooltip.h"

QT_CHARTS_USE_NAMESPACE

class ChartView : public QGraphicsView {
public:
  explicit ChartView(QChart *chart, QWidget *parent = nullptr);
  void keepTooltip();
  void tooltip(const QPointF point, const bool state);
  void removeTooltips();

protected:
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void resizeEvent(QResizeEvent *event) override;

private:
  QChart *m_chart;
  QGraphicsSimpleTextItem *m_coordX = nullptr;
  QGraphicsSimpleTextItem *m_coordY = nullptr;
  ChartTooltip *m_tooltip = nullptr;
  QList<ChartTooltip *> m_tooltips;
};

#endif // CHARTVIEW_H
