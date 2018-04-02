#include <QMessageBox>
#include <QSqlError>

#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "ui_widgetfinanceirocompra.h"
#include "widgetfinanceirocompra.h"

WidgetFinanceiroCompra::WidgetFinanceiroCompra(QWidget *parent) : Widget(parent), ui(new Ui::WidgetFinanceiroCompra) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetFinanceiroCompra::on_lineEditBusca_textChanged);
  connect(ui->table, &TableView::activated, this, &WidgetFinanceiroCompra::on_table_activated);
  connect(ui->table, &TableView::entered, this, &WidgetFinanceiroCompra::on_table_entered);
}

WidgetFinanceiroCompra::~WidgetFinanceiroCompra() { delete ui; }

bool WidgetFinanceiroCompra::updateTables() {
  if (modelViewComprasFinanceiro.tableName().isEmpty()) setupTables();

  if (not modelViewComprasFinanceiro.select()) {
    emit errorSignal("Erro lendo tabela de compras: " + modelViewComprasFinanceiro.lastError().text());
    return false;
  }

  return true;
}

void WidgetFinanceiroCompra::setupTables() {
  // REFAC: refactor this to not select in here

  modelViewComprasFinanceiro.setTable("view_compras_financeiro");
  modelViewComprasFinanceiro.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (not modelViewComprasFinanceiro.select()) emit errorSignal("Erro lendo tabela de compras: " + modelViewComprasFinanceiro.lastError().text());

  ui->table->setModel(&modelViewComprasFinanceiro);
  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
  ui->table->hideColumn("desativado");
}

void WidgetFinanceiroCompra::on_table_activated(const QModelIndex &index) {
  InputDialogFinanceiro input(InputDialogFinanceiro::Tipo::Financeiro);
  input.setFilter(modelViewComprasFinanceiro.data(index.row(), "Compra").toString());

  if (input.exec() != InputDialogFinanceiro::Accepted) { return; }
}

void WidgetFinanceiroCompra::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetFinanceiroCompra::on_lineEditBusca_textChanged(const QString &text) {
  const QString filtroBusca = text.isEmpty() ? "" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  modelViewComprasFinanceiro.setFilter(filtroBusca);

  if (not modelViewComprasFinanceiro.select()) emit errorSignal("Erro lendo tabela: " + modelViewComprasFinanceiro.lastError().text());
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4associar notas com cada produto? e verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota
