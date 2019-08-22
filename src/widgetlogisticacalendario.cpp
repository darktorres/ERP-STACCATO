#include <QCheckBox>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "collapsiblewidget.h"
#include "ui_widgetlogisticacalendario.h"
#include "widgetlogisticacalendario.h"

WidgetLogisticaCalendario::WidgetLogisticaCalendario(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaCalendario) { ui->setupUi(this); }

WidgetLogisticaCalendario::~WidgetLogisticaCalendario() { delete ui; }

void WidgetLogisticaCalendario::setConnections() {
  connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, &WidgetLogisticaCalendario::on_calendarWidget_selectionChanged);
  connect(ui->checkBoxMostrarFiltros, &QCheckBox::toggled, this, &WidgetLogisticaCalendario::on_checkBoxMostrarFiltros_toggled);
  connect(ui->pushButtonAnterior, &QPushButton::clicked, this, &WidgetLogisticaCalendario::on_pushButtonAnterior_clicked);
  connect(ui->pushButtonProximo, &QPushButton::clicked, this, &WidgetLogisticaCalendario::on_pushButtonProximo_clicked);
}

void WidgetLogisticaCalendario::listarVeiculos() {
  QSqlQuery query;

  if (not query.exec("SELECT t.razaoSocial, tv.modelo FROM transportadora t LEFT JOIN transportadora_has_veiculo tv ON t.idTransportadora = tv.idTransportadora ORDER BY razaoSocial, modelo")) {
    return qApp->enqueueError("Erro buscando veiculos: " + query.lastError().text(), this);
  }

  while (query.next()) {
    auto *checkbox = new QCheckBox(this);
    checkbox->setText(query.value("razaoSocial").toString() + " / " + query.value("modelo").toString());
    checkbox->setChecked(true);
    connect(checkbox, &QAbstractButton::toggled, this, &WidgetLogisticaCalendario::updateFilter);
    ui->groupBoxVeiculos->layout()->addWidget(checkbox);
  }

  ui->groupBoxVeiculos->layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void WidgetLogisticaCalendario::updateTables() {
  if (not isSet) {
    listarVeiculos();
    setConnections();
    ui->checkBoxMostrarFiltros->toggle();
    isSet = true;
  }

  if (not modelIsSet) { modelIsSet = true; }

  const QDate date = ui->calendarWidget->selectedDate();
  updateCalendar(date.addDays(date.dayOfWeek() * -1));
}

void WidgetLogisticaCalendario::resetTables() { modelIsSet = false; }

void WidgetLogisticaCalendario::updateFilter() {
  const QDate date = ui->calendarWidget->selectedDate();
  updateCalendar(date.addDays(date.dayOfWeek() * -1));
}

void WidgetLogisticaCalendario::updateCalendar(const QDate &startDate) {
  ui->tableWidget->clearContents();

  ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);

  int veiculos = 0;
  const int start = startDate.day();

  QStringList list;

  const auto children = ui->groupBoxVeiculos->findChildren<QCheckBox *>();

  for (const auto &child : children) {
    if (not child->isChecked()) { continue; }

    veiculos++;

    QStringList temp = child->text().split(" / ");

    list << "Manhã\n" + temp.at(0) + "\n" + temp.at(1);
    list << "Tarde\n" + temp.at(0) + "\n" + temp.at(1);
  }

  ui->tableWidget->setRowCount(veiculos * 2); // manha/tarde
  ui->tableWidget->setVerticalHeaderLabels(list);

  ui->tableWidget->setColumnCount(7); // dias
  ui->tableWidget->setHorizontalHeaderLabels({"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"});

  int dia = start;
  const QDate date = ui->calendarWidget->selectedDate();
  const int diasMes = date.addDays(date.dayOfWeek() * -1).daysInMonth();

  for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
    const auto item = ui->tableWidget->horizontalHeaderItem(col);
    item->setText(QString::number(dia) + " " + item->text());
    dia++;
    if (dia > diasMes) { dia = 1; }
  }

  QSqlQuery query;
  query.prepare("SELECT * FROM view_calendario WHERE data BETWEEN :start AND :end");
  query.bindValue(":start", startDate);
  query.bindValue(":end", startDate.addDays(6));

  if (not query.exec()) { return qApp->enqueueError("Erro query: " + query.lastError().text(), this); }

  while (query.next()) {
    const QString transportadora = query.value("razaoSocial").toString() + "\n" + query.value("modelo").toString();

    int row = -1;

    for (int i = 0; i < list.size(); ++i) {
      if (list.at(i).contains(transportadora)) {
        row = query.value("data").toTime().hour() < 12 ? i : i + 1; // manha/tarde
        break;
      }
    }

    if (row == -1) { continue; }

    const int diaSemana = query.value("data").toDate().dayOfWeek();

    auto *widget = ui->tableWidget->cellWidget(row, diaSemana) ? static_cast<CollapsibleWidget *>(ui->tableWidget->cellWidget(row, diaSemana)) : new CollapsibleWidget(this);

    const QString oldText = widget->getHtml();

    QString text = oldText.isEmpty() ? "" : oldText + R"(<p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">&nbsp;</p>
                                                         <p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">-----------------------------------------</p>
                                                         <p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">&nbsp;</p>)";

    QStringList produtos = query.value("produtos").toString().split("/");
    QString produtosList;

    for (auto &produto : produtos) { produtosList += QString(R"(<li style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">%1</li>)").arg(produto); }

    const QString destino = query.value("logradouro").toString().replace(" ", "+") + "," + query.value("numero").toString().replace(" ", "+") + "," +
                            query.value("cidade").toString().replace(" ", "+") + "," + query.value("uf").toString().replace(" ", "+");

    // TODO: colocar origem como 'arg'

    text += QString(R"(<p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">10:00 Kg: %1, Cx.: %2</p>
           <p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">%3</p>
           <p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">%4</p>
           <ul>
           %5
           </ul>
           <p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">Status: %6</p>
           <p style="-qt-block-indent: 0; text-indent: 0px; margin: 0px;">
           <a href="https://www.google.com/maps/dir/?api=1&amp;origin=Rua+Sales&oacute;polis,27,Barueri,SP&amp;destination=%7&amp;
           travelmode=driving" target="_blank" rel="noopener">Google Maps</a></p>)")
                .arg(query.value("kg").toString())
                .arg(query.value("caixas").toString())
                .arg(query.value("idVenda").toString())
                .arg(query.value("bairro").toString() + " - " + query.value("cidade").toString())
                .arg(produtosList)
                .arg(query.value("status").toString())
                .arg(destino);

    widget->setHtml(text);
    ui->tableWidget->setCellWidget(row, diaSemana, widget);
    connect(widget, &CollapsibleWidget::toggled, ui->tableWidget, &QTableWidget::resizeColumnsToContents);
    connect(widget, &CollapsibleWidget::toggled, ui->tableWidget, &QTableWidget::resizeRowsToContents);
  }

  ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  const QString range = startDate.toString("dd-MM-yyyy") + " - " + startDate.addDays(6).toString("dd-MM-yyyy");

  ui->lineEditRange->setText(range);
}

void WidgetLogisticaCalendario::on_checkBoxMostrarFiltros_toggled(bool checked) {
  ui->calendarWidget->setVisible(checked);
  ui->groupBoxVeiculos->setVisible(checked);
}

void WidgetLogisticaCalendario::on_pushButtonProximo_clicked() { ui->calendarWidget->setSelectedDate(ui->calendarWidget->selectedDate().addDays(7)); }

void WidgetLogisticaCalendario::on_pushButtonAnterior_clicked() { ui->calendarWidget->setSelectedDate(ui->calendarWidget->selectedDate().addDays(-7)); }

void WidgetLogisticaCalendario::on_calendarWidget_selectionChanged() { updateFilter(); }

// TODO: esconder veiculos que não possuem agendamento na semana
