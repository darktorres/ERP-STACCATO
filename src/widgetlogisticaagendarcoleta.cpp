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
#include "doubledelegate.h"
#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "ui_widgetlogisticaagendarcoleta.h"
#include "usersession.h"
#include "venda.h"
#include "widgetlogisticaagendarcoleta.h"

WidgetLogisticaAgendarColeta::WidgetLogisticaAgendarColeta(QWidget *parent) : Widget(parent), ui(new Ui::WidgetLogisticaAgendarColeta) {
  ui->setupUi(this);

  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled);
  connect(ui->checkBoxSul, &QCheckBox::toggled, this, &WidgetLogisticaAgendarColeta::on_checkBoxSul_toggled);
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

  ui->frameCaminhao->hide();
  ui->pushButtonCancelarCarga->hide();

  ui->dateTimeEdit->setDate(QDate::currentDate());
}

WidgetLogisticaAgendarColeta::~WidgetLogisticaAgendarColeta() { delete ui; }

void WidgetLogisticaAgendarColeta::setupTables() {
  // REFAC: refactor this to not select in here

  modelEstoque.setTable("view_agendar_coleta");
  modelEstoque.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelEstoque.setFilter("0");

  if (not modelEstoque.select()) {
    emit errorSignal("Erro lendo tabela estoque: " + modelEstoque.lastError().text());
    return;
  }

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

  //

  modelTransp.setTable("veiculo_has_produto");
  modelTransp.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTransp.setHeaderData("idEstoque", "Estoque");
  modelTransp.setHeaderData("status", "Status");
  modelTransp.setHeaderData("produto", "Produto");
  modelTransp.setHeaderData("caixas", "Cx.");
  modelTransp.setHeaderData("quant", "Quant.");
  modelTransp.setHeaderData("un", "Un.");
  modelTransp.setHeaderData("codComercial", "Cód. Com.");

  modelTransp.setFilter("0");

  if (not modelTransp.select()) {
    emit errorSignal("Erro lendo tabela transportadora: " + modelTransp.lastError().text());
    return;
  }

  ui->tableTransp->setModel(&modelTransp);
  ui->tableTransp->hideColumn("id");
  ui->tableTransp->hideColumn("idEvento");
  ui->tableTransp->hideColumn("idVeiculo");
  ui->tableTransp->hideColumn("idVendaProduto");
  ui->tableTransp->hideColumn("idCompra");
  ui->tableTransp->hideColumn("idNFeSaida");
  ui->tableTransp->hideColumn("fornecedor");
  ui->tableTransp->hideColumn("idVenda");
  ui->tableTransp->hideColumn("idLoja");
  ui->tableTransp->hideColumn("idProduto");
  ui->tableTransp->hideColumn("obs");
  ui->tableTransp->hideColumn("unCaixa");
  ui->tableTransp->hideColumn("formComercial");
  ui->tableTransp->hideColumn("data");

  //

  modelTransp2.setTable("veiculo_has_produto");
  modelTransp2.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTransp2.setHeaderData("idEstoque", "Estoque");
  modelTransp2.setHeaderData("data", "Agendado");
  modelTransp2.setHeaderData("status", "Status");
  modelTransp2.setHeaderData("produto", "Produto");
  modelTransp2.setHeaderData("caixas", "Cx.");
  modelTransp2.setHeaderData("quant", "Quant.");
  modelTransp2.setHeaderData("un", "Un.");
  modelTransp2.setHeaderData("codComercial", "Cód. Com.");

  modelTransp2.setFilter("0");

  if (not modelTransp2.select()) {
    emit errorSignal("Erro lendo tabela transportadora: " + modelTransp2.lastError().text());
    return;
  }

  ui->tableTransp2->setModel(&modelTransp2);
  ui->tableTransp2->hideColumn("id");
  ui->tableTransp2->hideColumn("idEvento");
  ui->tableTransp2->hideColumn("idVeiculo");
  ui->tableTransp2->hideColumn("idVendaProduto");
  ui->tableTransp2->hideColumn("idCompra");
  ui->tableTransp2->hideColumn("idNFeSaida");
  ui->tableTransp2->hideColumn("fornecedor");
  ui->tableTransp2->hideColumn("idVenda");
  ui->tableTransp2->hideColumn("idLoja");
  ui->tableTransp2->hideColumn("idProduto");
  ui->tableTransp2->hideColumn("unCaixa");
  ui->tableTransp2->hideColumn("obs");
  ui->tableTransp2->hideColumn("formComercial");
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

bool WidgetLogisticaAgendarColeta::updateTables() {
  if (modelEstoque.tableName().isEmpty()) {
    setupTables();

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));

    connect(ui->tableEstoque->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarColeta::calcularPeso);
  }

  montaFiltro();

  if (not modelEstoque.select()) {
    emit errorSignal("Erro lendo tabela estoque: " + modelEstoque.lastError().text());
    return false;
  }

  ui->tableEstoque->resizeColumnsToContents();

  if (not modelTransp2.select()) {
    emit errorSignal("Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return false;
  }

  ui->tableTransp2->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaAgendarColeta::tableFornLogistica_activated(const QString &fornecedor) {
  this->fornecedor = fornecedor;

  ui->lineEditBusca->clear();

  montaFiltro();

  if (not modelEstoque.select()) {
    emit errorSignal("Erro lendo tabela estoque: " + modelEstoque.lastError().text());
    return;
  }

  ui->tableEstoque->sortByColumn("prazoEntrega");

  ui->tableEstoque->resizeColumnsToContents();
}

void WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked() {
  if (not ui->frameCaminhao->isVisible()) {
    ui->frameCaminhao->setVisible(true);
    ui->pushButtonAgendarColeta->hide();
    ui->pushButtonCancelarCarga->show();
    return;
  }

  if (modelTransp.rowCount() == 0) {
    emit errorSignal("Nenhum item no veículo!");
    return;
  }

  QModelIndexList list;

  for (int row = 0; row < modelTransp.rowCount(); ++row) list << modelTransp.index(row, 0);

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not processRows(list, ui->dateTimeEdit->date(), true)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Agendado com sucesso!");

  ui->frameCaminhao->setVisible(false);
}

void WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  InputDialog input(InputDialog::Tipo::AgendarColeta, this);
  // TODO: 5colocar qual a linha/id esta sendo trabalhada para o usuario nao se perder ao trocar de janela e voltar

  if (input.exec() != InputDialog::Accepted) { return; }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not processRows(list, input.getNextDate())) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Agendado com sucesso!");
}

