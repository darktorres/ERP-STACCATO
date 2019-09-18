#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "ui_widgetgraficos.h"
#include "usersession.h"
#include "widgetgraficos.h"

WidgetGraficos::WidgetGraficos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGraficos) {
  ui->setupUi(this);

  connect(ui->checkBox, &QCheckBox::toggled, this, &WidgetGraficos::on_checkBox_toggled);
  connect(ui->comboBoxTheme, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetGraficos::on_comboBox_currentIndexChanged);
  connect(ui->pushButtonCleanTooltips, &QPushButton::clicked, this, &WidgetGraficos::on_pushButtonCleanTooltips_clicked);
}

WidgetGraficos::~WidgetGraficos() { delete ui; }

void WidgetGraficos::resetTables() {}

void WidgetGraficos::updateTables() {
  if (not isSet) {
    if (UserSession::tipoUsuario() == "GERENTE LOJA") {
      ui->comboBoxLojas->addItem(UserSession::fromLoja("descricao")->toString(), UserSession::idLoja());
    } else {
      QSqlQuery query;

      if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE ORDER BY descricao")) { return qApp->enqueueError("Erro: " + query.lastError().text(), this); }

      while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }
    }

    connect(ui->comboBoxLojas, &QComboBox::currentTextChanged, this, &WidgetGraficos::updateTables);

    // ----------------------------------------------------

    const QDate now = QDate::currentDate();
    const QVector<QPen> colors = {QPen(QBrush(QColor(255, 70, 70)), 2), QPen(QBrush(QColor(255, 100, 0)), 2), QPen(QBrush(QColor(255, 170, 0)), 2), QPen(QBrush(QColor(255, 255, 0)), 2),
                                  QPen(QBrush(QColor(195, 255, 0)), 2), QPen(QBrush(QColor(17, 166, 0)), 2),  QPen(QBrush(QColor(0, 217, 148)), 2), QPen(QBrush(QColor(0, 255, 255)), 2),
                                  QPen(QBrush(QColor(0, 30, 255)), 2),  QPen(QBrush(QColor(140, 0, 255)), 2), QPen(QBrush(QColor(255, 0, 255)), 2), QPen(QBrush(QColor(180, 80, 60)), 2),
                                  QPen(QBrush(QColor(255, 40, 40)), 3)};

    for (int i = 0; i < 13; ++i) {
      auto serie = new QLineSeries(this);

      serie->setName(now.addMonths(i - 12).toString("MMM"));
      serie->setPen(colors.at(i));

      chart.addSeries(serie);
      series << serie;
    }

    chart.setTheme(QChart::ChartThemeLight);
    chart.setTitle("Acumulado mensal");
    chart.legend()->setAlignment(Qt::AlignBottom);
    chart.setLocalizeNumbers(true);

    chart.createDefaultAxes();

    auto axes = chart.axes();

    if (axes.isEmpty()) { return qApp->enqueueError("Sem eixos X e Y!", this); }

    auto axisX = static_cast<QValueAxis *>(axes.at(0));
    auto axisY = static_cast<QValueAxis *>(axes.at(1));

    axisX->setLabelFormat("%.0f");
    axisY->setLabelFormat("R$ %.0f");
    axisX->setTickCount(31);
    axisY->setTickCount(10);

    const auto markers = chart.legend()->markers();

    for (const auto marker : markers) { connect(marker, &QLegendMarker::clicked, this, &WidgetGraficos::handleMarkerClicked); }

    chartView = new ChartView(&chart, this);

    layout()->addWidget(chartView);

    isSet = true;
  }

  if (isSet) {
    QSqlQuery queryChart;

    if (ui->comboBoxLojas->currentText().isEmpty()) {
      if (not queryChart.exec("SELECT dia, @s12:=@s12 + mes12 AS mes12, @s11:=@s11 + mes11 AS mes11, @s10:=@s10 + mes10 AS mes10, @s9:=@s9 + mes9 AS mes9, @s8:=@s8 + mes8 AS mes8, @s7:=@s7 + mes7 AS "
                              "mes7, @s6:=@s6 + mes6 AS mes6, @s5:=@s5 + mes5 AS mes5, @s4:=@s4 + mes4 AS mes4, @s3:=@s3 + mes3 AS mes3, @s2:=@s2 + mes2 AS mes2, @s1:=@s1 + mes1 AS mes1, @s0:=@s0 + "
                              "mes0 AS mes0 FROM view_grafico_lojas v JOIN (SELECT @s12:=0, @s11:=0,@s10:=0,@s9:=0,@s8:=0,@s7:=0,@s6:=0,@s5:=0,@s4:=0,@s3:=0,@s2:=0,@s1:=0,@s0:=0) s")) {
        return qApp->enqueueError("Erro lendo tabela: " + queryChart.lastError().text(), this);
      }
    } else {
      if (not queryChart.exec("SELECT dia, @s12:=@s12 + mes12 AS mes12, @s11:=@s11 + mes11 AS mes11, @s10:=@s10 + mes10 AS mes10, @s9:=@s9 + mes9 AS mes9, @s8:=@s8 + mes8 AS mes8, @s7:=@s7 + mes7 AS "
                              "mes7, @s6:=@s6 + mes6 AS mes6, @s5:=@s5 + mes5 AS mes5, @s4:=@s4 + mes4 AS mes4, @s3:=@s3 + mes3 AS mes3, @s2:=@s2 + mes2 AS mes2, @s1:=@s1 + mes1 AS mes1, @s0:=@s0 + "
                              "mes0 AS mes0 FROM view_grafico_loja v JOIN (SELECT @s12:=0, @s11:=0,@s10:=0,@s9:=0,@s8:=0,@s7:=0,@s6:=0,@s5:=0,@s4:=0,@s3:=0,@s2:=0,@s1:=0,@s0:=0) s WHERE idLoja = " +
                              ui->comboBoxLojas->getCurrentValue().toString() + " ORDER BY dia")) {
        return qApp->enqueueError("Erro lendo tabela: " + queryChart.lastError().text(), this);
      }
    }

    for (auto serie : series) { serie->clear(); }

    double max = 0;

    while (queryChart.next()) {
      for (int i = 0; i < 13; ++i) {
        const double value = queryChart.value("mes" + QString::number(12 - i)).toDouble();

        if (value > max) { max = value; }

        series.at(i)->append(queryChart.value("dia").toInt(), value);
      }
    }

    auto axes = chart.axes();

    if (axes.isEmpty()) { return qApp->enqueueError("Sem eixos X e Y!", this); }

    auto axisX = static_cast<QValueAxis *>(axes.at(0));
    auto axisY = static_cast<QValueAxis *>(axes.at(1));

    axisX->setRange(0, 32);
    axisY->setRange(0, max * 1.05);
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

  default: break;
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

// TODO: hover está pegando o valor do pixel, passando o mouse por baixo sai um valor diferente de passar por cima,
// verificar se dá para pegar o valor da Series em vez do valor no gráfico
