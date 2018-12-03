#include <QMessageBox>
#include <QSqlError>

#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "ui_widgetfinanceirocompra.h"
#include "widgetfinanceirocompra.h"

WidgetFinanceiroCompra::WidgetFinanceiroCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroCompra) { ui->setupUi(this); }

WidgetFinanceiroCompra::~WidgetFinanceiroCompra() { delete ui; }

void WidgetFinanceiroCompra::setConnections() {
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetFinanceiroCompra::on_lineEditBusca_textChanged);
  connect(ui->table, &TableView::activated, this, &WidgetFinanceiroCompra::on_table_activated);
}

void WidgetFinanceiroCompra::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelViewComprasFinanceiro.select()) { return; }
}

void WidgetFinanceiroCompra::resetTables() { modelIsSet = false; }

void WidgetFinanceiroCompra::setupTables() {
  modelViewComprasFinanceiro.setTable("view_compras_financeiro");
  modelViewComprasFinanceiro.setEditStrategy(QSqlTableModel::OnManualSubmit);

  ui->table->setModel(&modelViewComprasFinanceiro);
  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
}

void WidgetFinanceiroCompra::on_table_activated(const QModelIndex &index) {
  InputDialogFinanceiro input(InputDialogFinanceiro::Tipo::Financeiro);
  input.setFilter(modelViewComprasFinanceiro.data(index.row(), "Compra").toString());

  if (input.exec() != InputDialogFinanceiro::Accepted) { return; }
}

void WidgetFinanceiroCompra::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetFinanceiroCompra::montaFiltro() {
  const QString text = ui->lineEditBusca->text();
  const QString filtroBusca = text.isEmpty() ? "" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  modelViewComprasFinanceiro.setFilter(filtroBusca);

  if (not modelViewComprasFinanceiro.select()) { return; }
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4associar notas com cada produto? e verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota
