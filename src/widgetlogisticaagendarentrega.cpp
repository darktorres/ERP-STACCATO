#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlRecord>

#include "checkboxdelegate.h"
#include "doubledelegate.h"
#include "financeiroproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "ui_widgetlogisticaagendarentrega.h"
#include "usersession.h"
#include "widgetlogisticaagendarentrega.h"

WidgetLogisticaAgendarEntrega::WidgetLogisticaAgendarEntrega(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarEntrega) {
  ui->setupUi(this);

  if (UserSession::tipoUsuario() == "VENDEDOR") {
    ui->tableVendas->hide();
    ui->labelEntregasCliente->hide();
  }

  ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));

  ui->dateTimeEdit->setDate(QDate::currentDate());

  connect(ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &WidgetLogisticaAgendarEntrega::calcularDisponivel);
  connect(ui->checkBoxMostrarSul, &QCheckBox::toggled, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
}

WidgetLogisticaAgendarEntrega::~WidgetLogisticaAgendarEntrega() { delete ui; }

void WidgetLogisticaAgendarEntrega::setupTables() {
  // REFAC: refactor this to not select in here

  modelProdutos.setTable("venda_has_produto");
  modelProdutos.setEditStrategy(QSqlTableModel::OnManualSubmit);

  // -----------------------------------------------------------------

  modelVendas.setTable("view_entrega_pendente");
  modelVendas.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelVendas.setHeaderData("idVenda", "Venda");
  modelVendas.setHeaderData("statusFinanceiro", "Financeiro");
  modelVendas.setHeaderData("prazoEntrega", "Prazo Limite");
  modelVendas.setHeaderData("novoPrazoEntrega", "Novo Prazo");
  modelVendas.setHeaderData("dataRealReceb", "Receb.");

  ui->tableVendas->setModel(new FinanceiroProxyModel(&modelVendas, this));
  ui->tableVendas->setItemDelegate(new DoubleDelegate(this));

  ui->tableVendas->hideColumn("data");

  if (UserSession::tipoUsuario() != "VENDEDOR ESPECIAL") ui->tableVendas->hideColumn("Indicou");

  modelTransp.setTable("veiculo_has_produto");
  modelTransp.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTransp.setHeaderData("idVenda", "Venda");
  modelTransp.setHeaderData("status", "Status");
  modelTransp.setHeaderData("produto", "Produto");
  modelTransp.setHeaderData("caixas", "Cx.");
  modelTransp.setHeaderData("kg", "Kg.");
  modelTransp.setHeaderData("quant", "Quant.");
  modelTransp.setHeaderData("un", "Un.");
  modelTransp.setHeaderData("codComercial", "Cód. Com.");
  modelTransp.setHeaderData("fornecedor", "Fornecedor");
  modelTransp.setHeaderData("unCaixa", "Un./Cx.");
  modelTransp.setHeaderData("formComercial", "Form. Com.");

  modelTransp.setFilter("0");

  if (not modelTransp.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela transportadora: " + modelTransp.lastError().text());
    return;
  }

  ui->tableTransp->setModel(&modelTransp);
  ui->tableTransp->hideColumn("id");
  ui->tableTransp->hideColumn("idEstoque");
  ui->tableTransp->hideColumn("idEvento");
  ui->tableTransp->hideColumn("idVeiculo");
  ui->tableTransp->hideColumn("idCompra");
  ui->tableTransp->hideColumn("idVendaProduto");
  ui->tableTransp->hideColumn("idNFeSaida");
  ui->tableTransp->hideColumn("idLoja");
  ui->tableTransp->hideColumn("idProduto");
  ui->tableTransp->hideColumn("obs");
  ui->tableTransp->hideColumn("data");

  //

  modelTransp2.setTable("veiculo_has_produto");
  modelTransp2.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTransp2.setHeaderData("idEstoque", "Estoque");
  modelTransp2.setHeaderData("idVenda", "Venda");
  modelTransp2.setHeaderData("data", "Agendado");
  modelTransp2.setHeaderData("status", "Status");
  modelTransp2.setHeaderData("produto", "Produto");
  modelTransp2.setHeaderData("caixas", "Cx.");
  modelTransp2.setHeaderData("kg", "Kg.");
  modelTransp2.setHeaderData("quant", "Quant.");
  modelTransp2.setHeaderData("un", "Un.");
  modelTransp2.setHeaderData("codComercial", "Cód. Com.");
  modelTransp2.setHeaderData("fornecedor", "Fornecedor");
  modelTransp2.setHeaderData("unCaixa", "Un./Cx.");
  modelTransp2.setHeaderData("formComercial", "Form. Com.");

  modelTransp2.setFilter("0");

  if (not modelTransp2.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela transportadora: " + modelTransp2.lastError().text());
    return;
  }

  ui->tableTransp2->setModel(&modelTransp2);
  ui->tableTransp2->hideColumn("id");
  ui->tableTransp2->hideColumn("idEvento");
  ui->tableTransp2->hideColumn("idVeiculo");
  ui->tableTransp2->hideColumn("idCompra");
  ui->tableTransp2->hideColumn("idVendaProduto");
  ui->tableTransp2->hideColumn("idNFeSaida");
  ui->tableTransp2->hideColumn("idLoja");
  ui->tableTransp2->hideColumn("idProduto");
  ui->tableTransp2->hideColumn("obs");
}

