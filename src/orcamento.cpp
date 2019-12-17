#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QSqlError>
#include <QtMath>

#include "application.h"
#include "baixaorcamento.h"
#include "cadastrocliente.h"
#include "cadastroprofissional.h"
#include "calculofrete.h"
#include "doubledelegate.h"
#include "excel.h"
#include "impressao.h"
#include "logindialog.h"
#include "orcamento.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "searchdialogproxymodel.h"
#include "ui_orcamento.h"
#include "usersession.h"
#include "venda.h"

Orcamento::Orcamento(QWidget *parent) : RegisterDialog("orcamento", "idOrcamento", parent), ui(new Ui::Orcamento) {
  ui->setupUi(this);

  setupTables();

  connect(ui->pushButtonCalculadora, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalculadora_clicked);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  ui->itemBoxCliente->setRegisterDialog(new CadastroCliente(this));
  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxConsultor->setSearchDialog(SearchDialog::vendedor(this));
  ui->itemBoxEndereco->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxProduto->setSearchDialog(SearchDialog::produto(false, false, false, false, this));
  ui->itemBoxProfissional->setRegisterDialog(new CadastroProfissional(this));
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(true, this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));

  setupMapper();
  newRegister();

  ui->labelConsultor->hide();
  ui->itemBoxConsultor->hide();
  ui->itemBoxConsultor->setReadOnlyItemBox(true);

  if (UserSession::tipoUsuario() == "ADMINISTRADOR" or UserSession::tipoUsuario() == "ADMINISTRATIVO") {
    ui->dataEmissao->setReadOnly(false);
    ui->dataEmissao->setCalendarPopup(true);
  }

  if (UserSession::tipoUsuario() == "VENDEDOR") { buscarParametrosFrete(); }

  on_checkBoxRepresentacao_toggled(false);

  ui->labelBaixa->hide();
  ui->plainTextEditBaixa->hide();
  ui->labelReplicaDe->hide();
  ui->labelReplicadoEm->hide();
  ui->lineEditReplicaDe->hide();
  ui->lineEditReplicadoEm->hide();

  setConnections();

  ui->pushButtonCalcularFrete->hide();
}

Orcamento::~Orcamento() { delete ui; }

void Orcamento::show() { RegisterDialog::show(); }

void Orcamento::on_tableProdutos_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoItem(); }

  if (isReadOnly) { return; }

  ui->pushButtonAtualizarItem->show();
  ui->pushButtonAdicionarItem->hide();
  ui->pushButtonRemoverItem->show();
  mapperItem.setCurrentModelIndex(index);
  currentRowItem = index.row();
}

void Orcamento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxFreteManual, &QCheckBox::clicked, this, &Orcamento::on_checkBoxFreteManual_clicked, connectionType);
  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &Orcamento::on_checkBoxRepresentacao_toggled, connectionType);
  connect(ui->dataEmissao, &QDateTimeEdit::dateChanged, this, &Orcamento::on_dataEmissao_dateChanged, connectionType);
  connect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxCaixas_valueChanged, connectionType);
  connect(ui->doubleSpinBoxDesconto, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxDesconto_valueChanged, connectionType);
  connect(ui->doubleSpinBoxDescontoGlobal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxDescontoGlobal_valueChanged, connectionType);
  connect(ui->doubleSpinBoxDescontoGlobalReais, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxDescontoGlobalReais_valueChanged, connectionType);
  connect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxFrete_valueChanged, connectionType);
  connect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxQuant_valueChanged, connectionType);
  connect(ui->doubleSpinBoxTotal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxTotal_valueChanged, connectionType);
  connect(ui->doubleSpinBoxTotalItem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxTotalItem_valueChanged, connectionType);
  connect(ui->itemBoxCliente, &ItemBox::textChanged, this, &Orcamento::on_itemBoxCliente_textChanged, connectionType);
  connect(ui->itemBoxProduto, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProduto_idChanged, connectionType);
  connect(ui->itemBoxVendedor, &ItemBox::textChanged, this, &Orcamento::on_itemBoxVendedor_textChanged, connectionType);
  connect(ui->pushButtonAdicionarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAdicionarItem_clicked, connectionType);
  connect(ui->pushButtonApagarOrc, &QPushButton::clicked, this, &Orcamento::on_pushButtonApagarOrc_clicked, connectionType);
  connect(ui->pushButtonAtualizarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarItem_clicked, connectionType);
  connect(ui->pushButtonAtualizarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarOrcamento_clicked, connectionType);
  connect(ui->pushButtonCadastrarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonCadastrarOrcamento_clicked, connectionType);
  connect(ui->pushButtonCalcularFrete, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalcularFrete_clicked, connectionType);
  connect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarExcel_clicked, connectionType);
  connect(ui->pushButtonGerarVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarVenda_clicked, connectionType);
  connect(ui->pushButtonImprimir, &QPushButton::clicked, this, &Orcamento::on_pushButtonImprimir_clicked, connectionType);
  connect(ui->pushButtonRemoverItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonRemoverItem_clicked, connectionType);
  connect(ui->pushButtonReplicar, &QPushButton::clicked, this, &Orcamento::on_pushButtonReplicar_clicked, connectionType);
  connect(ui->tableProdutos, &TableView::clicked, this, &Orcamento::on_tableProdutos_clicked, connectionType);
}

void Orcamento::unsetConnections() {
  disconnect(ui->checkBoxFreteManual, &QCheckBox::clicked, this, &Orcamento::on_checkBoxFreteManual_clicked);
  disconnect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &Orcamento::on_checkBoxRepresentacao_toggled);
  disconnect(ui->dataEmissao, &QDateTimeEdit::dateChanged, this, &Orcamento::on_dataEmissao_dateChanged);
  disconnect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxCaixas_valueChanged);
  disconnect(ui->doubleSpinBoxDesconto, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxDesconto_valueChanged);
  disconnect(ui->doubleSpinBoxDescontoGlobal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxDescontoGlobal_valueChanged);
  disconnect(ui->doubleSpinBoxDescontoGlobalReais, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxDescontoGlobalReais_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxQuant_valueChanged);
  disconnect(ui->doubleSpinBoxTotal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxTotal_valueChanged);
  disconnect(ui->doubleSpinBoxTotalItem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Orcamento::on_doubleSpinBoxTotalItem_valueChanged);
  disconnect(ui->itemBoxCliente, &ItemBox::textChanged, this, &Orcamento::on_itemBoxCliente_textChanged);
  disconnect(ui->itemBoxProduto, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProduto_idChanged);
  disconnect(ui->itemBoxVendedor, &ItemBox::textChanged, this, &Orcamento::on_itemBoxVendedor_textChanged);
  disconnect(ui->pushButtonAdicionarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAdicionarItem_clicked);
  disconnect(ui->pushButtonApagarOrc, &QPushButton::clicked, this, &Orcamento::on_pushButtonApagarOrc_clicked);
  disconnect(ui->pushButtonAtualizarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarItem_clicked);
  disconnect(ui->pushButtonAtualizarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarOrcamento_clicked);
  disconnect(ui->pushButtonCadastrarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonCadastrarOrcamento_clicked);
  disconnect(ui->pushButtonCalcularFrete, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalcularFrete_clicked);
  disconnect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarExcel_clicked);
  disconnect(ui->pushButtonGerarVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarVenda_clicked);
  disconnect(ui->pushButtonImprimir, &QPushButton::clicked, this, &Orcamento::on_pushButtonImprimir_clicked);
  disconnect(ui->pushButtonRemoverItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonRemoverItem_clicked);
  disconnect(ui->pushButtonReplicar, &QPushButton::clicked, this, &Orcamento::on_pushButtonReplicar_clicked);
  disconnect(ui->tableProdutos, &TableView::clicked, this, &Orcamento::on_tableProdutos_clicked);
}

bool Orcamento::viewRegister() {
  unsetConnections();

  auto load = [&] {
    if (not RegisterDialog::viewRegister()) { return false; }

    modelItem.setFilter("idOrcamento = '" + data(0, "idOrcamento").toString() + "'");

    if (not modelItem.select()) { return false; }

    if (not buscarParametrosFrete()) { return false; }

    novoItem();

    const QString status = data("status").toString();

    if (status == "FECHADO" or status == "PERDIDO") { ui->pushButtonApagarOrc->hide(); }

    if (status == "PERDIDO" or status == "CANCELADO") {
      ui->labelBaixa->show();
      ui->plainTextEditBaixa->show();
    }

    if (status == "ATIVO") { ui->pushButtonReplicar->hide(); }

    const bool expirado = ui->dataEmissao->dateTime().addDays(data("validade").toInt()).date() < qApp->serverDate();

    if (expirado or status != "ATIVO") {
      isReadOnly = true;

      ui->pushButtonGerarVenda->hide();
      ui->pushButtonAtualizarOrcamento->hide();
      ui->pushButtonReplicar->show();

      ui->pushButtonAdicionarItem->hide();
      ui->pushButtonAtualizarItem->hide();
      ui->pushButtonRemoverItem->hide();
      ui->pushButtonCalculadora->hide();

      ui->itemBoxCliente->setReadOnlyItemBox(true);
      ui->itemBoxEndereco->setReadOnlyItemBox(true);
      ui->itemBoxProduto->setReadOnlyItemBox(true);
      ui->itemBoxProfissional->setReadOnlyItemBox(true);
      ui->itemBoxVendedor->setReadOnlyItemBox(true);

      ui->spinBoxPrazoEntrega->setReadOnly(true);
      ui->dataEmissao->setReadOnly(true);

      ui->doubleSpinBoxDesconto->setReadOnly(true);
      ui->doubleSpinBoxDescontoGlobal->setReadOnly(true);
      ui->doubleSpinBoxDescontoGlobalReais->setReadOnly(true);
      ui->doubleSpinBoxFrete->setReadOnly(true);
      ui->doubleSpinBoxTotalItem->setReadOnly(true);
      ui->doubleSpinBoxQuant->setReadOnly(true);
      ui->doubleSpinBoxSubTotalBruto->setReadOnly(true);
      ui->doubleSpinBoxSubTotalLiq->setReadOnly(true);
      ui->doubleSpinBoxTotal->setReadOnly(true);

      ui->lineEditCodComercial->hide();
      ui->lineEditEstoque->hide();
      ui->lineEditFormComercial->hide();
      ui->lineEditFornecedor->hide();
      ui->spinBoxMinimo->hide();
      ui->lineEditObs->hide();
      ui->lineEditPrecoUn->hide();
      ui->lineEditUn->hide();
      ui->itemBoxProduto->hide();
      ui->doubleSpinBoxQuant->hide();
      ui->doubleSpinBoxCaixas->hide();
      ui->labelCaixa->hide();
      ui->spinBoxUnCx->hide();
      ui->doubleSpinBoxDesconto->hide();
      ui->doubleSpinBoxTotalItem->hide();
      ui->labelCaixas->hide();
      ui->labelCodComercial->hide();
      ui->labelProduto->hide();
      ui->labelDesconto->hide();
      ui->labelTotalItem->hide();
      ui->labelUn->hide();
      ui->labelPrecoUn->hide();
      ui->labelFornecedor->hide();
      ui->labelQuant->hide();
      ui->labelEstoque->hide();
      ui->labelFormComercial->hide();
      ui->labelMinimo->hide();
      ui->labelObs->hide();

      ui->plainTextEditObs->setReadOnly(true);

      ui->checkBoxFreteManual->setDisabled(true);
    } else {
      ui->pushButtonGerarVenda->show();
    }

    ui->lineEditReplicaDe->setReadOnly(true);
    ui->lineEditReplicadoEm->setReadOnly(true);

    ui->checkBoxRepresentacao->setDisabled(true);

    ui->plainTextEditObs->setPlainText(data("observacao").toString());

    const bool freteManual = ui->checkBoxFreteManual->isChecked();

    canChangeFrete = freteManual;

    ui->doubleSpinBoxFrete->setMinimum(freteManual ? 0 : ui->doubleSpinBoxFrete->value());

    ui->doubleSpinBoxDescontoGlobalReais->setMaximum(ui->doubleSpinBoxSubTotalLiq->value());

    if (ui->checkBoxRepresentacao->isChecked()) { ui->itemBoxProduto->setRepresentacao(true); }

    if (not data("replicadoDe").toString().isEmpty()) {
      ui->labelReplicaDe->show();
      ui->lineEditReplicaDe->show();
    }

    if (not data("replicadoEm").toString().isEmpty()) {
      ui->labelReplicadoEm->show();
      ui->lineEditReplicadoEm->show();
    }

    if (data("idUsuarioConsultor").toInt() != 0) {
      ui->labelConsultor->show();
      ui->itemBoxConsultor->show();
    } else {
      ui->labelConsultor->hide();
      ui->itemBoxConsultor->hide();
    }

    if (ui->lineEditOrcamento->text() != "Auto gerado") {
      const QString idLoja = UserSession::fromLoja("usuario.idLoja", ui->itemBoxVendedor->text()).value_or("-1").toString();
      ui->itemBoxVendedor->setFilter("idLoja = " + idLoja);
    }

    return true;
  }();

  setConnections();

  return load;
}

void Orcamento::novoItem() {
  ui->pushButtonAdicionarItem->show();
  ui->pushButtonAtualizarItem->hide();
  ui->pushButtonRemoverItem->hide();
  ui->itemBoxProduto->clear();
  ui->tableProdutos->clearSelection();
  // -----------------------

  ui->doubleSpinBoxCaixas->setDisabled(true);
  ui->doubleSpinBoxCaixas->setSingleStep(1.);
  ui->doubleSpinBoxCaixas->clear();
  ui->doubleSpinBoxDesconto->setDisabled(true);
  ui->doubleSpinBoxDesconto->clear();
  ui->doubleSpinBoxQuant->setDisabled(true);
  ui->doubleSpinBoxQuant->setSingleStep(1.);
  ui->doubleSpinBoxQuant->clear();
  ui->doubleSpinBoxTotalItem->clear();
  ui->doubleSpinBoxTotalItem->setDisabled(true);
  ui->lineEditCodComercial->clear();
  ui->lineEditEstoque->clear();
  ui->lineEditFormComercial->clear();
  ui->lineEditFornecedor->clear();
  ui->lineEditObs->clear();
  ui->lineEditPrecoUn->clear();
  ui->lineEditPrecoUn->setDisabled(true);
  ui->lineEditUn->clear();
  ui->lineEditUn->setDisabled(true);
  ui->spinBoxMinimo->clear();
  ui->spinBoxMinimo->setDisabled(true);
  ui->spinBoxUnCx->clear();
  ui->spinBoxUnCx->setDisabled(true);
}

