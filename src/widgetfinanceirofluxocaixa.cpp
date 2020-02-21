#include "widgetfinanceirofluxocaixa.h"
#include "ui_widgetfinanceirofluxocaixa.h"

#include "application.h"
#include "pagamentosdia.h"
#include "reaisdelegate.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

WidgetFinanceiroFluxoCaixa::WidgetFinanceiroFluxoCaixa(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroFluxoCaixa) { ui->setupUi(this); }

WidgetFinanceiroFluxoCaixa::~WidgetFinanceiroFluxoCaixa() { delete ui; }

void WidgetFinanceiroFluxoCaixa::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetFinanceiroFluxoCaixa::montaFiltro, connectionType);
  connect(ui->groupBoxCaixa1, &QGroupBox::toggled, this, &WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa1_toggled, connectionType);
  connect(ui->groupBoxCaixa2, &QGroupBox::toggled, this, &WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa2_toggled, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetFinanceiroFluxoCaixa::montaFiltro, connectionType);
  connect(ui->itemBoxCaixa1, &ItemBox::textChanged, this, &WidgetFinanceiroFluxoCaixa::montaFiltro, connectionType);
  connect(ui->itemBoxCaixa2, &ItemBox::textChanged, this, &WidgetFinanceiroFluxoCaixa::montaFiltro, connectionType);
  connect(ui->tableCaixa, &TableView::activated, this, &WidgetFinanceiroFluxoCaixa::on_tableCaixa_activated, connectionType);
  connect(ui->tableCaixa2, &TableView::activated, this, &WidgetFinanceiroFluxoCaixa::on_tableCaixa2_activated, connectionType);
}

void WidgetFinanceiroFluxoCaixa::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(qApp->serverDate());

    ui->itemBoxCaixa1->setSearchDialog(SearchDialog::conta(this));
    ui->itemBoxCaixa2->setSearchDialog(SearchDialog::conta(this));

    // REFAC: 0dont hardcode magic numbers
    const int contaSantander = 3;
    const int contaItau = 33;

    ui->itemBoxCaixa1->setId(contaSantander);
    ui->itemBoxCaixa2->setId(contaItau);

    ui->groupBoxCaixa1->setChecked(true);
    ui->groupBoxCaixa2->setChecked(true);

    setConnections();

    isSet = true;
  }

  if (not modelIsSet) { modelIsSet = true; }

  montaFiltro();
}

void WidgetFinanceiroFluxoCaixa::resetTables() { modelIsSet = false; }

