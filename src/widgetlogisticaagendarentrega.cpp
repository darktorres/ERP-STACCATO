#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "cadastrarnfe.h"
#include "doubledelegate.h"
#include "financeiroproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "sql.h"
#include "ui_widgetlogisticaagendarentrega.h"
#include "usersession.h"
#include "widgetlogisticaagendarentrega.h"

WidgetLogisticaAgendarEntrega::WidgetLogisticaAgendarEntrega(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarEntrega) { ui->setupUi(this); }

WidgetLogisticaAgendarEntrega::~WidgetLogisticaAgendarEntrega() { delete ui; }

void WidgetLogisticaAgendarEntrega::setupTables() {
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

  // -----------------------------------------------------------------

  modelTranspAtual.setTable("veiculo_has_produto");
  modelTranspAtual.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTranspAtual.setHeaderData("idVenda", "Venda");
  modelTranspAtual.setHeaderData("status", "Status");
  modelTranspAtual.setHeaderData("produto", "Produto");
  modelTranspAtual.setHeaderData("caixas", "Cx.");
  modelTranspAtual.setHeaderData("kg", "Kg.");
  modelTranspAtual.setHeaderData("quant", "Quant.");
  modelTranspAtual.setHeaderData("un", "Un.");
  modelTranspAtual.setHeaderData("codComercial", "Cód. Com.");
  modelTranspAtual.setHeaderData("fornecedor", "Fornecedor");
  modelTranspAtual.setHeaderData("unCaixa", "Un./Cx.");
  modelTranspAtual.setHeaderData("formComercial", "Form. Com.");

  modelTranspAtual.setFilter("0");

  if (not modelTranspAtual.select()) { return; }

  ui->tableTranspAtual->setModel(&modelTranspAtual);
  ui->tableTranspAtual->hideColumn("id");
  ui->tableTranspAtual->hideColumn("idEstoque");
  ui->tableTranspAtual->hideColumn("idEvento");
  ui->tableTranspAtual->hideColumn("idVeiculo");
  ui->tableTranspAtual->hideColumn("idCompra");
  ui->tableTranspAtual->hideColumn("idVendaProduto");
  ui->tableTranspAtual->hideColumn("idNFeSaida");
  ui->tableTranspAtual->hideColumn("idLoja");
  ui->tableTranspAtual->hideColumn("idProduto");
  ui->tableTranspAtual->hideColumn("obs");
  ui->tableTranspAtual->hideColumn("data");

  // -----------------------------------------------------------------

  modelTranspAgend.setTable("veiculo_has_produto");
  modelTranspAgend.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
  modelTranspAgend.setHeaderData("idVenda", "Venda");
  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("kg", "Kg.");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");
  modelTranspAgend.setHeaderData("fornecedor", "Fornecedor");
  modelTranspAgend.setHeaderData("unCaixa", "Un./Cx.");
  modelTranspAgend.setHeaderData("formComercial", "Form. Com.");

  modelTranspAgend.setFilter("0");

  if (not modelTranspAgend.select()) { return; }

  ui->tableTranspAgend->setModel(&modelTranspAgend);
  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idVendaProduto");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("obs");
}

