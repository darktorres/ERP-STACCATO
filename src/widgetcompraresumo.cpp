#include "widgetcompraresumo.h"
#include "ui_widgetcompraresumo.h"

#include <QSqlError>

WidgetCompraResumo::WidgetCompraResumo(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraResumo) { ui->setupUi(this); }

WidgetCompraResumo::~WidgetCompraResumo() { delete ui; }

void WidgetCompraResumo::setupTables() {
  modelResumo.setTable("view_fornecedor_compra");

  modelResumo.setFilter("");

  modelResumo.setHeaderData("fornecedor", "Forn.");

  ui->tableResumo->setModel(&modelResumo);
}

void WidgetCompraResumo::updateTables() {
  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not modelResumo.select()) { return; }
}

void WidgetCompraResumo::resetTables() { modelIsSet = false; }
