#include "widgetconsistencia.h"
#include "ui_widgetconsistencia.h"

#include "application.h"

#include <QSqlError>

WidgetConsistencia::WidgetConsistencia(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetConsistencia) {
  ui->setupUi(this);

  setConnections();
}

WidgetConsistencia::~WidgetConsistencia() { delete ui; }

void WidgetConsistencia::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetConsistencia::setupTables, connectionType);
}

void WidgetConsistencia::resetTables() { setupTables(); }

void WidgetConsistencia::updateTables() {
  if (not isSet) {
    ui->dateEditMes->setDate(qApp->serverDate());
    setupTables();
    isSet = true;
  }

  model1.select();
  model2.select();
  model3.select();
  model4.select();
  model5.select();
  model6.select();
  model7.select();
  model8.select();
}

void WidgetConsistencia::setupTables() {
  const QDate date = ui->dateEditMes->date();
  const QString dateBegin = QDate(date.year(), date.month(), 1).toString("yyyy-MM-dd");
  const QString dateEnd = QDate(date.year(), date.month(), date.daysInMonth()).toString("yyyy-MM-dd");

  //-------------------------------------

  model1.setTable("view_consistencia_compra");

  model1.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model1.select();

  ui->tableView->setModel(&model1);

  //-------------------------------------

  model2.setTable("view_consistencia_vp1_v_total");

  model2.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model2.select();

  ui->tableView_2->setModel(&model2);

  //-------------------------------------

  model3.setTable("view_consistencia_vp1_vp2_quant");

  model3.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model3.select();

  ui->tableView_3->setModel(&model3);

  //-------------------------------------

  model4.setTable("view_consistencia_vp1_vp2_total");

  model4.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model4.select();

  ui->tableView_4->setModel(&model4);

  //-------------------------------------

  model5.setTable("view_consistencia_vp2_ehc_quant");

  model5.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model5.select();

  ui->tableView_5->setModel(&model5);

  //-------------------------------------

  model6.setTable("view_consistencia_vp2_pf2_quant");

  model6.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model6.select();

  ui->tableView_6->setModel(&model6);

  //-------------------------------------

  model7.setTable("view_consistencia_vp_op_quant");

  model7.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model7.select();

  ui->tableView_7->setModel(&model7);

  //-------------------------------------

  model8.setTable("view_consistencia_vinculos");

  model8.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  model8.select();

  ui->tableView_8->setModel(&model8);
}
