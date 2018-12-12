#include <QCheckBox>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
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

  int veiculos = 0;
  const int start = startDate.day();

  QStringList list;

  Q_FOREACH (const auto &item, ui->groupBoxVeiculos->findChildren<QCheckBox *>()) {
    if (not item->isChecked()) { continue; }

    veiculos++;

    QStringList temp = item->text().split(" / ");

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

    QTableWidgetItem *item = ui->tableWidget->item(row, diaSemana) ? ui->tableWidget->item(row, diaSemana) : new QTableWidgetItem();

    const QString oldText = item->text();

    QString text = oldText.isEmpty() ? "" : oldText + "\n\n------------------------------------\n\n";

    text += query.value("data").toTime().toString("hh:mm") + "  Kg: " + query.value("kg").toString() + ", Cx.: " + query.value("caixas").toString();

    if (not query.value("idVenda").toString().isEmpty()) { text += "\n           " + query.value("idVenda").toString(); }

    if (not query.value("bairro").toString().isEmpty()) { text += " - " + query.value("bairro").toString() + " - " + query.value("cidade").toString(); }

    // TODO: 0dont show this to compact screen? or show this only on doubleclick
    text += "\n" + query.value("text").toString();

    text += "\n           Status: " + query.value("status").toString();

    item->setText(text);

    if (not ui->tableWidget->item(row, diaSemana)) { ui->tableWidget->setItem(row, diaSemana, item); }
  }

  ui->tableWidget->resizeColumnsToContents();
  ui->tableWidget->resizeRowsToContents();

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