void Orcamento::setupMapper() {
  addMapping(ui->checkBoxFreteManual, "freteManual");
  addMapping(ui->checkBoxRepresentacao, "representacao");
  addMapping(ui->dataEmissao, "data");
  addMapping(ui->doubleSpinBoxDescontoGlobal, "descontoPorc");
  addMapping(ui->doubleSpinBoxDescontoGlobalReais, "descontoReais");
  addMapping(ui->doubleSpinBoxFrete, "frete");
  addMapping(ui->doubleSpinBoxSubTotalBruto, "subTotalBru");
  addMapping(ui->doubleSpinBoxSubTotalLiq, "subTotalLiq");
  addMapping(ui->doubleSpinBoxTotal, "total");
  addMapping(ui->itemBoxCliente, "idCliente", "id");
  addMapping(ui->itemBoxEndereco, "idEnderecoEntrega", "id");
  addMapping(ui->itemBoxProfissional, "idProfissional", "id");
  addMapping(ui->itemBoxVendedor, "idUsuario", "id");
  addMapping(ui->itemBoxConsultor, "idUsuarioConsultor", "id");
  addMapping(ui->lineEditOrcamento, "idOrcamento");
  addMapping(ui->lineEditReplicaDe, "replicadoDe");
  addMapping(ui->lineEditReplicadoEm, "replicadoEm");
  addMapping(ui->plainTextEditBaixa, "observacaoCancelamento");
  addMapping(ui->plainTextEditObs, "observacao");
  addMapping(ui->spinBoxPrazoEntrega, "prazoEntrega");
  addMapping(ui->spinBoxValidade, "validade");

  mapperItem.setModel(ui->tableProdutos->model());
  mapperItem.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

  mapperItem.addMapping(ui->itemBoxProduto, modelItem.fieldIndex("idProduto"), "id");
  mapperItem.addMapping(ui->lineEditCodComercial, modelItem.fieldIndex("codComercial"));
  mapperItem.addMapping(ui->lineEditFormComercial, modelItem.fieldIndex("formComercial"));
  mapperItem.addMapping(ui->lineEditObs, modelItem.fieldIndex("obs"));
  mapperItem.addMapping(ui->lineEditPrecoUn, modelItem.fieldIndex("prcUnitario"), "value"); // TODO: replace this with a simple doubleSpinbox?
  mapperItem.addMapping(ui->lineEditUn, modelItem.fieldIndex("un"));
  mapperItem.addMapping(ui->doubleSpinBoxQuant, modelItem.fieldIndex("quant"));
  mapperItem.addMapping(ui->doubleSpinBoxDesconto, modelItem.fieldIndex("desconto"));
}

void Orcamento::registerMode() {
  ui->pushButtonCadastrarOrcamento->show();
  ui->pushButtonAtualizarOrcamento->hide();
  ui->pushButtonReplicar->hide();

  ui->pushButtonApagarOrc->setDisabled(true);
  ui->pushButtonGerarExcel->setDisabled(true);
  ui->pushButtonImprimir->setDisabled(true);
  ui->pushButtonGerarVenda->setEnabled(true);
  //  ui->itemBoxEndereco->setDisabled(true);
}

void Orcamento::updateMode() {
  ui->pushButtonCadastrarOrcamento->hide();
  ui->pushButtonAtualizarOrcamento->show();
  ui->pushButtonReplicar->show();

  ui->pushButtonApagarOrc->setEnabled(true);
  ui->pushButtonGerarExcel->setEnabled(true);
  ui->pushButtonImprimir->setEnabled(true);
  ui->pushButtonGerarVenda->setEnabled(true);
  ui->itemBoxEndereco->setVisible(true);
  ui->spinBoxValidade->setDisabled(true);
}

bool Orcamento::newRegister() {
  if (not RegisterDialog::newRegister()) { return false; }

  ui->lineEditOrcamento->setText("Auto gerado");
  ui->dataEmissao->setDateTime(qApp->serverDateTime());
  on_dataEmissao_dateChanged(ui->dataEmissao->date());
  ui->spinBoxValidade->setValue(7);
  novoItem();

  return true;
}

void Orcamento::removeItem() {
  if (modelItem.rowCount() == 1 and ui->lineEditOrcamento->text() != "Auto gerado") {
    qApp->enqueueError("Não pode cadastrar um orçamento sem itens!", this);
    ui->itemBoxProduto->setFocus();
    return;
  }

  unsetConnections();

  [&] {
    if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); } // save pending rows before submitAll

    if (not modelItem.removeRow(currentRowItem)) { return qApp->enqueueError("Erro removendo linha: " + modelItem.lastError().text(), this); }

    if (ui->lineEditOrcamento->text() != "Auto gerado") {
      if (not modelItem.submitAll()) { return; }
      calcPrecoGlobalTotal();
      save(true);
    } else {
      calcPrecoGlobalTotal();
    }

    if (modelItem.rowCount() == 0) {
      if (ui->lineEditOrcamento->text() == "Auto gerado") { ui->checkBoxRepresentacao->setEnabled(true); }
      ui->itemBoxProduto->setFornecedorRep("");
    }

    novoItem();
  }();

  setConnections();
}

bool Orcamento::generateId() {
  const auto siglaLoja = UserSession::fromLoja("sigla", ui->itemBoxVendedor->text());

  if (not siglaLoja) { return qApp->enqueueError(false, "Erro buscando sigla da loja!", this); }

  QString id = siglaLoja->toString() + "-" + qApp->serverDate().toString("yy");

  const QString replica = ui->lineEditReplicaDe->text();

  if (replica.isEmpty()) {
    QSqlQuery query;
    query.prepare("SELECT MAX(idOrcamento) AS idOrcamento FROM orcamento WHERE idOrcamento LIKE :id");
    query.bindValue(":id", id + "%");

    if (not query.exec()) { return qApp->enqueueError(false, "Erro buscando próximo id disponível: " + query.lastError().text(), this); }

    const int last = query.first() ? query.value("idOrcamento").toString().remove(id).leftRef(4).toInt() : 0;

    id += QString("%1").arg(last + 1, 4, 10, QChar('0'));
    id += ui->checkBoxRepresentacao->isChecked() ? "R" : "";
    id += "O";

    if (id.size() != 12 and id.size() != 13) { return qApp->enqueueError(false, "Tamanho do Id errado: " + id, this); }
  } else {
    QSqlQuery query;
    query.prepare("SELECT COALESCE(MAX(CAST(RIGHT(idOrcamento, LENGTH(idOrcamento) - LOCATE('Rev', idOrcamento) - 2) AS UNSIGNED)) + 1, 1) AS revisao FROM orcamento WHERE LENGTH(idOrcamento) > 16 "
                  "AND idOrcamento LIKE :idOrcamento");
    query.bindValue(":idOrcamento", replica.left(11) + "%");

    if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando próxima revisão disponível: " + query.lastError().text(), this); }

    id = replica.left(replica.indexOf("-Rev")) + "-Rev" + query.value("revisao").toString();
  }

  ui->lineEditOrcamento->setText(id);

  return true;
}

