#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "anteciparrecebimento.h"
#include "application.h"
#include "contas.h"
#include "doubledelegate.h"
#include "inserirlancamento.h"
#include "inserirtransferencia.h"
#include "reaisdelegate.h"
#include "ui_widgetfinanceirocontas.h"
#include "widgetfinanceirocontas.h"

WidgetFinanceiroContas::WidgetFinanceiroContas(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroContas) { ui->setupUi(this); }

WidgetFinanceiroContas::~WidgetFinanceiroContas() { delete ui; }

void WidgetFinanceiroContas::setupTables() {
  // TODO: refactor @running_total into running_total :- ifnull(running_total, 0) + ...
  modelVencidos.setQuery("SELECT v.*, @running_total := @running_total + v.Total AS Acumulado FROM " + QString(tipo == Tipo::Receber ? "view_a_receber_vencidos_base" : "view_a_pagar_vencidos_base") +
                         " v JOIN (SELECT @running_total := 0) r");

  if (modelVencidos.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencidos: " + modelVencidos.lastError().text()); }

  ui->tableVencidos->setModel(&modelVencidos);
  ui->tableVencidos->setItemDelegate(new ReaisDelegate(this));

  // -------------------------------------------------------------------------

  modelVencer.setQuery("SELECT v.*, @running_total := @running_total + v.Total AS Acumulado FROM " + QString(tipo == Tipo::Receber ? "view_a_receber_vencer_base" : "view_a_pagar_vencer_base") +
                       " v JOIN (SELECT @running_total := 0) r");

  if (modelVencer.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencer: " + modelVencer.lastError().text()); }

  ui->tableVencer->setModel(&modelVencer);
  ui->tableVencer->setItemDelegate(new ReaisDelegate(this));
}

void WidgetFinanceiroContas::setConnections() {
  connect(ui->dateEditAte, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->dateEditDe, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::on_dateEditDe_dateChanged);
  connect(ui->doubleSpinBoxAte, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->doubleSpinBoxDe, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetFinanceiroContas::on_doubleSpinBoxDe_valueChanged);
  connect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetFinanceiroContas::on_groupBoxData_toggled);
  connect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->itemBoxLojas, &ItemBox::textChanged, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->pushButtonAdiantarRecebimento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonAdiantarRecebimento_clicked);
  connect(ui->pushButtonExcluirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonExcluirLancamento_clicked);
  connect(ui->pushButtonInserirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked);
  connect(ui->pushButtonInserirTransferencia, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirTransferencia_clicked);
  connect(ui->pushButtonReverterPagamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked);
  connect(ui->radioButtonCancelado, &QRadioButton::toggled, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->radioButtonPendente, &QRadioButton::toggled, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->radioButtonRecebido, &QRadioButton::toggled, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetFinanceiroContas::montaFiltro);
  connect(ui->table, &TableView::activated, this, &WidgetFinanceiroContas::on_table_activated);
  connect(ui->table, &TableView::entered, this, &WidgetFinanceiroContas::on_table_entered);
  connect(ui->tableVencer, &TableView::doubleClicked, this, &WidgetFinanceiroContas::on_tableVencer_doubleClicked);
  connect(ui->tableVencer, &TableView::entered, this, &WidgetFinanceiroContas::on_tableVencer_entered);
  connect(ui->tableVencidos, &TableView::doubleClicked, this, &WidgetFinanceiroContas::on_tableVencidos_doubleClicked);
  connect(ui->tableVencidos, &TableView::entered, this, &WidgetFinanceiroContas::on_tableVencidos_entered);
}