void WidgetLogisticaAgendarEntrega::calcularPeso() {
  double peso = 0;

  for (const auto &item : ui->tableProdutos->selectionModel()->selectedRows()) {
    QSqlQuery query;
    query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");
    query.bindValue(":idProduto", modelViewProdutos.data(item.row(), "idProduto"));

    if (not query.exec() or not query.first()) {
      QMessageBox::critical(this, "Erro!", "Erro buscando peso do produto: " + query.lastError().text());
      return;
    }

    const double kg = query.value("kgcx").toDouble();
    const double caixas = modelViewProdutos.data(item.row(), "caixas").toDouble();
    peso += kg * caixas;
  }

  ui->doubleSpinBoxPeso->setValue(peso);

  ui->doubleSpinBoxPeso->setStyleSheet(ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value() ? "color: rgb(255, 0, 0);" : "");
}

bool WidgetLogisticaAgendarEntrega::updateTables() {
  if (modelVendas.tableName().isEmpty()) {
    setupTables();

    connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
    connect(ui->radioButtonParcialEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
    connect(ui->radioButtonSemEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
    connect(ui->radioButtonTotalEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
    connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaAgendarEntrega::montaFiltro);

    montaFiltro();
  }

  if (not modelVendas.select()) {
    emit errorSignal("Erro lendo tabela vendas: " + modelVendas.lastError().text());
    return false;
  }

  if (not modelVendas.select()) {
    emit errorSignal("Erro lendo tabela venda: " + modelVendas.lastError().text());
    return false;
  }

  ui->tableVendas->sortByColumn("prazoEntrega");

  ui->tableVendas->resizeColumnsToContents();

  modelViewProdutos.setQuery(modelViewProdutos.query().executedQuery());

  for (int row = 0; row < modelViewProdutos.rowCount(); ++row) ui->tableProdutos->openPersistentEditor(row, "selecionado");

  ui->tableProdutos->resizeColumnsToContents();

  if (not modelTransp2.select()) {
    emit errorSignal("Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return false;
  }

  ui->tableTransp2->resizeColumnsToContents();

  calcularDisponivel();

  return true;
}

void WidgetLogisticaAgendarEntrega::on_tableVendas_entered(const QModelIndex &) { ui->tableVendas->resizeColumnsToContents(); }

void WidgetLogisticaAgendarEntrega::on_tableVendas_clicked(const QModelIndex &index) {
  modelViewProdutos.setQuery(
      "SELECT `vp`.`idVendaProduto` AS `idVendaProduto`, `vp`.`idProduto` AS `idProduto`, `vp`.`dataPrevEnt` AS `dataPrevEnt`, `vp`.`dataRealEnt` AS `dataRealEnt`, `vp`.`status` AS `status`, "
      "`vp`.`fornecedor` AS `fornecedor`, `vp`.`idVenda` AS `idVenda`, `vp`.`produto` AS `produto`, `vp`.`caixas` AS `caixas`, `vp`.`quant` AS `quant`, `vp`.`un` AS `un`, `vp`.`unCaixa` AS "
      "`unCaixa`, `vp`.`codComercial` AS `codComercial`, `vp`.`formComercial`  AS `formComercial`, `ehc`.`idConsumo` AS `idConsumo` FROM (`mydb`.`venda_has_produto` `vp` LEFT JOIN "
      "`mydb`.`estoque_has_consumo` `ehc` ON ((`vp`.`idVendaProduto` = `ehc`.`idVendaProduto`))) where vp.idVenda = '" +
      modelVendas.data(index.row(), "idVenda").toString() + "' GROUP BY `vp`.`idVendaProduto`");

  if (modelViewProdutos.lastError().isValid()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando produtos: " + modelViewProdutos.lastError().text());
  }

  modelViewProdutos.setHeaderData("status", "Status");
  modelViewProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelViewProdutos.setHeaderData("idVenda", "Venda");
  modelViewProdutos.setHeaderData("produto", "Produto");
  modelViewProdutos.setHeaderData("caixas", "Caixas");
  modelViewProdutos.setHeaderData("quant", "Quant.");
  modelViewProdutos.setHeaderData("un", "Un.");
  modelViewProdutos.setHeaderData("unCaixa", "Un./Cx.");
  modelViewProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelViewProdutos.setHeaderData("formComercial", "Form. Com.");
  modelViewProdutos.setHeaderData("dataPrevEnt", "Prev. Ent.");

  auto *proxyFilter = new QSortFilterProxyModel(this);
  proxyFilter->setDynamicSortFilter(true);
  proxyFilter->setSourceModel(&modelViewProdutos);

  ui->tableProdutos->setModel(proxyFilter);

  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataRealEnt");
  ui->tableProdutos->hideColumn("idConsumo");

  ui->tableProdutos->resizeColumnsToContents();

  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarEntrega::calcularPeso);

  ui->lineEditAviso->setText(modelVendas.data(index.row(), "statusFinanceiro").toString() != "LIBERADO" ? "Financeiro não liberou!" : "");
  //  if (modelVendas.data(index.row(), "statusFinanceiro").toString() != "LIBERADO") QMessageBox::warning(this, "Aviso!", "Financeiro não liberou!");
}

void WidgetLogisticaAgendarEntrega::montaFiltro() {
  QString filtro;

  QString filtroCheck;

  if (ui->radioButtonEntregaLimpar->isChecked()) filtroCheck = "";
  if (ui->radioButtonTotalEstoque->isChecked()) filtroCheck = "Estoque > 0 AND Entregue >= 0 AND Outros = 0";
  if (ui->radioButtonParcialEstoque->isChecked()) filtroCheck = "Estoque > 0 AND (Entregue > 0 OR Outros > 0)";
  if (ui->radioButtonSemEstoque->isChecked()) filtroCheck = "Estoque = 0";

  filtro += filtroCheck;

  const QString textoBusca = ui->lineEditBusca->text();

  const QString filtroBusca =
      textoBusca.isEmpty() ? "" : "(idVenda LIKE '%" + textoBusca + "%' OR Bairro LIKE '%" + textoBusca + "%' OR Logradouro LIKE '%" + textoBusca + "%' OR Cidade LIKE '%" + textoBusca + "%')";

  if (not filtroBusca.isEmpty()) filtro += filtro.isEmpty() ? filtroBusca : " AND " + filtroBusca;

  const QString filtroSul = ui->checkBoxMostrarSul->isChecked() ? "" : "(idVenda NOT LIKE '%CAMB%')";

  if (not filtroSul.isEmpty()) filtro += filtro.isEmpty() ? filtroSul : " AND " + filtroSul;

  modelVendas.setFilter(filtro);

  if (not modelVendas.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + modelVendas.lastError().text());
    return;
  }

  ui->tableVendas->sortByColumn("prazoEntrega");
  ui->tableVendas->resizeColumnsToContents();

  modelViewProdutos.setQuery("");
}

void WidgetLogisticaAgendarEntrega::on_tableProdutos_entered(const QModelIndex &) { ui->tableProdutos->resizeColumnsToContents(); }

void WidgetLogisticaAgendarEntrega::on_pushButtonAgendarCarga_clicked() {
  if (modelTransp.rowCount() == 0) {
    QMessageBox::critical(this, "Erro!", "Carga vazia!");
    return;
  }

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not processRows()) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  QSqlQuery query;

  if (not query.exec("CALL update_venda_status()")) {
    QMessageBox::critical(this, "Erro!", "Erro atualizando status das vendas: " + query.lastError().text());
    return;
  }

  updateTables();
  QMessageBox::information(this, "Aviso!", "Confirmado agendamento!");
}