bool Orcamento::recalcularTotais() {
  // TODO: just change this function to call 'calcPrecoGlobalTotal' and be sure all is recalculated?

  double subTotalBruto = 0.;
  double subTotalLiq = 0.;
  double total = 0.;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    subTotalBruto += modelItem.data(row, "parcial").toDouble();
    subTotalLiq += modelItem.data(row, "parcialDesc").toDouble();
    total += modelItem.data(row, "total").toDouble();
  }

  if (abs(subTotalBruto - ui->doubleSpinBoxSubTotalBruto->value()) > 1) {
    calcPrecoGlobalTotal();
    return qApp->enqueueError(false, "Subtotal dos itens não confere com SubTotalBruto! Recalculando valores!", this);
  }

  if (abs(subTotalLiq - ui->doubleSpinBoxSubTotalLiq->value()) > 1) {
    calcPrecoGlobalTotal();
    return qApp->enqueueError(false, "Total dos itens não confere com SubTotalLíquido! Recalculando valores!", this);
  }

  if (abs(total - (ui->doubleSpinBoxTotal->value() - ui->doubleSpinBoxFrete->value())) > 1) {
    calcPrecoGlobalTotal();
    return qApp->enqueueError(false, "Total dos itens não confere com Total! Recalculando valores!", this);
  }

  return true;
}

bool Orcamento::verifyFields() {
  if (not recalcularTotais()) { return false; }

  if (ui->itemBoxCliente->text().isEmpty()) {
    qApp->enqueueError("Cliente inválido!", this);
    ui->itemBoxCliente->setFocus();
    return false;
  }

  if (ui->itemBoxVendedor->text().isEmpty()) {
    qApp->enqueueError("Vendedor inválido!", this);
    ui->itemBoxVendedor->setFocus();
    return false;
  }

  if (ui->itemBoxProfissional->text().isEmpty()) {
    qApp->enqueueError("Profissional inválido!", this);
    ui->itemBoxProfissional->setFocus();
    return false;
  }

  if (ui->itemBoxEndereco->text().isEmpty()) {
    qApp->enqueueError(R"(Endereço inválido! Se não possui endereço, escolha "Não há".)", this);
    ui->itemBoxEndereco->setFocus();
    return false;
  }

  if (modelItem.rowCount() == 0) {
    qApp->enqueueError("Não pode cadastrar um orçamento sem itens!", this);
    ui->itemBoxProduto->setFocus();
    return false;
  }

  return true;
}

bool Orcamento::savingProcedures() {
  if (tipo == Tipo::Cadastrar) {
    const auto idLoja = UserSession::fromLoja("usuario.idLoja", ui->itemBoxVendedor->text());

    if (not idLoja) { return qApp->enqueueError(false, "Erro buscando idLoja!", this); }

    if (not setData("idLoja", idLoja->toInt())) { return false; }

    if (not setData("idOrcamento", ui->lineEditOrcamento->text())) { return false; }
    if (not setData("idOrcamentoBase", ui->lineEditOrcamento->text().left(11))) { return false; }
    if (not setData("replicadoDe", ui->lineEditReplicaDe->text())) { return false; }
    if (not setData("representacao", ui->checkBoxRepresentacao->isChecked())) { return false; }
  }

  if (not setData("idUsuario", ui->itemBoxVendedor->getId())) { return false; }
  if (not setData("idCliente", ui->itemBoxCliente->getId())) { return false; }
  if (not setData("data", ui->dataEmissao->dateTime())) { return false; }
  if (not setData("descontoPorc", ui->doubleSpinBoxDescontoGlobal->value())) { return false; }
  if (not setData("descontoReais", ui->doubleSpinBoxSubTotalLiq->value() * ui->doubleSpinBoxDescontoGlobal->value() / 100.)) { return false; }
  if (not setData("frete", ui->doubleSpinBoxFrete->value())) { return false; }
  if (not setData("idEnderecoEntrega", ui->itemBoxEndereco->getId())) { return false; }
  if (not setData("idProfissional", ui->itemBoxProfissional->getId())) { return false; }
  if (not setData("observacao", ui->plainTextEditObs->toPlainText())) { return false; }
  if (not setData("prazoEntrega", ui->spinBoxPrazoEntrega->value())) { return false; }
  if (not setData("subTotalBru", ui->doubleSpinBoxSubTotalBruto->value())) { return false; }
  if (not setData("subTotalLiq", ui->doubleSpinBoxSubTotalLiq->value())) { return false; }
  if (not setData("total", ui->doubleSpinBoxTotal->value())) { return false; }
  if (not setData("validade", ui->spinBoxValidade->value())) { return false; }
  if (not setData("freteManual", ui->checkBoxFreteManual->isChecked())) { return false; }

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (not modelItem.setData(row, "idOrcamento", ui->lineEditOrcamento->text())) { return false; }
    if (not modelItem.setData(row, "idLoja", model.data(currentRow, "idLoja"))) { return false; }

    const double prcUnitario = modelItem.data(row, "prcUnitario").toDouble();
    const double desconto = modelItem.data(row, "desconto").toDouble() / 100.;

    if (not modelItem.setData(row, "descUnitario", prcUnitario - (prcUnitario * desconto))) { return false; }

    const bool mostrarDesconto = (modelItem.data(row, "parcialDesc").toDouble() - modelItem.data(row, "parcial").toDouble()) < -0.1;

    if (not modelItem.setData(row, "mostrarDesconto", mostrarDesconto)) { return false; }
  }

  if (not buscarCadastrarConsultor()) { return false; }

  return atualizaReplica();
}

bool Orcamento::buscarCadastrarConsultor() {
  QStringList fornecedores;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) { fornecedores << modelItem.data(row, "fornecedor").toString(); }

  fornecedores.removeDuplicates();

  for (auto &fornecedor : fornecedores) { fornecedor.prepend("'").append("'"); }

  QSqlQuery query;

  if (not query.exec("SELECT idUsuario FROM usuario WHERE especialidade > 0 AND especialidade IN (SELECT especialidade FROM fornecedor WHERE razaoSocial IN (" + fornecedores.join(", ") + "))")) {
    return qApp->enqueueError(false, "Erro buscando consultor: " + query.lastError().text(), this);
  }

  if (query.size() > 1) { return qApp->enqueueError(false, "Mais de um consultor disponível!", this); }

  if (query.size() == 1 and query.first() and not setData("idUsuarioConsultor", query.value("idUsuario"))) { return false; }

  if (query.size() == 0 and not model.setData(currentRow, "idUsuarioConsultor", QVariant(QVariant::UInt))) { return false; }

  return true;
}

bool Orcamento::atualizaReplica() {
  if (not ui->lineEditReplicaDe->text().isEmpty()) {
    QSqlQuery query;
    query.prepare("UPDATE orcamento SET status = 'REPLICADO', replicadoEm = :idReplica WHERE idOrcamento = :idOrcamento");
    query.bindValue(":idReplica", ui->lineEditOrcamento->text());
    query.bindValue(":idOrcamento", ui->lineEditReplicaDe->text());

    if (not query.exec()) { return qApp->enqueueError(false, "Erro salvando replicadoEm: " + query.lastError().text(), this); }
  }

  return true;
}

void Orcamento::clearFields() {
  RegisterDialog::clearFields();

  if (UserSession::tipoUsuario() == "VENDEDOR" or UserSession::tipoUsuario() == "VENDEDOR ESPECIAL") { ui->itemBoxVendedor->setId(UserSession::idUsuario()); }

  //  ui->itemBoxEndereco->setDisabled(true);
}

void Orcamento::on_pushButtonRemoverItem_clicked() { removeItem(); }

void Orcamento::on_doubleSpinBoxQuant_valueChanged(const double quant) {
  const double step = ui->doubleSpinBoxQuant->singleStep();
  const double quant2 = not qFuzzyIsNull(fmod(quant, step)) ? ceil(quant / step) * step : quant;

  if (not qFuzzyCompare(quant, quant2)) { ui->doubleSpinBoxQuant->setValue(quant2); }

  const double caixas = quant2 / ui->spinBoxUnCx->value();

  if (not qFuzzyCompare(ui->doubleSpinBoxCaixas->value(), caixas)) { ui->doubleSpinBoxCaixas->setValue(caixas); }
}

