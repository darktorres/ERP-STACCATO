#include "orcamento.h"
#include "ui_orcamento.h"

#include "application.h"
#include "baixaorcamento.h"
#include "cadastrocliente.h"
#include "cadastroprofissional.h"
#include "calculofrete.h"
#include "doubledelegate.h"
#include "excel.h"
#include "file.h"
#include "log.h"
#include "pdf.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "searchdialogproxymodel.h"
#include "user.h"
#include "venda.h"

#include <QAuthenticator>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSqlError>
#include <QtMath>

Orcamento::Orcamento(QWidget *parent) : RegisterDialog("orcamento", "idOrcamento", parent), ui(new Ui::Orcamento) {
  ui->setupUi(this);

  setupTables();
  connectLineEditsToDirty();
  setItemBoxes();
  setupMapper();
  newRegister();

  if (User::isAdministrativo()) {
    ui->dataEmissao->setReadOnly(false);
    ui->dataEmissao->setCalendarPopup(true);
  }

  if (User::isVendedor()) { buscarParametrosFrete(); }

  ui->labelEstoque->hide();
  ui->doubleSpinBoxEstoque->hide();

  ui->labelMinimo->hide();
  ui->doubleSpinBoxMinimo->hide();

  ui->labelUn->hide();
  ui->lineEditUn->hide();

  setConnections();
}

Orcamento::~Orcamento() { delete ui; }

void Orcamento::setItemBoxes() {
  ui->itemBoxCliente->setRegisterDialog("CadastroCliente");
  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxConsultor->setSearchDialog(SearchDialog::vendedor(this));
  ui->itemBoxEndereco->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxProduto->setSearchDialog(SearchDialog::produto(false, false, false, false, this));
  ui->itemBoxProfissional->setRegisterDialog("CadastroProfissional");
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(true, this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));
}

void Orcamento::show() {
  RegisterDialog::show();

  ui->groupBoxInfo->adjustSize();
  ui->groupBoxDados->adjustSize();

  ui->groupBoxInfo->setMaximumHeight(ui->groupBoxInfo->height());
  ui->groupBoxDados->setMaximumHeight(ui->groupBoxDados->height());
}

void Orcamento::on_tableProdutos_clicked(const QModelIndex &index) {
  if (isReadOnly) { return; }

  if (not index.isValid()) { return novoItem(); }

  ui->pushButtonAtualizarItem->show();
  ui->pushButtonRemoverItem->show();
  ui->pushButtonLimparSelecao->show();

  ui->pushButtonAdicionarItem->hide();

  currentRowItem = index.row();

  // -------------------------------------------------------------------------

  unsetConnections();

  try {
    mapperItem.setCurrentModelIndex(index);
    setarParametrosProduto();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  // -------------------------------------------------------------------------

  resizeSpinBoxes();
}

void Orcamento::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

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
  connect(ui->itemBoxProfissional, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProfissional_idChanged, connectionType);
  connect(ui->itemBoxVendedor, &ItemBox::textChanged, this, &Orcamento::on_itemBoxVendedor_textChanged, connectionType);
  connect(ui->pushButtonAdicionarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAdicionarItem_clicked, connectionType);
  connect(ui->pushButtonApagarOrc, &QPushButton::clicked, this, &Orcamento::on_pushButtonApagarOrc_clicked, connectionType);
  connect(ui->pushButtonAtualizarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarItem_clicked, connectionType);
  connect(ui->pushButtonAtualizarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarOrcamento_clicked, connectionType);
  connect(ui->pushButtonCadastrarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonCadastrarOrcamento_clicked, connectionType);
  connect(ui->pushButtonCalculadora, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalculadora_clicked, connectionType);
  connect(ui->pushButtonCalcularFrete, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalcularFrete_clicked, connectionType);
  connect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarExcel_clicked, connectionType);
  connect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarPdf_clicked, connectionType);
  connect(ui->pushButtonGerarVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarVenda_clicked, connectionType);
  connect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &Orcamento::novoItem, connectionType);
  connect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &Orcamento::on_pushButtonModelo3d_clicked, connectionType);
  connect(ui->pushButtonRemoverItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonRemoverItem_clicked, connectionType);
  connect(ui->pushButtonReplicar, &QPushButton::clicked, this, &Orcamento::on_pushButtonReplicar_clicked, connectionType);
  connect(ui->tableProdutos, &TableView::clicked, this, &Orcamento::on_tableProdutos_clicked, connectionType);
}

void Orcamento::unsetConnections() {
  blockingSignals.push(0);

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
  disconnect(ui->itemBoxProfissional, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProfissional_idChanged);
  disconnect(ui->itemBoxVendedor, &ItemBox::textChanged, this, &Orcamento::on_itemBoxVendedor_textChanged);
  disconnect(ui->pushButtonAdicionarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAdicionarItem_clicked);
  disconnect(ui->pushButtonApagarOrc, &QPushButton::clicked, this, &Orcamento::on_pushButtonApagarOrc_clicked);
  disconnect(ui->pushButtonAtualizarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarItem_clicked);
  disconnect(ui->pushButtonAtualizarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarOrcamento_clicked);
  disconnect(ui->pushButtonCadastrarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonCadastrarOrcamento_clicked);
  disconnect(ui->pushButtonCalculadora, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalculadora_clicked);
  disconnect(ui->pushButtonCalcularFrete, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalcularFrete_clicked);
  disconnect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarExcel_clicked);
  disconnect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarPdf_clicked);
  disconnect(ui->pushButtonGerarVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarVenda_clicked);
  disconnect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &Orcamento::novoItem);
  disconnect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &Orcamento::on_pushButtonModelo3d_clicked);
  disconnect(ui->pushButtonRemoverItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonRemoverItem_clicked);
  disconnect(ui->pushButtonReplicar, &QPushButton::clicked, this, &Orcamento::on_pushButtonReplicar_clicked);
  disconnect(ui->tableProdutos, &TableView::clicked, this, &Orcamento::on_tableProdutos_clicked);
}

