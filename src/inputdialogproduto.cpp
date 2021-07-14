#include "inputdialogproduto.h"
#include "ui_inputdialogproduto.h"

#include "application.h"
#include "editdelegate.h"
#include "noeditdelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

InputDialogProduto::InputDialogProduto(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogProduto) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->labelAliquota->hide();
  ui->doubleSpinBoxAliquota->hide();
  ui->labelST->hide();
  ui->doubleSpinBoxST->hide();
  ui->lineEditCodRep->hide();

  ui->dateEditEvento->setDate(qApp->serverDate());
  ui->dateEditProximo->setDate(qApp->serverDate());

  if (tipo == Tipo::GerarCompra) {
    ui->labelEvento->setText("Data compra:");
    ui->labelProximoEvento->setText("Data prevista confirmação:");

    connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogProduto::updateTableData);
  }

  if (tipo == Tipo::Faturamento) {
    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();
    ui->comboBoxST->hide();
    ui->labelAliquota->hide();
    ui->doubleSpinBoxAliquota->hide();
    ui->labelST->hide();
    ui->doubleSpinBoxST->hide();

    ui->labelEvento->setText("Data faturamento:");
  }

  setConnections();

  adjustSize();

  showMaximized();
}

InputDialogProduto::~InputDialogProduto() { delete ui; }

void InputDialogProduto::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogProduto::on_comboBoxST_currentTextChanged, connectionType);
  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogProduto::on_dateEditEvento_dateChanged, connectionType);
  connect(ui->doubleSpinBoxAliquota, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogProduto::on_doubleSpinBoxAliquota_valueChanged, connectionType);
  connect(ui->doubleSpinBoxST, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogProduto::on_doubleSpinBoxST_valueChanged, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogProduto::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->lineEditCodRep, &QLineEdit::textEdited, this, &InputDialogProduto::on_lineEditCodRep_textEdited, connectionType);
}

void InputDialogProduto::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogProduto::on_comboBoxST_currentTextChanged);
  disconnect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogProduto::on_dateEditEvento_dateChanged);
  disconnect(ui->doubleSpinBoxAliquota, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogProduto::on_doubleSpinBoxAliquota_valueChanged);
  disconnect(ui->doubleSpinBoxST, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogProduto::on_doubleSpinBoxST_valueChanged);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogProduto::on_pushButtonSalvar_clicked);
  disconnect(ui->lineEditCodRep, &QLineEdit::textEdited, this, &InputDialogProduto::on_lineEditCodRep_textEdited);
}

void InputDialogProduto::setupTables() {
  if (tipo == Tipo::GerarCompra) { modelPedidoFornecedor.setTable("pedido_fornecedor_has_produto"); }
  if (tipo == Tipo::Faturamento) { modelPedidoFornecedor.setTable("pedido_fornecedor_has_produto2"); }

  modelPedidoFornecedor.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  modelPedidoFornecedor.setHeaderData("idVenda", "Código");
  modelPedidoFornecedor.setHeaderData("fornecedor", "Fornecedor");
  modelPedidoFornecedor.setHeaderData("descricao", "Produto");
  modelPedidoFornecedor.setHeaderData("colecao", "Coleção");
  modelPedidoFornecedor.setHeaderData("caixas", "Caixas");
  modelPedidoFornecedor.setHeaderData("prcUnitario", "R$ Unit.");
  modelPedidoFornecedor.setHeaderData("quant", "Quant.");
  modelPedidoFornecedor.setHeaderData("preco", "Total");
  modelPedidoFornecedor.setHeaderData("un", "Un.");
  modelPedidoFornecedor.setHeaderData("codComercial", "Cód. Com.");
  modelPedidoFornecedor.setHeaderData("obs", "Obs.");
  modelPedidoFornecedor.setHeaderData("aliquotaSt", "Alíquota ST");
  modelPedidoFornecedor.setHeaderData("st", "ST");

  modelPedidoFornecedor.proxyModel = new SortFilterProxyModel(&modelPedidoFornecedor, this);

  ui->table->setModel(&modelPedidoFornecedor);

  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("idVendaProduto1");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("codFornecedor");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("selecionado");

  if (tipo == Tipo::GerarCompra) { ui->table->hideColumn("idPedido1"); }

  if (tipo == Tipo::Faturamento) {
    ui->table->hideColumn("idPedido2");
    ui->table->hideColumn("idPedidoFK");
  }

  ui->table->hideColumn("un2");
  ui->table->hideColumn("kgcx");

  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("codBarras");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("status");
  ui->table->hideColumn("dataPrevCompra");
  ui->table->hideColumn("dataRealCompra");
  ui->table->hideColumn("dataPrevConf");
  ui->table->hideColumn("dataRealConf");
  ui->table->hideColumn("dataPrevFat");
  ui->table->hideColumn("dataRealFat");
  ui->table->hideColumn("dataPrevColeta");
  ui->table->hideColumn("dataRealColeta");
  ui->table->hideColumn("dataPrevReceb");
  ui->table->hideColumn("dataRealReceb");
  ui->table->hideColumn("dataPrevEnt");
  ui->table->hideColumn("dataRealEnt");
  ui->table->hideColumn("formComercial");

  ui->table->setItemDelegate(new NoEditDelegate(this));

  ui->table->setItemDelegateForColumn("obs", new EditDelegate(this));

  if (tipo == Tipo::GerarCompra) {
    ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
    ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));
    ui->table->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(false, this));
    ui->table->setItemDelegateForColumn("quant", new EditDelegate(this));
    ui->table->setItemDelegateForColumn("caixas", new EditDelegate(this));
  }

  if (tipo == Tipo::Faturamento) { ui->table->setItemDelegateForColumn("ordemRepresentacao", new EditDelegate(this)); }

  ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));
}

