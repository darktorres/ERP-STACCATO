#include "widgetfinanceirocompra.h"
#include "ui_widgetfinanceirocompra.h"

#include "application.h"
#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"

WidgetFinanceiroCompra::WidgetFinanceiroCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroCompra) { ui->setupUi(this); }

WidgetFinanceiroCompra::~WidgetFinanceiroCompra() { delete ui; }

void WidgetFinanceiroCompra::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetFinanceiroCompra::on_lineEditBusca_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetFinanceiroCompra::delayFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetFinanceiroCompra::on_table_activated, connectionType);
}

void WidgetFinanceiroCompra::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  model.select();
}

void WidgetFinanceiroCompra::delayFiltro() { timer.start(qApp->delayedTimer); }

void WidgetFinanceiroCompra::resetTables() { modelIsSet = false; }

void WidgetFinanceiroCompra::setupTables() {
  model.setTable("view_compras_financeiro");

  ui->table->setModel(&model);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->table->hideColumn("Compra");
}

void WidgetFinanceiroCompra::on_table_activated(const QModelIndex &index) {
  auto *input = new InputDialogFinanceiro(InputDialogFinanceiro::Tipo::Financeiro, this);
  input->setAttribute(Qt::WA_DeleteOnClose);
  input->setFilter(model.data(index.row(), "Compra").toString());

  input->show();
}

void WidgetFinanceiroCompra::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetFinanceiroCompra::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = text.isEmpty() ? "" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  model.setFilter(filtroBusca);
}
