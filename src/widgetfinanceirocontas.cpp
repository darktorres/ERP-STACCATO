#include "widgetfinanceirocontas.h"
#include "ui_widgetfinanceirocontas.h"

#include "anteciparrecebimento.h"
#include "application.h"
#include "contas.h"
#include "doubledelegate.h"
#include "inserirlancamento.h"
#include "inserirtransferencia.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

WidgetFinanceiroContas::WidgetFinanceiroContas(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroContas) { ui->setupUi(this); }

WidgetFinanceiroContas::~WidgetFinanceiroContas() { delete ui; }

void WidgetFinanceiroContas::setupTables() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  if (tipo == Tipo::Receber) { modelVencidos.setQuery(Sql::view_a_receber_vencidos()); }
  if (tipo == Tipo::Pagar) { modelVencidos.setQuery(Sql::view_a_pagar_vencidos()); }

  if (modelVencidos.lastError().isValid()) { throw RuntimeException("Erro atualizando tabela vencidos: " + modelVencidos.lastError().text(), this); }

  ui->tableVencidos->setModel(&modelVencidos);

  ui->tableVencidos->setItemDelegate(new ReaisDelegate(this));

  // -------------------------------------------------------------------------

  if (tipo == Tipo::Receber) { modelVencer.setQuery(Sql::view_a_receber_vencer()); }
  if (tipo == Tipo::Pagar) { modelVencer.setQuery(Sql::view_a_pagar_vencer()); }

  if (modelVencer.lastError().isValid()) { throw RuntimeException("Erro atualizando tabela vencer: " + modelVencer.lastError().text(), this); }

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
  connect(ui->pushButtonImportarFolhaPag, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonImportarFolhaPag_clicked, connectionType);
  connect(ui->pushButtonInserirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked, connectionType);
  connect(ui->pushButtonInserirTransferencia, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirTransferencia_clicked, connectionType);
  connect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonRemessaItau_clicked, connectionType);
  connect(ui->pushButtonReverterPagamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked, connectionType);
  connect(ui->radioButtonAgendado, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
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

  if (model.lastError().isValid()) { throw RuntimeException("Erro atualizando tabela resumo: " + model.lastError().text(), this); }

  // -------------------------------------------------------------------------

  modelVencidos.setQuery(modelVencidos.query().executedQuery());

  if (modelVencidos.lastError().isValid()) { throw RuntimeException("Erro atualizando tabela vencidos: " + modelVencidos.lastError().text(), this); }

  // -------------------------------------------------------------------------

  modelVencer.setQuery(modelVencer.query().executedQuery());

  if (modelVencer.lastError().isValid()) { throw RuntimeException("Erro atualizando tabela vencer: " + modelVencer.lastError().text(), this); }
}

void WidgetFinanceiroContas::resetTables() { modelIsSet = false; }

void WidgetFinanceiroContas::on_table_activated(const QModelIndex &index) {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  if (tipo == Tipo::Receber) {
    auto *contas = new Contas(Contas::Tipo::Receber, this);
    contas->setAttribute(Qt::WA_DeleteOnClose);
    contas->viewContaReceber(model.data(index.row(), "idPagamento").toString(), model.data(index.row(), "ContraParte").toString());
  }

  if (tipo == Tipo::Pagar) {
    auto *contas = new Contas(Contas::Tipo::Pagar, this);
    contas->setAttribute(Qt::WA_DeleteOnClose);
    contas->viewContaPagar(model.data(index.row(), "dataPagamento").toString());
  }
}

