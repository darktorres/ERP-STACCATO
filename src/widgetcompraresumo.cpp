#include "widgetcompraresumo.h"
#include "ui_widgetcompraresumo.h"

#include <QSqlError>

#include "permissao.h"

WidgetCompraResumo::WidgetCompraResumo(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraResumo) { ui->setupUi(this); }

WidgetCompraResumo::~WidgetCompraResumo() { delete ui; }

void WidgetCompraResumo::setupTables() {
  modelResumo.setTable("view_fornecedor_compra");

  modelResumo.setFilter("");

  modelResumo.setHeaderData("fornecedor", "Forn.");

  ui->tableResumo->setModel(&modelResumo);
}

void WidgetCompraResumo::updateTables() {
  if (not isSet) {
    setupTables();
    Permissao::aplicarTela(this, "compras.resumo");
    isSet = true;
  }

  modelResumo.select();
}

void WidgetCompraResumo::resetTables() { setupTables(); }