bool WidgetLogisticaAgendarEntrega::processRows() {
  const QDateTime dataPrevEnt = ui->dateTimeEdit->dateTime();

  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto") or not query.first()) {
    emit errorSignal("Erro comunicando com o banco de dados: " + query.lastError().text());
    return false;
  }

  const int idEvento = query.value(0).toInt();

  QSqlQuery query1;
  query1.prepare("SELECT idVenda, codComercial FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'ENTREGA AGEND.', dataPrevEnt = :dataPrevEnt WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET status = 'ENTREGA AGEND.', dataPrevEnt = :dataPrevEnt WHERE idVendaProduto = :idVendaProduto");

  for (int row = 0; row < modelTransp.rowCount(); ++row) {
    if (not modelTransp.setData(row, "data", dataPrevEnt)) return false;
    if (not modelTransp.setData(row, "idEvento", idEvento)) return false;

    const int idVendaProduto = modelTransp.data(row, "idVendaProduto").toInt();

    query1.bindValue(":idVendaProduto", idVendaProduto);

    if (not query1.exec() or not query1.first()) {
      emit errorSignal("Erro buscando dados do produto: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":dataPrevEnt", dataPrevEnt);
    query2.bindValue(":idVenda", idVendaProduto);

    if (not query2.exec()) {
      emit errorSignal("Erro atualizando status da compra: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":dataPrevEnt", dataPrevEnt);
    query3.bindValue(":idVendaProduto", idVendaProduto);

    if (not query3.exec()) {
      emit errorSignal("Erro atualizando produtos venda: " + query3.lastError().text());
      return false;
    }
  }

  if (not modelTransp.submitAll()) {
    emit errorSignal("Erro salvando carga veiculo: " + modelTransp.lastError().text());
    return false;
  }

  return true;
}

bool WidgetLogisticaAgendarEntrega::adicionarProduto(const QModelIndexList &list) {
  for (const auto &item : list) {
    const int row = modelTransp.rowCount();
    modelTransp.insertRow(row);

    //
    QSqlQuery query;
    query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");
    query.bindValue(":idProduto", modelViewProdutos.data(item.row(), "idProduto"));

    if (not query.exec() or not query.first()) {
      QMessageBox::critical(this, "Erro!", "Erro buscando peso do produto: " + query.lastError().text());
      return false;
    }

    const double kg = query.value("kgcx").toDouble();
    const double caixas = modelViewProdutos.data(item.row(), "caixas").toDouble();
    const double peso = kg * caixas;

    //

    if (not modelTransp.setData(row, "fornecedor", modelViewProdutos.data(item.row(), "fornecedor"))) return false;
    if (not modelTransp.setData(row, "unCaixa", modelViewProdutos.data(item.row(), "unCaixa"))) return false;
    if (not modelTransp.setData(row, "formComercial", modelViewProdutos.data(item.row(), "formComercial"))) return false;
    if (not modelTransp.setData(row, "idVeiculo", ui->itemBoxVeiculo->getValue())) return false;
    if (not modelTransp.setData(row, "idVenda", modelViewProdutos.data(item.row(), "idVenda"))) return false;
    if (not modelTransp.setData(row, "idVendaProduto", modelViewProdutos.data(item.row(), "idVendaProduto"))) return false;
    if (not modelTransp.setData(row, "idProduto", modelViewProdutos.data(item.row(), "idProduto"))) return false;
    if (not modelTransp.setData(row, "produto", modelViewProdutos.data(item.row(), "produto"))) return false;
    if (not modelTransp.setData(row, "codComercial", modelViewProdutos.data(item.row(), "codComercial"))) return false;
    if (not modelTransp.setData(row, "un", modelViewProdutos.data(item.row(), "un"))) return false;
    if (not modelTransp.setData(row, "caixas", modelViewProdutos.data(item.row(), "caixas"))) return false;
    if (not modelTransp.setData(row, "kg", peso)) return false;
    if (not modelTransp.setData(row, "quant", modelViewProdutos.data(item.row(), "quant"))) return false;
    if (not modelTransp.setData(row, "status", "ENTREGA AGEND.")) return false;
  }

  ui->tableTransp->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getValue().isNull()) {
    QMessageBox::critical(this, "Erro!", "Deve escolher uma transportadora antes!");
    return;
  }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value()) {
    QMessageBox::warning(this, "Aviso!", "Peso maior que capacidade do veículo!");
  }

  for (const auto &item : list) {
    const int idVendaProduto = modelViewProdutos.data(item.row(), "idVendaProduto").toInt();

    auto listMatch = modelTransp.match(modelTransp.index(0, modelTransp.fieldIndex("idVendaProduto")), Qt::DisplayRole, idVendaProduto);

    if (listMatch.size() > 0) {
      QMessageBox::critical(this, "Erro!", "Item já inserido!");
      return;
    }

    if (modelViewProdutos.data(item.row(), "idConsumo").toInt() == 0) {
      QMessageBox::critical(this, "Erro!", "Produto ainda não possui nota de entrada!");
      return;
    }

    if (not modelViewProdutos.data(item.row(), "dataPrevEnt").isNull()) {
      QMessageBox::critical(this, "Erro!", "Produto já agendado!");
      return;
    }
  }

  if (not adicionarProduto(list)) modelTransp.select();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonRemoverProduto_clicked() {
  const auto list = ui->tableTransp->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  for (const auto &item : list) modelTransp.removeRow(item.row());

  modelTransp.submitAll();
}

void WidgetLogisticaAgendarEntrega::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getValue());

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando dados veiculo: " + query.lastError().text());
    return;
  }

  if (not modelTransp.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return;
  }

  modelTransp2.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getValue().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  if (not modelTransp2.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return;
  }

  ui->tableTransp2->resizeColumnsToContents();

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::on_tableTransp2_entered(const QModelIndex &) { ui->tableTransp2->resizeColumnsToContents(); }

