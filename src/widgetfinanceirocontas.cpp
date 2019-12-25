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
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  // TODO: refactor @running_total into running_total :- ifnull(running_total, 0) + ...
  modelVencidos.setQuery("SELECT v.*, @running_total := @running_total + v.Total AS Acumulado FROM " + QString(tipo == Tipo::Receber ? "view_a_receber_vencidos_base" : "view_a_pagar_vencidos_base") +
                         " v JOIN (SELECT @running_total := 0) r");

  if (modelVencidos.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencidos: " + modelVencidos.lastError().text(), this); }

  ui->tableVencidos->setModel(&modelVencidos);

  ui->tableVencidos->setItemDelegate(new ReaisDelegate(this));

  // -------------------------------------------------------------------------

  modelVencer.setQuery("SELECT v.*, @running_total := @running_total + v.Total AS Acumulado FROM " + QString(tipo == Tipo::Receber ? "view_a_receber_vencer_base" : "view_a_pagar_vencer_base") +
                       " v JOIN (SELECT @running_total := 0) r");

  if (modelVencer.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencer: " + modelVencer.lastError().text(), this); }

  ui->tableVencer->setModel(&modelVencer);

  ui->tableVencer->setItemDelegate(new ReaisDelegate(this));
}

void WidgetFinanceiroContas::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditAte, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->dateEditDe, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::on_dateEditDe_dateChanged, connectionType);
  connect(ui->doubleSpinBoxAte, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->doubleSpinBoxDe, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetFinanceiroContas::on_doubleSpinBoxDe_valueChanged, connectionType);
  connect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetFinanceiroContas::on_groupBoxData_toggled, connectionType);
  connect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->itemBoxLojas, &ItemBox::textChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->pushButtonAdiantarRecebimento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonAdiantarRecebimento_clicked, connectionType);
  connect(ui->pushButtonExcluirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonExcluirLancamento_clicked, connectionType);
  connect(ui->pushButtonInserirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked, connectionType);
  connect(ui->pushButtonInserirTransferencia, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirTransferencia_clicked, connectionType);
  connect(ui->pushButtonReverterPagamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked, connectionType);
  connect(ui->radioButtonCancelado, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonPago, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonPendente, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonRecebido, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetFinanceiroContas::on_table_activated, connectionType);
  connect(ui->tableVencer, &TableView::doubleClicked, this, &WidgetFinanceiroContas::on_tableVencer_doubleClicked, connectionType);
  connect(ui->tableVencidos, &TableView::doubleClicked, this, &WidgetFinanceiroContas::on_tableVencidos_doubleClicked, connectionType);
}