void Orcamento::on_pushButtonCadastrarOrcamento_clicked() {
  // TODO: ao fechar pedido calcular o frete com o endereco selecinado em 'end. entrega'
  // se o valor calculado for maior que o do campo frete pedir autorizacao do gerente para manter o valor atual
  // senao usa o valor calculado

  // pedir login caso o frete (manual ou automatico) seja menor que ou o valorPeso ou a porcentagem parametrizada

  save();
}

void Orcamento::on_pushButtonAtualizarOrcamento_clicked() { save(); }

void Orcamento::calcPrecoGlobalTotal() {
  double subTotalBruto = 0.;
  double subTotalItens = 0.;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    const double itemBruto = modelItem.data(row, "quant").toDouble() * modelItem.data(row, "prcUnitario").toDouble();
    const double descItem = modelItem.data(row, "desconto").toDouble() / 100.;
    const double stItem = itemBruto * (1. - descItem);
    subTotalBruto += itemBruto;
    subTotalItens += stItem;
  }

  ui->doubleSpinBoxSubTotalBruto->setValue(subTotalBruto);
  ui->doubleSpinBoxSubTotalLiq->setValue(subTotalItens);

  // calcula totais considerando desconto global atual

  if (not ui->checkBoxFreteManual->isChecked()) {
    const double frete = qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete);

    ui->doubleSpinBoxFrete->setMinimum(frete);
    ui->doubleSpinBoxFrete->setValue(frete);
  }

  const double frete = ui->doubleSpinBoxFrete->value();
  const double descGlobal = ui->doubleSpinBoxDescontoGlobal->value();

  ui->doubleSpinBoxDescontoGlobalReais->setMaximum(subTotalItens);
  ui->doubleSpinBoxDescontoGlobalReais->setValue(subTotalItens * descGlobal / 100);

  ui->doubleSpinBoxTotal->setMaximum(subTotalItens + frete);
  ui->doubleSpinBoxTotal->setValue(subTotalItens * (1 - (descGlobal / 100)) + frete);
}

void Orcamento::on_pushButtonImprimir_clicked() {
  Impressao impressao(data("idOrcamento").toString(), Impressao::Tipo::Orcamento, this);
  impressao.print();
}