void WidgetFinanceiroContas::montaFiltro() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  if (tipo == Tipo::Pagar) {
    QStringList filtros;
    QString status;

    const auto children = ui->groupBoxFiltros->findChildren<QRadioButton *>();

    for (const auto &child : children) {
      if (child->isChecked()) {
        if (child->text() == "Todos") { break; }

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

    const QString valor = (not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value()))
                              ? "cp.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
                              : "";
    if (not valor.isEmpty()) { filtros << valor; }

    //-------------------------------------

    const QString dataPag =
        ui->groupBoxData->isChecked() ? "cp.dataPagamento BETWEEN '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'" : "";
    if (not dataPag.isEmpty()) { filtros << dataPag; }

    //-------------------------------------

    const QString loja = (ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()) ? "cp.idLoja = " + ui->itemBoxLojas->getId().toString() : "";
    if (not loja.isEmpty()) { filtros << loja; }

    //-------------------------------------

    const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
    const QString busca = text.isEmpty() ? ""
                                         : " WHERE (ordemCompra LIKE '%" + text + "%' OR contraparte LIKE '%" + text + "%' OR numeroNFe LIKE '%" + text + "%' OR cp_idVenda LIKE '%" + text +
                                               "%' OR pf2_idVenda LIKE '%" + text + "%' OR observacao LIKE '%" + text + "%' OR codFornecedor LIKE '%" + text + "%')";

    //-------------------------------------

    filtros << "cp.desativado = FALSE";

    model.setQuery(
        "SELECT * FROM (SELECT `cp`.`idPagamento` AS `idPagamento`, `cp`.`idLoja` AS `idLoja`, cp.idVenda AS `cp_idVenda`, `cp`.`contraParte` AS `contraparte`, `cp`.`dataPagamento` AS "
        "`dataPagamento`, `cp`.`dataEmissao` AS `dataEmissao`, `cp`.`valor` AS `valor`, `cp`.`status` AS `status`, GROUP_CONCAT(DISTINCT `pf2`.`ordemCompra` SEPARATOR ',') AS `ordemCompra`, "
        "GROUP_CONCAT(DISTINCT `n`.`numeroNFe` SEPARATOR ', ') AS `numeroNFe`, `cp`.`tipo` AS `tipo`, `cp`.`parcela` AS `parcela`, `cp`.`observacao` AS `observacao`, GROUP_CONCAT(DISTINCT "
        "`pf2`.`statusFinanceiro` SEPARATOR ',') AS `statusFinanceiro`, GROUP_CONCAT(DISTINCT `pf2`.`idVenda` SEPARATOR ', ') AS `pf2_idVenda`, GROUP_CONCAT(DISTINCT pf2.codFornecedor SEPARATOR ', "
        "') AS codFornecedor FROM `conta_a_pagar_has_pagamento` `cp` LEFT JOIN `pedido_fornecedor_has_produto2` `pf2` ON `cp`.`idCompra` = `pf2`.`idCompra` LEFT JOIN `estoque_has_compra` `ehc` ON "
        "`ehc`.`idPedido2` = `pf2`.`idPedido2` LEFT JOIN `estoque` `e` ON `ehc`.`idEstoque` = `e`.`idEstoque` LEFT JOIN `nfe` `n` ON `n`.`idNFe` = `e`.`idNFe` WHERE " +
        filtros.join(" AND ") + " GROUP BY `cp`.`idPagamento`) x" + busca);
  }

  if (tipo == Tipo::Receber) {
    QStringList filtros;
    QString status;

    const auto children = ui->groupBoxFiltros->findChildren<QRadioButton *>();

    for (const auto &child : children) {
      if (child->isChecked()) {
        if (child->text() == "Todos") { break; }

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

    const QString valor = (not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value()))
                              ? "cr.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
                              : "";
    if (not valor.isEmpty()) { filtros << valor; }

    //-------------------------------------

    const QString dataPag =
        ui->groupBoxData->isChecked() ? "cr.dataPagamento BETWEEN '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'" : "";
    if (not dataPag.isEmpty()) { filtros << dataPag; }

    //-------------------------------------

    const QString loja = (ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()) ? "cr.idLoja = " + ui->itemBoxLojas->getId().toString() : "";
    if (not loja.isEmpty()) { filtros << loja; }

    //-------------------------------------

    const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
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

  if (model.lastError().isValid()) { throw RuntimeException("Erro lendo tabela: " + model.lastError().text(), this); }

  model.setHeaderData("dataEmissao", "Data Emissão");

  if (tipo == Tipo::Receber) { model.setHeaderData("idVenda", "Venda"); }

  if (tipo == Tipo::Pagar) {
    model.setHeaderData("cp_idVenda", "Venda");
    model.setHeaderData("pf2_idVenda", "Venda");
    model.setHeaderData("ordemCompra", "OC");
    model.setHeaderData("numeroNFe", "NFe");
    model.setHeaderData("codFornecedor", "Cód. Forn.");
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
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  if (tipo == Tipo::Receber) { ui->table->hideColumn("representacao"); }
  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
}

void WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  auto *lancamento = new InserirLancamento((tipo == Tipo::Receber) ? InserirLancamento::Tipo::Receber : InserirLancamento::Tipo::Pagar, this);
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
  if (novoTipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  tipo = novoTipo;

  if (tipo == Tipo::Pagar) {
    ui->pushButtonAdiantarRecebimento->hide();
    ui->radioButtonRecebido->hide();
    ui->lineEditBusca->setPlaceholderText("OC/Contraparte/NFe/Venda/Obs./Cód. Forn.");
  }

  if (tipo == Tipo::Receber) {
    ui->pushButtonImportarFolhaPag->hide();
    ui->radioButtonPago->hide();
    ui->radioButtonAgendado->hide();
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
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  // TODO: se o grupo for 'Transferencia' procurar a outra metade e cancelar tambem
  // usar 'grupo', 'data', 'valor'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja excluir?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Excluir");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    SqlQuery query;
    query.prepare("UPDATE " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET desativado = TRUE WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

    if (not query.exec()) { throw RuntimeException("Erro excluindo lançamento: " + query.lastError().text(), this); }

    montaFiltro();

    qApp->enqueueInformation("Lançamento excluído com sucesso!", this);
  }
}

void WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  // TODO: bloquear se o pagamento já estiver PENDENTE

  // TODO: verificar se precisa limpar os campos que foram preenchidos

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  SqlQuery queryPagamento;
  queryPagamento.prepare("SELECT dataPagamento, grupo FROM " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " WHERE idPagamento = :idPagamento");
  queryPagamento.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

  if (not queryPagamento.exec() or not queryPagamento.first()) { throw RuntimeException("Erro buscando pagamento: " + queryPagamento.lastError().text(), this); }

  if (queryPagamento.value("grupo").toString() == "TRANSFERÊNCIA") { throw RuntimeError("Não pode reverter transferência!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja reverter?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Reverter");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    SqlQuery query;
    query.prepare("UPDATE " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET status = 'PENDENTE' WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", model.data(list.first().row(), "idPagamento"));

    if (not query.exec()) { throw RuntimeException("Erro revertendo lançamento: " + query.lastError().text(), this); }

    updateTables();

    qApp->enqueueInformation("Lançamento revertido com sucesso!", this);
  }
}

void WidgetFinanceiroContas::verificaCabecalho(QXlsx::Document &xlsx) {
  if (xlsx.read(1, 1).toString() != "Data Emissão") { throw RuntimeError("Cabeçalho errado na coluna 1!"); }
  if (xlsx.read(1, 2).toString() != "Centro de Custo") { throw RuntimeError("Cabeçalho errado na coluna 2!"); }
  if (xlsx.read(1, 3).toString() != "Contraparte") { throw RuntimeError("Cabeçalho errado na coluna 3!"); }
  if (xlsx.read(1, 4).toString() != "") { throw RuntimeError("Cabeçalho errado na coluna 4!"); }
  if (xlsx.read(1, 5).toString() != "Tipo") { throw RuntimeError("Cabeçalho errado na coluna 5!"); }
  if (xlsx.read(1, 6).toString() != "Vencimento") { throw RuntimeError("Cabeçalho errado na coluna 6!"); }
  if (xlsx.read(1, 7).toString() != "Banco") { throw RuntimeError("Cabeçalho errado na coluna 7!"); }
  if (xlsx.read(1, 8).toString() != "Obs") { throw RuntimeError("Cabeçalho errado na coluna 8!"); }
  if (xlsx.read(1, 9).toString() != "Grupo") { throw RuntimeError("Cabeçalho errado na coluna 9!"); }
}

void WidgetFinanceiroContas::on_pushButtonImportarFolhaPag_clicked() {
  const QString file = QFileDialog::getOpenFileName(this, "Importar arquivo do Excel", "", "Excel (*.xlsx)");

  if (file.isEmpty()) { return; }

  SqlTableModel modelImportar;
  modelImportar.setTable("conta_a_pagar_has_pagamento");

  QXlsx::Document xlsx(file, this);

  if (not xlsx.selectSheet("Planilha1")) { throw RuntimeException("Não encontrou 'Planilha1' na tabela!", this); }

  verificaCabecalho(xlsx);

  const int rows = xlsx.dimension().rowCount();

  qApp->startTransaction("WidgetFinanceiroContas::pushButtonImportarFolhaPag");

  for (int rowExcel = 2; rowExcel <= rows; ++rowExcel) {
    if (xlsx.read(rowExcel, 1).toString().isEmpty()) { continue; }

    SqlQuery queryLoja;

    if (not queryLoja.exec("SELECT idLoja FROM loja WHERE nomeFantasia = '" + xlsx.read(rowExcel, 2).toString() + "'") or not queryLoja.first()) {
      throw RuntimeException("Erro buscando idLoja: " + queryLoja.lastError().text());
    }

    SqlQuery queryConta;

    if (not queryConta.exec("SELECT idConta FROM loja_has_conta WHERE banco = '" + xlsx.read(rowExcel, 7).toString() + "'") or not queryConta.first()) {
      throw RuntimeException("Erro buscando idConta: " + queryConta.lastError().text());
    }

    const int rowModel = modelImportar.insertRowAtEnd();

    modelImportar.setData(rowModel, "dataEmissao", xlsx.read(rowExcel, 1));
    modelImportar.setData(rowModel, "idLoja", queryLoja.value("idLoja"));
    modelImportar.setData(rowModel, "contraParte", xlsx.read(rowExcel, 3));
    modelImportar.setData(rowModel, "valor", xlsx.read(rowExcel, 4));
    modelImportar.setData(rowModel, "tipo", xlsx.read(rowExcel, 5));
    modelImportar.setData(rowModel, "dataPagamento", xlsx.read(rowExcel, 6));
    modelImportar.setData(rowModel, "observacao", xlsx.read(rowExcel, 8));
    modelImportar.setData(rowModel, "idConta", queryConta.value("idConta"));
    modelImportar.setData(rowModel, "centroCusto", queryLoja.value("idLoja"));
    modelImportar.setData(rowModel, "grupo", xlsx.read(rowExcel, 9));

    if (xlsx.read(rowExcel, 7).toString().toUpper() == "SANTANDER") { // marcar direto como pago
      modelImportar.setData(rowModel, "dataRealizado", xlsx.read(rowExcel, 6));
      modelImportar.setData(rowModel, "status", "PAGO");
      modelImportar.setData(rowModel, "valorReal", xlsx.read(rowExcel, 4));
      modelImportar.setData(rowModel, "tipoReal", xlsx.read(rowExcel, 5));
    }
  }

  modelImportar.submitAll();

  qApp->endTransaction();

  qApp->enqueueInformation("Tabela importada com sucesso!", this);
}

void WidgetFinanceiroContas::on_pushButtonRemessaItau_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  for (const auto index : selection) {
    if (model.data(index.row(), "status").toString() == "PAGO") { throw RuntimeError("Linha selecionada já paga!", this); }
    if (model.data(index.row(), "ContraParte").toString() != "PORTINARI") { throw RuntimeError("Linha selecionada não é da PORTINARI!", this); }
    if (not model.data(index.row(), "tipo").toString().contains("TRANSF. ITAÚ")) { throw RuntimeError("Pagamento selecionado não é transferência ITAÚ!", this); }
    // TODO: se linha já estiver como agendado confirmar com usuario antes de gerar outro arquivo
  }

  CNAB cnab(this);
  QString idCnab = cnab.remessaPagamentoItau240(montarPagamento(selection));

  QStringList ids;

  for (const auto &index : selection) { ids << model.data(index.row(), "idPagamento").toString(); }

  SqlQuery query;

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET status = 'AGENDADO', idCnab = " + idCnab + " WHERE idPagamento IN (" + ids.join(",") + ")")) {
    throw RuntimeException("Erro alterando GARE: " + query.lastError().text(), this);
  }

  updateTables();
}

QVector<CNAB::Pagamento> WidgetFinanceiroContas::montarPagamento(const QModelIndexList &selection) {
  QVector<CNAB::Pagamento> pagamentos;

  for (const auto index : selection) {
    CNAB::Pagamento pagamento;

    pagamento.codBanco = 341;
    pagamento.valor = QString::number(model.data(index.row(), "valor").toDouble(), 'f', 2).remove('.').toULong();
    pagamento.data = QDate::currentDate().toString("ddMMyyyy");
    pagamento.cnpjDest = "10633753000198";
    pagamento.agenciaConta = "00628 000000042175 2";
    pagamento.nome = "CECRISA REV CERAMICOS S";
    pagamento.codFornecedor = model.data(index.row(), "codFornecedor").toString() + " " + model.data(index.row(), "pf2_idVenda").toString();

    pagamentos << pagamento;
  }

  return pagamentos;
}

// TODO: [Verificar com Midi] contareceber.status e venda.statusFinanceiro deveriam ser o mesmo porem em diversas linhas eles tem valores diferentes