void WidgetFinanceiroContas::updateTables() {
  if (not isSet) {
    ui->radioButtonPendente->setChecked(true);
    ui->dateEditAte->setDate(qApp->serverDate());
    ui->dateEditDe->setDate(qApp->serverDate());

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

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela resumo: " + model.lastError().text(), this); }

  // -------------------------------------------------------------------------

  modelVencidos.setQuery(modelVencidos.query().executedQuery());

  if (modelVencidos.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencidos: " + modelVencidos.lastError().text(), this); }

  // -------------------------------------------------------------------------

  modelVencer.setQuery(modelVencer.query().executedQuery());

  if (modelVencer.lastError().isValid()) { return qApp->enqueueError("Erro atualizando tabela vencer: " + modelVencer.lastError().text(), this); }
}

void WidgetFinanceiroContas::resetTables() { modelIsSet = false; }

void WidgetFinanceiroContas::on_table_activated(const QModelIndex &index) {
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  auto *contas = new Contas(tipo == Tipo::Receber ? Contas::Tipo::Receber : Contas::Tipo::Pagar, this);
  contas->setAttribute(Qt::WA_DeleteOnClose);
  const QString idPagamento = model.data(index.row(), "idPagamento").toString();
  const QString contraparte = model.data(index.row(), "Contraparte").toString();
  contas->viewConta(idPagamento, contraparte);
  // TODO: 2poder selecionar mais de um idPagamento (contraParte é estético)
  // ajustar para selecionar mais de uma linha e ajustar no filtro da Contas
}

void WidgetFinanceiroContas::montaFiltro() {
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  if (tipo == Tipo::Pagar) {
    QStringList filtros;
    QString status;

    const auto children = ui->groupBoxFiltros->findChildren<QRadioButton *>();

    for (const auto &child : children) {
      if (child->text() == "Todos") { break; }

      if (child->isChecked()) {
        status = child->text();
        break;
      }
    }

    if (ui->radioButtonPendente->isChecked()) {
      filtros << "cp.status IN ('PENDENTE', 'CONFERIDO')";
    } else {
      if (not status.isEmpty()) { filtros << "cp.status = '" + status + "'"; }
    }

    //-------------------------------------

    const QString valor = not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value())
                              ? "cp.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
                              : "";
    if (not valor.isEmpty()) { filtros << valor; }

    //-------------------------------------

    const QString dataPag =
        ui->groupBoxData->isChecked() ? "cp.dataPagamento BETWEEN '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'" : "";
    if (not dataPag.isEmpty()) { filtros << dataPag; }

    //-------------------------------------

    const QString loja = ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty() ? "cp.idLoja = " + ui->itemBoxLojas->getId().toString() : "";
    if (not loja.isEmpty()) { filtros << loja; }

    //-------------------------------------

    const QString text = ui->lineEditBusca->text();
    const QString busca = text.isEmpty() ? "" : " WHERE (ordemCompra LIKE '%" + text + "%' OR contraparte LIKE '%" + text + "%' OR numeroNFe LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%')";

    //-------------------------------------

    filtros << "cp.desativado = FALSE";

    model.setQuery(
        "SELECT * FROM (SELECT `cp`.`idPagamento` AS `idPagamento`, `cp`.`idLoja` AS `idLoja`, `cp`.`contraParte` AS `contraparte`, `cp`.`dataPagamento` AS `dataPagamento`, "
        "`cp`.`dataEmissao` AS `dataEmissao`, `cp`.`valor` AS `valor`, `cp`.`status` AS `status`, GROUP_CONCAT(DISTINCT `pf`.`ordemCompra` SEPARATOR ',') AS `ordemCompra`, "
        "GROUP_CONCAT(DISTINCT `pf`.`idVenda` SEPARATOR ', ') AS `idVenda`, GROUP_CONCAT(DISTINCT `n`.`numeroNFe` SEPARATOR ', ') AS `numeroNFe`, `cp`.`tipo` AS `tipo`, `cp`.`parcela` AS "
        "`parcela`, `cp`.`observacao` AS `observacao`, GROUP_CONCAT(DISTINCT `pf`.`statusFinanceiro` SEPARATOR ',') AS `statusFinanceiro` FROM `conta_a_pagar_has_pagamento` `cp` "
        "LEFT JOIN `pedido_fornecedor_has_produto2` `pf` ON `cp`.`idCompra` = `pf`.`idCompra` LEFT JOIN `estoque_has_compra` `ehc` ON `ehc`.`idPedido2` = `pf`.`idPedido2` LEFT JOIN `estoque` "
        "`e` ON `ehc`.`idEstoque` = `e`.`idEstoque` LEFT JOIN `nfe` `n` ON `n`.`idNFe` = `e`.`idNFe` WHERE " +
        filtros.join(" AND ") + " GROUP BY `cp`.`idPagamento`) x" + busca);
  }

  if (tipo == Tipo::Receber) {
    QStringList filtros;
    QString status;

    const auto children = ui->groupBoxFiltros->findChildren<QRadioButton *>();

    for (const auto &child : children) {
      if (child->text() == "Todos") { break; }

      if (child->isChecked()) {
        status = child->text();
        break;
      }
    }

    if (ui->radioButtonPendente->isChecked()) {
      filtros << "cr.status IN ('PENDENTE', 'CONFERIDO')";
    } else {
      if (not status.isEmpty()) { filtros << "cr.status = '" + status + "'"; }
    }

    //-------------------------------------

    const QString valor = not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value())
                              ? "cr.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
                              : "";
    if (not valor.isEmpty()) { filtros << valor; }

    //-------------------------------------

    const QString dataPag =
        ui->groupBoxData->isChecked() ? "cr.dataPagamento BETWEEN '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'" : "";
    if (not dataPag.isEmpty()) { filtros << dataPag; }

    //-------------------------------------

    const QString loja = ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty() ? "cr.idLoja = " + ui->itemBoxLojas->getId().toString() : "";
    if (not loja.isEmpty()) { filtros << loja; }

    //-------------------------------------

    const QString text = ui->lineEditBusca->text();
    const QString busca = "(cr.idVenda LIKE '%" + text + "%' OR cr.contraparte LIKE '%" + text + "%')";
    if (not text.isEmpty()) { filtros << busca; }

    //-------------------------------------

    filtros << "cr.desativado = FALSE";
    filtros << "cr.representacao = FALSE";

    model.setQuery("SELECT `cr`.`idPagamento` AS `idPagamento`, `cr`.`idLoja` AS `idLoja`, `cr`.`representacao` AS `representacao`, `cr`.`contraParte` AS `contraparte`, `cr`.`dataPagamento` AS "
                   "`dataPagamento`, `cr`.`dataEmissao` AS `dataEmissao`, `cr`.`idVenda` AS `idVenda`, `cr`.`valor` AS `valor`, `cr`.`tipo` AS `tipo`, `cr`.`parcela` AS `parcela`, `cr`.`observacao` "
                   "AS `observacao`, `cr`.`status` AS `status`, `v`.`statusFinanceiro` AS `statusFinanceiro` FROM (`conta_a_receber_has_pagamento` `cr` LEFT JOIN `venda` `v` ON "
                   "((`cr`.`idVenda` = `v`.`idVenda`))) WHERE " +
                   filtros.join(" AND ") + " GROUP BY `cr`.`idPagamento` ORDER BY `cr`.`dataPagamento`, `cr`.`idVenda`, `cr`.`tipo`, `cr`.`parcela` DESC");
  }

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela: " + model.lastError().text(), this); }

  model.setHeaderData("dataEmissao", "Data Emissão");
  model.setHeaderData("idVenda", "Código");

  if (tipo == Tipo::Pagar) {
    model.setHeaderData("ordemCompra", "OC");
    model.setHeaderData("numeroNFe", "NFe");
  }

  model.setHeaderData("contraParte", "ContraParte");
  model.setHeaderData("valor", "R$");
  model.setHeaderData("tipo", "Tipo");
  model.setHeaderData("parcela", "Parcela");
  model.setHeaderData("dataPagamento", "Data Pag.");
  model.setHeaderData("observacao", "Obs.");
  model.setHeaderData("status", "Status");
  model.setHeaderData("statusFinanceiro", "Status Financeiro");

  model.proxyModel = new SortFilterProxyModel(&model, this);

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this, 2));

  if (tipo == Tipo::Receber) { ui->table->hideColumn("representacao"); }
  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
}

void WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked() {
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

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

void WidgetFinanceiroContas::setTipo(const Tipo &novoTipo) {
  if (novoTipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  tipo = novoTipo;

  if (tipo == Tipo::Pagar) {
    ui->pushButtonAdiantarRecebimento->hide();
    ui->radioButtonRecebido->hide();
    ui->lineEditBusca->setPlaceholderText("OC/Contraparte/NFe/Venda");
  }

  if (tipo == Tipo::Receber) {
    ui->radioButtonPago->hide();
    ui->lineEditBusca->setPlaceholderText("Venda/Contraparte");
  }
}

void WidgetFinanceiroContas::on_groupBoxData_toggled(const bool enabled) {
  const auto children = ui->groupBoxData->findChildren<QDateEdit *>();

  for (const auto &child : children) { child->setEnabled(enabled); }
}

void WidgetFinanceiroContas::on_tableVencidos_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  ui->dateEditDe->setDate(modelVencidos.record(index.row()).value("Data Pagamento").toDate());
  ui->dateEditAte->setDate(modelVencidos.record(index.row()).value("Data Pagamento").toDate());

  ui->groupBoxData->setChecked(true);

  ui->tableVencer->clearSelection();
}

void WidgetFinanceiroContas::on_tableVencer_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

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
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  // TASK: se o grupo for 'Transferencia' procurar a outra metade e cancelar tambem
  // usar 'grupo', 'data', 'valor'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja excluir?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Excluir");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    QSqlQuery query;
    query.prepare("UPDATE " + QString(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET desativado = TRUE WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

    if (not query.exec()) { return qApp->enqueueError("Erro excluindo lançamento: " + query.lastError().text(), this); }

    montaFiltro();

    qApp->enqueueInformation("Lançamento excluído com sucesso!", this);
  }
}

// TODO: [Verificar com Midi] contareceber.status e venda.statusFinanceiro deveriam ser o mesmo creio eu porem em diversas linhas eles tem valores diferentes

void WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked() {
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  // TODO: bloquear se o pagamento já estiver PENDENTE

  // TODO: verificar se precisa limpar os campos que foram preenchidos

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  QSqlQuery queryPagamento;
  queryPagamento.prepare("SELECT dataPagamento, grupo FROM " + QString(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " WHERE idPagamento = :idPagamento");
  queryPagamento.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

  if (not queryPagamento.exec() or not queryPagamento.first()) { return qApp->enqueueError("Erro buscando pagamento: " + queryPagamento.lastError().text(), this); }

  if (queryPagamento.value("dataPagamento").toDate().daysTo(qApp->serverDate()) > 5) { return qApp->enqueueError("No máximo 5 dias para reverter!", this); }

  if (queryPagamento.value("grupo").toString() == "Transferência") { return qApp->enqueueError("Não pode reverter transferência!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja reverter?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Reverter");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    QSqlQuery query;
    query.prepare("UPDATE " + QString(tipo == Tipo::Pagar ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET status = 'PENDENTE' WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

    if (not query.exec()) { return qApp->enqueueError("Erro revertendo lançamento: " + query.lastError().text(), this); }

    updateTables();

    qApp->enqueueInformation("Lançamento revertido com sucesso!", this);
  }
}