void Orcamento::setupTables() {
  modelItem.setTable("orcamento_has_produto");

  modelItem.setHeaderData("produto", "Produto");
  modelItem.setHeaderData("fornecedor", "Fornecedor");
  modelItem.setHeaderData("obs", "Obs.");
  modelItem.setHeaderData("prcUnitario", "Preço/Un.");
  modelItem.setHeaderData("caixas", "Caixas");
  modelItem.setHeaderData("quant", "Quant.");
  modelItem.setHeaderData("un", "Un.");
  modelItem.setHeaderData("codComercial", "Código");
  modelItem.setHeaderData("formComercial", "Formato");
  modelItem.setHeaderData("unCaixa", "Un./Caixa");
  modelItem.setHeaderData("parcial", "Subtotal");
  modelItem.setHeaderData("desconto", "Desc. %");
  modelItem.setHeaderData("parcialDesc", "Total");

  modelItem.proxyModel = new SearchDialogProxyModel(&modelItem, this);

  ui->tableProdutos->setModel(&modelItem);

  ui->tableProdutos->hideColumn("idOrcamentoProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idOrcamento");
  ui->tableProdutos->hideColumn("idLoja");
  ui->tableProdutos->hideColumn("unCaixa");
  ui->tableProdutos->hideColumn("descUnitario");
  ui->tableProdutos->hideColumn("descGlobal");
  ui->tableProdutos->hideColumn("total");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("mostrarDesconto");

  ui->tableProdutos->setItemDelegate(new DoubleDelegate(this));

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(this, 4));
  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("parcial", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("parcialDesc", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("desconto", new PorcentagemDelegate(this));
}

void Orcamento::atualizarItem() { adicionarItem(Tipo::Atualizar); }

void Orcamento::adicionarItem(const Tipo tipoItem) {
  if (ui->itemBoxProduto->text().isEmpty()) { return qApp->enqueueError("Item inválido!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuant->value())) { return qApp->enqueueError("Quantidade inválida!", this); }

  unsetConnections();

  [&] {
    if (tipoItem == Tipo::Cadastrar) { currentRowItem = modelItem.insertRowAtEnd(); }

    if (not modelItem.setData(currentRowItem, "idProduto", ui->itemBoxProduto->getId().toInt())) { return; }
    if (not modelItem.setData(currentRowItem, "fornecedor", ui->lineEditFornecedor->text())) { return; }
    if (not modelItem.setData(currentRowItem, "produto", ui->itemBoxProduto->text())) { return; }
    if (not modelItem.setData(currentRowItem, "obs", ui->lineEditObs->text())) { return; }
    if (not modelItem.setData(currentRowItem, "prcUnitario", ui->lineEditPrecoUn->getValue())) { return; }
    if (not modelItem.setData(currentRowItem, "caixas", ui->doubleSpinBoxCaixas->value())) { return; }
    if (not modelItem.setData(currentRowItem, "quant", ui->doubleSpinBoxQuant->value())) { return; }
    if (not modelItem.setData(currentRowItem, "unCaixa", ui->doubleSpinBoxQuant->singleStep())) { return; }
    if (not modelItem.setData(currentRowItem, "un", ui->lineEditUn->text())) { return; }
    if (not modelItem.setData(currentRowItem, "codComercial", ui->lineEditCodComercial->text())) { return; }
    if (not modelItem.setData(currentRowItem, "formComercial", ui->lineEditFormComercial->text())) { return; }
    if (not modelItem.setData(currentRowItem, "desconto", ui->doubleSpinBoxDesconto->value())) { return; }
    if (not modelItem.setData(currentRowItem, "estoque", currentItemIsEstoque)) { return; }
    if (not modelItem.setData(currentRowItem, "promocao", currentItemIsPromocao)) { return; }
    if (not modelItem.setData(currentRowItem, "parcial", modelItem.data(currentRowItem, "quant").toDouble() * modelItem.data(currentRowItem, "prcUnitario").toDouble())) { return; }
    if (not modelItem.setData(currentRowItem, "parcialDesc", ui->doubleSpinBoxTotalItem->value())) { return; }
    if (not modelItem.setData(currentRowItem, "descGlobal", ui->doubleSpinBoxDescontoGlobal->value())) { return; }
    if (not modelItem.setData(currentRowItem, "total", ui->doubleSpinBoxTotalItem->value() * (1 - (ui->doubleSpinBoxDescontoGlobal->value() / 100)))) { return; }

    if (modelItem.rowCount() == 1 and ui->checkBoxRepresentacao->isChecked()) { ui->itemBoxProduto->setFornecedorRep(modelItem.data(currentRowItem, "fornecedor").toString()); }

    if (tipoItem == Tipo::Cadastrar) { backupItem.append(modelItem.record(currentRowItem)); }

    isDirty = true;
    ui->checkBoxRepresentacao->setDisabled(true);
  }();

  novoItem();

  calcPrecoGlobalTotal();

  setConnections();
}

void Orcamento::on_pushButtonAdicionarItem_clicked() { adicionarItem(); }

void Orcamento::on_pushButtonAtualizarItem_clicked() { atualizarItem(); }

void Orcamento::on_pushButtonGerarVenda_clicked() {
  if (not save(true)) { return; }

  const QDateTime time = ui->dataEmissao->dateTime();

  if (not time.isValid()) { return; }

  if (time.addDays(data("validade").toInt()).date() < qApp->serverDate()) { return qApp->enqueueError("Orçamento vencido!", this); }

  if (ui->itemBoxEndereco->text().isEmpty()) {
    qApp->enqueueError("Deve selecionar endereço!", this);
    ui->itemBoxEndereco->setFocus();
    return;
  }

  if (not verificaCadastroCliente()) { return; }

  if (not verificaDisponibilidadeEstoque()) { return; }

  auto *venda = new Venda(parentWidget());
  venda->prepararVenda(ui->lineEditOrcamento->text());

  close();
}

void Orcamento::on_doubleSpinBoxCaixas_valueChanged(const double caixas) {
  const double caixas2 = not qFuzzyIsNull(fmod(caixas, ui->doubleSpinBoxCaixas->singleStep())) ? ceil(caixas) : caixas;
  const double quant = caixas2 * ui->spinBoxUnCx->value();
  const double prcUn = ui->lineEditPrecoUn->getValue();
  const double desc = ui->doubleSpinBoxDesconto->value() / 100.;
  const double itemBruto = quant * prcUn;

  if (not qFuzzyCompare(caixas, caixas2)) { ui->doubleSpinBoxCaixas->setValue(caixas2); }

  unsetConnections();

  [&] {
    ui->doubleSpinBoxQuant->setValue(quant);
    ui->doubleSpinBoxTotalItem->setValue(itemBruto * (1. - desc));
  }();

  setConnections();
}

void Orcamento::on_pushButtonApagarOrc_clicked() {
  auto *baixa = new BaixaOrcamento(data("idOrcamento").toString(), this);
  baixa->show();
}

void Orcamento::on_itemBoxProduto_idChanged(const QVariant &) {
  if (ui->itemBoxProduto->text().isEmpty()) { return; }

  // -------------------------------------------------------------------------

  ui->doubleSpinBoxCaixas->clear();
  ui->doubleSpinBoxDesconto->clear();
  ui->doubleSpinBoxQuant->clear();
  ui->doubleSpinBoxTotalItem->clear();
  ui->lineEditCodComercial->clear();
  ui->lineEditEstoque->clear();
  ui->lineEditFormComercial->clear();
  ui->lineEditFornecedor->clear();
  if (ui->pushButtonAdicionarItem->isVisible()) { ui->lineEditObs->clear(); }
  ui->lineEditPrecoUn->clear();
  ui->lineEditUn->clear();
  ui->spinBoxMinimo->clear();
  ui->spinBoxUnCx->clear();

  // -------------------------------------------------------------------------

  QSqlQuery query;
  query.prepare("SELECT un, precoVenda, estoqueRestante, fornecedor, codComercial, formComercial, m2cx, pccx, minimo, multiplo, estoque, promocao FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro na busca do produto: " + query.lastError().text(), this); }

  const QString un = query.value("un").toString().toUpper();

  ui->lineEditUn->setText(un);
  ui->lineEditPrecoUn->setValue(query.value("precoVenda").toDouble());
  ui->lineEditEstoque->setValue(query.value("estoqueRestante").toDouble());
  ui->lineEditFornecedor->setText(query.value("fornecedor").toString());
  ui->lineEditCodComercial->setText(query.value("codComercial").toString());
  ui->lineEditFormComercial->setText(query.value("formComercial").toString());

  const QString uncxString = un.contains("M2") or un.contains("M²") or un.contains("ML") ? "m2cx" : "pccx";

  ui->spinBoxUnCx->setValue(query.value(uncxString).toDouble());

  const double minimo = query.value("minimo").toDouble();
  const double multiplo = query.value("multiplo").toDouble();
  const double uncx = query.value(uncxString).toDouble();

  ui->spinBoxMinimo->setValue(minimo);
  ui->doubleSpinBoxQuant->setMinimum(minimo);
  ui->doubleSpinBoxCaixas->setMinimum(minimo / uncx);

  currentItemIsEstoque = query.value("estoque").toBool();
  currentItemIsPromocao = query.value("promocao").toInt();

  if (currentItemIsEstoque) {
    ui->doubleSpinBoxQuant->setMaximum(query.value("estoqueRestante").toDouble());
    ui->doubleSpinBoxCaixas->setMaximum(query.value("estoqueRestante").toDouble() / uncx);
  } else {
    ui->doubleSpinBoxQuant->setMaximum(9999999.000000);
    ui->doubleSpinBoxCaixas->setMaximum(9999999.000000);
  }

  ui->doubleSpinBoxCaixas->setEnabled(true);
  ui->doubleSpinBoxCaixas->setEnabled(true);
  ui->doubleSpinBoxDesconto->setEnabled(true);
  ui->doubleSpinBoxQuant->setEnabled(true);
  ui->doubleSpinBoxTotalItem->setEnabled(true);
  ui->lineEditPrecoUn->setEnabled(true);
  ui->lineEditUn->setEnabled(true);
  ui->spinBoxMinimo->setEnabled(true);
  ui->spinBoxUnCx->setEnabled(true);

  ui->doubleSpinBoxCaixas->setSingleStep(1.);
  ui->doubleSpinBoxQuant->setSingleStep(uncx);

  // TODO: 0verificar se preciso tratar os casos sem multiplo
  // if (minimo != 0) ...
  if (not qFuzzyIsNull(minimo) and not qFuzzyIsNull(multiplo)) {
    ui->doubleSpinBoxCaixas->setSingleStep(multiplo / uncx);
    ui->doubleSpinBoxQuant->setSingleStep(multiplo);
  }

  ui->doubleSpinBoxQuant->setValue(0.);
  ui->doubleSpinBoxCaixas->setValue(0.);
  ui->doubleSpinBoxDesconto->setValue(0.);

  on_doubleSpinBoxCaixas_valueChanged(ui->doubleSpinBoxCaixas->value());
}

void Orcamento::on_itemBoxCliente_textChanged(const QString &) {
  const QString idCliente = QString::number(ui->itemBoxCliente->getId().toInt());
  ui->itemBoxEndereco->setFilter("(idCliente = " + idCliente + " OR idEndereco = 1) AND desativado = FALSE");

  QSqlQuery queryCliente;
  queryCliente.prepare("SELECT idProfissionalRel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not queryCliente.exec() or not queryCliente.first()) { return qApp->enqueueError("Erro ao buscar cliente: " + queryCliente.lastError().text(), this); }

  ui->itemBoxProfissional->setId(queryCliente.value("idProfissionalRel"));
  ui->itemBoxEndereco->setEnabled(true);
  ui->itemBoxEndereco->clear();
}

void Orcamento::on_checkBoxFreteManual_clicked(const bool checked) {
  if (not canChangeFrete) {
    qApp->enqueueInformation("Necessário autorização de um gerente ou administrador!", this);

    LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

    if (dialog.exec() != QDialog::Accepted) {
      ui->checkBoxFreteManual->setChecked(not checked);
      return;
    }

    canChangeFrete = true;
  }

  const double frete = qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete);

  ui->doubleSpinBoxFrete->setMinimum(checked ? 0 : frete);

  if (not checked) { ui->doubleSpinBoxFrete->setValue(frete); }
}

void Orcamento::on_pushButtonReplicar_clicked() {
  // passar por cada produto verificando sua validade/descontinuado
  QStringList produtos;
  QVector<int> skipRows;

  QSqlQuery queryProduto;
  queryProduto.prepare("SELECT (descontinuado OR desativado) AS invalido FROM produto WHERE idProduto = :idProduto");

  QSqlQuery queryEquivalente;
  queryEquivalente.prepare("SELECT idProduto FROM produto WHERE codComercial = :codComercial AND descontinuado = FALSE AND desativado = FALSE AND estoque = FALSE");

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    queryProduto.bindValue(":idProduto", modelItem.data(row, "idProduto"));

    if (not queryProduto.exec() or not queryProduto.first()) { return qApp->enqueueError("Erro verificando validade dos produtos: " + queryProduto.lastError().text()); }

    if (queryProduto.value("invalido").toBool()) {
      queryEquivalente.bindValue(":codComercial", modelItem.data(row, "codComercial"));

      if (not queryEquivalente.exec()) { return qApp->enqueueError("Erro procurando produto equivalente: " + queryEquivalente.lastError().text(), this); }

      if (queryEquivalente.first()) {
        if (not modelItem.setData(row, "idProduto", queryEquivalente.value("idProduto"))) {}
      } else {
        produtos << QString::number(row + 1) + " - " + modelItem.data(row, "produto").toString();
        skipRows << row;
      }
    }
  }

  if (not produtos.isEmpty()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Os seguintes itens estão descontinuados e serão removidos da réplica:\n" + produtos.join("\n"), QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  auto *replica = new Orcamento(parentWidget());

  replica->ui->pushButtonReplicar->hide();

  replica->ui->itemBoxCliente->setId(data("idCliente"));
  replica->ui->itemBoxProfissional->setId(data("idProfissional"));
  replica->ui->itemBoxVendedor->setId(data("idUsuario"));
  replica->ui->itemBoxEndereco->setId(data("idEnderecoEntrega"));
  replica->ui->spinBoxValidade->setValue(data("validade").toInt());
  replica->ui->dataEmissao->setDateTime(qApp->serverDateTime());
  replica->ui->checkBoxRepresentacao->setChecked(ui->checkBoxRepresentacao->isChecked());
  replica->ui->lineEditReplicaDe->setText(data("idOrcamento").toString());
  replica->ui->plainTextEditObs->setPlainText(data("observacao").toString());

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (skipRows.contains(row)) { continue; }

    replica->ui->itemBoxProduto->setId(modelItem.data(row, "idProduto"));
    replica->ui->doubleSpinBoxQuant->setValue(modelItem.data(row, "quant").toDouble());
    replica->ui->doubleSpinBoxDesconto->setValue(modelItem.data(row, "desconto").toDouble());
    replica->ui->lineEditObs->setText(modelItem.data(row, "obs").toString());
    replica->adicionarItem();
  }

  replica->show();
}

bool Orcamento::cadastrar() {
  if (not qApp->startTransaction()) { return false; }

  const bool success = [&] {
    if (tipo == Tipo::Cadastrar) {
      if (not generateId()) { return false; }

      currentRow = model.insertRowAtEnd();
    }

    if (not savingProcedures()) { return false; }

    if (not model.submitAll()) { return false; }

    primaryId = ui->lineEditOrcamento->text();

    if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

    if (not modelItem.submitAll()) { return false; }

    return true;
  }();

  if (success) {
    if (not qApp->endTransaction()) { return false; }

    backupItem.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelItem.setFilter(primaryKey + " = '" + primaryId + "'");
  } else {
    qApp->rollbackTransaction();
    void(model.select());
    void(modelItem.select());

    for (auto &record : backupItem) { modelItem.insertRecord(-1, record); }
  }

  return success;
}

bool Orcamento::verificaCadastroCliente() {
  const int idCliente = ui->itemBoxCliente->getId().toInt();

  // REFAC: simplify this function

  QSqlQuery queryCliente;
  queryCliente.prepare("SELECT cpf, cnpj FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", idCliente);

  if (not queryCliente.exec() or not queryCliente.first()) { return qApp->enqueueError(false, "Erro verificando se cliente possui CPF/CNPJ: " + queryCliente.lastError().text(), this); }

  if (queryCliente.value("cpf").toString().isEmpty() and queryCliente.value("cnpj").toString().isEmpty()) {
    qApp->enqueueError("Cliente não possui CPF/CNPJ cadastrado!", this);

    auto *cadCliente = new CadastroCliente(this);
    cadCliente->viewRegisterById(idCliente);
    cadCliente->show();

    return false;
  }

  QSqlQuery queryCadastro;
  queryCadastro.prepare("SELECT idCliente FROM cliente_has_endereco WHERE idCliente = :idCliente");
  queryCadastro.bindValue(":idCliente", idCliente);

  if (not queryCadastro.exec()) { return qApp->enqueueError(false, "Erro verificando se cliente possui endereço: " + queryCadastro.lastError().text(), this); }

  if (not queryCadastro.first()) {
    qApp->enqueueError("Cliente não possui endereço cadastrado!", this);

    auto *cadCliente = new CadastroCliente(this);
    cadCliente->viewRegisterById(idCliente);
    cadCliente->show();

    return false;
  }

  queryCadastro.prepare("SELECT c.incompleto FROM orcamento o LEFT JOIN cliente c ON o.idCliente = c.idCliente WHERE c.idCliente = :idCliente AND c.incompleto = TRUE");
  queryCadastro.bindValue(":idCliente", idCliente);

  if (not queryCadastro.exec()) { return qApp->enqueueError(false, "Erro verificando se cadastro do cliente está completo: " + queryCadastro.lastError().text(), this); }

  if (queryCadastro.first()) {
    qApp->enqueueError("Cadastro incompleto, deve preencher pelo menos:\n  -Telefone Principal\n  -Email\n  -Endereço", this);

    auto *cadCliente = new CadastroCliente(this);
    cadCliente->viewRegisterById(idCliente);
    cadCliente->show();

    return false;
  }

  return true;
}

void Orcamento::on_pushButtonGerarExcel_clicked() {
  Excel excel(ui->lineEditOrcamento->text(), Excel::Tipo::Orcamento);
  excel.gerarExcel();
}

void Orcamento::on_checkBoxRepresentacao_toggled(const bool checked) {
  ui->itemBoxProduto->setRepresentacao(checked);
  novoItem();
}

void Orcamento::on_doubleSpinBoxDesconto_valueChanged(const double desconto) {
  const double caixas = ui->doubleSpinBoxCaixas->value();
  const double caixas2 = not qFuzzyIsNull(fmod(caixas, ui->doubleSpinBoxCaixas->singleStep())) ? ceil(caixas) : caixas;
  const double quant = caixas2 * ui->spinBoxUnCx->value();

  unsetConnections();

  [&] {
    const double prcUn = ui->lineEditPrecoUn->getValue();
    const double itemBruto = quant * prcUn;

    ui->doubleSpinBoxTotalItem->setValue(itemBruto * (1. - (desconto / 100)));
  }();

  setConnections();
}

void Orcamento::on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) {
  unsetConnections();

  [&] {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "descGlobal", descontoPorc * 100)) { return; }

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      if (not modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc))) { return; }
    }

    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - descontoReais + frete);
  }();

  setConnections();
}

