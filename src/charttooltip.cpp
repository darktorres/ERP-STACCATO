#include "charttooltip.h"

ChartTooltip::ChartTooltip(QChart *chart) : QGraphicsItem(chart), m_chart(chart) {}

void ChartTooltip::setText(const QString &text) {
  m_text = QLocale(QLocale::Portuguese).toString(text.toDouble(), 'f', 0);
  QFontMetrics metrics(m_font);
  m_textRect = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, m_text);
  m_textRect.translate(5, 5);
  prepareGeometryChange();
  m_rect = m_textRect.adjusted(-5, 0, 20, 5);
}

void ChartTooltip::setAnchor(const QPointF point) { m_anchor = point; }

void ChartTooltip::updateGeometry() {
  prepareGeometryChange();
  setPos(m_chart->mapToPosition(m_anchor) + QPoint(10, -50));
}

QRectF ChartTooltip::boundingRect() const {
  QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));
  QRectF rect;
  rect.setLeft(qMin(m_rect.left(), anchor.x()));
  rect.setRight(qMax(m_rect.right(), anchor.x()));
  rect.setTop(qMin(m_rect.top(), anchor.y()));
  rect.setBottom(qMax(m_rect.bottom(), anchor.y()));
  return rect;
}

void ChartTooltip::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option)
  Q_UNUSED(widget)

  QPainterPath path;
  path.addRoundedRect(m_rect, 5, 5, Qt::RelativeSize);

  const QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));

  if (not m_rect.contains(anchor)) {
    // establish the position of the anchor point in relation to m_rect
    const bool above = anchor.y() <= m_rect.top();
    const bool aboveCenter = anchor.y() > m_rect.top() and anchor.y() <= m_rect.center().y();
    const bool belowCenter = anchor.y() > m_rect.center().y() and anchor.y() <= m_rect.bottom();
    const bool below = anchor.y() > m_rect.bottom();

    const bool onLeft = anchor.x() <= m_rect.left();
    const bool leftOfCenter = anchor.x() > m_rect.left() and anchor.x() <= m_rect.center().x();
    const bool rightOfCenter = anchor.x() > m_rect.center().x() and anchor.x() <= m_rect.right();
    const bool onRight = anchor.x() > m_rect.right();

    // get the nearest m_rect corner.
    const qreal x = (onRight + rightOfCenter) * m_rect.width();
    const qreal y = (below + belowCenter) * m_rect.height();
    const bool cornerCase = (above and onLeft) or (above and onRight) or (below and onLeft) or (below and onRight);
    const bool vertical = qAbs(anchor.x() - x) > qAbs(anchor.y() - y);

    const qreal x1 = x + leftOfCenter * 10 - rightOfCenter * 20 + cornerCase * !vertical * (onLeft * 10 - onRight * 20);
    const qreal y1 = y + aboveCenter * 10 - belowCenter * 20 + cornerCase * vertical * (above * 10 - below * 20);

    const qreal x2 = x + leftOfCenter * 20 - rightOfCenter * 10 + cornerCase * !vertical * (onLeft * 20 - onRight * 10);
    const qreal y2 = y + aboveCenter * 20 - belowCenter * 10 + cornerCase * vertical * (above * 20 - below * 10);

    path.moveTo(x1, y1 + 5);
    path.lineTo(anchor);
    path.lineTo(x2, y2 + 5);
    path = path.simplified();
  }

  painter->setBrush(QColor(0, 0, 0));
  painter->setPen(QColor(255, 255, 255));
  painter->drawPath(path);
  painter->drawText(m_textRect.bottomLeft(), "R$ " + m_text);
}

void ChartTooltip::mousePressEvent(QGraphicsSceneMouseEvent *event) { event->setAccepted(true); }

void ChartTooltip::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
    event->setAccepted(true);
  } else {
    event->setAccepted(false);
  }
}
