#include "widgetfinanceirofluxocaixa.h"
#include "ui_widgetfinanceirofluxocaixa.h"

#include "application.h"
#include "pagamentosdia.h"
#include "reaisdelegate.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>

WidgetFinanceiroFluxoCaixa::WidgetFinanceiroFluxoCaixa(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroFluxoCaixa) {
  ui->setupUi(this);

  ui->tableCaixa1->setDisabled(true);
  ui->tableCaixa2->setDisabled(true);
  ui->tableFuturo->setDisabled(true);
}

WidgetFinanceiroFluxoCaixa::~WidgetFinanceiroFluxoCaixa() { delete ui; }

void WidgetFinanceiroFluxoCaixa::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetFinanceiroFluxoCaixa::alterarData, connectionType);
  connect(ui->groupBoxCaixa1, &QGroupBox::toggled, this, &WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa1_toggled, connectionType);
  connect(ui->groupBoxCaixa2, &QGroupBox::toggled, this, &WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa2_toggled, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetFinanceiroFluxoCaixa::alterarData, connectionType);
  connect(ui->itemBoxCaixa1, &ItemBox::textChanged, this, &WidgetFinanceiroFluxoCaixa::montaTabela1, connectionType);
  connect(ui->itemBoxCaixa2, &ItemBox::textChanged, this, &WidgetFinanceiroFluxoCaixa::montaTabela2, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &WidgetFinanceiroFluxoCaixa::montaFiltro, connectionType);
  connect(ui->tableCaixa1, &TableView::activated, this, &WidgetFinanceiroFluxoCaixa::on_tableCaixa1_activated, connectionType);
  connect(ui->tableCaixa2, &TableView::activated, this, &WidgetFinanceiroFluxoCaixa::on_tableCaixa2_activated, connectionType);
}

void WidgetFinanceiroFluxoCaixa::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(qApp->serverDate());

    ui->itemBoxCaixa1->setSearchDialog(SearchDialog::conta(this));
    ui->itemBoxCaixa2->setSearchDialog(SearchDialog::conta(this));

    // TODO: 0dont hardcode magic numbers
    // TODO: colocar um icone de engrenagem para o usuario mudar as contas padrao. salvar os ids no banco de dados
    const int contaSafra = 41;
    const int contaItau = 33;

    ui->itemBoxCaixa1->setId(contaSafra);
    ui->itemBoxCaixa2->setId(contaItau);

    ui->groupBoxCaixa1->setChecked(true);
    ui->groupBoxCaixa2->setChecked(true);

    filtroData = (ui->groupBoxMes->isChecked()) ? "WHERE DATE_FORMAT(`dataRealizado`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";

    setConnections();

    isSet = true;
  }

  if (not modelIsSet) { modelIsSet = true; }
}

void WidgetFinanceiroFluxoCaixa::resetTables() { modelIsSet = false; }

void WidgetFinanceiroFluxoCaixa::montaFiltro() {
  ui->tableCaixa1->setDisabled(true);
  ui->tableCaixa2->setDisabled(true);
  ui->tableFuturo->setDisabled(true);

  qDebug() << "repaint";
  repaint();

  montaTabela1();
  montaTabela2();
  montaTabela3();

  qDebug() << "end";
}

void WidgetFinanceiroFluxoCaixa::montaTabela1() {
  qDebug() << "tabela1";

  const QString filtroConta = (ui->groupBoxCaixa1->isChecked() and ui->itemBoxCaixa1->getId().isValid()) ? "WHERE idConta = " + ui->itemBoxCaixa1->getId().toString() : "";

  // TODO: move query to Sql class
  modelCaixa.setQuery("WITH x AS (SELECT v.*, SUM(v.`R$`) OVER (ORDER BY dataRealizado) AS Acumulado FROM view_fluxo_resumo_realizado v " + filtroConta + " ORDER BY dataRealizado) SELECT * FROM x " +
                      filtroData + " ORDER BY dataRealizado");

  modelCaixa.select();

  modelCaixa.setHeaderData("dataRealizado", "Data Realizado");
  modelCaixa.setHeaderData("contaDestino", "Conta");

  ui->tableCaixa1->setModel(&modelCaixa);

  ui->tableCaixa1->setItemDelegateForColumn("SAIDA", new ReaisDelegate(this));
  ui->tableCaixa1->setItemDelegateForColumn("ENTRADA", new ReaisDelegate(this));
  ui->tableCaixa1->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->tableCaixa1->setItemDelegateForColumn("Acumulado", new ReaisDelegate(this));

  filtroConta.isEmpty() ? ui->tableCaixa1->showColumn("contaDestino") : ui->tableCaixa1->hideColumn("contaDestino");
  ui->tableCaixa1->hideColumn("idConta");

  qDebug() << "tabela1.1";

  // calcular saldo

  double saldo1 = 0;

  if (modelCaixa.rowCount() > 0) { saldo1 = modelCaixa.data(modelCaixa.rowCount() - 1, "Acumulado").toDouble(); }

  ui->doubleSpinBoxSaldo1->setValue(saldo1);

  ui->tableCaixa1->setEnabled(true);
  repaint();
}

