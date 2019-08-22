#pragma once

#include <QGraphicsItem>
#include <QtCharts>

QT_CHARTS_USE_NAMESPACE

class ChartTooltip final : public QGraphicsItem {
public:
  explicit ChartTooltip(QChart *chart);

  auto setText(const QString &text) -> void;
  auto setAnchor(const QPointF point) -> void;
  auto updateGeometry() -> void;

  auto boundingRect() const -> QRectF final;
  auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void final;

protected:
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto mouseMoveEvent(QGraphicsSceneMouseEvent *event) -> void final;

private:
  QRectF m_textRect;
  QRectF m_rect;
  QPointF m_anchor;
  QFont m_font;
  QChart *m_chart;
  QString m_text;
};
