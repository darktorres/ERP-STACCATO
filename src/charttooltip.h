#pragma once

#include <QGraphicsItem>
#include <QtCharts>

QT_CHARTS_USE_NAMESPACE

class ChartTooltip final : public QGraphicsItem {
public:
  explicit ChartTooltip(QChart *chart);

  auto boundingRect() const -> QRectF final;
  auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void final;
  auto setAnchor(const QPointF point) -> void;
  auto setText(const QString &text) -> void;
  auto updateGeometry() -> void;

protected:
  auto mouseMoveEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void final;

private:
  // attributes
  QChart *m_chart = nullptr;
  QFont m_font;
  QPointF m_anchor;
  QRectF m_rect;
  QRectF m_textRect;
  QString m_text;
};
