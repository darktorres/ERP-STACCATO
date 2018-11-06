#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QUrl>

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "ui_widgetlogisticaagendarcoleta.h"
#include "usersession.h"
#include "venda.h"
#include "widgetlogisticaagendarcoleta.h"

WidgetLogisticaAgendarColeta::WidgetLogisticaAgendarColeta(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarColeta) { ui->setupUi(this); }

WidgetLogisticaAgendarColeta::~WidgetLogisticaAgendarColeta() { delete ui; }

void WidgetLogisticaAgendarColeta::setConnections() {
  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked);
  connect(ui->pushButtonAgendarColeta, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked);
  connect(ui->pushButtonCancelarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked);
  connect(ui->pushButtonMontarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked);
  connect(ui->pushButtonRemoverProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked);
  connect(ui->tableEstoque, &TableView::entered, this, &WidgetLogisticaAgendarColeta::on_tableEstoque_entered);
}

void WidgetLogisticaAgendarColeta::setupTables() {
  modelEstoque.setTable("view_agendar_coleta");
  modelEstoque.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelEstoque.setHeaderData("dataRealFat", "Data Faturado");
  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("fornecedor", "Fornecedor");
  modelEstoque.setHeaderData("numeroNFe", "NFe");
  modelEstoque.setHeaderData("produto", "Produto");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Cx.");
  modelEstoque.setHeaderData("kgcx", "Kg./Cx.");
  modelEstoque.setHeaderData("idVenda", "Venda");
  modelEstoque.setHeaderData("ordemCompra", "OC");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("prazoEntrega", "Prazo Limite");

  ui->tableEstoque->setModel(new EstoquePrazoProxyModel(&modelEstoque, this));
  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));
  ui->tableEstoque->hideColumn("prazoEntrega");
  ui->tableEstoque->hideColumn("fornecedor");
  ui->tableEstoque->hideColumn("unCaixa");
  ui->tableEstoque->hideColumn("formComercial");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("ordemCompra");

  connect(ui->tableEstoque->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarColeta::calcularPeso);

  // -------------------------------------------------------------------------

  modelTranspAtual.setTable("veiculo_has_produto");
  modelTranspAtual.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTranspAtual.setHeaderData("idEstoque", "Estoque");
  modelTranspAtual.setHeaderData("status", "Status");
  modelTranspAtual.setHeaderData("produto", "Produto");
  modelTranspAtual.setHeaderData("caixas", "Cx.");
  modelTranspAtual.setHeaderData("quant", "Quant.");
  modelTranspAtual.setHeaderData("un", "Un.");
  modelTranspAtual.setHeaderData("codComercial", "Cód. Com.");

  modelTranspAtual.setFilter("0");

  if (not modelTranspAtual.select()) { return; }

  ui->tableTranspAtual->setModel(&modelTranspAtual);
  ui->tableTranspAtual->hideColumn("id");
  ui->tableTranspAtual->hideColumn("idEvento");
  ui->tableTranspAtual->hideColumn("idVeiculo");
  ui->tableTranspAtual->hideColumn("idVendaProduto");
  ui->tableTranspAtual->hideColumn("idCompra");
  ui->tableTranspAtual->hideColumn("idNFeSaida");
  ui->tableTranspAtual->hideColumn("fornecedor");
  ui->tableTranspAtual->hideColumn("idVenda");
  ui->tableTranspAtual->hideColumn("idLoja");
  ui->tableTranspAtual->hideColumn("idProduto");
  ui->tableTranspAtual->hideColumn("obs");
  ui->tableTranspAtual->hideColumn("unCaixa");
  ui->tableTranspAtual->hideColumn("formComercial");
  ui->tableTranspAtual->hideColumn("data");

  // -------------------------------------------------------------------------

  modelTranspAgend.setTable("veiculo_has_produto");
  modelTranspAgend.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");

  modelTranspAgend.setFilter("0");

  if (not modelTranspAgend.select()) { return; }

  ui->tableTranspAgend->setModel(&modelTranspAgend);
  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idVendaProduto");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("fornecedor");
  ui->tableTranspAgend->hideColumn("idVenda");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("unCaixa");
  ui->tableTranspAgend->hideColumn("obs");
  ui->tableTranspAgend->hideColumn("formComercial");
}