void WidgetLogisticaAgendarEntrega::calcularPeso() {
  double peso = 0;

  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

  Q_FOREACH (const auto &item, ui->tableProdutos->selectionModel()->selectedRows()) {
    query.bindValue(":idProduto", modelViewProdutos.data(item.row(), "idProduto"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando peso do produto: " + query.lastError().text()); }

    const double kg = query.value("kgcx").toDouble();
    const double caixas = modelViewProdutos.data(item.row(), "caixas").toDouble();
    peso += kg * caixas;
  }

  ui->doubleSpinBoxPeso->setValue(peso);

  ui->doubleSpinBoxPeso->setStyleSheet(ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value() ? "color: rgb(255, 0, 0);" : "");
}

void WidgetLogisticaAgendarEntrega::setConnections() {
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetLogisticaAgendarEntrega::on_dateTimeEdit_dateChanged);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetLogisticaAgendarEntrega::on_itemBoxVeiculo_textChanged);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
  connect(ui->pushButtonAdicionarParcial, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarParcial_clicked);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarProduto_clicked);
  connect(ui->pushButtonAgendarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonAgendarCarga_clicked);
  connect(ui->pushButtonGerarNFeFutura, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonGerarNFeFutura_clicked);
  connect(ui->pushButtonReagendarPedido, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonReagendarPedido_clicked);
  connect(ui->pushButtonRemoverProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonRemoverProduto_clicked);
  connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
  connect(ui->radioButtonParcialEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
  connect(ui->radioButtonSemEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
  connect(ui->radioButtonTotalEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro);
  connect(ui->tableVendas, &TableView::clicked, this, &WidgetLogisticaAgendarEntrega::on_tableVendas_clicked);
  connect(ui->tableVendas, &TableView::doubleClicked, this, &WidgetLogisticaAgendarEntrega::on_tableVendas_doubleClicked);
}

void WidgetLogisticaAgendarEntrega::updateTables() {
  if (not isSet) {
    if (UserSession::tipoUsuario() == "VENDEDOR") {
      ui->tableVendas->hide();
      ui->labelEntregasCliente->hide();
    }

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(QDate::currentDate());

    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelVendas.select()) { return; }

  ui->tableVendas->sortByColumn("prazoEntrega");
  ui->tableVendas->resizeColumnsToContents();

  // -------------------------------------------------------------------------

  modelViewProdutos.setQuery(modelViewProdutos.query().executedQuery());

  // -------------------------------------------------------------------------

  if (not modelTranspAgend.select()) { return; }

  ui->tableTranspAgend->resizeColumnsToContents();

  // -------------------------------------------------------------------------

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::resetTables() { modelIsSet = false; }

void WidgetLogisticaAgendarEntrega::on_tableVendas_clicked(const QModelIndex &index) {
  modelViewProdutos.setQuery(
      "SELECT `vp`.`idVendaProduto` AS `idVendaProduto`, `vp`.`idProduto` AS `idProduto`, `vp`.`dataPrevEnt` AS `dataPrevEnt`, `vp`.`dataRealEnt` AS `dataRealEnt`, `vp`.`status` AS `status`, "
      "`vp`.`fornecedor` AS `fornecedor`, `vp`.`idVenda` AS `idVenda`, `vp`.`idNFeFutura` AS `NFe Fut.`, `vp`.`produto` AS `produto`, e.idEstoque, e.lote, e.local, e.bloco, `vp`.`caixas` AS "
      "`caixas`, `vp`.`quant` AS `quant`, `vp`.`un` AS `un`, `vp`.`unCaixa` AS `unCaixa`, `vp`.`codComercial` AS `codComercial`, `vp`.`formComercial`  AS `formComercial`, `ehc`.`idConsumo` AS "
      "`idConsumo` FROM `venda_has_produto` `vp` LEFT JOIN `estoque_has_consumo` `ehc` ON `vp`.`idVendaProduto` = `ehc`.`idVendaProduto` LEFT JOIN estoque e ON ehc.idEstoque = "
      "e.idEstoque WHERE vp.idVenda = '" +
      modelVendas.data(index.row(), "idVenda").toString() + "' GROUP BY `vp`.`idVendaProduto`, e.idEstoque, ehc.idConsumo");

  if (modelViewProdutos.lastError().isValid()) { return qApp->enqueueError("Erro buscando produtos: " + modelViewProdutos.lastError().text()); }

  modelViewProdutos.setHeaderData("status", "Status");
  modelViewProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelViewProdutos.setHeaderData("idVenda", "Venda");
  modelViewProdutos.setHeaderData("produto", "Produto");
  modelViewProdutos.setHeaderData("idEstoque", "Estoque");
  modelViewProdutos.setHeaderData("lote", "Lote");
  modelViewProdutos.setHeaderData("local", "Local");
  modelViewProdutos.setHeaderData("bloco", "Bloco");
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
}

void WidgetLogisticaAgendarEntrega::montaFiltro() {
  QStringList filtros;

  QString filtroCheck;

  if (ui->radioButtonEntregaLimpar->isChecked()) { filtroCheck = "(Estoque > 0 OR Outros > 0)"; }
  if (ui->radioButtonTotalEstoque->isChecked()) { filtroCheck = "Estoque > 0 AND Outros = 0"; }
  if (ui->radioButtonParcialEstoque->isChecked()) { filtroCheck = "Estoque > 0 AND Outros > 0"; }
  if (ui->radioButtonSemEstoque->isChecked()) { filtroCheck = "Estoque = 0 AND Outros > 0"; }

  if (not filtroCheck.isEmpty()) { filtros << filtroCheck; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();

  const QString filtroBusca = "(idVenda LIKE '%" + textoBusca + "%' OR Bairro LIKE '%" + textoBusca + "%' OR Logradouro LIKE '%" + textoBusca + "%' OR Cidade LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelVendas.setFilter(filtros.join(" AND "));

  if (not modelVendas.select()) { return; }

  ui->tableVendas->sortByColumn("prazoEntrega");

  modelViewProdutos.setQuery("");

  //-------------------------------------

  ui->tableVendas->resizeColumnsToContents();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAgendarCarga_clicked() {
  // TODO: não deixar agendar carga com mistura de produtos com e sem nfe futura

  if (modelTranspAtual.rowCount() == 0) { return qApp->enqueueError("Carga vazia!"); }

  QStringList idVendas;

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) { idVendas << modelTranspAtual.data(row, "idVenda").toString(); }

  if (not qApp->startTransaction()) { return; }

  if (not processRows()) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVendas)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Confirmado agendamento!");
}

bool WidgetLogisticaAgendarEntrega::processRows() {
  const QDateTime dataPrevEnt = ui->dateTimeEdit->dateTime();

  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto") or not query.first()) {
    return qApp->enqueueError(false, "Erro comunicando com o banco de dados: " + query.lastError().text());
  }

  const int idEvento = query.value(0).toInt();

  QSqlQuery query1;
  query1.prepare("SELECT idVenda, codComercial FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'ENTREGA AGEND.', dataPrevEnt = :dataPrevEnt WHERE idVendaProduto = :idVendaProduto AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET status = 'ENTREGA AGEND.', dataPrevEnt = :dataPrevEnt WHERE idVendaProduto = :idVendaProduto AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) {
    if (not modelTranspAtual.setData(row, "data", dataPrevEnt)) { return false; }
    if (not modelTranspAtual.setData(row, "idEvento", idEvento)) { return false; }

    const int idVendaProduto = modelTranspAtual.data(row, "idVendaProduto").toInt();

    query1.bindValue(":idVendaProduto", idVendaProduto);

    if (not query1.exec() or not query1.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + query1.lastError().text()); }

    query2.bindValue(":dataPrevEnt", dataPrevEnt);
    query2.bindValue(":idVenda", idVendaProduto);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + query2.lastError().text()); }

    query3.bindValue(":dataPrevEnt", dataPrevEnt);
    query3.bindValue(":idVendaProduto", idVendaProduto);

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro atualizando produtos venda: " + query3.lastError().text()); }
  }

  return modelTranspAtual.submitAll();
}

