#include <QSqlError>
#include <QSqlQuery>
#include <QtCharts>

#include "application.h"
#include "ui_widgetgraficos.h"
#include "widgetgraficos.h"

using namespace QtCharts;

WidgetGraficos::WidgetGraficos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGraficos) { ui->setupUi(this); }

WidgetGraficos::~WidgetGraficos() { delete ui; }

void WidgetGraficos::resetTables() {}

void WidgetGraficos::updateTables() {
  if (not isSet) {
    QSqlQuery queryChart;

    if (not queryChart.exec(
            "SELECT dia,@s1:=@s1 + jane AS jane,@s2:=@s2 + feve AS feve,@s3:=@s3 + marc AS marc,@s4:=@s4 + abri AS abri,@s5:=@s5 + maio AS maio,@s6:=@s6 + junh AS junh,@s7:=@s7 + julh AS julh,"
            "@s8:=@s8 + agos AS agos,@s9:=@s9 + sete AS sete,@s10:=@s10 + outu AS outu,@s11:=@s11 + nove AS nove,@s12:=@s12 + deze AS deze FROM view_relatorio_temp v JOIN (SELECT @s1:=0,"
            "@s2:=0,@s3:=0,@s4:=0,@s5:=0,@s6:=0,@s7:=0,@s8:=0,@s9:=0,@s10:=0,@s11:=0,@s12:=0) s")) {
      return qApp->enqueueError("Erro lendo tabela: " + queryChart.lastError().text());
    }

    int dia = 1;

    QLineSeries *seriesJan = new QLineSeries(this);
    seriesJan->setName("Jan");
    QLineSeries *seriesFev = new QLineSeries(this);
    seriesFev->setName("Fev");
    QLineSeries *seriesMar = new QLineSeries(this);
    seriesMar->setName("Mar");
    QLineSeries *seriesAbr = new QLineSeries(this);
    seriesAbr->setName("Abr");
    QLineSeries *seriesMai = new QLineSeries(this);
    seriesMai->setName("Mai");
    QLineSeries *seriesJun = new QLineSeries(this);
    seriesJun->setName("Jun");
    QLineSeries *seriesJul = new QLineSeries(this);
    seriesJul->setName("Jul");
    QLineSeries *seriesAgo = new QLineSeries(this);
    seriesAgo->setName("Ago");
    QLineSeries *seriesSet = new QLineSeries(this);
    seriesSet->setName("Set");
    QLineSeries *seriesOut = new QLineSeries(this);
    seriesOut->setName("Out");
    QLineSeries *seriesNov = new QLineSeries(this);
    seriesNov->setName("Nov");
    QLineSeries *seriesDez = new QLineSeries(this);
    seriesDez->setName("Dez");

    while (queryChart.next()) {

      seriesJan->append(dia, queryChart.value("jane").toDouble());
      seriesFev->append(dia, queryChart.value("feve").toDouble());
      seriesMar->append(dia, queryChart.value("marc").toDouble());
      seriesAbr->append(dia, queryChart.value("abri").toDouble());
      seriesMai->append(dia, queryChart.value("maio").toDouble());
      seriesJun->append(dia, queryChart.value("junh").toDouble());
      seriesJul->append(dia, queryChart.value("julh").toDouble());
      seriesAgo->append(dia, queryChart.value("agos").toDouble());
      seriesSet->append(dia, queryChart.value("sete").toDouble());
      seriesOut->append(dia, queryChart.value("outu").toDouble());
      seriesNov->append(dia, queryChart.value("nove").toDouble());
      seriesDez->append(dia, queryChart.value("deze").toDouble());

      dia++;
    }

    QChart *chart = new QChart();
    chart->addSeries(seriesJan);
    chart->addSeries(seriesFev);
    chart->addSeries(seriesMar);
    chart->addSeries(seriesAbr);
    chart->addSeries(seriesMai);
    chart->addSeries(seriesJun);
    chart->addSeries(seriesJul);
    chart->addSeries(seriesAgo);
    chart->addSeries(seriesSet);
    chart->addSeries(seriesOut);
    chart->addSeries(seriesNov);
    chart->addSeries(seriesDez);
    chart->createDefaultAxes();
    chart->setTheme(QChart::ChartThemeBlueCerulean);
    chart->setTitle("Acumulado mensal");
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setLocalizeNumbers(true);
    static_cast<QValueAxis *>(chart->axisY())->setLabelFormat("R$ %.0f");
    static_cast<QValueAxis *>(chart->axisX())->setLabelFormat("%.0f");
    static_cast<QValueAxis *>(chart->axisY())->setTickCount(10);
    static_cast<QValueAxis *>(chart->axisX())->setTickCount(31);

    QChartView *chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::RectangleRubberBand);

    layout()->addWidget(chartView);

    // NOTE: fazer o mes atual ate o dia corrente
    // fazer o mes atual com a linha em bold
    // fazer o mesmo mes do ano anterior em bold
    // fazer uma linha diferente com a media

    isSet = true;
  }
}

// TODO: hover tooltip
// TODO: fix view to select only current year
// TODO: parameterize view to select different years
// TODO: load chart theme depending on app theme