void WidgetLogisticaAgendarEntrega::calcularDisponivel() {
  double peso = 0;

  for (int row = 0; row < modelTransp2.rowCount(); ++row) {
    const int hour = modelTransp2.data(row, "data").toDateTime().time().hour();

    if (hour != ui->dateTimeEdit->time().hour()) continue;

    peso += modelTransp2.data(row, "kg").toDouble();
  }

  const double capacidade = ui->doubleSpinBoxCapacidade->value();

  ui->doubleSpinBoxDisponivel->setValue(capacidade - peso);
}

void WidgetLogisticaAgendarEntrega::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) return;

  modelTransp2.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getValue().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");

  if (not modelTransp2.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return;
  }

  ui->tableTransp2->resizeColumnsToContents();

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarParcial_clicked() {
  if (ui->itemBoxVeiculo->getValue().isNull()) {
    QMessageBox::critical(this, "Erro!", "Deve escolher uma transportadora antes!");
    return;
  }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  if (list.size() > 1) {
    QMessageBox::critical(this, "Erro!", "Deve selecionar apenas um item para agendamento parcial!");
    return;
  }

  const int row = list.first().row();

  const int idVendaProduto = modelViewProdutos.data(row, "idVendaProduto").toInt();

  auto list2 = modelTransp.match(modelTransp.index(0, modelTransp.fieldIndex("idVendaProduto")), Qt::DisplayRole, idVendaProduto);

  if (list2.size() > 0) {
    QMessageBox::critical(this, "Erro!", "Item já inserido!");
    return;
  }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value()) {
    QMessageBox::warning(this, "Aviso!", "Peso maior que capacidade do veículo!");
  }

  if (modelViewProdutos.data(row, "idConsumo").toInt() == 0) {
    QMessageBox::critical(this, "Erro!", "Produto ainda não possui nota de entrada!");
    return;
  }

  if (modelViewProdutos.data(row, "caixas").toInt() == 1) {
    QMessageBox::critical(this, "Erro!", "Produto tem apenas uma caixa!");
    return;
  }

  if (not modelViewProdutos.data(row, "dataPrevEnt").isNull()) {
    QMessageBox::critical(this, "Erro!", "Produto já agendado!");
    return;
  }

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not adicionarProdutoParcial(row)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();
}