bool WidgetLogisticaAgendarEntrega::adicionarProduto(const QModelIndexList &list) {
  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

  for (const auto &item : list) {
    query.bindValue(":idProduto", modelViewProdutos.data(item.row(), "idProduto"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando peso do produto: " + query.lastError().text()); }

    const double kg = query.value("kgcx").toDouble();
    const double caixas = modelViewProdutos.data(item.row(), "caixas").toDouble();
    const double peso = kg * caixas;

    // -------------------------------------------------------------------------

    const int row = modelTranspAtual.rowCount();
    modelTranspAtual.insertRow(row);

    if (not modelTranspAtual.setData(row, "fornecedor", modelViewProdutos.data(item.row(), "fornecedor"))) { return false; }
    if (not modelTranspAtual.setData(row, "unCaixa", modelViewProdutos.data(item.row(), "unCaixa"))) { return false; }
    if (not modelTranspAtual.setData(row, "formComercial", modelViewProdutos.data(item.row(), "formComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "idVeiculo", ui->itemBoxVeiculo->getId())) { return false; }
    if (not modelTranspAtual.setData(row, "idVenda", modelViewProdutos.data(item.row(), "idVenda"))) { return false; }
    if (not modelTranspAtual.setData(row, "idVendaProduto", modelViewProdutos.data(item.row(), "idVendaProduto"))) { return false; }
    if (not modelTranspAtual.setData(row, "idProduto", modelViewProdutos.data(item.row(), "idProduto"))) { return false; }
    if (not modelTranspAtual.setData(row, "produto", modelViewProdutos.data(item.row(), "produto"))) { return false; }
    if (not modelTranspAtual.setData(row, "codComercial", modelViewProdutos.data(item.row(), "codComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "un", modelViewProdutos.data(item.row(), "un"))) { return false; }
    if (not modelTranspAtual.setData(row, "caixas", modelViewProdutos.data(item.row(), "caixas"))) { return false; }
    if (not modelTranspAtual.setData(row, "kg", peso)) { return false; }
    if (not modelTranspAtual.setData(row, "quant", modelViewProdutos.data(item.row(), "quant"))) { return false; }
    if (not modelTranspAtual.setData(row, "status", "ENTREGA AGEND.")) { return false; }
  }

  return true;
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { return qApp->enqueueError("Deve escolher uma transportadora antes!"); }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!"); }

  for (const auto &item : list) {
    const auto listMatch = modelTranspAtual.match("idVendaProduto", modelViewProdutos.data(item.row(), "idVendaProduto"), Qt::MatchExactly);

    if (not listMatch.isEmpty()) { return qApp->enqueueError("Item já inserido!"); }

    if (modelViewProdutos.data(item.row(), "status").toString() != "ESTOQUE") { return qApp->enqueueError("Produto não está em estoque!"); }

    if (modelViewProdutos.data(item.row(), "idConsumo").toInt() == 0) { return qApp->enqueueError("Produto ainda não possui nota de entrada!"); }

    if (not modelViewProdutos.data(item.row(), "dataPrevEnt").isNull()) { return qApp->enqueueError("Produto já agendado!"); }
  }

  if (not adicionarProduto(list) and not modelTranspAtual.select()) { return; }
}

void WidgetLogisticaAgendarEntrega::on_pushButtonRemoverProduto_clicked() {
  const auto list = ui->tableTranspAtual->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  for (const auto &item : list) { modelTranspAtual.removeRow(item.row()); }

  if (not modelTranspAtual.submitAll()) { return; }
}

void WidgetLogisticaAgendarEntrega::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados veiculo: " + query.lastError().text()); }

  if (not modelTranspAtual.select()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { return; }

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::calcularDisponivel() {
  double peso = 0;

  for (int row = 0; row < modelTranspAgend.rowCount(); ++row) {
    const int hour = modelTranspAgend.data(row, "data").toDateTime().time().hour();

    if (hour != ui->dateTimeEdit->time().hour()) { continue; }

    peso += modelTranspAgend.data(row, "kg").toDouble();
  }

  const double capacidade = ui->doubleSpinBoxCapacidade->value();

  ui->doubleSpinBoxDisponivel->setValue(capacidade - peso);
}

void WidgetLogisticaAgendarEntrega::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { return; }

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarParcial_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { return qApp->enqueueError("Deve escolher uma transportadora antes!"); }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  if (list.size() > 1) { return qApp->enqueueError("Deve selecionar apenas um item para agendamento parcial!"); }

  const int row = list.first().row();

  const auto list2 = modelTranspAtual.match("idVendaProduto", modelViewProdutos.data(row, "idVendaProduto"), -1, Qt::MatchExactly);

  if (not list2.isEmpty()) { return qApp->enqueueError("Item já inserido!"); }

  if (modelViewProdutos.data(row, "status").toString() != "ESTOQUE") { return qApp->enqueueError("Produto não está em estoque!"); }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!"); }

  if (modelViewProdutos.data(row, "idConsumo").toInt() == 0) { return qApp->enqueueError("Produto ainda não possui nota de entrada!"); }

  if (modelViewProdutos.data(row, "caixas").toInt() == 1) { return qApp->enqueueError("Produto tem apenas uma caixa!"); }

  if (not modelViewProdutos.data(row, "dataPrevEnt").isNull()) { return qApp->enqueueError("Produto já agendado!"); }

  // perguntar quantidade
  const int quantTotal = modelViewProdutos.data(row, "caixas").toInt();

  bool ok;

  const int quantAgendar = QInputDialog::getInt(this, "Agendar", "Quantidade de caixas: ", quantTotal, 0, quantTotal, 1, &ok);

  if (quantAgendar == 0 or not ok) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not adicionarProdutoParcial(row, quantAgendar, quantTotal)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }
}

bool WidgetLogisticaAgendarEntrega::adicionarProdutoParcial(const int row, const int quantAgendar, const int quantTotal) {
  if (quantAgendar < quantTotal) {
    if (not quebrarProduto(row, quantAgendar, quantTotal)) { return false; }
  }

  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelViewProdutos.data(row, "idProduto"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando peso do produto: " + query.lastError().text()); }

  const double unCaixa = modelViewProdutos.data(row, "unCaixa").toDouble();
  const double kg = query.value("kgcx").toDouble();

  const int newRow = modelTranspAtual.rowCount();
  modelTranspAtual.insertRow(newRow);

  if (not modelTranspAtual.setData(newRow, "fornecedor", modelViewProdutos.data(row, "fornecedor"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "unCaixa", modelViewProdutos.data(row, "unCaixa"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "formComercial", modelViewProdutos.data(row, "formComercial"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "idVeiculo", ui->itemBoxVeiculo->getId())) { return false; }
  if (not modelTranspAtual.setData(newRow, "idVenda", modelViewProdutos.data(row, "idVenda"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "idVendaProduto", modelViewProdutos.data(row, "idVendaProduto"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "idProduto", modelViewProdutos.data(row, "idProduto"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "produto", modelViewProdutos.data(row, "produto"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "codComercial", modelViewProdutos.data(row, "codComercial"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "un", modelViewProdutos.data(row, "un"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "caixas", quantAgendar)) { return false; }
  if (not modelTranspAtual.setData(newRow, "kg", kg * quantAgendar)) { return false; }
  if (not modelTranspAtual.setData(newRow, "quant", quantAgendar * unCaixa)) { return false; }
  if (not modelTranspAtual.setData(newRow, "status", "ENTREGA AGEND.")) { return false; }

  return true;
}

bool WidgetLogisticaAgendarEntrega::quebrarProduto(const int row, const int quantAgendar, const int quantTotal) {
  // TODO: rename this to 'dividirProduto'
  // TODO: marcar idRelacionado
  // TODO: quebrar linha em pedido_fornecedor tambem para manter 1:1

  SqlRelationalTableModel modelProdutos;
  modelProdutos.setTable("venda_has_produto");
  modelProdutos.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelProdutos.setFilter("idVendaProduto = " + modelViewProdutos.data(row, "idVendaProduto").toString());

  if (not modelProdutos.select()) { return false; }

  const int newRow = modelProdutos.rowCount();
  // NOTE: *quebralinha venda_produto/pedido_fornecedor
  modelProdutos.insertRow(newRow);

  // copiar colunas
  for (int column = 0, columnCount = modelProdutos.columnCount(); column < columnCount; ++column) {
    if (modelProdutos.fieldIndex("idVendaProduto") == column) { continue; }
    if (modelProdutos.fieldIndex("created") == column) { continue; }
    if (modelProdutos.fieldIndex("lastUpdated") == column) { continue; }

    const QVariant value = modelProdutos.data(0, column);

    if (not modelProdutos.setData(newRow, column, value)) { return false; }
  }

  const double unCaixa = modelProdutos.data(0, "unCaixa").toDouble();

  const double proporcao = double(quantAgendar) / quantTotal;
  const double parcial = modelProdutos.data(0, "parcial").toDouble() * proporcao;
  const double parcialDesc = modelProdutos.data(0, "parcialDesc").toDouble() * proporcao;
  const double total = modelProdutos.data(0, "total").toDouble() * proporcao;

  if (not modelProdutos.setData(0, "quant", quantAgendar * unCaixa)) { return false; }
  if (not modelProdutos.setData(0, "caixas", quantAgendar)) { return false; }
  if (not modelProdutos.setData(0, "parcial", parcial)) { return false; }
  if (not modelProdutos.setData(0, "parcialDesc", parcialDesc)) { return false; }
  if (not modelProdutos.setData(0, "total", total)) { return false; }

  // alterar quant, precos, etc da linha nova
  const double proporcaoNovo = double((quantTotal - quantAgendar)) / quantTotal;
  const double parcialNovo = modelProdutos.data(newRow, "parcial").toDouble() * proporcaoNovo;
  const double parcialDescNovo = modelProdutos.data(newRow, "parcialDesc").toDouble() * proporcaoNovo;
  const double totalNovo = modelProdutos.data(newRow, "total").toDouble() * proporcaoNovo;

  if (not modelProdutos.setData(newRow, "quant", (quantTotal - quantAgendar) * unCaixa)) { return false; }
  if (not modelProdutos.setData(newRow, "caixas", quantTotal - quantAgendar)) { return false; }
  if (not modelProdutos.setData(newRow, "parcial", parcialNovo)) { return false; }
  if (not modelProdutos.setData(newRow, "parcialDesc", parcialDescNovo)) { return false; }
  if (not modelProdutos.setData(newRow, "total", totalNovo)) { return false; }

  if (not modelProdutos.submitAll()) { return false; }

  if (not quebrarConsumo(row, proporcao, proporcaoNovo, modelProdutos.query().lastInsertId().toInt())) { return false; }

  return true;
}

bool WidgetLogisticaAgendarEntrega::quebrarConsumo(const int row, const double proporcao, const double proporcaoNovo, const int idVendaProduto) {
  // TODO: rename this to 'dividirConsumo'
  SqlRelationalTableModel modelConsumo;
  modelConsumo.setTable("estoque_has_consumo");
  modelConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelConsumo.setFilter("idVendaProduto = " + modelViewProdutos.data(row, "idVendaProduto").toString());

  if (not modelConsumo.select()) { return false; }

  const int rowConsumo = modelConsumo.rowCount();
  modelConsumo.insertRow(rowConsumo);

  for (int column = 0, columnCount = modelConsumo.columnCount(); column < columnCount; ++column) {
    if (modelConsumo.fieldIndex("idConsumo") == column) { continue; }
    if (modelConsumo.fieldIndex("idVendaProduto") == column) { continue; }
    if (modelConsumo.fieldIndex("created") == column) { continue; }
    if (modelConsumo.fieldIndex("lastUpdated") == column) { continue; }

    const QVariant value = modelConsumo.data(0, column);

    if (not modelConsumo.setData(rowConsumo, column, value)) { return false; }
  }

  // alterar quant, caixas, valor

  const double quantConsumo = modelConsumo.data(0, "quant").toDouble() * proporcao;
  const double caixasConsumo = modelConsumo.data(0, "caixas").toDouble() * proporcao;
  const double valorConsumo = modelConsumo.data(0, "valor").toDouble() * proporcao;

  const double desconto = modelConsumo.data(0, "desconto").toDouble() * proporcao;
  const double vBC = modelConsumo.data(0, "vBC").toDouble() * proporcao;
  const double vICMS = modelConsumo.data(0, "vICMS").toDouble() * proporcao;
  const double vBCST = modelConsumo.data(0, "vBCST").toDouble() * proporcao;
  const double vICMSST = modelConsumo.data(0, "vICMSST").toDouble() * proporcao;
  const double vBCPIS = modelConsumo.data(0, "vBCPIS").toDouble() * proporcao;
  const double vPIS = modelConsumo.data(0, "vPIS").toDouble() * proporcao;
  const double vBCCOFINS = modelConsumo.data(0, "vBCCOFINS").toDouble() * proporcao;
  const double vCOFINS = modelConsumo.data(0, "vCOFINS").toDouble() * proporcao;

  if (not modelConsumo.setData(0, "quant", quantConsumo)) { return false; }
  if (not modelConsumo.setData(0, "caixas", caixasConsumo)) { return false; }
  if (not modelConsumo.setData(0, "valor", valorConsumo)) { return false; }
  if (not modelConsumo.setData(0, "desconto", desconto)) { return false; }
  if (not modelConsumo.setData(0, "vBC", vBC)) { return false; }
  if (not modelConsumo.setData(0, "vICMS", vICMS)) { return false; }
  if (not modelConsumo.setData(0, "vBCST", vBCST)) { return false; }
  if (not modelConsumo.setData(0, "vICMSST", vICMSST)) { return false; }
  if (not modelConsumo.setData(0, "vBCPIS", vBCPIS)) { return false; }
  if (not modelConsumo.setData(0, "vPIS", vPIS)) { return false; }
  if (not modelConsumo.setData(0, "vBCCOFINS", vBCCOFINS)) { return false; }
  if (not modelConsumo.setData(0, "vCOFINS", vCOFINS)) { return false; }

  // alterar linha nova
  const double quantConsumo2 = modelConsumo.data(rowConsumo, "quant").toDouble() * proporcaoNovo;
  const double caixasConsumo2 = modelConsumo.data(rowConsumo, "caixas").toDouble() * proporcaoNovo;
  const double valorConsumo2 = modelConsumo.data(rowConsumo, "valor").toDouble() * proporcaoNovo;

  const double desconto2 = modelConsumo.data(rowConsumo, "desconto").toDouble() * proporcaoNovo;
  const double vBC2 = modelConsumo.data(rowConsumo, "vBC").toDouble() * proporcaoNovo;
  const double vICMS2 = modelConsumo.data(rowConsumo, "vICMS").toDouble() * proporcaoNovo;
  const double vBCST2 = modelConsumo.data(rowConsumo, "vBCST").toDouble() * proporcaoNovo;
  const double vICMSST2 = modelConsumo.data(rowConsumo, "vICMSST").toDouble() * proporcaoNovo;
  const double vBCPIS2 = modelConsumo.data(rowConsumo, "vBCPIS").toDouble() * proporcaoNovo;
  const double vPIS2 = modelConsumo.data(rowConsumo, "vPIS").toDouble() * proporcaoNovo;
  const double vBCCOFINS2 = modelConsumo.data(rowConsumo, "vBCCOFINS").toDouble() * proporcaoNovo;
  const double vCOFINS2 = modelConsumo.data(rowConsumo, "vCOFINS").toDouble() * proporcaoNovo;

  if (not modelConsumo.setData(rowConsumo, "idVendaProduto", idVendaProduto)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "quant", quantConsumo2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "caixas", caixasConsumo2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "valor", valorConsumo2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "desconto", desconto2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vBC", vBC2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vICMS", vICMS2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vBCST", vBCST2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vICMSST", vICMSST2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vBCPIS", vBCPIS2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vPIS", vPIS2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vBCCOFINS", vBCCOFINS2)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "vCOFINS", vCOFINS2)) { return false; }

  if (not modelConsumo.submitAll()) { return false; }

  return true;
}

// TODO: 1'em entrega' deve entrar na categoria 100% estoque?
// TODO: 5adicionar botao para cancelar agendamento (verificar com Anderson)

void WidgetLogisticaAgendarEntrega::on_pushButtonReagendarPedido_clicked() {
  // get idVenda from view_entrega_pendente/modelVendas and set 'novoPrazo'

  const auto list = ui->tableVendas->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  InputDialog input(InputDialog::Tipo::ReagendarPedido);

  if (input.exec() != InputDialog::Accepted) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not reagendar(list, input.getNextDate(), input.getObservacao())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!");
}

bool WidgetLogisticaAgendarEntrega::reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao) {
  QSqlQuery query1;
  query1.prepare("UPDATE venda SET novoPrazoEntrega = :novoPrazoEntrega WHERE idVenda = :idVenda");

  QSqlQuery query2;
  query2.prepare(
      "INSERT INTO venda_has_followup (idVenda, idLoja, idUsuario, tipoOperacao, observacao, dataFollowup) VALUES (:idVenda, :idLoja, :idUsuario, :tipoOperacao, :observacao, :dataFollowup)");

  for (const auto &item : list) {
    query1.bindValue(":novoPrazoEntrega", modelVendas.data(item.row(), "data").toDate().daysTo(dataPrev));
    query1.bindValue(":idVenda", modelVendas.data(item.row(), "idVenda"));

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro atualizando novo prazo: " + query1.lastError().text()); }

    query2.bindValue(":idVenda", modelVendas.data(item.row(), "idVenda"));
    query2.bindValue(":idLoja", UserSession::idLoja());
    query2.bindValue(":idUsuario", UserSession::idUsuario());
    query2.bindValue(":tipoOperacao", "Alteração do prazo de entrega");
    query2.bindValue(":observacao", observacao);
    query2.bindValue(":dataFollowup", QDate::currentDate());

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando followup: " + query2.lastError().text()); }
  }

  return true;
}

void WidgetLogisticaAgendarEntrega::on_tableVendas_doubleClicked(const QModelIndex &index) {
  if (index.column() == modelVendas.record().indexOf("novoPrazoEntrega")) {
    const auto list = ui->tableVendas->selectionModel()->selectedRows();

    if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

    FollowUp *followup = new FollowUp(modelVendas.data(list.first().row(), "idVenda").toString(), FollowUp::Tipo::Venda, this);
    followup->setAttribute(Qt::WA_DeleteOnClose);
    followup->show();
  }
}

// TODO: 5refazer filtros do estoque (casos 'devolvido', 'cancelado', 'em entrega')

void WidgetLogisticaAgendarEntrega::on_pushButtonGerarNFeFutura_clicked() {
  if (not ui->tableProdutos->model()) { return; }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  for (const auto &item : list) {
    if (not modelViewProdutos.data(item.row(), "NFe Fut.").isNull()) { return qApp->enqueueError("Produto já possui nota futura!"); }

    if (modelViewProdutos.data(item.row(), "idConsumo").toInt() == 0) { return qApp->enqueueError("Nem todos os produtos selecionados possuem nota de entrada!"); }
  }

  const QString idVenda = modelViewProdutos.data(list.first().row(), "idVenda").toString();

  if (idVenda.isEmpty()) { return qApp->enqueueError("Erro buscando 'Venda'!"); }

  QList<int> lista;

  for (const auto &item : list) { lista << modelViewProdutos.data(item.row(), "idVendaProduto").toInt(); }

  auto *nfe = new CadastrarNFe(idVenda, lista, CadastrarNFe::Tipo::Futura, this);
  nfe->setAttribute(Qt::WA_DeleteOnClose);

  nfe->show();
}