bool Orcamento::viewRegister() {
  unsetConnections();

  auto load = [&] {
    if (not RegisterDialog::viewRegister()) { return false; }

    //-----------------------------------------------------------------

    modelItem.setFilter("idOrcamento = '" + model.data(0, "idOrcamento").toString() + "'");

    modelItem.select();

    //-----------------------------------------------------------------

    buscarParametrosFrete();

    novoItem();

    const int validade = data("validade").toInt();
    ui->spinBoxValidade->setMaximum(validade);
    ui->spinBoxValidade->setValue(validade);

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

      ui->pushButtonReplicar->show();

      ui->frameProduto->hide();

      ui->pushButtonGerarVenda->hide();
      ui->pushButtonAtualizarOrcamento->hide();

      ui->checkBoxFreteManual->setDisabled(true);

      ui->itemBoxCliente->setReadOnlyItemBox(true);
      ui->itemBoxEndereco->setReadOnlyItemBox(true);
      ui->itemBoxProduto->setReadOnlyItemBox(true);
      ui->itemBoxProfissional->setReadOnlyItemBox(true);
      ui->itemBoxVendedor->setReadOnlyItemBox(true);

      ui->dataEmissao->setReadOnly(true);
      ui->doubleSpinBoxDesconto->setReadOnly(true);
      ui->doubleSpinBoxDescontoGlobal->setReadOnly(true);
      ui->doubleSpinBoxDescontoGlobalReais->setReadOnly(true);
      ui->doubleSpinBoxFrete->setReadOnly(true);
      ui->doubleSpinBoxQuant->setReadOnly(true);
      ui->doubleSpinBoxSubTotalBruto->setReadOnly(true);
      ui->doubleSpinBoxSubTotalLiq->setReadOnly(true);
      ui->doubleSpinBoxTotal->setReadOnly(true);
      ui->doubleSpinBoxTotalItem->setReadOnly(true);
      ui->plainTextEditObs->setReadOnly(true);
      ui->spinBoxPrazoEntrega->setReadOnly(true);

      ui->dataEmissao->setReadOnly(true);
      ui->dataEmissao->setCalendarPopup(false);

      ui->spinBoxValidade->setReadOnly(true);
      ui->spinBoxValidade->setButtonSymbols(QSpinBox::NoButtons);

      ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::NoButtons);
      ui->doubleSpinBoxDescontoGlobalReais->setButtonSymbols(QDoubleSpinBox::NoButtons);
      ui->doubleSpinBoxFrete->setButtonSymbols(QDoubleSpinBox::NoButtons);
      ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::NoButtons);
    } else {
      ui->pushButtonGerarVenda->show();
    }

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
      const QString idLoja = User::fromLoja("usuario.idLoja", ui->itemBoxVendedor->text()).toString();
      ui->itemBoxVendedor->setFilter("idLoja = " + idLoja);
    }

    const QString idCliente = QString::number(ui->itemBoxCliente->getId().toInt());
    ui->itemBoxEndereco->setFilter("(idCliente = " + idCliente + " OR idEndereco = 1) AND desativado = FALSE");

    calcularPesoTotal();

    return true;
  }();

  setConnections();

  return load;
}

QVariant Orcamento::dataItem(const QString &key) const { return modelItem.data(currentRowItem, key); }

void Orcamento::setDataItem(const QString &key, const QVariant &value, const bool adjustValue) { modelItem.setData(currentRowItem, key, value, adjustValue); }

void Orcamento::novoItem() {
  ui->pushButtonAdicionarItem->show();

  ui->pushButtonAtualizarItem->hide();
  ui->pushButtonRemoverItem->hide();
  ui->pushButtonLimparSelecao->hide();

  ui->itemBoxProduto->clear();
  ui->tableProdutos->clearSelection();

  // -----------------------

  ui->doubleSpinBoxEstoque->setSuffix("");
  ui->doubleSpinBoxMinimo->setSuffix("");
  ui->doubleSpinBoxQuant->setSuffix("");
  ui->doubleSpinBoxQuantCx->setSuffix("");

  ui->doubleSpinBoxCaixas->setDisabled(true);
  ui->doubleSpinBoxDesconto->setDisabled(true);
  ui->doubleSpinBoxEstoque->setDisabled(true);
  ui->doubleSpinBoxMinimo->setDisabled(true);
  ui->doubleSpinBoxPrecoUn->setDisabled(true);
  ui->doubleSpinBoxQuant->setDisabled(true);
  ui->doubleSpinBoxQuantCx->setDisabled(true);
  ui->doubleSpinBoxTotalItem->setDisabled(true);
  ui->lineEditCodComercial->setDisabled(true);
  ui->lineEditFormComercial->setDisabled(true);
  ui->lineEditFornecedor->setDisabled(true);
  ui->lineEditObs->setDisabled(true);
  ui->lineEditUn->setDisabled(true);

  ui->doubleSpinBoxCaixas->setSingleStep(1.);
  ui->doubleSpinBoxQuant->setSingleStep(1.);

  ui->doubleSpinBoxCaixas->clear();
  ui->doubleSpinBoxDesconto->clear();
  ui->doubleSpinBoxEstoque->clear();
  ui->doubleSpinBoxMinimo->clear();
  ui->doubleSpinBoxPrecoUn->clear();
  ui->doubleSpinBoxQuant->clear();
  ui->doubleSpinBoxQuantCx->clear();
  ui->doubleSpinBoxTotalItem->clear();
  ui->lineEditCodComercial->clear();
  ui->lineEditFormComercial->clear();
  ui->lineEditFornecedor->clear();
  ui->lineEditObs->clear();
  ui->lineEditUn->clear();
}

void Orcamento::setupMapper() {
  // widgets são alterados durante a execução na ordem em que foram mapeados

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
  addMapping(ui->itemBoxConsultor, "idUsuarioConsultor", "id");
  addMapping(ui->itemBoxEndereco, "idEnderecoEntrega", "id");
  addMapping(ui->itemBoxProfissional, "idProfissional", "id");
  addMapping(ui->itemBoxVendedor, "idUsuario", "id");
  addMapping(ui->lineEditOrcamento, "idOrcamento");
  addMapping(ui->lineEditReplicaDe, "replicadoDe");
  addMapping(ui->lineEditReplicadoEm, "replicadoEm");
  addMapping(ui->plainTextEditBaixa, "observacaoCancelamento");
  addMapping(ui->plainTextEditObs, "observacao");
  addMapping(ui->spinBoxPrazoEntrega, "prazoEntrega");
  addMapping(ui->spinBoxValidade, "validade");

  mapperItem.setModel(ui->tableProdutos->model());
  mapperItem.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

  mapperItem.addMapping(ui->doubleSpinBoxCaixas, modelItem.fieldIndex("caixas"));
  mapperItem.addMapping(ui->doubleSpinBoxDesconto, modelItem.fieldIndex("desconto"));
  mapperItem.addMapping(ui->doubleSpinBoxPrecoUn, modelItem.fieldIndex("prcUnitario"));
  mapperItem.addMapping(ui->doubleSpinBoxQuant, modelItem.fieldIndex("quant"));
  mapperItem.addMapping(ui->doubleSpinBoxTotalItem, modelItem.fieldIndex("parcialDesc"));
  mapperItem.addMapping(ui->itemBoxProduto, modelItem.fieldIndex("idProduto"), "id");
  mapperItem.addMapping(ui->lineEditCodComercial, modelItem.fieldIndex("codComercial"));
  mapperItem.addMapping(ui->lineEditFormComercial, modelItem.fieldIndex("formComercial"));
  mapperItem.addMapping(ui->lineEditFornecedor, modelItem.fieldIndex("fornecedor"));
  mapperItem.addMapping(ui->lineEditObs, modelItem.fieldIndex("obs"));
  mapperItem.addMapping(ui->lineEditUn, modelItem.fieldIndex("un"));
}

void Orcamento::registerMode() {
  ui->pushButtonCadastrarOrcamento->show();

  ui->itemBoxConsultor->hide();
  ui->labelBaixa->hide();
  ui->labelConsultor->hide();
  ui->labelReplicaDe->hide();
  ui->labelReplicadoEm->hide();
  ui->lineEditReplicaDe->hide();
  ui->lineEditReplicadoEm->hide();
  ui->plainTextEditBaixa->hide();
  ui->pushButtonAtualizarOrcamento->hide();
  ui->pushButtonCalcularFrete->hide();
  ui->pushButtonReplicar->hide();

  ui->itemBoxConsultor->setReadOnlyItemBox(true);

  ui->pushButtonApagarOrc->setDisabled(true);
  ui->pushButtonGerarExcel->setDisabled(true);
  ui->pushButtonGerarPdf->setDisabled(true);
  ui->pushButtonGerarVenda->setEnabled(true);
  //  ui->itemBoxEndereco->setDisabled(true);
}

void Orcamento::updateMode() {
  ui->itemBoxEndereco->show();
  ui->pushButtonAtualizarOrcamento->show();
  ui->pushButtonReplicar->show();

  ui->pushButtonCadastrarOrcamento->hide();

  ui->pushButtonApagarOrc->setEnabled(true);
  ui->pushButtonGerarExcel->setEnabled(true);
  ui->pushButtonGerarPdf->setEnabled(true);
  ui->pushButtonGerarVenda->setEnabled(true);

  ui->spinBoxValidade->setReadOnly(true);
  ui->checkBoxRepresentacao->setDisabled(true);

  ui->lineEditReplicaDe->setReadOnly(true);
  ui->lineEditReplicadoEm->setReadOnly(true);
}

bool Orcamento::newRegister() {
  if (not RegisterDialog::newRegister()) { return false; }

  ui->lineEditOrcamento->setText("Auto gerado");
  ui->dataEmissao->setDate(qApp->serverDate());
  on_dataEmissao_dateChanged(ui->dataEmissao->date());
  ui->spinBoxValidade->setValue(7);
  novoItem();

  return true;
}

void Orcamento::removeItem() {
  unsetConnections();

  try {
    if (not modelItem.removeRow(currentRowItem)) { throw RuntimeException("Erro removendo linha: " + modelItem.lastError().text()); }

    calcPrecoGlobalTotal();
    calcularPesoTotal();

    if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); }

    if (modelItem.rowCount() == 0) {
      if (ui->lineEditOrcamento->text() == "Auto gerado") { ui->checkBoxRepresentacao->setEnabled(true); }

      ui->itemBoxProduto->setFornecedorRep("");
    }

    redoBackupItem();

    novoItem();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::generateId() {
  const QString siglaLoja = User::fromLoja("sigla", ui->itemBoxVendedor->text()).toString();

  if (siglaLoja.isEmpty()) { throw RuntimeException("Erro buscando sigla da loja!"); }

  QString id = siglaLoja + "-" + qApp->serverDate().toString("yy");

  const QString replica = ui->lineEditReplicaDe->text();

  if (replica.isEmpty()) {
    SqlQuery query;
    query.prepare("SELECT MAX(idOrcamento) AS idOrcamento FROM orcamento WHERE idOrcamento LIKE :id");
    query.bindValue(":id", id + "%");

    if (not query.exec()) { throw RuntimeException("Erro buscando próximo id disponível: " + query.lastError().text()); }

    const int last = query.first() ? query.value("idOrcamento").toString().remove(id).left(4).toInt() : 0;

    id += QString("%1").arg(last + 1, 4, 10, QChar('0'));
    id += ui->checkBoxRepresentacao->isChecked() ? "R" : "";
    id += "O";

    if (id.size() != 12 and id.size() != 13) { throw RuntimeException("Tamanho do Id errado: " + id); }
  } else {
    SqlQuery query;
    query.prepare(
        "SELECT COALESCE(MAX(CAST(RIGHT(idOrcamento, CHAR_LENGTH(idOrcamento) - LOCATE('Rev', idOrcamento) - 2) AS UNSIGNED)) + 1, 1) AS revisao FROM orcamento WHERE CHAR_LENGTH(idOrcamento) > 16 "
        "AND idOrcamento LIKE :idOrcamento");
    query.bindValue(":idOrcamento", replica.left(11) + "%");

    if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando próxima revisão disponível: " + query.lastError().text()); }

    id = replica.left(replica.indexOf("-REV")) + "-REV" + query.value("revisao").toString();
  }

  ui->lineEditOrcamento->setText(id);
}

void Orcamento::corrigirValores() {
  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    const double quant = modelItem.data(row, "quant").toDouble();
    const double prcUnitario = modelItem.data(row, "prcUnitario").toDouble();
    const double descUnitario = modelItem.data(row, "descUnitario").toDouble();
    const double descGlobal = modelItem.data(row, "descGlobal").toDouble();

    modelItem.setData(row, "parcial", prcUnitario * quant);
    modelItem.setData(row, "parcialDesc", descUnitario * quant);
    modelItem.setData(row, "total", (descUnitario * quant) * (1 - (descGlobal / 100)));
  }
}

std::tuple<double, double, double> Orcamento::calcularTotais() {
  double subTotalBruto = 0.;
  double subTotalLiq = 0.;
  double total = 0.;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    subTotalBruto += modelItem.data(row, "parcial").toDouble();
    subTotalLiq += modelItem.data(row, "parcialDesc").toDouble();
    total += modelItem.data(row, "total").toDouble();
  }

  return std::make_tuple<>(subTotalBruto, subTotalLiq, total);
}

QString Orcamento::montarLog() {
  const auto [subTotalBruto, subTotalLiq, total] = calcularTotais();

  QStringList logString;

  logString << "IdOrcamento: " + ui->lineEditOrcamento->text();
  logString << "\nsubTotalBruto: " + QString::number(subTotalBruto) + "\nspinBoxBruto: " + QString::number(ui->doubleSpinBoxSubTotalBruto->value());
  logString << "\nsubTotalLiq: " + QString::number(subTotalLiq) + "\nspinBoxLiq: " + QString::number(ui->doubleSpinBoxSubTotalLiq->value());
  logString << "\ntotal: " + QString::number(total) + "\nspinBoxTotal: " + QString::number(ui->doubleSpinBoxTotal->value());
  logString << "\nspinBoxFrete: " + QString::number(ui->doubleSpinBoxFrete->value());
  logString << "";

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    logString << "--------------------";

    logString << "\nId: " + modelItem.data(row, "idOrcamentoProduto").toString() + "\nprcUnitario: " + modelItem.data(row, "prcUnitario").toString() +
                     "\ndescUnitario: " + modelItem.data(row, "descUnitario").toString() + "\nquant: " + modelItem.data(row, "quant").toString() +
                     "\ncodComercial: " + modelItem.data(row, "codComercial").toString() + "\nparcial: " + modelItem.data(row, "parcial").toString() +
                     "\ndesconto: " + modelItem.data(row, "desconto").toString() + "\nparcialDesc: " + modelItem.data(row, "parcialDesc").toString() +
                     "\ndescGlobal: " + modelItem.data(row, "descGlobal").toString() + "\ntotal: " + modelItem.data(row, "total").toString();
  }

  return logString.join("\n");
}