bool WidgetLogisticaAgendarEntrega::adicionarProdutoParcial(const int row) {
  // perguntar quantidade
  const int quantTotal = modelViewProdutos.data(row, "caixas").toInt();

  bool ok;

  // TODO: 0put this outside transaction
  const int quantAgendar = QInputDialog::getInt(this, "Agendar", "Quantidade de caixas: ", quantTotal, 0, quantTotal, 1, &ok);

  if (quantAgendar == 0 or not ok) return false;

  // quebrar se necessario

  if (quantAgendar < quantTotal) {
    if (not quebrarProduto(row, quantAgendar, quantTotal)) return false;
  }

  //
  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelViewProdutos.data(row, "idProduto"));

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando peso do produto: " + query.lastError().text());
    return false;
  }

  const double unCaixa = modelViewProdutos.data(row, "unCaixa").toDouble();
  const double kg = query.value("kgcx").toDouble();

  const int newRow = modelTransp.rowCount();
  modelTransp.insertRow(newRow);

  if (not modelTransp.setData(newRow, "fornecedor", modelViewProdutos.data(row, "fornecedor"))) return false;
  if (not modelTransp.setData(newRow, "unCaixa", modelViewProdutos.data(row, "unCaixa"))) return false;
  if (not modelTransp.setData(newRow, "formComercial", modelViewProdutos.data(row, "formComercial"))) return false;
  if (not modelTransp.setData(newRow, "idVeiculo", ui->itemBoxVeiculo->getValue())) return false;
  if (not modelTransp.setData(newRow, "idVenda", modelViewProdutos.data(row, "idVenda"))) return false;
  if (not modelTransp.setData(newRow, "idVendaProduto", modelViewProdutos.data(row, "idVendaProduto"))) return false;
  if (not modelTransp.setData(newRow, "idProduto", modelViewProdutos.data(row, "idProduto"))) return false;
  if (not modelTransp.setData(newRow, "produto", modelViewProdutos.data(row, "produto"))) return false;
  if (not modelTransp.setData(newRow, "codComercial", modelViewProdutos.data(row, "codComercial"))) return false;
  if (not modelTransp.setData(newRow, "un", modelViewProdutos.data(row, "un"))) return false;
  if (not modelTransp.setData(newRow, "caixas", quantAgendar)) return false;
  if (not modelTransp.setData(newRow, "kg", kg * quantAgendar)) return false;
  if (not modelTransp.setData(newRow, "quant", quantAgendar * unCaixa)) return false;
  if (not modelTransp.setData(newRow, "status", "ENTREGA AGEND.")) return false;

  ui->tableTransp->resizeColumnsToContents();

  return true;
}