void WidgetLogisticaAgendarColeta::calcularPeso() {
  double peso = 0;
  double caixas = 0;

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  for (const auto &item : list) {
    const double kg = modelEstoque.data(item.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(item.row(), "caixas").toDouble();
    peso += kg * caixa;
    caixas += caixa;
  }

  ui->doubleSpinBoxPeso->setValue(peso);
  ui->doubleSpinBoxPeso->setSuffix(" Kg (" + QString::number(caixas) + " Cx.)");
}

void WidgetLogisticaAgendarColeta::updateTables() {
  if (not isSet) {
    ui->frameCaminhao->hide();
    ui->pushButtonCancelarCarga->hide();
    ui->dateTimeEdit->setDate(QDate::currentDate());
    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelEstoque.select()) { return; }

  ui->tableEstoque->resizeColumnsToContents();

  if (not modelTranspAgend.select()) { return; }

  ui->tableTranspAgend->resizeColumnsToContents();
}

void WidgetLogisticaAgendarColeta::tableFornLogistica_activated(const QString &fornecedor) {
  this->fornecedor = fornecedor;

  ui->lineEditBusca->clear();

  montaFiltro();

  if (not modelEstoque.select()) { return; }

  ui->tableEstoque->sortByColumn("prazoEntrega");

  ui->tableEstoque->resizeColumnsToContents();
}

void WidgetLogisticaAgendarColeta::resetTables() { modelIsSet = false; }

void WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked() {
  if (not ui->frameCaminhao->isVisible()) {
    ui->frameCaminhao->setVisible(true);
    ui->pushButtonAgendarColeta->hide();
    ui->pushButtonCancelarCarga->show();
    return;
  }

  if (modelTranspAtual.rowCount() == 0) { return qApp->enqueueError("Nenhum item no veículo!"); }

  QModelIndexList list;

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) { list << modelTranspAtual.index(row, 0); }

  if (not qApp->startTransaction()) { return; }

  if (not processRows(list, ui->dateTimeEdit->date(), true)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Agendado com sucesso!");

  ui->frameCaminhao->setVisible(false);
}

void WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  InputDialog input(InputDialog::Tipo::AgendarColeta, this);
  // TODO: 5colocar qual a linha/id esta sendo trabalhada para o usuario nao se perder ao trocar de janela e voltar

  if (input.exec() != InputDialog::Accepted) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not processRows(list, input.getNextDate())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Agendado com sucesso!");
}

bool WidgetLogisticaAgendarColeta::processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga) {
  QSqlQuery queryTemp;
  queryTemp.prepare("SELECT codComercial FROM estoque WHERE idEstoque = :idEstoque");

  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM COLETA' WHERE idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevColeta = :dataPrevColeta WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND codComercial = :codComercial AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET dataPrevColeta = :dataPrevColeta WHERE idVendaProduto IN (SELECT idVendaProduto FROM estoque_has_consumo WHERE idEstoque = :idEstoque) "
                 "AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  for (const auto &item : list) {
    int idEstoque;
    QString codComercial;

    if (montarCarga) {
      QSqlQuery query;
      query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto");
      query.first();

      const int idEvento = query.value(0).toInt();

      if (not modelTranspAtual.setData(item.row(), "data", dataPrevColeta)) { return false; }
      if (not modelTranspAtual.setData(item.row(), "idEvento", idEvento)) { return false; }

      idEstoque = modelTranspAtual.data(item.row(), "idEstoque").toInt();

      queryTemp.bindValue(":idEstoque", idEstoque);

      if (not queryTemp.exec() or not queryTemp.first()) { return qApp->enqueueError(false, "Erro buscando codComercial: " + queryTemp.lastError().text()); }

      codComercial = queryTemp.value("codComercial").toString();
    } else {
      idEstoque = modelEstoque.data(item.row(), "idEstoque").toInt();
      codComercial = modelEstoque.data(item.row(), "codComercial").toString();
    }

    query1.bindValue(":idEstoque", idEstoque);

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro salvando status no estoque: " + query1.lastError().text()); }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando status no pedido_fornecedor: " + query2.lastError().text()); }

    query3.bindValue(":dataPrevColeta", dataPrevColeta);
    query3.bindValue(":idEstoque", idEstoque);

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro salvando status na venda_produto: " + query3.lastError().text()); }
  }

  return modelTranspAtual.submitAll();
}

