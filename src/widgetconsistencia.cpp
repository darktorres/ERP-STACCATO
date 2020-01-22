#include <QSqlError>

#include "application.h"
#include "ui_widgetconsistencia.h"
#include "widgetconsistencia.h"

WidgetConsistencia::WidgetConsistencia(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetConsistencia) {
  ui->setupUi(this);
  ui->dateEditMes->setDate(QDate::currentDate());
  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetConsistencia::setupTables);
}

WidgetConsistencia::~WidgetConsistencia() { delete ui; }

void WidgetConsistencia::resetTables() { modelIsSet = false; }

void WidgetConsistencia::updateTables() {
  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not model1.select()) { qApp->enqueueError("Erro1: " + model1.lastError().text(), this); }

  if (not model2.select()) { qApp->enqueueError("Erro2: " + model2.lastError().text(), this); }

  if (not model3.select()) { qApp->enqueueError("Erro3: " + model3.lastError().text(), this); }

  if (not model4.select()) { qApp->enqueueError("Erro4: " + model4.lastError().text(), this); }

  if (not model5.select()) { qApp->enqueueError("Erro5: " + model5.lastError().text(), this); }

  if (not model6.select()) { qApp->enqueueError("Erro6: " + model6.lastError().text(), this); }

  if (not model7.select()) { qApp->enqueueError("Erro7: " + model7.lastError().text(), this); }

  if (not model8.select()) { qApp->enqueueError("Erro8: " + model8.lastError().text(), this); }
}

void WidgetConsistencia::setupTables() {
  const QDate date = ui->dateEditMes->date();
  const QString dateBegin = QDate(date.year(), date.month(), 1).toString("yyyy-MM-dd");
  const QString dateEnd = QDate(date.year(), date.month(), date.daysInMonth()).toString("yyyy-MM-dd");

  //-------------------------------------

  model1.setTable("view_consistencia_compra");

  model1.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model1.select()) { qApp->enqueueError("Erro1: " + model1.lastError().text(), this); }

  ui->tableView->setModel(&model1);

  //-------------------------------------

  model2.setTable("view_consistencia_vp1_v_total");

  model2.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model2.select()) { qApp->enqueueError("Erro2: " + model2.lastError().text(), this); }

  ui->tableView_2->setModel(&model2);

  //-------------------------------------

  model3.setTable("view_consistencia_vp1_vp2_quant");

  model3.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model3.select()) { qApp->enqueueError("Erro3: " + model3.lastError().text(), this); }

  ui->tableView_3->setModel(&model3);

  //-------------------------------------

  model4.setTable("view_consistencia_vp1_vp2_total");

  model4.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model4.select()) { qApp->enqueueError("Erro4: " + model4.lastError().text(), this); }

  ui->tableView_4->setModel(&model4);

  //-------------------------------------

  model5.setTable("view_consistencia_vp2_ehc_quant");

  model5.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model5.select()) { qApp->enqueueError("Erro5: " + model5.lastError().text(), this); }

  ui->tableView_5->setModel(&model5);

  //-------------------------------------

  model6.setTable("view_consistencia_vp2_pf2_quant");

  model6.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model6.select()) { qApp->enqueueError("Erro6: " + model6.lastError().text(), this); }

  ui->tableView_6->setModel(&model6);

  //-------------------------------------

  model7.setTable("view_consistencia_vp_op_quant");

  model7.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model7.select()) { qApp->enqueueError("Erro7: " + model7.lastError().text(), this); }

  ui->tableView_7->setModel(&model7);

  //-------------------------------------

  model8.setTable("view_consistencia_vinculos");

  model8.setFilter("DATE(created) BETWEEN '" + dateBegin + "' AND '" + dateEnd + "'");

  if (not model8.select()) { qApp->enqueueError("Erro8: " + model8.lastError().text(), this); }

  ui->tableView_8->setModel(&model8);
}