bool WidgetLogisticaAgendarEntrega::quebrarProduto(const int row, const int quantAgendar, const int quantTotal) {
  modelProdutos.setFilter("idVendaProduto = " + modelViewProdutos.data(row, "idVendaProduto").toString());

  if (not modelProdutos.select()) {
    emit errorSignal("Erro lendo tabela venda_produto: " + modelProdutos.lastError().text());
    return false;
  }

  const int newRow = modelProdutos.rowCount();
  modelProdutos.insertRow(newRow);

  // copiar colunas
  for (int column = 0, columnCount = modelProdutos.columnCount(); column < columnCount; ++column) {
    if (modelProdutos.fieldIndex("idVendaProduto") == column) continue;
    if (modelProdutos.fieldIndex("created") == column) continue;
    if (modelProdutos.fieldIndex("lastUpdated") == column) continue;

    const QVariant value = modelProdutos.data(0, column);

    if (not modelProdutos.setData(newRow, column, value)) return false;
  }

  const double unCaixa = modelProdutos.data(0, "unCaixa").toDouble();

  const double proporcao = double(quantAgendar) / quantTotal;
  const double parcial = modelProdutos.data(0, "parcial").toDouble() * proporcao;
  const double parcialDesc = modelProdutos.data(0, "parcialDesc").toDouble() * proporcao;
  const double total = modelProdutos.data(0, "total").toDouble() * proporcao;

  if (not modelProdutos.setData(0, "quant", quantAgendar * unCaixa)) return false;
  if (not modelProdutos.setData(0, "caixas", quantAgendar)) return false;
  if (not modelProdutos.setData(0, "parcial", parcial)) return false;
  if (not modelProdutos.setData(0, "parcialDesc", parcialDesc)) return false;
  if (not modelProdutos.setData(0, "total", total)) return false;

  // alterar quant, precos, etc da linha nova
  const double proporcaoNovo = double((quantTotal - quantAgendar)) / quantTotal;
  const double parcialNovo = modelProdutos.data(newRow, "parcial").toDouble() * proporcaoNovo;
  const double parcialDescNovo = modelProdutos.data(newRow, "parcialDesc").toDouble() * proporcaoNovo;
  const double totalNovo = modelProdutos.data(newRow, "total").toDouble() * proporcaoNovo;

  if (not modelProdutos.setData(newRow, "quant", (quantTotal - quantAgendar) * unCaixa)) return false;
  if (not modelProdutos.setData(newRow, "caixas", quantTotal - quantAgendar)) return false;
  if (not modelProdutos.setData(newRow, "parcial", parcialNovo)) return false;
  if (not modelProdutos.setData(newRow, "parcialDesc", parcialDescNovo)) return false;
  if (not modelProdutos.setData(newRow, "total", totalNovo)) return false;

  if (not modelProdutos.submitAll()) {
    emit errorSignal("Erro salvando dados venda_produto: " + modelProdutos.lastError().text());
    return false;
  }

  const QVariant lastId = RegisterDialog::getLastInsertId();

  //  if (not modelViewProdutos.select()) {
  //    error = "Erro lendo tabela: " + modelViewProdutos.lastError().text();
  //    return false;
  //  }

  // refactor below into another function

  // get idVendaProduto lastInsertId

  //  qDebug() << "lastInsert: " << lastId;
  // break consumo into two lines

  SqlRelationalTableModel modelConsumo;
  modelConsumo.setTable("estoque_has_consumo");
  modelConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelConsumo.setFilter("idVendaProduto = " + modelViewProdutos.data(row, "idVendaProduto").toString());

  if (not modelConsumo.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + modelConsumo.lastError().text());
    return false;
  }

  const int rowConsumo = modelConsumo.rowCount();
  modelConsumo.insertRow(rowConsumo);

  for (int column = 0, columnCount = modelConsumo.columnCount(); column < columnCount; ++column) {
    if (modelConsumo.fieldIndex("idConsumo") == column) continue;
    if (modelConsumo.fieldIndex("idVendaProduto") == column) continue;
    if (modelConsumo.fieldIndex("created") == column) continue;
    if (modelConsumo.fieldIndex("lastUpdated") == column) continue;

    const QVariant value = modelConsumo.data(0, column);

    if (not modelConsumo.setData(rowConsumo, column, value)) return false;
  }

  // alterar quant, caixas, valor

  const double quantConsumo = modelConsumo.data(0, "quant").toDouble() * proporcao;
  const int caixasConsumo = modelConsumo.data(0, "caixas").toInt() * proporcao; // REFAC: redo those to double
  const double valorConsumo = modelConsumo.data(0, "valor").toDouble() * proporcao;

  if (not modelConsumo.setData(0, "quant", quantConsumo)) return false;
  if (not modelConsumo.setData(0, "caixas", caixasConsumo)) return false;
  if (not modelConsumo.setData(0, "valor", valorConsumo)) return false;

  // alterar linha nova
  const double quantConsumo2 = modelConsumo.data(rowConsumo, "quant").toDouble() * proporcaoNovo;
  const int caixasConsumo2 = modelConsumo.data(rowConsumo, "caixas").toInt() * proporcaoNovo;
  const double valorConsumo2 = modelConsumo.data(rowConsumo, "valor").toDouble() * proporcaoNovo;

  if (not modelConsumo.setData(rowConsumo, "quant", quantConsumo2)) return false;
  if (not modelConsumo.setData(rowConsumo, "caixas", caixasConsumo2)) return false;
  if (not modelConsumo.setData(rowConsumo, "valor", valorConsumo2)) return false;
  if (not modelConsumo.setData(rowConsumo, "idVendaProduto", lastId)) return false;

  if (not modelConsumo.submitAll()) {
    emit errorSignal("Erro quebrando consumo em duas linhas: " + modelConsumo.lastError().text());
    return false;
  }

  return true;
}

