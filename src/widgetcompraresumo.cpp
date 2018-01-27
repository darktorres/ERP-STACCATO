#include <QSqlError>

#include "ui_widgetcompraresumo.h"
#include "widgetcompraresumo.h"

WidgetCompraResumo::WidgetCompraResumo(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraResumo) {
  ui->setupUi(this);

  setupTables();
}

WidgetCompraResumo::~WidgetCompraResumo() { delete ui; }

void WidgetCompraResumo::setupTables() {
  modelResumo.setTable("view_fornecedor_compra");

  modelResumo.setHeaderData("fornecedor", "Forn.");

  modelResumo.setFilter("(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  //  if (not modelResumo.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela resumo: " + modelResumo.lastError().text());

  ui->tableResumo->setModel(&modelResumo);
  ui->tableResumo->hideColumn("idVenda");
}

bool WidgetCompraResumo::updateTables() {
  if (not modelResumo.select()) {
    emit errorSignal("Erro lendo tabela resumo: " + modelResumo.lastError().text());
    return false;
  }

  ui->tableResumo->resizeColumnsToContents();

  return true;
}