void Orcamento::verificarTotais() {
  const auto [subTotalBruto, subTotalLiq, total] = calcularTotais();

  const bool brutoErrado = abs(subTotalBruto - ui->doubleSpinBoxSubTotalBruto->value()) > 0.1;
  const bool liquidoErrado = abs(subTotalLiq - ui->doubleSpinBoxSubTotalLiq->value()) > 0.1;
  const bool totalErrado = abs(total - (ui->doubleSpinBoxTotal->value() - ui->doubleSpinBoxFrete->value())) > 0.1;

  if (brutoErrado or liquidoErrado or totalErrado) {
    corrigirValores();

    const auto [subTotalBruto2, subTotalLiq2, total2] = calcularTotais();

    const bool brutoErrado2 = abs(subTotalBruto2 - ui->doubleSpinBoxSubTotalBruto->value()) > 0.1;
    const bool liquidoErrado2 = abs(subTotalLiq2 - ui->doubleSpinBoxSubTotalLiq->value()) > 0.1;
    const bool totalErrado2 = abs(total2 - (ui->doubleSpinBoxTotal->value() - ui->doubleSpinBoxFrete->value())) > 0.1;

    if (brutoErrado2 or liquidoErrado2 or totalErrado2) {
      Log::createLog("Exceção", montarLog());

      throw RuntimeException("Erro nos valores! Entre em contato com o suporte!");
    }
  }
}

void Orcamento::verifyFields() {
  verificaSeFoiAlterado();

  verificaDisponibilidadeEstoque();

  verificarTotais();

  if (ui->itemBoxCliente->text().isEmpty()) { throw RuntimeError("Cliente inválido!", this); }

  if (ui->itemBoxVendedor->text().isEmpty()) { throw RuntimeError("Vendedor inválido!", this); }

  if (ui->itemBoxProfissional->text().isEmpty()) { throw RuntimeError("Profissional inválido!", this); }

  if (ui->itemBoxEndereco->text().isEmpty()) { throw RuntimeError(R"(Endereço inválido! Se não possui endereço, escolha "NÃO HÁ/RETIRA"!)", this); }
}

void Orcamento::savingProcedures() {
  if (tipo == Tipo::Cadastrar) {
    generateId();

    const int idLoja = User::fromLoja("usuario.idLoja", ui->itemBoxVendedor->text()).toInt();
    setData("idLoja", idLoja);

    setData("idOrcamento", ui->lineEditOrcamento->text());
    setData("idOrcamentoBase", ui->lineEditOrcamento->text().left(11));
    setData("replicadoDe", ui->lineEditReplicaDe->text());
    setData("representacao", ui->checkBoxRepresentacao->isChecked());

    atualizaReplica();
  }

  buscarConsultor();

  setData("data", ui->dataEmissao->isReadOnly() ? qApp->serverDateTime() : ui->dataEmissao->dateTime());
  setData("descontoPorc", ui->doubleSpinBoxDescontoGlobal->value());
  setData("descontoReais", ui->doubleSpinBoxSubTotalLiq->value() * ui->doubleSpinBoxDescontoGlobal->value() / 100.);
  setData("frete", ui->doubleSpinBoxFrete->value());
  setData("freteManual", ui->checkBoxFreteManual->isChecked());
  setData("idCliente", ui->itemBoxCliente->getId());
  setData("idEnderecoEntrega", ui->itemBoxEndereco->getId());
  setData("idProfissional", ui->itemBoxProfissional->getId());
  setData("idUsuario", ui->itemBoxVendedor->getId());
  setData("observacao", ui->plainTextEditObs->toPlainText());
  setData("prazoEntrega", ui->spinBoxPrazoEntrega->value());
  setData("subTotalBru", ui->doubleSpinBoxSubTotalBruto->value());
  setData("subTotalLiq", ui->doubleSpinBoxSubTotalLiq->value());
  setData("total", ui->doubleSpinBoxTotal->value());
  setData("validade", ui->spinBoxValidade->value());

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    modelItem.setData(row, "idOrcamento", ui->lineEditOrcamento->text());
    modelItem.setData(row, "idLoja", model.data(currentRow, "idLoja"));
  }
}

void Orcamento::buscarConsultor() {
  if (modelItem.rowCount() == 0) { return; }

  QStringList fornecedores;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) { fornecedores << modelItem.data(row, "fornecedor").toString(); }

  fornecedores.removeDuplicates();

  for (auto &fornecedor : fornecedores) { fornecedor.prepend("'").append("'"); }

  SqlQuery query;

  if (not query.exec("SELECT idUsuario FROM usuario WHERE desativado = FALSE AND especialidade > 0 AND especialidade IN (SELECT especialidade FROM fornecedor WHERE razaoSocial IN (" +
                     fornecedores.join(", ") + "))")) {
    throw RuntimeException("Erro buscando consultor: " + query.lastError().text());
  }

  if (query.size() > 1) { throw RuntimeException("Mais de um consultor disponível!"); }

  if (query.size() == 1 and query.first()) { setData("idUsuarioConsultor", query.value("idUsuario")); }

  if (query.size() == 0) { model.setData(currentRow, "idUsuarioConsultor", QVariant(QVariant::UInt)); }
}

void Orcamento::atualizaReplica() {
  if (ui->lineEditReplicaDe->text().isEmpty()) { return; }

  SqlQuery query;
  query.prepare("UPDATE orcamento SET status = 'REPLICADO', replicadoEm = :idReplica WHERE idOrcamento = :idOrcamento");
  query.bindValue(":idReplica", ui->lineEditOrcamento->text());
  query.bindValue(":idOrcamento", ui->lineEditReplicaDe->text());

  if (not query.exec()) { throw RuntimeException("Erro salvando replicadoEm: " + query.lastError().text()); }
}

void Orcamento::clearFields() {
  RegisterDialog::clearFields();

  if (User::isVendedor()) { ui->itemBoxVendedor->setId(User::idUsuario); }

  //  ui->itemBoxEndereco->setDisabled(true);
}

void Orcamento::on_pushButtonRemoverItem_clicked() { removeItem(); }