void WidgetFinanceiroFluxoCaixa::montaTabela2() {
  qDebug() << "tabela2";

  const QString filtroConta2 = (ui->groupBoxCaixa2->isChecked() and ui->itemBoxCaixa2->getId().isValid()) ? "WHERE idConta = " + ui->itemBoxCaixa2->getId().toString() : "";

  // TODO: move query to Sql class
  modelCaixa2.setQuery("WITH x AS (SELECT v.*, SUM(v.`R$`) OVER (ORDER BY dataRealizado) AS Acumulado FROM view_fluxo_resumo_realizado v " + filtroConta2 +
                       " ORDER BY dataRealizado) SELECT * FROM x " + filtroData + " ORDER BY dataRealizado");

  modelCaixa2.select();

  modelCaixa2.setHeaderData("dataRealizado", "Data Realizado");
  modelCaixa2.setHeaderData("contaDestino", "Conta");

  ui->tableCaixa2->setModel(&modelCaixa2);

  ui->tableCaixa2->setItemDelegateForColumn("SAIDA", new ReaisDelegate(this));
  ui->tableCaixa2->setItemDelegateForColumn("ENTRADA", new ReaisDelegate(this));
  ui->tableCaixa2->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->tableCaixa2->setItemDelegateForColumn("Acumulado", new ReaisDelegate(this));

  filtroConta2.isEmpty() ? ui->tableCaixa2->showColumn("contaDestino") : ui->tableCaixa2->hideColumn("contaDestino");
  ui->tableCaixa2->hideColumn("idConta");

  qDebug() << "tabela2.1";

  // calcular saldo

  double saldo2 = 0;

  if (modelCaixa2.rowCount() > 0) { saldo2 = modelCaixa2.data(modelCaixa2.rowCount() - 1, "Acumulado").toDouble(); }

  ui->doubleSpinBoxSaldo2->setValue(saldo2);

  ui->tableCaixa2->setEnabled(true);
  repaint();
}

void WidgetFinanceiroFluxoCaixa::montaTabela3() {
  qDebug() << "tabela3";

  // TODO: move query to Sql class
  modelFuturo.setQuery("SELECT v.*, SUM(v.`R$`) OVER (ORDER BY dataPagamento) AS Acumulado FROM view_fluxo_resumo_pendente v ORDER BY dataPagamento");

  modelFuturo.select();

  modelFuturo.setHeaderData("dataPagamento", "Data Pag.");

  ui->tableFuturo->setModel(&modelFuturo);

  ui->tableFuturo->setItemDelegateForColumn("SAIDA", new ReaisDelegate(this));
  ui->tableFuturo->setItemDelegateForColumn("ENTRADA", new ReaisDelegate(this));
  ui->tableFuturo->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->tableFuturo->setItemDelegateForColumn("Acumulado", new ReaisDelegate(this));

  ui->tableFuturo->setEnabled(true);
  repaint();
}

void WidgetFinanceiroFluxoCaixa::on_tableCaixa2_activated(const QModelIndex &index) {
  const QDate date = modelCaixa2.data(index.row(), "dataRealizado").toDate();
  const QString idConta = modelCaixa2.data(index.row(), "idConta").toString();

  auto *dia = new PagamentosDia(this);
  dia->setFilter(date, idConta);
  dia->show();
}

void WidgetFinanceiroFluxoCaixa::on_tableCaixa1_activated(const QModelIndex &index) {
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
}

void WidgetFinanceiroFluxoCaixa::on_groupBoxCaixa2_toggled(const bool checked) {
  if (not checked) {
    ui->itemBoxCaixa2->clear();
    return;
  }
}

void WidgetFinanceiroFluxoCaixa::alterarData() {
  filtroData = (ui->groupBoxMes->isChecked()) ? "WHERE DATE_FORMAT(`dataRealizado`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";

  montaFiltro();
}

// TODO: 0nao agrupar contas no view_fluxo_resumo (apenas quando filtrado)
// TODO: 0fazer delegate para reduzir tamanho da fonte
// TODO: separar a tabela 'Futuro' em duas telas, uma 'vencidos' e a outra mantem igual a atual