bool WidgetLogisticaAgendarColeta::processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga) {
  QSqlQuery queryTemp;
  queryTemp.prepare("SELECT codComercial FROM estoque WHERE idEstoque = :idEstoque");

  QSqlQuery query1;
  query1.prepare("UPDATE estoque SET status = 'EM COLETA' WHERE idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevColeta = :dataPrevColeta WHERE idCompra IN (SELECT idCompra FROM estoque_has_compra WHERE idEstoque = :idEstoque) "
                 "AND codComercial = :codComercial");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET dataPrevColeta = :dataPrevColeta WHERE idVendaProduto IN (SELECT idVendaProduto FROM estoque_has_consumo WHERE idEstoque = :idEstoque)");

  for (const auto &item : list) {
    int idEstoque;
    QString codComercial;

    if (montarCarga) {
      QSqlQuery query;
      query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto");
      query.first();

      const int idEvento = query.value(0).toInt();

      if (not modelTransp.setData(item.row(), "data", dataPrevColeta)) { return false; }
      if (not modelTransp.setData(item.row(), "idEvento", idEvento)) { return false; }

      idEstoque = modelTransp.data(item.row(), "idEstoque").toInt();

      queryTemp.bindValue(":idEstoque", idEstoque);

      if (not queryTemp.exec() or not queryTemp.first()) {
        emit errorSignal("Erro buscando codComercial: " + queryTemp.lastError().text());
        return false;
      }

      codComercial = queryTemp.value("codComercial").toString();
    } else {
      idEstoque = modelEstoque.data(item.row(), "idEstoque").toInt();
      codComercial = modelEstoque.data(item.row(), "codComercial").toString();
    }

    query1.bindValue(":idEstoque", idEstoque);

    if (not query1.exec()) {
      emit errorSignal("Erro salvando status no estoque: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);
    query2.bindValue(":codComercial", codComercial);

    if (not query2.exec()) {
      emit errorSignal("Erro salvando status no pedido_fornecedor: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":dataPrevColeta", dataPrevColeta);
    query3.bindValue(":idEstoque", idEstoque);

    if (not query3.exec()) {
      emit errorSignal("Erro salvando status na venda_produto: " + query3.lastError().text());
      return false;
    }
  }

  if (not modelTransp.submitAll()) {
    emit errorSignal("Erro salvando carga veiculo: " + modelTransp.lastError().text());
    return false;
  }

  return true;
}

void WidgetLogisticaAgendarColeta::on_tableEstoque_entered(const QModelIndex &) { ui->tableEstoque->resizeColumnsToContents(); }

void WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getValue());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando dados veiculo: " + query.lastError().text());
    return;
  }

  modelTransp2.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getValue().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  if (not modelTransp2.select()) {
    emit errorSignal("Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return;
  }

  ui->tableTransp2->resizeColumnsToContents();

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());
}

bool WidgetLogisticaAgendarColeta::adicionarProduto(const QModelIndexList &list) {
  for (const auto &item : list) {
    const int row = modelTransp.rowCount();
    modelTransp.insertRow(row);

    //

    const double kg = modelEstoque.data(item.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(item.row(), "caixas").toDouble();
    const double peso = kg * caixa;

    //

    if (not modelTransp.setData(row, "fornecedor", modelEstoque.data(item.row(), "fornecedor"))) { return false; }
    if (not modelTransp.setData(row, "unCaixa", modelEstoque.data(item.row(), "unCaixa"))) { return false; }
    if (not modelTransp.setData(row, "formComercial", modelEstoque.data(item.row(), "formComercial"))) { return false; }
    if (not modelTransp.setData(row, "idVeiculo", ui->itemBoxVeiculo->getValue())) { return false; }
    if (not modelTransp.setData(row, "idEstoque", modelEstoque.data(item.row(), "idEstoque"))) { return false; }
    if (not modelTransp.setData(row, "idProduto", modelEstoque.data(item.row(), "idProduto"))) { return false; }
    if (not modelTransp.setData(row, "produto", modelEstoque.data(item.row(), "produto"))) { return false; }
    if (not modelTransp.setData(row, "codComercial", modelEstoque.data(item.row(), "codComercial"))) { return false; }
    if (not modelTransp.setData(row, "un", modelEstoque.data(item.row(), "un"))) { return false; }
    if (not modelTransp.setData(row, "caixas", modelEstoque.data(item.row(), "caixas"))) { return false; }
    if (not modelTransp.setData(row, "kg", peso)) { return false; }
    if (not modelTransp.setData(row, "quant", modelEstoque.data(item.row(), "quant"))) { return false; }
    if (not modelTransp.setData(row, "status", "EM COLETA")) { return false; }
  }

  ui->tableTransp->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getValue().isNull()) {
    emit errorSignal("Deve escolher uma transportadora antes!");
    return;
  }

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxCapacidade->value()) {
    emit errorSignal("Peso maior que capacidade do veículo!");
    return;
  }

  if (not adicionarProduto(list)) modelTransp.select();
}

void WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked() {
  const auto list = ui->tableTransp->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  for (const auto &item : list) modelTransp.removeRow(item.row());

  if (not modelTransp.submitAll()) {
    emit errorSignal("Erro comunicando com banco de dados: " + modelTransp.lastError().text());
    return;
  }
}

void WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();

  if (not modelTransp.select()) emit errorSignal("Erro lendo tabela: " + modelTransp.lastError().text());
}

void WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  if (not ACBr::gerarDanfe(modelEstoque.data(list.first().row(), "idNFe").toInt())) { return; }
}

void WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTransp2.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getValue().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");

  if (not modelTransp2.select()) {
    emit errorSignal("Erro lendo tabela veiculo: " + modelTransp2.lastError().text());
    return;
  }

  ui->tableTransp2->resizeColumnsToContents();
}

void WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

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

void WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled(const bool checked) {
  if (checked) { ui->checkBoxSul->setChecked(false); }
  montaFiltro();
}

void WidgetLogisticaAgendarColeta::on_checkBoxSul_toggled(const bool checked) {
  if (checked) { ui->checkBoxEstoque->setChecked(false); }
  montaFiltro();
}

void WidgetLogisticaAgendarColeta::montaFiltro() {
  QString filtro;
  const QString filterFornecedor = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";
  filtro += filterFornecedor;
  const QString filterEstoque = "idVenda " + QString(ui->checkBoxEstoque->isChecked() ? "IS NULL" : "IS NOT NULL");
  filtro += QString(filtro.isEmpty() ? "" : " AND ") + filterEstoque;
  const QString filterSul = ui->checkBoxEstoque->isChecked() ? "" : "idVenda " + QString(ui->checkBoxSul->isChecked() ? "LIKE 'CAMB%'" : "NOT LIKE 'CAMB%'");
  filtro += filterSul.isEmpty() ? "" : QString(filtro.isEmpty() ? "" : " AND ") + filterSul;

  if (const QString text = ui->lineEditBusca->text(); not text.isEmpty()) {
    const QString filterText = "(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')";
    filtro = filterText;
  }

  modelEstoque.setFilter(filtro);

  if (not modelEstoque.select()) { emit errorSignal("Erro: " + modelEstoque.lastError().text()); }
}

// TODO: 1poder marcar nota de entrada como cancelada (talvez direto na tela de nfe's e retirar dos fluxos os estoques?)
// TODO: 5importar nota de amostra nesta tela dizendo para qual loja ela vai e no final do fluxo gerar nota de
// tranferencia