void Orcamento::on_doubleSpinBoxFrete_valueChanged(const double frete) {
  const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
  const double desconto = ui->doubleSpinBoxDescontoGlobalReais->value();

  unsetConnections();

  [&] {
    ui->doubleSpinBoxTotal->setMinimum(frete);
    ui->doubleSpinBoxTotal->setMaximum(ui->doubleSpinBoxSubTotalLiq->value() + frete);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - desconto + frete);
  }();

  setConnections();
}

void Orcamento::on_itemBoxVendedor_textChanged(const QString &) {
  if (ui->itemBoxVendedor->text().isEmpty()) { return; }

  if (not buscarParametrosFrete()) { return; }

  if (not ui->checkBoxFreteManual->isChecked()) {
    const double frete = qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete);

    ui->doubleSpinBoxFrete->setMinimum(frete);
    ui->doubleSpinBoxFrete->setValue(frete);
  }
}

bool Orcamento::buscarParametrosFrete() {
  const auto idLoja = UserSession::fromLoja("usuario.idLoja", ui->itemBoxVendedor->text());

  if (not idLoja) { return qApp->enqueueError(false, "Erro buscando idLoja!", this); }

  QSqlQuery queryFrete;
  queryFrete.prepare("SELECT valorMinimoFrete, porcentagemFrete FROM loja WHERE idLoja = :idLoja");
  queryFrete.bindValue(":idLoja", idLoja->toInt());

  if (not queryFrete.exec() or not queryFrete.next()) { return qApp->enqueueError(false, "Erro buscando parâmetros do frete: " + queryFrete.lastError().text(), this); }

  minimoFrete = queryFrete.value("valorMinimoFrete").toDouble();
  porcFrete = queryFrete.value("porcentagemFrete").toDouble();

  return true;
}

