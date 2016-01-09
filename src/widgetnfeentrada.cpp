#include <QSqlError>

#include "doubledelegate.h"
#include "ui_widgetnfeentrada.h"
#include "widgetnfeentrada.h"
#include "xml_viewer.h"

WidgetNfeEntrada::WidgetNfeEntrada(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeEntrada) {
  ui->setupUi(this);

  setupTables();
}

WidgetNfeEntrada::~WidgetNfeEntrada() { delete ui; }

QString WidgetNfeEntrada::updateTables() {
  if (not model.select()) return "Erro lendo tabela NFe: " + model.lastError().text();

  ui->table->resizeColumnsToContents();

  return QString();
}

void WidgetNfeEntrada::setupTables() {
  DoubleDelegate *doubledelegate = new DoubleDelegate(this);

  model.setTable("nfe");
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);
  model.setFilter("tipo = 'ENTRADA'");

  ui->table->setModel(&model);
  ui->table->hideColumn("xml");
  ui->table->setItemDelegate(doubledelegate);
}

void WidgetNfeEntrada::on_table_activated(const QModelIndex &index) {
  XML_Viewer *viewer = new XML_Viewer(this);
  viewer->exibirXML(model.data(index.row(), "xml").toByteArray());
}