void WidgetLogisticaAgendarColeta::on_tableEstoque_entered(const QModelIndex &) { ui->tableEstoque->resizeColumnsToContents(); }

void WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getValue());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados veiculo: " + query.lastError().text()); }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getValue().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { return; }

  ui->tableTranspAgend->resizeColumnsToContents();

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());
}

bool WidgetLogisticaAgendarColeta::adicionarProduto(const QModelIndexList &list) {
  for (const auto &item : list) {
    const double kg = modelEstoque.data(item.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(item.row(), "caixas").toDouble();
    const double peso = kg * caixa;

    const int row = modelTranspAtual.rowCount();
    modelTranspAtual.insertRow(row);

    if (not modelTranspAtual.setData(row, "fornecedor", modelEstoque.data(item.row(), "fornecedor"))) { return false; }
    if (not modelTranspAtual.setData(row, "unCaixa", modelEstoque.data(item.row(), "unCaixa"))) { return false; }
    if (not modelTranspAtual.setData(row, "formComercial", modelEstoque.data(item.row(), "formComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "idVeiculo", ui->itemBoxVeiculo->getValue())) { return false; }
    if (not modelTranspAtual.setData(row, "idEstoque", modelEstoque.data(item.row(), "idEstoque"))) { return false; }
    if (not modelTranspAtual.setData(row, "idProduto", modelEstoque.data(item.row(), "idProduto"))) { return false; }
    if (not modelTranspAtual.setData(row, "produto", modelEstoque.data(item.row(), "produto"))) { return false; }
    if (not modelTranspAtual.setData(row, "codComercial", modelEstoque.data(item.row(), "codComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "un", modelEstoque.data(item.row(), "un"))) { return false; }
    if (not modelTranspAtual.setData(row, "caixas", modelEstoque.data(item.row(), "caixas"))) { return false; }
    if (not modelTranspAtual.setData(row, "kg", peso)) { return false; }
    if (not modelTranspAtual.setData(row, "quant", modelEstoque.data(item.row(), "quant"))) { return false; }
    if (not modelTranspAtual.setData(row, "status", "EM COLETA")) { return false; }
  }

  ui->tableTranspAtual->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked() {
  // TODO: nao deixar adicionar o mesmo item mais de uma vez

  if (ui->itemBoxVeiculo->getValue().isNull()) { return qApp->enqueueError("Deve escolher uma transportadora antes!"); }

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  // TODO: apenas avisar e não bloquear?
  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxCapacidade->value()) { return qApp->enqueueError("Peso maior que capacidade do veículo!"); }

  if (not adicionarProduto(list) and not modelTranspAtual.select()) { return; }
}

void WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked() {
  const auto list = ui->tableTranspAtual->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  for (const auto &item : list) { modelTranspAtual.removeRow(item.row()); }
}

void WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();

  if (not modelTranspAtual.select()) { return; }
}

void WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  if (ACBr acbr; not acbr.gerarDanfe(modelEstoque.data(list.first().row(), "idNFe").toInt())) { return; }
}

void WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getValue().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { return; }

  ui->tableTranspAgend->resizeColumnsToContents();
}

void WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  for (const auto &item : list) {
    const QString idVenda = modelEstoque.data(item.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) { return; }

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
    }
  }
}

void WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled() { montaFiltro(); }

void WidgetLogisticaAgendarColeta::montaFiltro() {
  QStringList filtros;

  const QString filtroFornecedor = "fornecedor = '" + fornecedor + "'";
  if (not fornecedor.isEmpty()) { filtros << filtroFornecedor; }

  //-------------------------------------

  const QString filtroEstoque = "idVenda " + QString(ui->checkBoxEstoque->isChecked() ? "IS NULL" : "IS NOT NULL");
  if (not filtroEstoque.isEmpty()) { filtros << filtroEstoque; }

  //-------------------------------------

  const QString text = ui->lineEditBusca->text();

  const QString filtroBusca = "(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')";
  if (not text.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelEstoque.setFilter(filtros.join(" AND "));

  if (not modelEstoque.select()) { return; }
}

// TODO: 1poder marcar nota de entrada como cancelada (talvez direto na tela de nfe's e retirar dos fluxos os estoques?)
// TODO: 5importar nota de amostra nesta tela dizendo para qual loja ela vai e no final do fluxo gerar nota de
// tranferencia