void Orcamento::on_doubleSpinBoxQuant_valueChanged(const double quant) {
  const double stepQt = ui->doubleSpinBoxQuant->singleStep();
  const double prcUn = ui->doubleSpinBoxPrecoUn->value();
  const double desc = ui->doubleSpinBoxDesconto->value() / 100.;

  unsetConnections();

  try {
    const double resto = fmod(quant, stepQt);
    const double quant2 = not qFuzzyIsNull(resto) ? ceil(quant / stepQt) * stepQt : quant;
    ui->doubleSpinBoxQuant->setValue(quant2);

    const double caixas2 = quant2 / stepQt;
    ui->doubleSpinBoxCaixas->setValue(caixas2);

    const double itemBruto2 = quant2 * prcUn;
    ui->doubleSpinBoxTotalItem->setValue(itemBruto2 * (1. - desc));
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
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
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

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

void Orcamento::on_pushButtonGerarPdf_clicked() {
  PDF pdf(data("idOrcamento").toString(), PDF::Tipo::Orcamento, this);
  pdf.gerarPdf();
}

void Orcamento::setupTables() {
  modelItem.setTable("orcamento_has_produto");

  modelItem.setHeaderData("produto", "Produto");
  modelItem.setHeaderData("fornecedor", "Fornecedor");
  modelItem.setHeaderData("obs", "Obs.");
  modelItem.setHeaderData("prcUnitario", "Preço/Un.");
  modelItem.setHeaderData("kg", "Kg.");
  modelItem.setHeaderData("caixas", "Caixas");
  modelItem.setHeaderData("quant", "Quant.");
  modelItem.setHeaderData("un", "Un.");
  modelItem.setHeaderData("codComercial", "Código");
  modelItem.setHeaderData("formComercial", "Formato");
  modelItem.setHeaderData("quantCaixa", "Quant./Cx.");
  modelItem.setHeaderData("parcial", "Subtotal");
  modelItem.setHeaderData("desconto", "Desc. %");
  modelItem.setHeaderData("parcialDesc", "Total");

  modelItem.proxyModel = new SearchDialogProxyModel(&modelItem, this);

  ui->tableProdutos->setModel(&modelItem);

  ui->tableProdutos->hideColumn("idOrcamentoProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idOrcamento");
  ui->tableProdutos->hideColumn("idLoja");
  ui->tableProdutos->hideColumn("quantCaixa");
  ui->tableProdutos->hideColumn("descUnitario");
  ui->tableProdutos->hideColumn("descGlobal");
  ui->tableProdutos->hideColumn("total");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("mostrarDesconto");

  ui->tableProdutos->setItemDelegate(new DoubleDelegate(this));

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));
  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("parcial", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("parcialDesc", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("desconto", new PorcentagemDelegate(false, this));
}

void Orcamento::atualizarItem() { adicionarItem(Tipo::Atualizar); }

void Orcamento::adicionarItem(const Tipo tipoItem) {
  if (ui->itemBoxProduto->text().isEmpty()) { throw RuntimeError("Item inválido!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuant->value())) { throw RuntimeError("Quantidade inválida!", this); }

  unsetConnections();

  try {
    if (tipoItem == Tipo::Cadastrar) { currentRowItem = modelItem.insertRowAtEnd(); }

    setDataItem("idProduto", ui->itemBoxProduto->getId().toInt());
    setDataItem("fornecedor", ui->lineEditFornecedor->text());
    setDataItem("produto", ui->itemBoxProduto->text());
    setDataItem("obs", ui->lineEditObs->text());
    setDataItem("prcUnitario", ui->doubleSpinBoxPrecoUn->value());
    setDataItem("kg", calcularPeso());
    setDataItem("caixas", ui->doubleSpinBoxCaixas->value());
    setDataItem("quant", ui->doubleSpinBoxQuant->value());
    setDataItem("quantCaixa", ui->doubleSpinBoxQuant->singleStep());
    setDataItem("un", ui->lineEditUn->text());
    setDataItem("codComercial", ui->lineEditCodComercial->text());
    setDataItem("formComercial", ui->lineEditFormComercial->text());
    setDataItem("desconto", ui->doubleSpinBoxDesconto->value());
    setDataItem("estoque", currentItemIsEstoque);
    setDataItem("promocao", currentItemIsPromocao);
    setDataItem("parcial", dataItem("quant").toDouble() * dataItem("prcUnitario").toDouble());
    setDataItem("parcialDesc", ui->doubleSpinBoxTotalItem->value());
    setDataItem("descGlobal", ui->doubleSpinBoxDescontoGlobal->value());
    setDataItem("total", ui->doubleSpinBoxTotalItem->value() * (1 - (ui->doubleSpinBoxDescontoGlobal->value() / 100)));

    //------------------------------------------

    const double prcUnitario = dataItem("prcUnitario").toDouble();
    const double desconto = dataItem("desconto").toDouble() / 100.;

    setDataItem("descUnitario", prcUnitario * (1 - desconto));

    const bool mostrarDesconto = (dataItem("parcialDesc").toDouble() - dataItem("parcial").toDouble()) < -0.1;

    setDataItem("mostrarDesconto", mostrarDesconto);

    //------------------------------------------

    if (modelItem.rowCount() == 1 and ui->checkBoxRepresentacao->isChecked()) { ui->itemBoxProduto->setFornecedorRep(dataItem("fornecedor").toString()); }

    redoBackupItem();

    isDirty = true;
    ui->checkBoxRepresentacao->setDisabled(true);

    novoItem();

    calcPrecoGlobalTotal();
    calcularPesoTotal();

    if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_pushButtonAdicionarItem_clicked() { adicionarItem(); }

void Orcamento::on_pushButtonAtualizarItem_clicked() { atualizarItem(); }

void Orcamento::on_pushButtonGerarVenda_clicked() {
  save(true);

  const QDateTime time = ui->dataEmissao->dateTime();

  if (not time.isValid()) { return; }

  if (time.addDays(data("validade").toInt()).date() < qApp->serverDate()) { throw RuntimeError("Orçamento vencido!"); }

  if (ui->itemBoxEndereco->text().isEmpty()) { throw RuntimeError("Deve selecionar endereço!"); }

  verificaCadastroCliente();

  auto *venda = new Venda(parentWidget());
  venda->setAttribute(Qt::WA_DeleteOnClose);
  venda->prepararVenda(ui->lineEditOrcamento->text());
  venda->show();

  close();
}

void Orcamento::on_doubleSpinBoxCaixas_valueChanged(const double caixas) {
  const double stepQt = ui->doubleSpinBoxQuant->singleStep();
  const double stepCx = ui->doubleSpinBoxCaixas->singleStep();
  const double prcUn = ui->doubleSpinBoxPrecoUn->value();
  const double desc = ui->doubleSpinBoxDesconto->value() / 100.;

  unsetConnections();

  try {
    const double resto = fmod(caixas, stepCx);
    const double caixas2 = not qFuzzyIsNull(resto) ? ceil(caixas) : caixas;
    ui->doubleSpinBoxCaixas->setValue(caixas2);

    const double quant2 = caixas2 * stepQt;
    ui->doubleSpinBoxQuant->setValue(quant2);

    const double itemBruto2 = quant2 * prcUn;
    ui->doubleSpinBoxTotalItem->setValue(itemBruto2 * (1. - desc));
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_pushButtonApagarOrc_clicked() {
  auto *baixa = new BaixaOrcamento(data("idOrcamento").toString(), this);
  baixa->show();
}

void Orcamento::resizeSpinBoxes() {
  ui->doubleSpinBoxPrecoUn->resizeToContent();
  ui->doubleSpinBoxCaixas->resizeToContent();
  ui->doubleSpinBoxQuant->resizeToContent();
  ui->doubleSpinBoxMinimo->resizeToContent();
  ui->doubleSpinBoxQuantCx->resizeToContent();
  ui->doubleSpinBoxDesconto->resizeToContent();
  ui->doubleSpinBoxTotalItem->resizeToContent();
  ui->doubleSpinBoxEstoque->resizeToContent();
}

void Orcamento::on_itemBoxProduto_idChanged(const QVariant &) {
  if (ui->itemBoxProduto->text().isEmpty()) { return; }

  // -------------------------------------------------------------------------

  ui->doubleSpinBoxCaixas->clear();
  ui->doubleSpinBoxDesconto->clear();
  ui->doubleSpinBoxEstoque->clear();
  ui->doubleSpinBoxPrecoUn->clear();
  ui->doubleSpinBoxQuant->clear();
  ui->doubleSpinBoxTotalItem->clear();
  ui->lineEditCodComercial->clear();
  ui->lineEditFormComercial->clear();
  ui->lineEditFornecedor->clear();
  ui->lineEditUn->clear();
  ui->doubleSpinBoxMinimo->clear();
  ui->doubleSpinBoxQuantCx->clear();

  // não apagar observação caso esteja atualizando produto
  if (ui->pushButtonAdicionarItem->isVisible()) { ui->lineEditObs->clear(); }

  // -------------------------------------------------------------------------

  unsetConnections();

  try {
    ui->doubleSpinBoxCaixas->setValue(0.);
    ui->doubleSpinBoxQuant->setValue(0.);
    ui->doubleSpinBoxDesconto->setValue(0.);
    ui->doubleSpinBoxTotalItem->setValue(0.);
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  // -------------------------------------------------------------------------

  setarParametrosProduto();

  // -------------------------------------------------------------------------

  resizeSpinBoxes();
}

void Orcamento::setarParametrosProduto() {
  SqlQuery query;
  query.prepare("SELECT un, precoVenda, estoqueRestante, fornecedor, codComercial, formComercial, quantCaixa, minimo, multiplo, estoque, promocao FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro na busca do produto: " + query.lastError().text()); }

  // -------------------------------------------------------------------------

  ui->doubleSpinBoxEstoque->setValue(query.value("estoqueRestante").toDouble());
  ui->doubleSpinBoxPrecoUn->setValue(query.value("precoVenda").toDouble());
  ui->lineEditCodComercial->setText(query.value("codComercial").toString());
  ui->lineEditFormComercial->setText(query.value("formComercial").toString());
  ui->lineEditFornecedor->setText(query.value("fornecedor").toString());
  ui->lineEditUn->setText(query.value("un").toString().toUpper());

  // -------------------------------------------------------------------------

  const double minimo = query.value("minimo").toDouble();
  const double quantCaixa = query.value("quantCaixa").toDouble();

  ui->doubleSpinBoxQuantCx->setValue(quantCaixa);
  ui->doubleSpinBoxMinimo->setValue(minimo);

  ui->doubleSpinBoxQuant->setMinimum(minimo);
  ui->doubleSpinBoxCaixas->setMinimum(minimo / quantCaixa);

  ui->doubleSpinBoxCaixas->setSingleStep(1);
  ui->doubleSpinBoxQuant->setSingleStep(quantCaixa);

  // -------------------------------------------------------------------------

  const bool mostraMinimo = not qFuzzyIsNull(minimo);
  const double multiplo = query.value("multiplo").toDouble();

  ui->doubleSpinBoxMinimo->setVisible(mostraMinimo);
  ui->labelMinimo->setVisible(mostraMinimo);

  if (not qFuzzyIsNull(multiplo)) {
    ui->doubleSpinBoxCaixas->setSingleStep(multiplo / quantCaixa);
    ui->doubleSpinBoxQuant->setSingleStep(multiplo);
  }

  // -------------------------------------------------------------------------

  currentItemIsEstoque = query.value("estoque").toBool();
  currentItemIsPromocao = query.value("promocao").toInt();

  if (currentItemIsEstoque) {
    ui->doubleSpinBoxCaixas->setMaximum(query.value("estoqueRestante").toDouble() / quantCaixa);
    ui->doubleSpinBoxQuant->setMaximum(query.value("estoqueRestante").toDouble());
  } else {
    ui->doubleSpinBoxCaixas->setMaximum(9'999'999.000000);
    ui->doubleSpinBoxQuant->setMaximum(9'999'999.000000);
  }

  ui->labelEstoque->setVisible(currentItemIsEstoque);
  ui->doubleSpinBoxEstoque->setVisible(currentItemIsEstoque);

  // -------------------------------------------------------------------------

  ui->doubleSpinBoxCaixas->setEnabled(true);
  ui->doubleSpinBoxDesconto->setEnabled(true);
  ui->doubleSpinBoxEstoque->setEnabled(true);
  ui->doubleSpinBoxMinimo->setEnabled(true);
  ui->doubleSpinBoxPrecoUn->setEnabled(true);
  ui->doubleSpinBoxQuant->setEnabled(true);
  ui->doubleSpinBoxQuantCx->setEnabled(true);
  ui->doubleSpinBoxTotalItem->setEnabled(true);
  ui->lineEditCodComercial->setEnabled(true);
  ui->lineEditFormComercial->setEnabled(true);
  ui->lineEditFornecedor->setEnabled(true);
  ui->lineEditObs->setEnabled(true);
  ui->lineEditUn->setEnabled(true);

  // -------------------------------------------------------------------------

  ui->doubleSpinBoxEstoque->setSuffix(" " + ui->lineEditUn->text());
  ui->doubleSpinBoxMinimo->setSuffix(" " + ui->lineEditUn->text());
  ui->doubleSpinBoxQuant->setSuffix(" " + ui->lineEditUn->text());
  ui->doubleSpinBoxQuantCx->setSuffix(" " + ui->lineEditUn->text());
}

void Orcamento::on_itemBoxProfissional_idChanged(const QVariant &) {
  const auto idProfissional = ui->itemBoxProfissional->getId();

  SqlQuery query;

  if (not query.exec("SELECT comissao FROM profissional WHERE idProfissional = " + idProfissional.toString()) or not query.first()) {
    throw RuntimeException("Erro buscando dados do profissional: " + query.lastError().text());
  }

  ui->itemBoxProfissional->setStyleSheet(query.value("comissao").toDouble() > 5 ? "background-color: rgb(255, 255, 127); color: rgb(0, 0, 0);" : "");
}

void Orcamento::on_itemBoxCliente_textChanged(const QString &) {
  const QString idCliente = QString::number(ui->itemBoxCliente->getId().toInt());
  ui->itemBoxEndereco->setFilter("(idCliente = " + idCliente + " OR idEndereco = 1) AND desativado = FALSE");

  SqlQuery queryCliente;
  queryCliente.prepare("SELECT idProfissionalRel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not queryCliente.exec() or not queryCliente.first()) { throw RuntimeException("Erro ao buscar cliente: " + queryCliente.lastError().text()); }

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

  ui->doubleSpinBoxFrete->setFocus();
}

void Orcamento::on_pushButtonReplicar_clicked() {
  // passar por cada produto verificando sua validade/descontinuado
  QStringList produtos;
  QStringList estoques;
  QVector<int> skipRows;

  SqlQuery queryProduto;
  queryProduto.prepare("SELECT (descontinuado OR desativado) AS invalido FROM produto WHERE idProduto = :idProduto");

  SqlQuery queryEquivalente;
  queryEquivalente.prepare("SELECT idProduto FROM produto WHERE fornecedor = :fornecedor AND codComercial = :codComercial AND descontinuado = FALSE AND desativado = FALSE AND estoque = FALSE");

  SqlQuery queryEstoque;
  queryEstoque.prepare("SELECT 0 FROM produto WHERE idProduto = :idProduto AND estoqueRestante >= :quant");

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    const bool isEstoque = modelItem.data(row, "estoque").toBool();

    if (not isEstoque) {
      queryProduto.bindValue(":idProduto", modelItem.data(row, "idProduto"));

      if (not queryProduto.exec() or not queryProduto.first()) { throw RuntimeException("Erro verificando validade dos produtos: " + queryProduto.lastError().text()); }

      if (queryProduto.value("invalido").toBool()) {
        queryEquivalente.bindValue(":fornecedor", modelItem.data(row, "fornecedor"));
        queryEquivalente.bindValue(":codComercial", modelItem.data(row, "codComercial"));

        if (not queryEquivalente.exec()) { throw RuntimeException("Erro procurando produto equivalente: " + queryEquivalente.lastError().text()); }

        if (queryEquivalente.first()) {
          modelItem.setData(row, "idProduto", queryEquivalente.value("idProduto"));
        } else {
          produtos << QString::number(row + 1) + " - " + modelItem.data(row, "produto").toString();
          skipRows << row;
        }
      }
    }

    if (isEstoque) {
      queryEstoque.bindValue(":idProduto", modelItem.data(row, "idProduto"));
      queryEstoque.bindValue(":quant", modelItem.data(row, "quant"));

      if (not queryEstoque.exec()) { throw RuntimeException("Erro verificando estoque: " + queryEstoque.lastError().text()); }

      if (not queryEstoque.first()) {
        estoques << modelItem.data(row, "produto").toString();
        skipRows << row;
      }
    }
  }

  if (not produtos.isEmpty()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Os seguintes itens estão descontinuados e serão removidos da réplica:\n    -" + produtos.join("\n    -"), QMessageBox::Yes | QMessageBox::No,
                       this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  if (not estoques.isEmpty()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!",
                       "Os seguintes produtos de estoque não estão mais disponíveis na quantidade selecionada e serão removidos da réplica:\n    -" + estoques.join("\n    -"),
                       QMessageBox::Yes | QMessageBox::No, this);
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
  replica->ui->dataEmissao->setDate(qApp->serverDate());
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

void Orcamento::cadastrar() {
  try {
    qApp->startTransaction("Orcamento::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = ui->lineEditOrcamento->text();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    modelItem.submitAll();

    qApp->endTransaction();

    backupItem.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelItem.setFilter(primaryKey + " = '" + primaryId + "'");
  } catch (std::exception &) {
    qApp->rollbackTransaction();
    model.select();
    modelItem.select();

    for (auto &record : backupItem) { modelItem.insertRecord(-1, record); }

    if (tipo == Tipo::Cadastrar) { ui->lineEditOrcamento->setText("Auto gerado"); }

    throw;
  }
}

void Orcamento::verificaCadastroCliente() {
  SqlQuery queryCadastro;

  if (not queryCadastro.exec("SELECT incompleto FROM cliente WHERE idCliente = " + ui->itemBoxCliente->getId().toString()) or not queryCadastro.first()) {
    throw RuntimeException("Erro verificando se cadastro do cliente está completo: " + queryCadastro.lastError().text());
  }

  const bool incompleto = queryCadastro.value("incompleto").toBool();

  if (incompleto) {
    auto *cadCliente = new CadastroCliente(this);
    cadCliente->viewRegisterById(ui->itemBoxCliente->getId());
    cadCliente->marcarCompletar();
    cadCliente->show();

    throw RuntimeError("Cadastro incompleto, preencha os campos obrigatórios!");
  }
}

void Orcamento::on_pushButtonGerarExcel_clicked() {
  Excel excel(ui->lineEditOrcamento->text(), Excel::Tipo::Orcamento, this);
  excel.gerarExcel();
}

void Orcamento::on_checkBoxRepresentacao_toggled(const bool checked) {
  ui->itemBoxProduto->setRepresentacao(checked);
  novoItem();
}

void Orcamento::on_doubleSpinBoxDesconto_valueChanged(const double desconto) {
  const double caixas = ui->doubleSpinBoxCaixas->value();
  const double caixas2 = not qFuzzyIsNull(fmod(caixas, ui->doubleSpinBoxCaixas->singleStep())) ? ceil(caixas) : caixas;
  const double quant = caixas2 * ui->doubleSpinBoxQuantCx->value();

  unsetConnections();

  try {
    const double prcUn = ui->doubleSpinBoxPrecoUn->value();
    const double itemBruto = quant * prcUn;

    ui->doubleSpinBoxTotalItem->setValue(itemBruto * (1. - (desconto / 100)));
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) {
  unsetConnections();

  try {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      modelItem.setData(row, "descGlobal", descontoPorc * 100);

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc));
    }

    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - descontoReais + frete);
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_doubleSpinBoxFrete_valueChanged(const double frete) {
  const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
  const double desconto = ui->doubleSpinBoxDescontoGlobalReais->value();

  unsetConnections();

  try {
    ui->doubleSpinBoxTotal->setMinimum(frete);
    ui->doubleSpinBoxTotal->setMaximum(ui->doubleSpinBoxSubTotalLiq->value() + frete);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - desconto + frete);
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_itemBoxVendedor_textChanged(const QString &) {
  if (ui->itemBoxVendedor->text().isEmpty()) { return; }

  buscarParametrosFrete();

  if (not ui->checkBoxFreteManual->isChecked()) {
    const double frete = qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete);

    ui->doubleSpinBoxFrete->setMinimum(frete);
    ui->doubleSpinBoxFrete->setValue(frete);
  }
}

void Orcamento::buscarParametrosFrete() {
  const int idLoja = User::fromLoja("usuario.idLoja", ui->itemBoxVendedor->text()).toInt();

  if (idLoja == 0) { throw RuntimeException("Erro buscando idLoja!"); }

  SqlQuery queryFrete;
  queryFrete.prepare("SELECT valorMinimoFrete, porcentagemFrete FROM loja WHERE idLoja = :idLoja");
  queryFrete.bindValue(":idLoja", idLoja);

  if (not queryFrete.exec() or not queryFrete.next()) { throw RuntimeException("Erro buscando parâmetros do frete: " + queryFrete.lastError().text()); }

  minimoFrete = queryFrete.value("valorMinimoFrete").toDouble();
  porcFrete = queryFrete.value("porcentagemFrete").toDouble();
}

void Orcamento::on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) {
  unsetConnections();

  try {
    const double descontoPorc2 = descontoPorc / 100;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      modelItem.setData(row, "descGlobal", descontoPorc);

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc2));
    }

    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobalReais->setValue(subTotalLiq * descontoPorc2);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq * (1 - descontoPorc2) + frete);
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_doubleSpinBoxTotal_valueChanged(const double total) {
  unsetConnections();

  try {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();
    const double descontoReais = subTotalLiq + frete - total;
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      modelItem.setData(row, "descGlobal", descontoPorc * 100);

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc));
    }

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxDescontoGlobalReais->setValue(descontoReais);
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Orcamento::on_doubleSpinBoxTotalItem_valueChanged(const double) {
  if (ui->itemBoxProduto->text().isEmpty()) { return; }

  const double quant = ui->doubleSpinBoxQuant->value();
  const double prcUn = ui->doubleSpinBoxPrecoUn->value();
  const double itemBruto = quant * prcUn;
  const double subTotalItem = ui->doubleSpinBoxTotalItem->value();
  const double desconto = (itemBruto - subTotalItem) / itemBruto * 100.;

  if (qFuzzyIsNull(itemBruto)) { return; }

  unsetConnections();

  ui->doubleSpinBoxDesconto->setValue(desconto);

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

  if (qFuzzyIsNull(dist)) { throw RuntimeException("Não foi possível determinar a distância!"); }

  int peso = 0;

  SqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    query.bindValue(":idProduto", modelItem.data(row, "idProduto"));

    if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando peso do produto: " + query.lastError().text()); }

    peso += modelItem.data(row, "caixas").toInt() * query.value("kgcx").toInt();
  }

  if (not query.exec("SELECT custoTransporteTon, custoTransporte1, custoTransporte2, custoFuncionario FROM loja WHERE nomeFantasia = 'Geral'") or not query.first()) {
    throw RuntimeException("Erro buscando parâmetros: " + query.lastError().text());
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

void Orcamento::on_dataEmissao_dateChanged(const QDate date) { ui->spinBoxValidade->setMaximum(date.daysInMonth() - date.day()); }

void Orcamento::verificaDisponibilidadeEstoque() {
  SqlQuery query;

  QStringList produtos;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion
    if (modelItem.data(row, "estoque").toInt() != 1) { continue; }

    const QString idProduto = modelItem.data(row, "idProduto").toString();
    const QString quant = modelItem.data(row, "quant").toString();

    if (not query.exec("SELECT 0 FROM produto WHERE idProduto = " + idProduto + " AND estoqueRestante >= " + quant)) {
      throw RuntimeException("Erro verificando a disponibilidade do estoque: " + query.lastError().text());
    }

    if (not query.first()) { produtos << modelItem.data(row, "produto").toString(); }
  }

  if (not produtos.isEmpty()) {
    throw RuntimeError("Os seguintes produtos de estoque não estão mais disponíveis na quantidade selecionada:\n    -" + produtos.join("\n    -") + "\n\nRemova ou diminua a quant. para prosseguir!");
  }
}

// TODO: esse código está repetido em venda e searchDialog, refatorar
void Orcamento::on_pushButtonModelo3d_clicked() {
  const auto selection = ui->tableProdutos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const int row = selection.first().row();

  const QString ip = qApp->getWebDavIp();
  const QString fornecedor = modelItem.data(row, "fornecedor").toString();
  const QString codComercial = modelItem.data(row, "codComercial").toString();

  const QString url = "https://" + ip + "/webdav/METAIS_VIVIANE/MODELOS 3D/" + fornecedor + "/" + codComercial + ".skp";

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    authenticator->setUser(User::usuario);
    authenticator->setPassword(User::senha);
  });

  auto reply = manager->get(QNetworkRequest(QUrl(url)));

  connect(reply, &QNetworkReply::finished, this, [=] {
    if (reply->error() != QNetworkReply::NoError) {
      if (reply->error() == QNetworkReply::ContentNotFoundError) { throw RuntimeError("Produto não possui modelo 3D!"); }

      throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this);
    }

    const QString filename = QDir::currentPath() + "/arquivos/" + url.split("/").last();

    File file(filename);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

    file.write(reply->readAll());

    file.close();

    if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filename))) { throw RuntimeException("Não foi possível abrir o arquivo 3D!"); }
  });
}