void WidgetFinanceiroContas::updateTables() {
  if (not isSet) {
    ui->radioButtonPendente->setChecked(true);
    ui->dateEditAte->setDate(QDate::currentDate());
    ui->dateEditDe->setDate(QDate::currentDate());

    ui->itemBoxLojas->setSearchDialog(SearchDialog::loja(this));

    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    montaFiltro();
    setupTables();
    modelIsSet = true;
  }

  model.setQuery(model.query().executedQuery());

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela resumo: " + model.lastError().text()); }

  ui->table->resizeColumnsToContents();

  // -------------------------------------------------------------------------

  modelVencidos.setQuery(modelVencidos.query().executedQuery());

  if (modelVencidos.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencidos: " + modelVencidos.lastError().text()); }

  ui->tableVencidos->resizeColumnsToContents();

  // -------------------------------------------------------------------------

  modelVencer.setQuery(modelVencer.query().executedQuery());

  if (modelVencer.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencer: " + modelVencer.lastError().text()); }

  ui->tableVencer->resizeColumnsToContents();
}

void WidgetFinanceiroContas::resetTables() { modelIsSet = false; }

void WidgetFinanceiroContas::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetFinanceiroContas::on_table_activated(const QModelIndex &index) {
  auto *contas = new Contas(tipo == Tipo::Receber ? Contas::Tipo::Receber : Contas::Tipo::Pagar, this);
  contas->setAttribute(Qt::WA_DeleteOnClose);
  const QString idPagamento = model.data(index.row(), "idPagamento").toString();
  const QString contraparte = model.data(index.row(), "Contraparte").toString();
  contas->viewConta(idPagamento, contraparte);
  // TODO: 2poder selecionar mais de um idPagamento (contraParte é estético)
  // ajustar para selecionar mais de uma linha e ajustar no filtro da Contas
}

void WidgetFinanceiroContas::montaFiltro() {
  if (tipo == Tipo::Pagar) {
    QString status;

    Q_FOREACH (const auto &child, ui->groupBoxFiltros->findChildren<QRadioButton *>()) {
      if (child->text() == "Todos") { break; }

      if (child->isChecked() and child->text() == "Pendente/Conferido") {
        status = "(cp.status = 'PENDENTE' OR cp.status = 'CONFERIDO')";
        break;
      }

      if (child->isChecked()) { status = child->text(); }
    }

    if (not status.isEmpty() and status != "(cp.status = 'PENDENTE' OR cp.status = 'CONFERIDO')") status = "cp.status = '" + status + "'";

    const QString valor =
        not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value())
            ? QString(status.isEmpty() ? "" : " AND ") + "cp.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
            : "";

    const QString dataPag = ui->groupBoxData->isChecked() ? QString(status.isEmpty() and valor.isEmpty() ? "" : " AND ") + "cp.dataPagamento BETWEEN '" +
                                                                ui->dateEditDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'"
                                                          : "";

    const QString loja = ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()
                             ? QString(status.isEmpty() and valor.isEmpty() and dataPag.isEmpty() ? "" : " AND ") + "cp.idLoja = " + ui->itemBoxLojas->getValue().toString()
                             : "";

    const QString text = ui->lineEditBusca->text();
    const QString busca = "(ordemCompra LIKE '%" + text + "%' OR contraparte LIKE '%" + text + "%' OR numeroNFe LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%')";

    model.setQuery(
        "SELECT * FROM (SELECT `cp`.`idPagamento` AS `idPagamento`, `cp`.`idLoja` AS `idLoja`, `cp`.`contraParte` AS `contraparte`, `cp`.`dataPagamento` AS `dataPagamento`, "
        "`cp`.`dataEmissao` AS `dataEmissao`, `cp`.`valor` AS `valor`, `cp`.`status` AS `status`, group_concat(DISTINCT `pf`.`ordemCompra` SEPARATOR ',') AS `ordemCompra`, "
        "group_concat(DISTINCT `pf`.`idVenda` SEPARATOR ', ') AS `idVenda`, group_concat(DISTINCT `n`.`numeroNFe` SEPARATOR ', ') AS `numeroNFe`, `cp`.`tipo` AS `tipo`, `cp`.`parcela` AS "
        "`parcela`, `cp`.`observacao` AS `observacao`, group_concat(DISTINCT `pf`.`statusFinanceiro` SEPARATOR ',') AS `statusFinanceiro` FROM ((((`mydb`.`conta_a_pagar_has_pagamento` `cp` "
        "LEFT JOIN `mydb`.`pedido_fornecedor_has_produto` `pf` ON ((`cp`.`idCompra` = `pf`.`idCompra`))) LEFT JOIN `mydb`.`estoque_has_compra` `ehc` ON ((`ehc`.`idCompra` = "
        "`cp`.`idCompra`))) LEFT JOIN `mydb`.`estoque_has_nfe` `ehn` ON ((`ehc`.`idEstoque` = `ehn`.`idEstoque`))) LEFT JOIN `mydb`.`nfe` `n` ON ((`n`.`idNFe` = `ehn`.`idNFe`))) WHERE " +
        status + valor + dataPag + loja + QString(status.isEmpty() and valor.isEmpty() and dataPag.isEmpty() and loja.isEmpty() ? "" : " AND ") +
        "`cp`.`desativado` = FALSE GROUP BY `cp`.`idPagamento` ORDER BY `cp`.`dataPagamento`) view WHERE " + busca + " ORDER BY `dataPagamento` , `idVenda` , `tipo` , `parcela` DESC");
  }

  if (tipo == Tipo::Receber) {
    QString status;

    Q_FOREACH (const auto &child, ui->groupBoxFiltros->findChildren<QRadioButton *>()) {
      if (child->text() == "Todos") { break; }

      if (child->isChecked() and child->text() == "Pendente/Conferido") {
        status = "(cr.status = 'PENDENTE' OR cr.status = 'CONFERIDO')";
        break;
      }

      if (child->isChecked()) { status = child->text(); }
    }

    if (not status.isEmpty() and status != "(cr.status = 'PENDENTE' OR cr.status = 'CONFERIDO')") { status = "cr.status = '" + status + "'"; }

    const QString valor =
        not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value())
            ? QString(status.isEmpty() ? "" : " AND ") + "cr.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
            : "";

    const QString dataPag = ui->groupBoxData->isChecked() ? QString(status.isEmpty() and valor.isEmpty() ? "" : " AND ") + "cr.dataPagamento BETWEEN '" +
                                                                ui->dateEditDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'"
                                                          : "";

    const QString loja = ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()
                             ? QString(status.isEmpty() and valor.isEmpty() and dataPag.isEmpty() ? "" : " AND ") + "cr.idLoja = " + ui->itemBoxLojas->getValue().toString()
                             : "";

    const QString text = ui->lineEditBusca->text();
    const QString busca =
        QString(status.isEmpty() and valor.isEmpty() and dataPag.isEmpty() and loja.isEmpty() ? "" : " AND ") + "(cr.idVenda LIKE '%" + text + "%' OR cr.contraparte LIKE '%" + text + "%')";

    model.setQuery("SELECT `cr`.`idPagamento` AS `idPagamento`, `cr`.`idLoja` AS `idLoja`, `cr`.`representacao` AS `representacao`, `cr`.`contraParte` AS `contraparte`, `cr`.`dataPagamento` AS "
                   "`dataPagamento`, `cr`.`dataEmissao` AS `dataEmissao`, `cr`.`idVenda` AS `idVenda`, `cr`.`valor` AS `valor`, `cr`.`tipo` AS `tipo`, `cr`.`parcela` AS `parcela`, `cr`.`observacao` "
                   "AS `observacao`, `cr`.`status` AS `status`, `v`.`statusFinanceiro` AS `statusFinanceiro` FROM (`mydb`.`conta_a_receber_has_pagamento` `cr` LEFT JOIN `mydb`.`venda` `v` ON "
                   "((`cr`.`idVenda` = `v`.`idVenda`))) WHERE " +
                   status + valor + dataPag + loja + busca +
                   " AND `cr`.`desativado` = FALSE AND `cr`.`representacao` = FALSE GROUP BY `cr`.`idPagamento` ORDER BY `cr`.`dataPagamento`, `cr`.`idVenda`, "
                   "`cr`.`tipo`, `cr`.`parcela` DESC");
  }

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela: " + model.lastError().text()); }

  model.setHeaderData("dataEmissao", "Data Emissão");
  model.setHeaderData("idVenda", "Código");
  if (tipo == Tipo::Pagar) { model.setHeaderData("ordemCompra", "OC"); }
  model.setHeaderData("numeroNFe", "NFe");
  model.setHeaderData("contraParte", "ContraParte");
  model.setHeaderData("valor", "R$");
  model.setHeaderData("tipo", "Tipo");
  model.setHeaderData("parcela", "Parcela");
  model.setHeaderData("dataPagamento", "Data Pag.");
  model.setHeaderData("observacao", "Obs.");
  model.setHeaderData("status", "Status");
  model.setHeaderData("statusFinanceiro", "Status Financeiro");

  ui->table->setModel(&model);
  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this, 2));

  ui->table->resizeColumnsToContents();
}

void WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked() {
  auto *lancamento = new InserirLancamento(tipo == Tipo::Receber ? InserirLancamento::Tipo::Receber : InserirLancamento::Tipo::Pagar, this);
  lancamento->setAttribute(Qt::WA_DeleteOnClose);
  lancamento->show();
}

void WidgetFinanceiroContas::on_pushButtonAdiantarRecebimento_clicked() {
  auto *adiantar = new AnteciparRecebimento(this);
  adiantar->setAttribute(Qt::WA_DeleteOnClose);
  adiantar->show();
}

void WidgetFinanceiroContas::on_doubleSpinBoxDe_valueChanged(const double value) { ui->doubleSpinBoxAte->setValue(value); }

void WidgetFinanceiroContas::on_dateEditDe_dateChanged(const QDate &date) { ui->dateEditAte->setDate(date); }

void WidgetFinanceiroContas::setTipo(const Tipo &value) {
  tipo = value;

  if (tipo == Tipo::Pagar) {
    ui->pushButtonAdiantarRecebimento->hide();
    ui->radioButtonRecebido->hide();
  }

  if (tipo == Tipo::Receber) { ui->radioButtonPago->hide(); }
}

void WidgetFinanceiroContas::on_groupBoxData_toggled(const bool enabled) {
  Q_FOREACH (const auto &child, ui->groupBoxData->findChildren<QDateEdit *>()) { child->setEnabled(enabled); }
}

