#include <QMessageBox>
#include <QSqlError>

#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "ui_widgethistoricocompra.h"
#include "widgethistoricocompra.h"

WidgetHistoricoCompra::WidgetHistoricoCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetHistoricoCompra) { ui->setupUi(this); }

WidgetHistoricoCompra::~WidgetHistoricoCompra() { delete ui; }

void WidgetHistoricoCompra::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetHistoricoCompra::on_lineEditBusca_textChanged, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetHistoricoCompra::on_table_activated, connectionType);
}

void WidgetHistoricoCompra::updateTables() {
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

void WidgetHistoricoCompra::setTipo(const WidgetHistoricoCompra::Tipo novotipo) { tipo = novotipo; }

void WidgetHistoricoCompra::resetTables() { modelIsSet = false; }

void WidgetHistoricoCompra::setupTables() {
  modelViewComprasFinanceiro.setTable("view_compras_financeiro");

  ui->table->setModel(&modelViewComprasFinanceiro);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
}

void WidgetHistoricoCompra::on_table_activated(const QModelIndex &index) {
  const auto tipoFinanceiro = (tipo == Tipo::Compra) ? InputDialogFinanceiro::Tipo::Historico : InputDialogFinanceiro::Tipo::Financeiro;

  InputDialogFinanceiro *input = new InputDialogFinanceiro(tipoFinanceiro, this);
  input->setAttribute(Qt::WA_DeleteOnClose);
  input->setFilter(modelViewComprasFinanceiro.data(index.row(), "Compra").toString());

  input->show();
}

void WidgetHistoricoCompra::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetHistoricoCompra::montaFiltro() {
  const QString text = ui->lineEditBusca->text();
  const QString filtroBusca = text.isEmpty() ? "" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  modelViewComprasFinanceiro.setFilter(filtroBusca);
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4associar notas com cada produto? e verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota
