#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "ui_widgetgraficos.h"
#include "widgetgraficos.h"

WidgetGraficos::WidgetGraficos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGraficos) {
  ui->setupUi(this);
  connect(ui->checkBox, &QCheckBox::toggled, this, &WidgetGraficos::on_checkBox_toggled);
  connect(ui->comboBoxTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WidgetGraficos::on_comboBox_currentIndexChanged);
  connect(ui->pushButtonCleanTooltips, &QPushButton::clicked, this, &WidgetGraficos::on_pushButtonCleanTooltips_clicked);
}

WidgetGraficos::~WidgetGraficos() { delete ui; }

void WidgetGraficos::resetTables() {}

void WidgetGraficos::updateTables() {
  if (not isSet) {
    if (not queryChart.exec("SELECT dia, @s12:=@s12 + mes12 AS mes12, @s11:=@s11 + mes11 AS mes11, @s10:=@s10 + mes10 AS mes10, @s9:=@s9 + mes9 AS mes9, @s8:=@s8 + mes8 AS mes8, @s7:=@s7 + mes7 AS "
                            "mes7, @s6:=@s6 + mes6 AS mes6, @s5:=@s5 + mes5 AS mes5, @s4:=@s4 + mes4 AS mes4, @s3:=@s3 + mes3 AS mes3, @s2:=@s2 + mes2 AS mes2, @s1:=@s1 + mes1 AS mes1, @s0:=@s0 + "
                            "mes0 AS mes0 FROM view_relatorio_temp2 v JOIN (SELECT @s12:=0, @s11:=0,@s10:=0,@s9:=0,@s8:=0,@s7:=0,@s6:=0,@s5:=0,@s4:=0,@s3:=0,@s2:=0,@s1:=0,@s0:=0) s")) {
      return qApp->enqueueError("Erro lendo tabela: " + queryChart.lastError().text());
    }

    const QDate now = QDate::currentDate();

    series12.setName(now.addMonths(-12).toString("MMM"));
    series11.setName(now.addMonths(-11).toString("MMM"));
    series10.setName(now.addMonths(-10).toString("MMM"));
    series9.setName(now.addMonths(-9).toString("MMM"));
    series8.setName(now.addMonths(-8).toString("MMM"));
    series7.setName(now.addMonths(-7).toString("MMM"));
    series6.setName(now.addMonths(-6).toString("MMM"));
    series5.setName(now.addMonths(-5).toString("MMM"));
    series4.setName(now.addMonths(-4).toString("MMM"));
    series3.setName(now.addMonths(-3).toString("MMM"));
    series2.setName(now.addMonths(-2).toString("MMM"));
    series1.setName(now.addMonths(-1).toString("MMM"));
    series0.setName(now.toString("MMM"));

    int dia = 1;

    while (queryChart.next()) {
      series12.append(dia, queryChart.value("mes12").toDouble());
      series11.append(dia, queryChart.value("mes11").toDouble());
      series10.append(dia, queryChart.value("mes10").toDouble());
      series9.append(dia, queryChart.value("mes9").toDouble());
      series8.append(dia, queryChart.value("mes8").toDouble());
      series7.append(dia, queryChart.value("mes7").toDouble());
      series6.append(dia, queryChart.value("mes6").toDouble());
      series5.append(dia, queryChart.value("mes5").toDouble());
      series4.append(dia, queryChart.value("mes4").toDouble());
      series3.append(dia, queryChart.value("mes3").toDouble());
      series2.append(dia, queryChart.value("mes2").toDouble());
      series1.append(dia, queryChart.value("mes1").toDouble());
      series0.append(dia, queryChart.value("mes0").toDouble());

      dia++;
    }

    chart.addSeries(&series12);
    chart.addSeries(&series11);
    chart.addSeries(&series10);
    chart.addSeries(&series9);
    chart.addSeries(&series8);
    chart.addSeries(&series7);
    chart.addSeries(&series6);
    chart.addSeries(&series5);
    chart.addSeries(&series4);
    chart.addSeries(&series3);
    chart.addSeries(&series2);
    chart.addSeries(&series1);
    chart.addSeries(&series0);
    chart.createDefaultAxes();
    chart.setTheme(QChart::ChartThemeLight);
    chart.setTitle("Acumulado mensal");
    chart.legend()->setAlignment(Qt::AlignBottom);
    chart.setLocalizeNumbers(true);
    static_cast<QValueAxis *>(chart.axisY())->setLabelFormat("R$ %.0f");
    static_cast<QValueAxis *>(chart.axisX())->setLabelFormat("%.0f");
    static_cast<QValueAxis *>(chart.axisY())->setTickCount(10);
    static_cast<QValueAxis *>(chart.axisX())->setTickCount(31);

    const auto markers = chart.legend()->markers();

    for (const auto marker : markers) { connect(marker, &QLegendMarker::clicked, this, &WidgetGraficos::handleMarkerClicked); }

    chartView = new ChartView(&chart, this);

    layout()->addWidget(chartView);

    isSet = true;

    return;
  }

  if (isSet) {
    series12.clear();
    series11.clear();
    series10.clear();
    series9.clear();
    series8.clear();
    series7.clear();
    series6.clear();
    series5.clear();
    series4.clear();
    series3.clear();
    series2.clear();
    series1.clear();
    series0.clear();

    if (not queryChart.exec(queryChart.lastQuery())) { return qApp->enqueueError("Erro lendo tabela: " + queryChart.lastError().text()); }

    int dia = 1;

    while (queryChart.next()) {
      series12.append(dia, queryChart.value("mes12").toDouble());
      series11.append(dia, queryChart.value("mes11").toDouble());
      series10.append(dia, queryChart.value("mes10").toDouble());
      series9.append(dia, queryChart.value("mes9").toDouble());
      series8.append(dia, queryChart.value("mes8").toDouble());
      series7.append(dia, queryChart.value("mes7").toDouble());
      series6.append(dia, queryChart.value("mes6").toDouble());
      series5.append(dia, queryChart.value("mes5").toDouble());
      series4.append(dia, queryChart.value("mes4").toDouble());
      series3.append(dia, queryChart.value("mes3").toDouble());
      series2.append(dia, queryChart.value("mes2").toDouble());
      series1.append(dia, queryChart.value("mes1").toDouble());
      series0.append(dia, queryChart.value("mes0").toDouble());

      dia++;
    }
  }
}

void WidgetGraficos::handleMarkerClicked() {
  QLegendMarker *marker = qobject_cast<QLegendMarker *>(sender());

  toggleMarker(marker);
}

void WidgetGraficos::toggleMarker(QLegendMarker *marker) {
  switch (marker->type()) {
  case QLegendMarker::LegendMarkerTypeXY: {
    marker->series()->setVisible(not marker->series()->isVisible());

    // Turn legend marker back to visible, since hiding series also hides the marker and we don't want it to happen now.
    marker->setVisible(true);

    const qreal alpha = marker->series()->isVisible() ? 1.0 : 0.5;

    QColor color = marker->labelBrush().color();
    color.setAlphaF(alpha);
    marker->setLabelBrush(QBrush(color));

    QColor color2 = marker->brush().color();
    color2.setAlphaF(alpha);
    marker->setBrush(QBrush(color2));

    QColor color3 = marker->pen().color();
    color3.setAlphaF(alpha);
    marker->setPen(QPen(color3));

    break;
  }
  default: { break; }
  }
}

void WidgetGraficos::on_checkBox_toggled() {
  const auto markers = chart.legend()->markers();

  for (const auto marker : markers) { toggleMarker(marker); }
}

void WidgetGraficos::on_comboBox_currentIndexChanged(int index) { chart.setTheme(static_cast<QChart::ChartTheme>(index)); }

void WidgetGraficos::on_pushButtonCleanTooltips_clicked() { chartView->removeTooltips(); }

// TODO: parameterize view to select different years
// NOTE: fazer o mes atual ate o dia corrente
// fazer o mes atual com a linha em bold
// fazer o mesmo mes do ano anterior em bold
// fazer uma linha diferente com a media