void InputDialogProduto::setFilter(const QStringList &ids) {
  if (ids.isEmpty()) { throw RuntimeException("Ids vazio!"); }

  QString filter;

  if (tipo == Tipo::GerarCompra) { filter = "`idPedido1` IN (" + ids.join(", ") + ") AND status = 'PENDENTE'"; }
  if (tipo == Tipo::Faturamento) { filter = "idCompra IN (" + ids.join(", ") + ") AND status = 'EM FATURAMENTO'"; }

  if (filter.isEmpty()) { throw RuntimeException("Filtro vazio!"); }

  modelPedidoFornecedor.setFilter(filter);

  modelPedidoFornecedor.select();

  calcularTotal();

  SqlQuery query;
  query.prepare("SELECT aliquotaSt, st, representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  query.bindValue(":razaoSocial", modelPedidoFornecedor.data(0, "fornecedor"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando substituicao tributaria do fornecedor: " + query.lastError().text()); }

  ui->comboBoxST->setCurrentText(query.value("st").toString());
  ui->doubleSpinBoxAliquota->setValue(query.value("aliquotaSt").toDouble());

  if (query.value("representacao").toBool() and tipo == Tipo::Faturamento) { ui->lineEditCodRep->show(); }

  if (tipo == Tipo::GerarCompra) { qApp->enqueueInformation("Ajustar preço e quantidade se necessário!", this); }
}

QDate InputDialogProduto::getDate() const { return ui->dateEditEvento->date(); }

QDate InputDialogProduto::getNextDate() const { return ui->dateEditProximo->date(); }

void InputDialogProduto::updateTableData(const QModelIndex &topLeft) {
  disconnect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogProduto::updateTableData);

  try {
    const QString header = modelPedidoFornecedor.headerData(topLeft.column(), Qt::Horizontal).toString();
    const int row = topLeft.row();

    // TODO: se alterar quant. tem que alterar caixas

    if (header == "Quant." or header == "R$ Unit.") {
      const double preco = modelPedidoFornecedor.data(row, "quant").toDouble() * modelPedidoFornecedor.data(row, "prcUnitario").toDouble();
      modelPedidoFornecedor.setData(row, "preco", preco);
    }

    if (header == "Total") {
      const double preco = modelPedidoFornecedor.data(row, "preco").toDouble() / modelPedidoFornecedor.data(row, "quant").toDouble();
      modelPedidoFornecedor.setData(row, "prcUnitario", preco);
    }
  } catch (std::exception &) {
    connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogProduto::updateTableData);
    throw;
  }

  connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogProduto::updateTableData);

  calcularTotal();
}