void WidgetFinanceiroContas::on_tableVencidos_entered(const QModelIndex &) { ui->tableVencidos->resizeColumnsToContents(); }

void WidgetFinanceiroContas::on_tableVencer_entered(const QModelIndex &) { ui->tableVencer->resizeColumnsToContents(); }

void WidgetFinanceiroContas::on_tableVencidos_doubleClicked(const QModelIndex &index) {
  ui->dateEditDe->setDate(modelVencidos.record(index.row()).value("Data Pagamento").toDate());
  ui->dateEditAte->setDate(modelVencidos.record(index.row()).value("Data Pagamento").toDate());

  ui->groupBoxData->setChecked(true);

  ui->tableVencer->clearSelection();
}

void WidgetFinanceiroContas::on_tableVencer_doubleClicked(const QModelIndex &index) {
  ui->dateEditDe->setDate(modelVencer.record(index.row()).value("Data Pagamento").toDate());
  ui->dateEditAte->setDate(modelVencer.record(index.row()).value("Data Pagamento").toDate());

  ui->groupBoxData->setChecked(true);

  ui->tableVencidos->clearSelection();
}

void WidgetFinanceiroContas::on_pushButtonInserirTransferencia_clicked() {
  auto *transferencia = new InserirTransferencia(this);
  transferencia->setAttribute(Qt::WA_DeleteOnClose);

  transferencia->show();
}

void WidgetFinanceiroContas::on_pushButtonExcluirLancamento_clicked() {
  // TASK: se o grupo for 'Transferencia' procurar a outra metade e cancelar tambem
  // usar 'grupo', 'data', 'valor'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja excluir?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Excluir");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    QSqlQuery query;
    query.prepare("UPDATE " + QString(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET desativado = TRUE WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

    if (not query.exec()) { return qApp->enqueueError("Erro excluindo lançamento: " + query.lastError().text()); }

    montaFiltro();

    qApp->enqueueInformation("Lançamento excluído com sucesso!");
  }
}

// TODO: [Verificar com Midi] contareceber.status e venda.statusFinanceiro deveriam ser o mesmo creio eu porem em diversas linhas eles tem valores diferentes

void WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

  QSqlQuery queryPagamento;
  queryPagamento.prepare("SELECT dataPagamento, grupo FROM " + QString(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " WHERE idPagamento = :idPagamento");
  queryPagamento.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

  if (not queryPagamento.exec() or not queryPagamento.first()) { return qApp->enqueueError("Erro buscando pagamento: " + queryPagamento.lastError().text()); }

  if (queryPagamento.value("dataPagamento").toDate().daysTo(QDate::currentDate()) > 5) { return qApp->enqueueError("No máximo 5 dias para reverter!"); }

  if (queryPagamento.value("grupo").toString() == "Transferência") { return qApp->enqueueError("Não pode reverter transferência!"); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja reverter?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Reverter");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    QSqlQuery query;
    query.prepare("UPDATE " + QString(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET status = 'PENDENTE' WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

    if (not query.exec()) { return qApp->enqueueError("Erro revertendo lançamento: " + query.lastError().text()); }

    updateTables();

    qApp->enqueueInformation("Lançamento revertido com sucesso!");
  }
}
