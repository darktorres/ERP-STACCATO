#include <QSqlError>

#include "ui_widgetcompraresumo.h"
#include "widgetcompraresumo.h"

WidgetCompraResumo::WidgetCompraResumo(QWidget *parent) : Widget(parent), ui(new Ui::WidgetCompraResumo) { ui->setupUi(this); }

WidgetCompraResumo::~WidgetCompraResumo() { delete ui; }

void WidgetCompraResumo::setupTables() {
  modelResumo.setTable("view_fornecedor_compra");

  modelResumo.setHeaderData("fornecedor", "Forn.");

  modelResumo.setFilter("(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  ui->tableResumo->setModel(&modelResumo);
  ui->tableResumo->hideColumn("idVenda");
}

void WidgetCompraResumo::updateTables() {
  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not modelResumo.select()) { return; }

  ui->tableResumo->resizeColumnsToContents();
}

void WidgetCompraResumo::resetTables() { modelIsSet = false; }