void InputDialogProduto::calcularTotal() {
  double total = 0;

  for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { total += modelPedidoFornecedor.data(row, "preco").toDouble(); }

  ui->doubleSpinBoxTotal->setValue(total + ui->doubleSpinBoxST->value());

  ui->doubleSpinBoxST->setMaximum(total * 0.2);
}

void InputDialogProduto::on_pushButtonSalvar_clicked() {
  if (ui->lineEditCodRep->isVisible() and ui->lineEditCodRep->text().isEmpty()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Não preencheu 'Cód. Rep.'. Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Salvar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  cadastrar();

  QDialog::accept();
  close();
}

void InputDialogProduto::cadastrar() {
  qApp->startTransaction("InputDialog::cadastrar");

  if (tipo == Tipo::GerarCompra) {
    SqlQuery queryUpdate;
    queryUpdate.prepare(
        "UPDATE pedido_fornecedor_has_produto2 SET aliquotaSt = :aliquotaSt, st = :st, quant = :quant, caixas = :caixas, prcUnitario = :prcUnitario, preco = :preco WHERE idPedidoFK = :idPedido1");

    for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) {
      queryUpdate.bindValue(":aliquotaSt", modelPedidoFornecedor.data(row, "aliquotaSt"));
      queryUpdate.bindValue(":st", modelPedidoFornecedor.data(row, "st"));
      queryUpdate.bindValue(":quant", modelPedidoFornecedor.data(row, "quant"));
      queryUpdate.bindValue(":caixas", modelPedidoFornecedor.data(row, "caixas"));
      queryUpdate.bindValue(":prcUnitario", modelPedidoFornecedor.data(row, "prcUnitario"));
      queryUpdate.bindValue(":preco", modelPedidoFornecedor.data(row, "preco"));
      queryUpdate.bindValue(":idPedido1", modelPedidoFornecedor.data(row, "idPedido1"));

      if (not queryUpdate.exec()) { throw RuntimeException("Erro copiando dados para tabela 2: " + queryUpdate.lastError().text()); }
    }
  }

  if (tipo == Tipo::Faturamento) {
    for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) {
      if (modelPedidoFornecedor.data(row, "fornecedor").toString() == "ATELIER STACCATO") { modelPedidoFornecedor.setData(row, "status", "ENTREGUE"); }
    }
  }

  modelPedidoFornecedor.submitAll();

  qApp->endTransaction();
}

void InputDialogProduto::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) { ui->dateEditProximo->setDate(date); }
}

void InputDialogProduto::on_doubleSpinBoxAliquota_valueChanged(double aliquota) {
  unsetConnections();

  try {
    [&] {
      double total = 0;

      for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { total += modelPedidoFornecedor.data(row, "preco").toDouble(); }

      const double valueSt = total * aliquota / 100;

      ui->doubleSpinBoxST->setValue(valueSt);

      for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { modelPedidoFornecedor.setData(row, "aliquotaSt", aliquota); }

      ui->doubleSpinBoxTotal->setValue(total + valueSt);
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogProduto::on_doubleSpinBoxST_valueChanged(double valueSt) {
  unsetConnections();

  try {
    [&] {
      double total = 0;

      for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { total += modelPedidoFornecedor.data(row, "preco").toDouble(); }

      const double aliquota = valueSt * 100 / total;

      ui->doubleSpinBoxAliquota->setValue(aliquota);

      for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { modelPedidoFornecedor.setData(row, "aliquotaSt", aliquota); }

      ui->doubleSpinBoxTotal->setValue(total + valueSt);
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogProduto::on_comboBoxST_currentTextChanged(const QString &text) {
  if (text == "Sem ST") {
    ui->doubleSpinBoxST->setValue(0);

    ui->labelAliquota->hide();
    ui->doubleSpinBoxAliquota->hide();
    ui->labelST->hide();
    ui->doubleSpinBoxST->hide();
  }

  if (text == "ST Fornecedor" or text == "ST Loja") {
    ui->labelAliquota->show();
    ui->doubleSpinBoxAliquota->show();
    ui->labelST->show();
    ui->doubleSpinBoxST->show();

    ui->doubleSpinBoxAliquota->setValue(4.68);
  }

  for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { modelPedidoFornecedor.setData(row, "st", text); }
}

void InputDialogProduto::on_lineEditCodRep_textEdited(const QString &text) {
  for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { modelPedidoFornecedor.setData(row, "ordemRepresentacao", text); }
}
