#include "chartview.h"

#include "application.h"

ChartView::ChartView(QChart *chart, QWidget *parent) : QGraphicsView(new QGraphicsScene, parent), m_chart(chart) {
  setDragMode(QGraphicsView::NoDrag);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_chart->setAcceptHoverEvents(true);

  setRenderHint(QPainter::Antialiasing);
  scene()->addItem(m_chart);

  const double width = m_chart->size().width();
  const double height = m_chart->size().height();

  m_coordX = new QGraphicsSimpleTextItem(m_chart);
  m_coordX->setPos(width / 2 - 50, height);
  m_coordY = new QGraphicsSimpleTextItem(m_chart);
  m_coordY->setPos(width / 2 + 50, height);

  //  for (const auto &abstractSerie : chart->series()) {
  //    const auto serie = static_cast<QLineSeries *>(abstractSerie);
  //    connect(serie, &QLineSeries::clicked, this, &ChartView::keepTooltip);
  //    connect(serie, &QLineSeries::hovered, this, &ChartView::tooltip);
  //  }

  setMouseTracking(true);
}

// void ChartView::keepTooltip() {
//  m_tooltips.append(m_tooltip);
//  m_tooltip = new ChartTooltip(m_chart);
//  m_tooltip->hide();
//}

void ChartView::tooltip(const QPointF point, const bool state) {
  if (not m_tooltip) { m_tooltip = new ChartTooltip(m_chart); }

  if (state) {
    m_tooltip->setText(QString::number(point.y()));
    m_tooltip->setAnchor(point);
    m_tooltip->setZValue(11);
    m_tooltip->updateGeometry();
    m_tooltip->show();
  } else {
    m_tooltip->hide();
  }
}

void ChartView::setTheme(const QChart::ChartTheme theme) {
  m_chart->setTheme(theme);

  // light themes
  if (theme == QChart::ChartThemeLight or theme == QChart::ChartThemeBrownSand or theme == QChart::ChartThemeBlueNcs or theme == QChart::ChartThemeHighContrast or theme == QChart::ChartThemeBlueIcy or
      theme == QChart::ChartThemeQt) {
    m_coordX->setBrush(QBrush(QRgb(0x404044)));
    m_coordY->setBrush(QBrush(QRgb(0x404044)));
  }

  // dark themes
  if (theme == QChart::ChartThemeBlueCerulean or theme == QChart::ChartThemeDark) {
    m_coordX->setBrush(QBrush(QRgb(0xffffff)));
    m_coordY->setBrush(QBrush(QRgb(0xffffff)));
  }
}

void ChartView::removeTooltips() {
  qDeleteAll(m_tooltips);
  m_tooltips.clear();
}

void ChartView::mouseMoveEvent(QMouseEvent *event) {
  const auto point = m_chart->mapToValue(event->pos());

  // TODO: implicit conversion qreal -> qint64
  if (formatX == "QDateTime") { m_coordX->setText(labelX + ": " + QDateTime::fromMSecsSinceEpoch(point.x()).toString("dd/MM/yy")); }
  if (formatX == "QString") { m_coordX->setText(labelX + ": " + QLocale(QLocale::Portuguese).toString(point.x(), 'f', 0)); }

  // TODO: implicit conversion qreal -> qint64
  if (formatY == "QDateTime") { m_coordY->setText(labelY + ": " + QDateTime::fromMSecsSinceEpoch(point.y()).toString("dd/MM/yy")); }
  if (formatY == "QString") { m_coordY->setText(labelY + ": " + QLocale(QLocale::Portuguese).toString(point.y(), 'f', 0)); }

  QGraphicsView::mouseMoveEvent(event);
}

void ChartView::resizeEvent(QResizeEvent *event) {
  if (scene()) {
    scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
    m_chart->resize(event->size());

    const double width = m_chart->size().width();
    const double height = m_chart->size().height();

    m_coordX->setPos(width / 2 - 50, height - 30);
    m_coordY->setPos(width / 2 + 50, height - 30);

    for (auto *tooltip : qAsConst(m_tooltips)) { tooltip->updateGeometry(); }
  }

  QGraphicsView::resizeEvent(event);
}

void ChartView::setFormatY(const QString &newFormatY) { formatY = newFormatY; }

void ChartView::setFormatX(const QString &newFormatX) { formatX = newFormatX; }

void ChartView::setLabelY(const QString &newLabelY) { labelY = newLabelY; }

void ChartView::setLabelX(const QString &newLabelX) { labelX = newLabelX; }

void ChartView::resetRange(const bool startXZero, const bool startYZero) {
  if (m_chart->axes().isEmpty()) { return; }

  // ---------------------------------------------------------------

  QList<QPointF> vec;

  const auto seriesList = m_chart->series();

  for (auto *const series : seriesList) {
    auto *const xySeries = qobject_cast<QXYSeries *>(series);
    if (xySeries) { vec << xySeries->points(); }
  }

  if (vec.isEmpty()) { return; }

  // ---------------------------------------------------------------

  auto *const axisX = m_chart->axes(Qt::Horizontal).at(0);

  if (axisX) {
    QVariant min_x = std::max_element(vec.cbegin(), vec.cend(), [](const QPointF a, const QPointF b) { return a.x() > b.x(); })->x();
    QVariant max_x = std::max_element(vec.cbegin(), vec.cend(), [](const QPointF a, const QPointF b) { return a.x() < b.x(); })->x();

    if (qobject_cast<QBarCategoryAxis *>(axisX)) { throw RuntimeException("Não implementado para QBarCategoryAxis"); } // QString

    if (qobject_cast<QDateTimeAxis *>(axisX)) {
      // TODO: implicit conversion qreal -> qint64
      min_x = QDateTime::fromMSecsSinceEpoch(min_x.toReal());
      // TODO: implicit conversion qreal -> qint64
      max_x = QDateTime::fromMSecsSinceEpoch(max_x.toReal());
    }

    axisX->setRange(min_x, max_x);

    if (startXZero) { axisX->setMin(0); }
  }

  // ---------------------------------------------------------------

  auto *const axisY = m_chart->axes(Qt::Vertical).at(0);

  if (axisY) {
    QVariant min_y = std::max_element(vec.cbegin(), vec.cend(), [](const QPointF a, const QPointF b) { return a.y() > b.y(); })->y();
    QVariant max_y = std::max_element(vec.cbegin(), vec.cend(), [](const QPointF a, const QPointF b) { return a.y() < b.y(); })->y();

    if (qobject_cast<QBarCategoryAxis *>(axisY)) { throw RuntimeException("Não implementado para QBarCategoryAxis"); } // QString

    if (qobject_cast<QDateTimeAxis *>(axisY)) {
      // TODO: implicit conversion qreal -> qint64
      min_y = QDateTime::fromMSecsSinceEpoch(min_y.toReal());
      // TODO: implicit conversion qreal -> qint64
      max_y = QDateTime::fromMSecsSinceEpoch(max_y.toReal());
    }

    max_y = max_y.toDouble() * 1.1;

    axisY->setRange(min_y, max_y);

    if (startYZero) { axisY->setMin(0); }
  }
}