// TODO: 1'em entrega' deve entrar na categoria 100% estoque?
// TODO: 5adicionar botao para cancelar agendamento (verificar com Anderson)

void WidgetLogisticaAgendarEntrega::on_pushButtonReagendarPedido_clicked() {
  // get idVenda from view_entrega_pendente/modelVendas and set 'novoPrazo'

  const auto list = ui->tableVendas->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  InputDialog input(InputDialog::Tipo::ReagendarPedido);

  if (input.exec() != InputDialog::Accepted) return;

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not reagendar(list, input.getNextDate(), input.getObservacao())) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  updateTables();
  QMessageBox::information(this, "Aviso!", "Reagendado com sucesso!");
}

bool WidgetLogisticaAgendarEntrega::reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao) {
  QSqlQuery query;

  for (const auto &item : list) {
    query.prepare("UPDATE venda SET novoPrazoEntrega = :novoPrazoEntrega WHERE idVenda = :idVenda");
    query.bindValue(":novoPrazoEntrega", modelVendas.data(item.row(), "data").toDate().daysTo(dataPrev));
    query.bindValue(":idVenda", modelVendas.data(item.row(), "idVenda"));

    if (not query.exec()) {
      emit errorSignal("Erro atualizando novo prazo: " + query.lastError().text());
      return false;
    }

    query.prepare(
        "INSERT INTO venda_has_followup (idVenda, idLoja, idUsuario, tipoOperacao, observacao, dataFollowup) VALUES (:idVenda, :idLoja, :idUsuario, :tipoOperacao, :observacao, :dataFollowup)");
    query.bindValue(":idVenda", modelVendas.data(item.row(), "idVenda"));
    query.bindValue(":idLoja", UserSession::idLoja());
    query.bindValue(":idUsuario", UserSession::idUsuario());
    query.bindValue(":tipoOperacao", "Alteração do prazo de entrega");
    query.bindValue(":observacao", observacao);
    query.bindValue(":dataFollowup", QDate::currentDate());

    if (not query.exec()) {
      emit errorSignal("Erro salvando followup: " + query.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaAgendarEntrega::on_tableVendas_doubleClicked(const QModelIndex &index) {
  if (index.column() == modelVendas.record().indexOf("novoPrazoEntrega")) {
    const auto list = ui->tableVendas->selectionModel()->selectedRows();

    if (list.isEmpty()) {
      QMessageBox::critical(this, "Erro!", "Nenhuma linha selecionada!");
      return;
    }

    FollowUp *followup = new FollowUp(modelVendas.data(list.first().row(), "idVenda").toString(), FollowUp::Tipo::Venda, this);
    followup->setAttribute(Qt::WA_DeleteOnClose);
    followup->show();
  }
}

// TODO: 5refazer filtros do estoque (casos 'devolvido', 'cancelado', 'em entrega')