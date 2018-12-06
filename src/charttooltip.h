#ifndef CHARTTOOLTIP_H
#define CHARTTOOLTIP_H

#include <QGraphicsItem>
#include <QtCharts>

QT_CHARTS_USE_NAMESPACE

class ChartTooltip : public QGraphicsItem {
public:
  explicit ChartTooltip(QChart *chart);

  auto setText(const QString &text) -> void;
  auto setAnchor(const QPointF point) -> void;
  auto updateGeometry() -> void;

  virtual auto boundingRect() const -> QRectF override;
  virtual auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void override;

protected:
  virtual auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void override;
  virtual auto mouseMoveEvent(QGraphicsSceneMouseEvent *event) -> void override;

private:
  QRectF m_textRect;
  QRectF m_rect;
  QPointF m_anchor;
  QFont m_font;
  QChart *m_chart;
  QString m_text;
};

#endif // CHARTTOOLTIP_H
