#include "widgetlogisticaagendarcoleta.h"
#include "ui_widgetlogisticaagendarcoleta.h"

#include "acbrlib.h"
#include "application.h"
#include "doubledelegate.h"
#include "estoqueprazoproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "user.h"
#include "venda.h"

#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSqlError>
#include <QStandardItemModel>
#include <QUrl>

WidgetLogisticaAgendarColeta::WidgetLogisticaAgendarColeta(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarColeta) { ui->setupUi(this); }

WidgetLogisticaAgendarColeta::~WidgetLogisticaAgendarColeta() { delete ui; }

void WidgetLogisticaAgendarColeta::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged, connectionType);
  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaAgendarColeta::delayFiltro, connectionType);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked, connectionType);
  connect(ui->pushButtonAgendarColeta, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked, connectionType);
  connect(ui->pushButtonCancelarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked, connectionType);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonMontarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked, connectionType);
  connect(ui->pushButtonRemoverProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked, connectionType);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked, connectionType);
}

void WidgetLogisticaAgendarColeta::setupTables() {
  modelEstoque.setTable("view_agendar_coleta");

  modelEstoque.setSort("prazoEntrega");

  modelEstoque.setHeaderData("prazoEntrega", "Prazo Limite");
  modelEstoque.setHeaderData("dataRealFat", "Data Faturado");
  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");
  modelEstoque.setHeaderData("numeroNFe", "NFe");
  modelEstoque.setHeaderData("idVenda", "Venda");
  modelEstoque.setHeaderData("ordemCompra", "OC");
  modelEstoque.setHeaderData("produto", "Produto");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Cx.");
  modelEstoque.setHeaderData("kgcx", "Kg./Cx.");

  modelEstoque.proxyModel = new EstoquePrazoProxyModel(&modelEstoque, this);

  ui->tableEstoque->setModel(&modelEstoque);

  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));

  ui->tableEstoque->hideColumn("fornecedor");
  ui->tableEstoque->hideColumn("quantCaixa");
  ui->tableEstoque->hideColumn("formComercial");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("idNFe");

  connect(ui->tableEstoque->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarColeta::calcularPeso);

  // -------------------------------------------------------------------------

  modelTranspAtual.setTable("veiculo_has_produto");

  modelTranspAtual.setHeaderData("idEstoque", "Estoque");
  modelTranspAtual.setHeaderData("status", "Status");
  modelTranspAtual.setHeaderData("produto", "Produto");
  modelTranspAtual.setHeaderData("caixas", "Cx.");
  modelTranspAtual.setHeaderData("kg", "Kg");
  modelTranspAtual.setHeaderData("quant", "Quant.");
  modelTranspAtual.setHeaderData("un", "Un.");
  modelTranspAtual.setHeaderData("codComercial", "Cód. Com.");

  ui->tableTranspAtual->setModel(&modelTranspAtual);

  ui->tableTranspAtual->hideColumn("fotoEntrega");
  ui->tableTranspAtual->hideColumn("id");
  ui->tableTranspAtual->hideColumn("idEvento");
  ui->tableTranspAtual->hideColumn("idVeiculo");
  ui->tableTranspAtual->hideColumn("idVendaProduto1");
  ui->tableTranspAtual->hideColumn("idVendaProduto2");
  ui->tableTranspAtual->hideColumn("idCompra");
  ui->tableTranspAtual->hideColumn("idNFeSaida");
  ui->tableTranspAtual->hideColumn("fornecedor");
  ui->tableTranspAtual->hideColumn("idVenda");
  ui->tableTranspAtual->hideColumn("idLoja");
  ui->tableTranspAtual->hideColumn("idProduto");
  ui->tableTranspAtual->hideColumn("obs");
  ui->tableTranspAtual->hideColumn("quantCaixa");
  ui->tableTranspAtual->hideColumn("formComercial");
  ui->tableTranspAtual->hideColumn("data");

  // -------------------------------------------------------------------------

  modelTranspAgend.setTable("veiculo_has_produto");

  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("kg", "Kg");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");

  ui->tableTranspAgend->setModel(&modelTranspAgend);

  ui->tableTranspAgend->hideColumn("fotoEntrega");
  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idVendaProduto1");
  ui->tableTranspAgend->hideColumn("idVendaProduto2");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("fornecedor");
  ui->tableTranspAgend->hideColumn("idVenda");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("quantCaixa");
  ui->tableTranspAgend->hideColumn("obs");
  ui->tableTranspAgend->hideColumn("formComercial");
}

void WidgetLogisticaAgendarColeta::calcularPeso() {
  double peso = 0;
  double caixas = 0;

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  for (const auto &index : list) {
    const double kg = modelEstoque.data(index.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(index.row(), "caixas").toDouble();
    peso += kg * caixa;
    caixas += caixa;
  }

  ui->doubleSpinBoxPeso->setValue(peso);
  ui->doubleSpinBoxPeso->setSuffix(" Kg (" + QString::number(caixas) + " Cx.)");
}

void WidgetLogisticaAgendarColeta::delayFiltro() { timer.start(500); }

void WidgetLogisticaAgendarColeta::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    ui->frameCaminhao->hide();
    ui->pushButtonCancelarCarga->hide();
    ui->dateTimeEdit->setDate(qApp->serverDate());
    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelEstoque.select();

  modelTranspAgend.select();
}

void WidgetLogisticaAgendarColeta::tableFornLogistica_clicked(const QString &fornecedor) {
  this->fornecedor = fornecedor;

  ui->lineEditBusca->clear();

  montaFiltro();
}

void WidgetLogisticaAgendarColeta::resetTables() { modelIsSet = false; }

void WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked() {
  if (not ui->frameCaminhao->isVisible()) {
    ui->frameCaminhao->show();
    ui->pushButtonAgendarColeta->hide();
    ui->pushButtonCancelarCarga->show();
    return;
  }

  if (modelTranspAtual.rowCount() == 0) { throw RuntimeError("Nenhum item no veículo!", this); }

  QModelIndexList list;

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) { list << modelTranspAtual.index(row, 0); }

  qApp->startTransaction("WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga");

  processRows(list, ui->dateTimeEdit->date(), true);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Agendado com sucesso!", this);

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();
}

void WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarColeta, this);
  // TODO: 5colocar qual a linha/id esta sendo trabalhada para o usuario nao se perder ao trocar de janela e voltar

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta");

  processRows(list, input.getNextDate());

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Agendado com sucesso!", this);
}

void WidgetLogisticaAgendarColeta::processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga) {
  SqlQuery queryTemp;
  queryTemp.prepare("SELECT codComercial FROM estoque WHERE idEstoque = :idEstoque");

  SqlQuery query2;
  query2.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE status = 'EM COLETA' AND idPedido2 IN (SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  SqlQuery query3;
  query3.prepare("UPDATE venda_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE status = 'EM COLETA' AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = "
                 ":idEstoque)");

  for (const auto &index : list) {
    int idEstoque;
    QString codComercial;

    if (montarCarga) {
      SqlQuery query;
      query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto");
      query.first();

      const int idEvento = query.value(0).toInt();

      modelTranspAtual.setData(index.row(), "data", dataPrevColeta);
      modelTranspAtual.setData(index.row(), "idEvento", idEvento);

      idEstoque = modelTranspAtual.data(index.row(), "idEstoque").toInt();

      queryTemp.bindValue(":idEstoque", idEstoque);

      if (not queryTemp.exec() or not queryTemp.first()) { throw RuntimeException("Erro buscando codComercial: " + queryTemp.lastError().text()); }

      codComercial = queryTemp.value("codComercial").toString();
    } else {
      idEstoque = modelEstoque.data(index.row(), "idEstoque").toInt();
      codComercial = modelEstoque.data(index.row(), "codComercial").toString();
    }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query2.lastError().text()); }

    query3.bindValue(":dataPrevColeta", dataPrevColeta);
    query3.bindValue(":idEstoque", idEstoque);

    if (not query3.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query3.lastError().text()); }
  }

  modelTranspAtual.submitAll();
}

void WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged(const QString &) {
  SqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getId());

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando dados veiculo: " + query.lastError().text(), this); }

  modelTranspAtual.select();

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  modelTranspAgend.select();

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());
}

void WidgetLogisticaAgendarColeta::adicionarProduto(const QModelIndexList &list) {
  for (const auto &index : list) {
    const double kg = modelEstoque.data(index.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(index.row(), "caixas").toDouble();
    const double peso = kg * caixa;

    const int row = modelTranspAtual.insertRowAtEnd();

    modelTranspAtual.setData(row, "fornecedor", modelEstoque.data(index.row(), "fornecedor"));
    modelTranspAtual.setData(row, "quantCaixa", modelEstoque.data(index.row(), "quantCaixa"));
    modelTranspAtual.setData(row, "formComercial", modelEstoque.data(index.row(), "formComercial"));
    modelTranspAtual.setData(row, "idVeiculo", ui->itemBoxVeiculo->getId());
    modelTranspAtual.setData(row, "idEstoque", modelEstoque.data(index.row(), "idEstoque"));
    modelTranspAtual.setData(row, "idProduto", modelEstoque.data(index.row(), "idProduto"));
    modelTranspAtual.setData(row, "produto", modelEstoque.data(index.row(), "produto"));
    modelTranspAtual.setData(row, "codComercial", modelEstoque.data(index.row(), "codComercial"));
    modelTranspAtual.setData(row, "un", modelEstoque.data(index.row(), "un"));
    modelTranspAtual.setData(row, "caixas", modelEstoque.data(index.row(), "caixas"));
    modelTranspAtual.setData(row, "kg", peso);
    modelTranspAtual.setData(row, "quant", modelEstoque.data(index.row(), "quant"));
    modelTranspAtual.setData(row, "status", "EM COLETA");
  }
}

void WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { throw RuntimeError("Deve escolher uma transportadora antes!", this); }

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxCapacidade->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!", this); }

  for (const auto &index : list) {
    const auto listMatch = modelTranspAtual.match("idEstoque", modelEstoque.data(index.row(), "idEstoque"), 1, Qt::MatchExactly);

    if (not listMatch.isEmpty()) { throw RuntimeError("Item '" + modelEstoque.data(index.row(), "produto").toString() + "' já inserido!", this); }
  }

  adicionarProduto(list);
}

void WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked() {
  const auto selection = ui->tableTranspAtual->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  modelTranspAtual.removeSelection(selection);
}

void WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();

  modelTranspAtual.select();
}

void WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!"); }

  ACBrLib::gerarDanfe(modelEstoque.data(list.first().row(), "idNFe").toInt());
}

void WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");
}

void WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  for (const auto &index : list) {
    const QString idVenda = modelEstoque.data(index.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) { return; }

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
      venda->show();
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

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  // TODO: OC não está aparecendo nessa tabela
  const QString filtroBusca = "(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')";

  if (not text.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelEstoque.setFilter(filtros.join(" AND "));
}

void WidgetLogisticaAgendarColeta::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tableEstoque->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeException("Nenhuma linha selecionada!"); }

  const QString idEstoque = modelEstoque.data(selection.first().row(), "idEstoque").toString();

  FollowUp *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: 5importar nota de amostra nesta tela dizendo para qual loja ela vai e no final do fluxo gerar nota de
// tranferencia
