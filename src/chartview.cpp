#include <QLineSeries>
#include <QSplineSeries>

#include "chartview.h"

ChartView::ChartView(QChart *chart, QWidget *parent) : QGraphicsView(new QGraphicsScene, parent), m_chart(chart) {
  setDragMode(QGraphicsView::NoDrag);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_chart->setAcceptHoverEvents(true);

  setRenderHint(QPainter::Antialiasing);
  scene()->addItem(m_chart);

  m_coordX = new QGraphicsSimpleTextItem(m_chart);
  m_coordX->setPos(m_chart->size().width() / 2 - 50, m_chart->size().height());
  m_coordX->setText("X: ");
  m_coordY = new QGraphicsSimpleTextItem(m_chart);
  m_coordY->setPos(m_chart->size().width() / 2 + 50, m_chart->size().height());
  m_coordY->setText("Y: ");

  const auto series = chart->series();

  for (const auto &AbstractSerie : series) {
    auto serie = static_cast<QLineSeries *>(AbstractSerie);
    connect(serie, &QLineSeries::clicked, this, &ChartView::keepTooltip);
    connect(serie, &QLineSeries::hovered, this, &ChartView::tooltip);
  }

  setMouseTracking(true);
}

void ChartView::keepTooltip() {
  m_tooltips.append(m_tooltip);
  m_tooltip = new ChartTooltip(m_chart);
  m_tooltip->hide();
}

void ChartView::tooltip(const QPointF point, const bool state) {
  if (m_tooltip == nullptr) { m_tooltip = new ChartTooltip(m_chart); }

  if (state) {
    m_tooltip->setText(QString("%1").arg(point.y()));
    m_tooltip->setAnchor(point);
    m_tooltip->setZValue(11);
    m_tooltip->updateGeometry();
    m_tooltip->show();
  } else {
    m_tooltip->hide();
  }
}

void ChartView::removeTooltips() {
  qDeleteAll(m_tooltips);
  m_tooltips.clear();
}

void ChartView::mouseMoveEvent(QMouseEvent *event) {
  m_coordX->setText(QString("X: %1").arg(m_chart->mapToValue(event->pos()).x()));
  m_coordY->setText(QString("Y: %1").arg(m_chart->mapToValue(event->pos()).y()));

  QGraphicsView::mouseMoveEvent(event);
}

void ChartView::resizeEvent(QResizeEvent *event) {
  if (scene()) {
    scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
    m_chart->resize(event->size());
    m_coordX->setPos(m_chart->size().width() / 2 - 50, m_chart->size().height() - 20);
    m_coordY->setPos(m_chart->size().width() / 2 + 50, m_chart->size().height() - 20);
    const auto tooltips = m_tooltips;
    for (auto *tooltip : tooltips) { tooltip->updateGeometry(); }
  }

  QGraphicsView::resizeEvent(event);
}