double Orcamento::calcularPeso() {
  SqlQuery queryProduto;

  if (not queryProduto.exec("SELECT kgcx FROM produto WHERE idProduto = " + ui->itemBoxProduto->getId().toString()) or not queryProduto.first()) {
    throw RuntimeException("Erro buscando kgcx: " + queryProduto.lastError().text());
  }

  return ui->doubleSpinBoxCaixas->value() * queryProduto.value("kgcx").toDouble();
}

void Orcamento::calcularPesoTotal() {
  int total = 0;

  SqlQuery queryProduto;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    if (not queryProduto.exec("SELECT kgcx FROM produto WHERE idProduto = " + modelItem.data(row, "idProduto").toString()) or not queryProduto.first()) {
      throw RuntimeException("Erro buscando kgcx: " + queryProduto.lastError().text());
    }

    const double kgcx = queryProduto.value("kgcx").toDouble();
    total += modelItem.data(row, "caixas").toInt() * kgcx;
  }

  ui->spinBoxPesoTotal->setValue(total);
}

void Orcamento::verificaSeFoiAlterado() {
  if (ui->lineEditOrcamento->text() == "Auto gerado") { return; }

  SqlQuery query;

  if (not query.exec("SELECT lastUpdated FROM orcamento WHERE idOrcamento = '" + data("idOrcamento").toString() + "'") or not query.first()) {
    throw RuntimeException("Erro verificando se orçamento foi alterado: " + query.lastError().text());
  }

  const QDateTime serverLastUpdated = query.value("lastUpdated").toDateTime();
  const QDateTime currentLastUpdated = data("lastUpdated").toDateTime();

  if (serverLastUpdated > currentLastUpdated) {
    viewRegisterById(primaryId);
    throw RuntimeError("Orçamento foi modificado por outro usuário!\nRecarregando orçamento!");
  }
}

void Orcamento::redoBackupItem() {
  backupItem.clear();

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) != "*") { continue; } // skip saved items

    backupItem.append(modelItem.record(row));
  }
}

// NOTE: model.submitAll faz mapper voltar para -1, select tambem (talvez porque submitAll chama select)
// TODO: 0se produto for estoque permitir vender por peça (setar minimo/multiplo)
// TODO: 2orcamento de reposicao nao pode ter profissional associado (bloquear)
// TODO: 4quando cadastrar cliente no itemBox mudar para o id dele
// TODO: ?permitir que o usuario digite um valor e o sistema faça o calculo na linha?
// TODO: limitar o total ao frete? se o desconto é 100% e o frete não é zero, o minimo é o frete
// TODO: implementar mover linha para baixo/cima
//           1. colocar um botao com seta para cima e outro para baixo
//           2. para permitir reordenar os produtos colocar um campo oculto 'item' numerado sequencialmente, ai quando ler a tabela ordenar por essa coluna
// TODO: após gerar id permitir mudar vendedor apenas para os da mesma loja
// FIXME: orçamento permite adicionar o mesmo estoque duas vezes (e provavelmente faz o consumo duas vezes)
// TODO: antes de gerar excel/pdf salvar o arquivo para não ficar dados divergentes