void Orcamento::on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) {
  unsetConnections();

  [&] {
    const double descontoPorc2 = descontoPorc / 100;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "descGlobal", descontoPorc)) { return; }

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      if (not modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc2))) { return; }
    }

    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobalReais->setValue(subTotalLiq * descontoPorc2);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq * (1 - descontoPorc2) + frete);
  }();

  setConnections();
}

void Orcamento::on_doubleSpinBoxTotal_valueChanged(const double total) {
  unsetConnections();

  [&] {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();
    const double descontoReais = subTotalLiq + frete - total;
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "descGlobal", descontoPorc * 100)) { return; }

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      if (not modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc))) { return; }
    }

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxDescontoGlobalReais->setValue(descontoReais);
  }();

  setConnections();
}

void Orcamento::on_doubleSpinBoxTotalItem_valueChanged(const double) {
  if (ui->itemBoxProduto->text().isEmpty()) { return; }

  const double quant = ui->doubleSpinBoxQuant->value();
  const double prcUn = ui->lineEditPrecoUn->getValue();
  const double itemBruto = quant * prcUn;
  const double subTotalItem = ui->doubleSpinBoxTotalItem->value();
  const double desconto = (itemBruto - subTotalItem) / itemBruto * 100.;

  if (qFuzzyIsNull(itemBruto)) { return; }

  unsetConnections();

  [&] { ui->doubleSpinBoxDesconto->setValue(desconto); }();

  setConnections();
}

void Orcamento::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Orçamento cadastrado com sucesso!", this); }

void Orcamento::on_pushButtonCalculadora_clicked() { QDesktopServices::openUrl(QUrl::fromLocalFile(R"(C:\Windows\System32\calc.exe)")); }

void Orcamento::on_pushButtonCalcularFrete_clicked() {
  LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

  if (dialog.exec() == QDialog::Rejected) { return; }

  auto *frete = new CalculoFrete(this);
  frete->setCliente(ui->itemBoxCliente->getId());
  frete->exec();

  const double dist = frete->getDistancia();

  if (qFuzzyIsNull(dist)) { return qApp->enqueueError("Não foi possível determinar a distância!", this); }

  int peso = 0;

  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    query.bindValue(":idProduto", modelItem.data(row, "idProduto"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando peso do produto: " + query.lastError().text(), this); }

    peso += modelItem.data(row, "caixas").toInt() * query.value("kgcx").toInt();
  }

  if (not query.exec("SELECT custoTransporteTon, custoTransporte1, custoTransporte2, custoFuncionario FROM loja WHERE nomeFantasia = 'Geral'") or not query.first()) {
    return qApp->enqueueError("Erro buscando parâmetros: " + query.lastError().text(), this);
  }

  const double custoTon = query.value("custoTransporteTon").toDouble();
  const double custo1 = query.value("custoTransporte1").toDouble();
  const double custo2 = query.value("custoTransporte2").toDouble();
  const double custoFuncionario = query.value("custoFuncionario").toDouble();

  qDebug() << "peso: " << peso;

  int cargas = peso / 4500;
  int restante = peso % 4500;

  qDebug() << "inteiro: " << cargas;
  qDebug() << "resto: " << restante;

  // TODO: se endereco for 'nao há/retira' calcular apenas o valorPeso
  const double valorPeso = (peso / 1000.0 * custoTon);

  const double valorDistCargaCheia = (cargas * custo2 * dist) + (cargas * 3 * custoFuncionario);
  const double valorDistMeiaCarga =
      cargas > 0 and restante < 200 ? 0 : (restante < 2000 ? dist * custo1 + (2 * custoFuncionario / 2000.0 * restante) : dist * custo2 + (3 * custoFuncionario / 4500.0 * restante));
  const double valorFrete = valorPeso + valorDistCargaCheia + valorDistMeiaCarga;

  qDebug() << "valorPeso: " << valorPeso;
  qDebug() << "valorDistCargaCheia: " << valorDistCargaCheia;
  qDebug() << "valorDistMeiaCarga: " << valorDistMeiaCarga;
  qDebug() << "frete: " << valorFrete;

  // frete = (pesoProduto(ton.) * 180) + (pesoProduto < 2ton. ? dist. * 1.5 : pesoProduto < 4.5 ? dist. * 2 : fracionar cargas)
}

void Orcamento::on_dataEmissao_dateChanged(const QDate &date) { ui->spinBoxValidade->setMaximum(date.daysInMonth() - date.day()); }

bool Orcamento::verificaDisponibilidadeEstoque() {
  QSqlQuery query;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.data(row, "estoque").toInt() != 1) { continue; }

    const QString idProduto = modelItem.data(row, "idProduto").toString();

    if (not query.exec("SELECT descontinuado FROM produto WHERE idProduto = " + idProduto) or not query.first()) {
      return qApp->enqueueError(false, "Erro verificando a disponibilidade do estoque: " + query.lastError().text(), this);
    }

    if (query.value("descontinuado").toBool()) { return qApp->enqueueError(false, "Item " + QString::number(row + 1) + " não está mais disponível!", this); }
  }

  return true;
}

// NOTE: model.submitAll faz mapper voltar para -1, select tambem (talvez porque submitAll chama select)
// TODO: 0se produto for estoque permitir vender por peça (setar minimo/multiplo)
// TODO: 2orcamento de reposicao nao pode ter profissional associado (bloquear)
// TODO: 4quando cadastrar cliente no itemBox mudar para o id dele
// TODO: ?permitir que o usuario digite um valor e o sistema faça o calculo na linha?
// TODO: limitar o total ao frete? se o desconto é 100% e o frete não é zero, o minimo é o frete
// TODO: implementar mover linha para baixo/cima (talvez com drag-n-drop?) http://apocalyptech.com/linux/qt/qtableview/
// TODO: após gerar id permitir mudar vendedor apenas para os da mesma loja
// TODO: limitar validade para o fim do mes
// FIXME: adicionar novamente botao para limpar selecao para quando a tabela de itens está cheia e não tem como clicar no espaço vazio