void WidgetFinanceiroFluxoCaixa::montaFiltro() {
  const QString filtroData =
      ui->groupBoxMes->isChecked() ? "`dataRealizado` IS NOT NULL AND DATE_FORMAT(`dataRealizado`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "`dataRealizado` IS NOT NULL";

  const QString filtroConta = (ui->groupBoxCaixa1->isChecked() and ui->itemBoxCaixa1->getId().isValid()) ? "idConta = " + ui->itemBoxCaixa1->getId().toString() + " AND " : "";

  // TODO: see if the outer select can be removed
  modelCaixa.setQuery("SELECT * FROM (SELECT v.*, @running_total := @running_total + COALESCE(v.`R$`, 0) AS Acumulado FROM view_fluxo_resumo_realizado v JOIN (SELECT @running_total := 0) r "
                      "WHERE " +
                      filtroConta + "`dataRealizado` IS NOT NULL ORDER BY dataRealizado, idConta) x WHERE " + filtroData);

  modelCaixa.setHeaderData("dataRealizado", "Data Realizado");

  ui->tableCaixa->setModel(&modelCaixa);

  ui->tableCaixa->setItemDelegateForColumn("SAIDA", new ReaisDelegate(this));
  ui->tableCaixa->setItemDelegateForColumn("ENTRADA", new ReaisDelegate(this));
  ui->tableCaixa->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->tableCaixa->setItemDelegateForColumn("Acumulado", new ReaisDelegate(this));

  ui->tableCaixa->hideColumn("contaDestino");
  ui->tableCaixa->hideColumn("idConta");

  // calcular saldo

  QSqlQuery query;

  if (not query.exec(modelCaixa.query().executedQuery() + " ORDER BY dataRealizado DESC LIMIT 1")) { return qApp->enqueueError("Erro buscando saldo: " + query.lastError().text(), this); }

  if (query.first()) { ui->doubleSpinBoxSaldo1->setValue(query.value("Acumulado").toDouble()); }

  // ----------------------------------------------------------------------------------------------------------

  const QString filtroConta2 = (ui->groupBoxCaixa2->isChecked() and ui->itemBoxCaixa2->getId().isValid()) ? "idConta = " + ui->itemBoxCaixa2->getId().toString() + " AND " : "";

  modelCaixa2.setQuery("SELECT * FROM (SELECT v.*, @running_total := @running_total + COALESCE(v.`R$`, 0) AS Acumulado FROM view_fluxo_resumo_realizado v JOIN (SELECT @running_total := 0) r "
                       "WHERE " +
                       filtroConta2 + "`dataRealizado` IS NOT NULL ORDER BY dataRealizado, idConta) x WHERE " + filtroData);

  modelCaixa2.setHeaderData("dataRealizado", "Data Realizado");
  modelCaixa2.setHeaderData("contaDestino", "Conta");

  ui->tableCaixa2->setModel(&modelCaixa2);

  ui->tableCaixa2->setItemDelegateForColumn("SAIDA", new ReaisDelegate(this));
  ui->tableCaixa2->setItemDelegateForColumn("ENTRADA", new ReaisDelegate(this));
  ui->tableCaixa2->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->tableCaixa2->setItemDelegateForColumn("Acumulado", new ReaisDelegate(this));

  filtroConta2.isEmpty() ? ui->tableCaixa2->showColumn("contaDestino") : ui->tableCaixa2->hideColumn("contaDestino");
  ui->tableCaixa2->hideColumn("idConta");

  // calcular saldo

  if (not query.exec(modelCaixa2.query().executedQuery() + " ORDER BY dataRealizado DESC LIMIT 1")) { return qApp->enqueueError("Erro buscando saldo: " + query.lastError().text(), this); }

  if (query.first()) { ui->doubleSpinBoxSaldo2->setValue(query.value("Acumulado").toDouble()); }

  // ----------------------------------------------------------------------------------------------------------

  modelFuturo.setQuery("SELECT v.*, @running_total := @running_total + COALESCE(v.`R$`, 0) AS Acumulado FROM view_fluxo_resumo_pendente v JOIN (SELECT @running_total := 0) r");

  if (modelFuturo.lastError().isValid()) { return qApp->enqueueError("Erro buscando dados futuros: " + modelFuturo.lastError().text(), this); }

  modelFuturo.setHeaderData("dataPagamento", "Data Pag.");

  ui->tableFuturo->setModel(&modelFuturo);

  ui->tableFuturo->setItemDelegateForColumn("SAIDA", new ReaisDelegate(this));
  ui->tableFuturo->setItemDelegateForColumn("ENTRADA", new ReaisDelegate(this));
  ui->tableFuturo->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->tableFuturo->setItemDelegateForColumn("Acumulado", new ReaisDelegate(this));
}

void WidgetFinanceiroFluxoCaixa::on_tableCaixa2_activated(const QModelIndex &index) {
  const QDate date = modelCaixa2.data(index.row(), "dataRealizado").toDate();
  const QString idConta = modelCaixa2.data(index.row(), "idConta").toString();

  auto *dia = new PagamentosDia(this);
  dia->setFilter(date, idConta);
  dia->show();
}

void WidgetFinanceiroFluxoCaixa::on_tableCaixa_activated(const QModelIndex &index) {
  const QDate date = modelCaixa.data(index.row(), "dataRealizado").toDate();
  const QString idConta = modelCaixa.data(index.row(), "idConta").toString();

  auto *dia = new PagamentosDia(this);
  dia->setFilter(date, idConta);
  dia->show();
}

void WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa1_toggled(const bool checked) {
  if (not checked) {
    ui->itemBoxCaixa1->clear();
    return;
  }

  montaFiltro();
}

void WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa2_toggled(const bool checked) {
  if (not checked) {
    ui->itemBoxCaixa2->clear();
    return;
  }

  montaFiltro();
}

// TODO: 0nao agrupar contas no view_fluxo_resumo (apenas quando filtrado)
// TODO: 0fazer delegate para reduzir tamanho da fonte
// TODO: separar a tabela 'Futuro' em duas telas, uma 'vencidos' e a outra mantem igual a atual
