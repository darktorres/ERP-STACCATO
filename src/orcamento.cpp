#include "orcamento.h"
#include "ui_orcamento.h"

#include "application.h"
#include "baixaorcamento.h"
#include "cadastrocliente.h"
#include "calculofrete.h"
#include "doubledelegate.h"
#include "excel.h"
#include "file.h"
#include "log.h"
#include "logindialog.h"
#include "pdf.h"
#include "porcentagemdelegate.h"
#include "produtoproxymodel.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "user.h"
#include "venda.h"

#include <QAuthenticator>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSignalBlocker>
#include <QSqlError>
#include <QtMath>

#include <vector>

namespace {
// Scoped, per-widget signal suppression for a render step — narrower than the class-wide
// unsetConnections()/setConnections() pattern used elsewhere: only the widgets actually being
// written to are blocked, not every connection Orcamento owns. Safe here because renderItemForm()/
// renderTotais() only ever touch plain QDoubleSpinBox widgets, never ItemBox or model-backed views
// (see the qsignalblocker-incompatibility note on why that boundary matters).
class RenderGuard {
public:
  explicit RenderGuard(const std::initializer_list<QWidget *> widgets) {
    blockers.reserve(widgets.size());
    for (auto *widget : widgets) { blockers.emplace_back(widget); }
  }

private:
  std::vector<QSignalBlocker> blockers;
};
} // namespace

Orcamento::Orcamento(QWidget *parent) : RegisterDialog("orcamento", "idOrcamento", parent), ui(new Ui::Orcamento) {
  ui->setupUi(this);

  for (auto spinbox : findChildren<QSpinBox *>()) {
    spinbox->installEventFilter(this);
  }

  for (auto doubleSpinbox : findChildren<QDoubleSpinBox *>()) {
    doubleSpinbox->installEventFilter(this);
  }

  connectLineEditsToDirty();
  setItemBoxes();
  setupTables();
  setupMapper();
  newRegister();

  if (User::isAdministrativo()) {
    ui->dataEmissao->setReadOnly(false);
    ui->dataEmissao->setCalendarPopup(true);
  }

  if (User::isVendedor()) { buscarParametrosFrete(); }

  ui->pushButtonModelo3d->hide();

  ui->labelEstoque->hide();
  ui->doubleSpinBoxEstoque->hide();

  ui->labelMinimo->hide();
  ui->doubleSpinBoxMinimo->hide();

  ui->labelUn->hide();
  ui->lineEditUn->hide();

  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 255);
  ui->splitter->setStretchFactor(2, 1);

  ui->lineEditCodComercial->setResizeToText();
  ui->lineEditFormComercial->setResizeToText();
  ui->lineEditFornecedor->setResizeToText();

  ui->lineEditOrcamento->setResizeToText();

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

void Orcamento::on_tableProdutos_selectionChanged() {
  const auto selection = ui->tableProdutos->selectionModel()->selectedRows();

  if (isReadOnly) { return; }

  if (selection.isEmpty()) { return novoItem(); }

  const auto index = selection.first();

  ui->pushButtonAdicionarItem->hide();

  ui->pushButtonAtualizarItem->show();
  ui->pushButtonLimparSelecao->show();
  ui->pushButtonRemoverItem->show();

  currentRowItem = index.row();

  // -------------------------------------------------------------------------

  unsetConnections();

  try {
    // NOTE: mapper sets values but fields still have maximum from previous line
    ui->doubleSpinBoxCaixas->setMaximum(9'999'999.000000);
    ui->doubleSpinBoxQuant->setMaximum(9'999'999.000000);

    mapperItem.setCurrentModelIndex(index);
    setarParametrosProduto();

    // setarParametrosProduto() só carrega stepQt/stepCx/prcUn; caixas/descPct vêm do mapper.
    itemFormState.caixas = ui->doubleSpinBoxCaixas->value();
    itemFormState.descPct = ui->doubleSpinBoxDesconto->value();
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
  connect(ui->itemBoxEndereco, &ItemBox::idChanged, this, &Orcamento::on_itemBoxEndereco_idChanged, connectionType);
  connect(ui->itemBoxProduto, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProduto_idChanged, connectionType);
  connect(ui->itemBoxProfissional, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProfissional_idChanged, connectionType);
  connect(ui->itemBoxVendedor, &ItemBox::textChanged, this, &Orcamento::on_itemBoxVendedor_textChanged, connectionType);
  connect(ui->pushButtonAbrirReplicaDe, &QPushButton::clicked, this, &Orcamento::on_pushButtonAbrirReplicaDe_clicked, connectionType);
  connect(ui->pushButtonAbrirReplicadoEm, &QPushButton::clicked, this, &Orcamento::on_pushButtonAbrirReplicadoEm_clicked, connectionType);
  connect(ui->pushButtonAbrirVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonAbrirVenda_clicked, connectionType);
  connect(ui->pushButtonAdicionarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAdicionarItem_clicked, connectionType);
  connect(ui->pushButtonApagarOrc, &QPushButton::clicked, this, &Orcamento::on_pushButtonApagarOrc_clicked, connectionType);
  connect(ui->pushButtonDescerItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonDescerItem_clicked, connectionType);
  connect(ui->pushButtonAtualizarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarItem_clicked, connectionType);
  connect(ui->pushButtonAtualizarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarOrcamento_clicked, connectionType);
  connect(ui->pushButtonCadastrarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonCadastrarOrcamento_clicked, connectionType);
  connect(ui->pushButtonCalculadora, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalculadora_clicked, connectionType);
  connect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarExcel_clicked, connectionType);
  connect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarPdf_clicked, connectionType);
  connect(ui->pushButtonGerarVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarVenda_clicked, connectionType);
  connect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &Orcamento::novoItem, connectionType);
  connect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &Orcamento::on_pushButtonModelo3d_clicked, connectionType);
  connect(ui->pushButtonRemoverItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonRemoverItem_clicked, connectionType);
  connect(ui->pushButtonReplicar, &QPushButton::clicked, this, &Orcamento::on_pushButtonReplicar_clicked, connectionType);
  connect(ui->pushButtonSubirItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonSubirItem_clicked, connectionType);
  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Orcamento::on_tableProdutos_selectionChanged, connectionType);
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
  disconnect(ui->itemBoxEndereco, &ItemBox::idChanged, this, &Orcamento::on_itemBoxEndereco_idChanged);
  disconnect(ui->itemBoxProduto, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProduto_idChanged);
  disconnect(ui->itemBoxProfissional, &ItemBox::idChanged, this, &Orcamento::on_itemBoxProfissional_idChanged);
  disconnect(ui->itemBoxVendedor, &ItemBox::textChanged, this, &Orcamento::on_itemBoxVendedor_textChanged);
  disconnect(ui->pushButtonAbrirReplicaDe, &QPushButton::clicked, this, &Orcamento::on_pushButtonAbrirReplicaDe_clicked);
  disconnect(ui->pushButtonAbrirReplicadoEm, &QPushButton::clicked, this, &Orcamento::on_pushButtonAbrirReplicadoEm_clicked);
  disconnect(ui->pushButtonAbrirVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonAbrirVenda_clicked);
  disconnect(ui->pushButtonAdicionarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAdicionarItem_clicked);
  disconnect(ui->pushButtonApagarOrc, &QPushButton::clicked, this, &Orcamento::on_pushButtonApagarOrc_clicked);
  disconnect(ui->pushButtonDescerItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonDescerItem_clicked);
  disconnect(ui->pushButtonAtualizarItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarItem_clicked);
  disconnect(ui->pushButtonAtualizarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonAtualizarOrcamento_clicked);
  disconnect(ui->pushButtonCadastrarOrcamento, &QPushButton::clicked, this, &Orcamento::on_pushButtonCadastrarOrcamento_clicked);
  disconnect(ui->pushButtonCalculadora, &QPushButton::clicked, this, &Orcamento::on_pushButtonCalculadora_clicked);
  disconnect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarExcel_clicked);
  disconnect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarPdf_clicked);
  disconnect(ui->pushButtonGerarVenda, &QPushButton::clicked, this, &Orcamento::on_pushButtonGerarVenda_clicked);
  disconnect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &Orcamento::novoItem);
  disconnect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &Orcamento::on_pushButtonModelo3d_clicked);
  disconnect(ui->pushButtonRemoverItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonRemoverItem_clicked);
  disconnect(ui->pushButtonReplicar, &QPushButton::clicked, this, &Orcamento::on_pushButtonReplicar_clicked);
  disconnect(ui->pushButtonSubirItem, &QPushButton::clicked, this, &Orcamento::on_pushButtonSubirItem_clicked);
  disconnect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Orcamento::on_tableProdutos_selectionChanged);
}

bool Orcamento::viewRegister() {
  unsetConnections();

  // Marca a carga para que a checagem de invariante em renderTotais() saiba distinguir "a linha já estava
  // inconsistente no disco" de "esta edição quebrou" — hoje impossível de separar depois do fato.
  carregando = true;
  totaisTrace.limpar();

  auto load = [&] {
    if (not RegisterDialog::viewRegister()) { return false; }

    //-----------------------------------------------------------------

    modelItem.setFilter("idOrcamento = '" + model.data(0, "idOrcamento").toString() + "'");

    modelItem.select();

    //-----------------------------------------------------------------

    // O mapper (dentro de RegisterDialog::viewRegister() acima) já escreveu frete/subTotalBruto/subTotalLiq/
    // descontoReais direto nos widgets — sincroniza 'totais' a partir deles agora. NUNCA lê 'total' do widget:
    // ele é sempre derivado por renderTotais() ao final desta função, nunca copiado bruto do banco (essa cópia
    // direta era a causa do "Erro nos valores!" ao gerar venda a partir de um orçamento).
    aplicarTotais("viewRegister:sync", "-",
                  {ui->doubleSpinBoxSubTotalBruto->value(), ui->doubleSpinBoxSubTotalLiq->value(), ui->doubleSpinBoxFrete->value(),
                   ui->doubleSpinBoxDescontoGlobalReais->value()});
    freteMinimoAtual = totais.frete;

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

    const bool expirado = (qApp->serverDate() > ui->dataEmissao->date().addDays(data("validade").toInt()));

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
      ui->spinBoxPrazoEntrega->setButtonSymbols(QSpinBox::NoButtons);

      ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::NoButtons);
      ui->doubleSpinBoxDescontoGlobalReais->setButtonSymbols(QDoubleSpinBox::NoButtons);
      ui->doubleSpinBoxFrete->setButtonSymbols(QDoubleSpinBox::NoButtons);
      ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::NoButtons);
    } else {
      ui->pushButtonGerarVenda->show();
    }

    ui->checkBoxFreteManual->setHidden(ui->checkBoxRepresentacao->isChecked());

    canChangeFrete = ui->checkBoxFreteManual->isChecked() or ui->checkBoxRepresentacao->isChecked();

    if (canChangeFrete) {
      ui->checkBoxFreteManual->setDisabled(true);
      freteMinimoAtual = 0.;
    }

    if (User::isGerente()) {
      if (const auto resultado = calcularFrete()) {
        if (resultado->forcado) { aplicarTotais("viewRegister:freteForcado", Log::dinheiro(resultado->valor), reduceSetFrete(totais, resultado->valor)); }
        freteMinimoAtual = resultado->minimo;
      }
    }

    if (ui->checkBoxRepresentacao->isChecked()) {
      ui->itemBoxProduto->setRepresentacao(true);
      freteMinimoAtual = 0.;
    }

    if (not data("replicadoDe").toString().isEmpty()) {
      ui->labelReplicaDe->show();
      ui->pushButtonAbrirReplicaDe->show();
      ui->lineEditReplicaDe->show();
    }

    if (not data("replicadoEm").toString().isEmpty()) {
      ui->labelReplicadoEm->show();
      ui->pushButtonAbrirReplicadoEm->show();
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
      const QString idLoja = User::fromLoja("usuario.idLoja", ui->itemBoxVendedor->getId().toString()).toString();
      ui->itemBoxVendedor->setFilter("idLoja = " + idLoja);
    }

    const QString idCliente = QString::number(ui->itemBoxCliente->getId().toInt());
    ui->itemBoxEndereco->setFilter("(idCliente = " + idCliente + " OR idEndereco = 1) AND desativado = FALSE");

    renderTotais();

    calcularPesoTotal();
    buscarIdVenda();

    return true;
  }();

  carregando = false;

  setConnections();

  return load;
}

QVariant Orcamento::dataItem(const QString &key) const { return modelItem.data(currentRowItem, key); }

void Orcamento::setDataItem(const QString &key, const QVariant &value, const bool adjustValue) { modelItem.setData(currentRowItem, key, value, adjustValue); }

void Orcamento::novoItem() {
  currentRowItem = -1;

  ui->pushButtonAdicionarItem->show();

  ui->pushButtonAtualizarItem->hide();
  ui->pushButtonLimparSelecao->hide();
  ui->pushButtonRemoverItem->hide();

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
  ui->labelVenda->hide();
  ui->lineEditReplicaDe->hide();
  ui->lineEditReplicadoEm->hide();
  ui->lineEditVenda->hide();
  ui->plainTextEditBaixa->hide();
  ui->pushButtonAbrirReplicaDe->hide();
  ui->pushButtonAbrirReplicadoEm->hide();
  ui->pushButtonAbrirVenda->hide();
  ui->pushButtonAtualizarOrcamento->hide();
  ui->pushButtonReplicar->hide();

  ui->itemBoxConsultor->setReadOnlyItemBox(true);

  ui->pushButtonApagarOrc->setDisabled(true);
  ui->pushButtonGerarExcel->setDisabled(true);
  ui->pushButtonGerarPdf->setDisabled(true);
  ui->pushButtonGerarVenda->setEnabled(true);
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

    registrarItens("removeItem", "row=" + QString::number(currentRowItem));

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
  const QString siglaLoja = User::fromLoja("sigla", ui->itemBoxVendedor->getId().toString()).toString();

  if (siglaLoja.isEmpty()) { throw RuntimeException("Erro buscando sigla da loja!"); }

  QString id = siglaLoja + "-" + qApp->serverDate().toString("yy");

  const QString replica = ui->lineEditReplicaDe->text();
  const QString siglaReplica = replica.left(4);

  if (replica.isEmpty() or siglaReplica != siglaLoja) {
    SqlQuery query;
    query.prepare("SELECT MAX(idOrcamento) AS idOrcamento FROM orcamento WHERE idOrcamento LIKE :id");
    query.bindValue(":id", id + "%");

    if (not query.exec()) { throw RuntimeException("Erro buscando próximo id disponível: " + query.lastError().text()); }

    int last = 0;

    if (query.first()) { last = query.value("idOrcamento").toString().mid(7, 4).toInt(); }

    id += QString::number(last + 1).rightJustified(4, '0');
    id += ui->checkBoxRepresentacao->isChecked() ? "R" : "";
    id += "O";

    if (id.size() != 12 and id.size() != 13) { throw RuntimeException("Tamanho do Id errado: " + id); }
  } else {
    SqlQuery query;
    query.prepare(
        "SELECT COALESCE(MAX(CAST(RIGHT(idOrcamento, CHAR_LENGTH(idOrcamento) - LOCATE('Rev', idOrcamento) - 2) AS UNSIGNED)) + 1, 1) AS revisao FROM orcamento WHERE CHAR_LENGTH(idOrcamento) > 16 "
        "AND idOrcamento LIKE :idOrcamento");
    query.bindValue(":idOrcamento", replica.left(11) + "%");

    if (not query.exec()) { throw RuntimeException("Erro buscando próxima revisão disponível: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("Erro buscando próxima revisão disponível!"); }

    id = replica.left(replica.indexOf("-REV")) + "-REV" + query.value("revisao").toString();
  }

  ui->lineEditOrcamento->setText(id);
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

// -----------------------------------------------------------------------------------------------
// Pure reducers — one per possible user edit, no Qt types, no side effects. Each is a direct port
// of the math that used to live inline in the corresponding on_doubleSpinBox*_valueChanged slot.
// -----------------------------------------------------------------------------------------------

ItemFormState Orcamento::reduceSetCaixas(ItemFormState state, const double caixasRaw) {
  // caixas precisa ser inteiro para quant()=caixas*stepQt ficar múltiplo de stepQt.
  state.caixas = ceil(qApp->roundDouble(caixasRaw));
  return state;
}

ItemFormState Orcamento::reduceSetQuant(ItemFormState state, const double quantRaw) {
  state.caixas = ceil(qApp->roundDouble(quantRaw / state.stepQt));
  return state;
}

ItemFormState Orcamento::reduceSetDesconto(ItemFormState state, const double descPct) {
  state.descPct = descPct;
  return state;
}

ItemFormState Orcamento::reduceSetTotalItem(ItemFormState state, const double totalItemValor) {
  const double itemBruto = state.quant() * state.prcUn;

  if (qFuzzyIsNull(itemBruto)) { return state; }

  state.descPct = (itemBruto - totalItemValor) / itemBruto * 100.;
  return state;
}

OrcamentoTotais Orcamento::reduceSetFrete(OrcamentoTotais totaisAtuais, const double frete) {
  totaisAtuais.frete = frete;
  return totaisAtuais;
}

OrcamentoTotais Orcamento::reduceSetDescontoReais(OrcamentoTotais totaisAtuais, const double descontoReais) {
  totaisAtuais.descontoReais = descontoReais;
  return totaisAtuais;
}

OrcamentoTotais Orcamento::reduceSetDescontoPorc(OrcamentoTotais totaisAtuais, const double descontoPorc) {
  totaisAtuais.descontoReais = totaisAtuais.subTotalLiq * (descontoPorc / 100.);
  return totaisAtuais;
}

OrcamentoTotais Orcamento::reduceSetTotal(OrcamentoTotais totaisAtuais, const double total) {
  totaisAtuais.descontoReais = totaisAtuais.subTotalLiq + totaisAtuais.frete - total;
  return totaisAtuais;
}

// -----------------------------------------------------------------------------------------------
// Render — the only place either group's widgets get written to. Whatever a reducer didn't touch
// gets written back unchanged (harmless no-op), so a single unconditional render is always safe.
// -----------------------------------------------------------------------------------------------

void Orcamento::renderItemForm() {
  const RenderGuard guard({ui->doubleSpinBoxCaixas, ui->doubleSpinBoxQuant, ui->doubleSpinBoxDesconto, ui->doubleSpinBoxTotalItem});

  ui->doubleSpinBoxCaixas->setValue(itemFormState.caixas);
  ui->doubleSpinBoxQuant->setValue(itemFormState.quant());
  ui->doubleSpinBoxDesconto->setValue(itemFormState.descPct);
  ui->doubleSpinBoxTotalItem->setValue(itemFormState.totalItem());
}

void Orcamento::aplicarDescontoAosItens(const double descontoPorc) {
  const double descontoFrac = descontoPorc / 100.;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();

    modelItem.setData(row, "descGlobal", descontoPorc);
    modelItem.setData(row, "total", parcialDesc * (1. - descontoFrac));
  }
}

void Orcamento::renderTotais() {
  const RenderGuard guard({ui->doubleSpinBoxSubTotalBruto, ui->doubleSpinBoxSubTotalLiq, ui->doubleSpinBoxFrete, ui->doubleSpinBoxDescontoGlobal,
                            ui->doubleSpinBoxDescontoGlobalReais, ui->doubleSpinBoxTotal});

  ui->doubleSpinBoxSubTotalBruto->setValue(totais.subTotalBruto);
  ui->doubleSpinBoxSubTotalLiq->setValue(totais.subTotalLiq);

  ui->doubleSpinBoxFrete->setMinimum(freteMinimoAtual);
  ui->doubleSpinBoxFrete->setValue(totais.frete);

  ui->doubleSpinBoxDescontoGlobalReais->setMaximum(totais.subTotalLiq);
  ui->doubleSpinBoxDescontoGlobalReais->setValue(totais.descontoReais);
  ui->doubleSpinBoxDescontoGlobal->setValue(totais.descontoPorc());

  ui->doubleSpinBoxTotal->setMinimum(totais.frete);
  ui->doubleSpinBoxTotal->setMaximum(totais.subTotalLiq + totais.frete);
  ui->doubleSpinBoxTotal->setValue(totais.total());

  aplicarDescontoAosItens(totais.descontoPorc());

  // Última instrução: o invariante só vale depois que os itens receberam o desconto global. Pega o
  // instante em que a divergência aparece, em vez de descobri-la no save muitas ações depois.
  verificarInvariante(carregando ? "render:load" : "render:edit");
}

void Orcamento::aplicarFreteCalculado(const FreteResultado &resultado) {
  freteMinimoAtual = resultado.minimo;
  aplicarTotais("aplicarFreteCalculado", Log::dinheiro(resultado.valor), reduceSetFrete(totais, resultado.valor));
  renderTotais();
}

// -----------------------------------------------------------------------------------------------
// Diagnóstico do invariante de totais.
//
// Substitui o antigo montarLog(), que gravava um retrato mudo no instante do throw: não dizia qual
// comparação falhou nem por quanto, imprimia os números com 6 dígitos significativos (mais grosso
// que a tolerância de 0.1 sendo testada, então valores diferentes saíam idênticos no log) e não
// registrava nada sobre como o estado chegou ali. Agora toda escrita em 'totais' passa por
// aplicarTotais() e fica no trace, e a checagem roda a cada render, não só no save.
// -----------------------------------------------------------------------------------------------

void Orcamento::aplicarTotais(const QString &origem, const QString &argumento, const OrcamentoTotais &novo) {
  totais = novo;

  registrarItens(origem, argumento);
}

void Orcamento::registrarItens(const QString &origem, const QString &argumento) {
  const double subTotalLiqItens = std::get<1>(calcularTotais());

  totaisTrace.registrar(origem, argumento, {totais.subTotalBruto, totais.subTotalLiq, totais.frete, totais.descontoReais}, modelItem.rowCount(), subTotalLiqItens);
}

QVector<TotaisCheck> Orcamento::montarChecks() {
  const auto [subTotalBruto, subTotalLiq, total] = calcularTotais();

  // Tolerância proporcional à base: os valores são DECIMAL(15,4) e os spin boxes de dinheiro têm 2 casas,
  // então um limite fixo de 0.1 é mais fino que a precisão dos dados e gera falso erro em bases grandes.
  const double tol = std::max(0.1, totais.subTotalLiq * 1e-6);

  // O invariante de frete/desconto/total é garantido por construção — renderTotais() sempre deriva Total a
  // partir de subTotalLiq/descontoReais/frete, não existe mais um caminho que o defina de forma independente
  // (era essa a causa do "Erro nos valores!" ao gerar venda a partir de um orçamento). O desvio ainda
  // possível é 'modelItem' divergir do que 'totais' pensa conter — recomputa a agregação a partir dele.
  return {
      {"bruto", subTotalBruto, totais.subTotalBruto, tol},
      {"liquido", subTotalLiq, totais.subTotalLiq, tol},
      {"itens", total, totais.subTotalLiq - totais.descontoReais, tol},
      // O widget de Total é derivado por renderTotais() a partir de 'totais'; divergir aqui significa que
      // alguma escrita escapou do par aplicarTotais()/renderTotais(). É exatamente este termo que falhava
      // nos casos históricos, e ele não era verificado nem impresso. Tolerância 0.01 = uma casa do spin
      // box, que tem 2 contra as 4 de DECIMAL(15,4).
      {"widgetTotal", ui->doubleSpinBoxTotal->value(), totais.total(), 0.01},
  };
}

QString Orcamento::montarFlags() const {
  return "freteManual=" + QString::number(ui->checkBoxFreteManual->isChecked()) +               //
         " representacao=" + QString::number(ui->checkBoxRepresentacao->isChecked()) +          //
         " freteMinimoAtual=" + Log::dinheiro(freteMinimoAtual) +                               //
         " canChangeFrete=" + QString::number(canChangeFrete) +                                 //
         " porcFrete=" + Log::dinheiro(porcFrete) +                                             //
         " tipo=" + (tipo == Tipo::Cadastrar ? QString("Cadastrar") : QString("Atualizar")) +   //
         " readOnly=" + QString::number(isReadOnly) +                                           //
         " replicando=" + QString::number(replicando) +                                         //
         " carregando=" + QString::number(carregando) +                                         //
         " itens=" + QString::number(modelItem.rowCount());
}

QString Orcamento::montarItensSujos() {
  const auto linhaSuja = [&](const int row) {
    for (int col = 0, colCount = modelItem.columnCount(); col < colCount; ++col) {
      if (modelItem.isDirty(modelItem.index(row, col))) { return true; }
    }

    return false;
  };

  QStringList linhas;
  int considerados = 0;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    ++considerados;

    // As linhas já persistidas estão em orcamento_has_produto e podem ser consultadas a qualquer momento
    // pelo id do cabeçalho — despejá-las aqui era a maior parte do volume do log antigo e não acrescentava
    // nada. Só o que ainda não foi gravado é informação exclusiva deste registro.
    if (not linhaSuja(row)) { continue; }

    linhas << "  row=" + QString::number(row) +                                                    //
                  " id=" + modelItem.data(row, "idOrcamentoProduto").toString() +                  //
                  " codComercial=" + modelItem.data(row, "codComercial").toString() +              //
                  " quant=" + Log::dinheiro(modelItem.data(row, "quant").toDouble()) +             //
                  " prcUn=" + Log::dinheiro(modelItem.data(row, "prcUnitario").toDouble()) +       //
                  " desc=" + Log::dinheiro(modelItem.data(row, "desconto").toDouble()) +           //
                  " parcial=" + Log::dinheiro(modelItem.data(row, "parcial").toDouble()) +         //
                  " parcialDesc=" + Log::dinheiro(modelItem.data(row, "parcialDesc").toDouble()) + //
                  " descGlobal=" + Log::dinheiro(modelItem.data(row, "descGlobal").toDouble()) +   //
                  " total=" + Log::dinheiro(modelItem.data(row, "total").toDouble());
  }

  if (linhas.isEmpty()) { return "itens sujos: nenhum (0 de " + QString::number(considerados) + " — todos já persistidos, consulte orcamento_has_produto)"; }

  linhas.prepend("itens sujos (" + QString::number(linhas.size()) + " de " + QString::number(considerados) + " — só linhas não persistidas):");

  return linhas.join("\n");
}

TotaisDiagnostico Orcamento::montarDiagnostico(const QString &contexto, const QVector<TotaisCheck> &checks) {
  return {"Orcamento",
          contexto,
          ui->lineEditOrcamento->text(),
          checks,
          {totais.subTotalBruto, totais.subTotalLiq, totais.frete, totais.descontoReais},
          montarFlags(),
          montarItensSujos()};
}

void Orcamento::verificarInvariante(const QString &contexto) {
  // Nunca lança nem interrompe quem está usando: só registra o instante em que o invariante quebrou, que é
  // a informação que faltava — no save o desvio já pode ter muitas ações de idade.
  try {
    const QVector<TotaisCheck> checks = montarChecks();

    // Caminho normal sai aqui: roda a cada render, então nada de montar flags/itens/mensagem à toa.
    if (not algumCheckFalhou(checks)) { return; }

    const TotaisDiagnostico diagnostico = montarDiagnostico(contexto, checks);

    // Uma falha idêntica repetida não gera linha nova; uma falha diferente gera.
    if (not totaisTrace.primeiraVez(diagnostico.assinatura())) { return; }

    Log::createLogTotais(diagnostico, totaisTrace);
  } catch (std::exception &e) { qDebug() << "verificarInvariante falhou:" << e.what(); }
}

void Orcamento::verificarTotais() {
  const QVector<TotaisCheck> checks = montarChecks();

  // Sem retry: se falhar aqui o desvio é real, não um estado transitório para corrigir.
  if (not algumCheckFalhou(checks)) { return; }

  // Sem dedup no save: cada tentativa que falha para o usuário é um evento que importa por si, e o trace
  // difere entre elas.
  Log::createLogTotais(montarDiagnostico("save", checks), totaisTrace);

  throw RuntimeException("Erro nos valores! Entre em contato com o suporte!");
}

void Orcamento::verifyFields() {
  verificaSeFoiAlterado();

  if (modelItem.rowCount() == 0) { throw RuntimeError("Adicione pelo menos um item ao orçamento!", this); }

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

    const int idLoja = User::fromLoja("usuario.idLoja", ui->itemBoxVendedor->getId().toString()).toInt();
    setData("idLoja", idLoja);

    setData("idOrcamento", ui->lineEditOrcamento->text());
    setData("idOrcamentoBase", ui->lineEditOrcamento->text().left(11));
    setData("replicadoDe", ui->lineEditReplicaDe->text());
    setData("representacao", ui->checkBoxRepresentacao->isChecked());

    atualizaReplica();
  }

  buscarConsultor();

  setData("data", ui->dataEmissao->isReadOnly() ? qApp->serverDateTime() : ui->dataEmissao->dateTime());
  setData("data2", data("data").toDate().toString("yyyy-MM"));
  // 'totais' já garante o invariante total = subTotalLiq - descontoReais + frete de forma exata — grava os
  // mesmos campos que o header exibe, sem reler os widgets (que renderTotais() sempre manteve em sincronia).
  setData("descontoPorc", totais.descontoPorc());
  setData("descontoReais", totais.descontoReais);
  setData("frete", totais.frete);
  setData("freteManual", ui->checkBoxFreteManual->isChecked());
  setData("idCliente", ui->itemBoxCliente->getId());
  setData("idEnderecoEntrega", ui->itemBoxEndereco->getId());
  setData("idProfissional", ui->itemBoxProfissional->getId());
  setData("idUsuario", ui->itemBoxVendedor->getId());
  setData("observacao", ui->plainTextEditObs->toPlainText());
  setData("prazoEntrega", ui->spinBoxPrazoEntrega->value());
  setData("subTotalBru", totais.subTotalBruto);
  setData("subTotalLiq", totais.subTotalLiq);
  setData("total", totais.total());
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

  QStringList placeholders;
  for (int i = 0; i < fornecedores.size(); ++i) { placeholders << ":f" + QString::number(i); }

  SqlQuery query;
  query.prepare("SELECT idUsuario FROM usuario WHERE desativado = FALSE AND especialidade > 0 AND especialidade IN (SELECT especialidade FROM fornecedor WHERE razaoSocial IN (" +
                placeholders.join(", ") + "))");
  for (int i = 0; i < fornecedores.size(); ++i) { query.bindValue(":f" + QString::number(i), fornecedores.at(i)); }

  if (not query.exec()) { throw RuntimeException("Erro buscando consultor: " + query.lastError().text()); }

  if (query.size() > 1) { throw RuntimeError("Mais de um consultor disponível para os fornecedores selecionados!", this); }

  if (query.size() == 1 and query.first()) { setData("idUsuarioConsultor", query.value("idUsuario")); }

  if (query.size() == 0) { model.setData(currentRow, "idUsuarioConsultor", {}); }
}

void Orcamento::atualizaReplica() {
  if (ui->lineEditReplicaDe->text().isEmpty()) { return; }

  SqlQuery query;
  query.prepare("UPDATE orcamento SET status = 'REPLICADO', replicadoEm = :idReplica WHERE idOrcamento = :idOrcamento AND status = 'EXPIRADO'");
  query.bindValue(":idReplica", ui->lineEditOrcamento->text());
  query.bindValue(":idOrcamento", ui->lineEditReplicaDe->text());

  if (not query.exec()) { throw RuntimeException("Erro salvando replicadoEm: " + query.lastError().text()); }
}

void Orcamento::clearFields() {
  RegisterDialog::clearFields();

  if (User::isVendedor()) { ui->itemBoxVendedor->setId(User::idUsuario); }
}

void Orcamento::on_pushButtonRemoverItem_clicked() { removeItem(); }

void Orcamento::on_pushButtonSubirItem_clicked() {
  if (currentRowItem <= 0) { return; }

  const int rowB = currentRowItem - 1;

  swapItens(currentRowItem, rowB);

  currentRowItem = rowB;

  if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); }

  ui->tableProdutos->selectRow(rowB);
}

void Orcamento::on_pushButtonDescerItem_clicked() {
  if (currentRowItem < 0 or currentRowItem >= modelItem.rowCount() - 1) { return; }

  const int rowB = currentRowItem + 1;

  swapItens(currentRowItem, rowB);

  currentRowItem = rowB;

  if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); }

  ui->tableProdutos->selectRow(rowB);
}

void Orcamento::swapItens(const int rowA, const int rowB) {
  // Troca o conteúdo das duas linhas mantendo a PK (idOrcamentoProduto) e a posição (ordem) de cada
  // slot. Isso reordena visualmente sem chamar proxyModel->sort(), preservando a invariante de que a
  // ordem do proxy é igual à ordem do model fonte (necessária para removeRow/insertRowAtEnd etc.).

  const QSqlRecord record = modelItem.record();

  for (int col = 0, colCount = record.count(); col < colCount; ++col) {
    const QString field = record.fieldName(col);

    if (field == "idOrcamentoProduto" or field == "ordem") { continue; }

    const QVariant valueA = modelItem.data(rowA, col);
    const QVariant valueB = modelItem.data(rowB, col);

    modelItem.setData(rowA, col, valueB, false);
    modelItem.setData(rowB, col, valueA, false);
  }

  registrarItens("swapItens", QString::number(rowA) + "<->" + QString::number(rowB));
}

void Orcamento::on_doubleSpinBoxQuant_valueChanged(const double quant) {
  itemFormState.stepQt = ui->doubleSpinBoxQuant->singleStep();
  itemFormState.prcUn = ui->doubleSpinBoxPrecoUn->value();

  itemFormState = reduceSetQuant(itemFormState, quant);

  renderItemForm();
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

    const double quant = modelItem.data(row, "quant").toDouble();
    const double prcUnitario = modelItem.data(row, "prcUnitario").toDouble();
    const double descItem = modelItem.data(row, "desconto").toDouble() / 100.;
    
    const double itemBruto = quant * prcUnitario;
    const double stItem = itemBruto * (1. - descItem);

    modelItem.setData(row, "parcial", itemBruto);
    modelItem.setData(row, "parcialDesc", stItem);

    subTotalBruto += itemBruto;
    subTotalItens += stItem;
  }

  // Preserva o desconto global % atual, reaplicado à base recém-recalculada — mesmo comportamento de antes.
  const double descGlobalFracAtual = totais.descontoPorc() / 100.;

  aplicarTotais("calcPrecoGlobalTotal", "-", {subTotalBruto, subTotalItens, totais.frete, subTotalItens * descGlobalFracAtual});

  if (not ui->checkBoxFreteManual->isChecked()) {
    if (const auto resultado = calcularFrete()) {
      freteMinimoAtual = resultado->minimo;
      aplicarTotais("calcularFrete:auto", Log::dinheiro(resultado->valor), reduceSetFrete(totais, resultado->valor));
    }
  }

  renderTotais();
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

  modelItem.setSort("ordem", Qt::AscendingOrder);

  modelItem.proxyModel = new ProdutoProxyModel(&modelItem, this);
  modelItem.proxyModel->sort(modelItem.fieldIndex("ordem"), Qt::AscendingOrder);

  ui->tableProdutos->setModel(&modelItem);

  ui->tableProdutos->hideColumn("idOrcamentoProduto");
  ui->tableProdutos->hideColumn("ordem");
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
    if (tipoItem == Tipo::Cadastrar) {
      currentRowItem = modelItem.insertRowAtEnd();

      int maxOrdem = -1;
      for (int row = 0; row < currentRowItem; ++row) {
        if (modelItem.headerData(row, Qt::Vertical) != "!") { maxOrdem = qMax(maxOrdem, modelItem.data(row, "ordem").toInt()); }
      }
      setDataItem("ordem", maxOrdem + 1);
    }

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

    registrarItens("adicionarItem", tipoItem == Tipo::Cadastrar ? "novo" : "atualiza");

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
  const QDate date = ui->dataEmissao->date();

  if (not date.isValid()) { return; }

  if (qApp->serverDate() > date.addDays(ui->spinBoxValidade->value())) { throw RuntimeError("Orçamento vencido!"); }

  if (ui->itemBoxEndereco->text().isEmpty()) { throw RuntimeError("Deve selecionar endereço!"); }

  verificaCadastroCliente();

  save(true);

  auto *venda = new Venda(parentWidget());
  venda->setAttribute(Qt::WA_DeleteOnClose);
  venda->prepararVenda(ui->lineEditOrcamento->text());
  venda->show();

  close();
}

void Orcamento::on_doubleSpinBoxCaixas_valueChanged(const double caixas) {
  itemFormState.stepQt = ui->doubleSpinBoxQuant->singleStep();
  itemFormState.stepCx = ui->doubleSpinBoxCaixas->singleStep();
  itemFormState.prcUn = ui->doubleSpinBoxPrecoUn->value();

  itemFormState = reduceSetCaixas(itemFormState, caixas);

  renderItemForm();
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

void Orcamento::on_itemBoxProduto_idChanged() {
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

  itemFormState = ItemFormState{};
  renderItemForm();

  // -------------------------------------------------------------------------

  setarParametrosProduto();

  // -------------------------------------------------------------------------

  resizeSpinBoxes();
}

void Orcamento::setarParametrosProduto() {
  SqlQuery query;
  query.prepare("SELECT un, precoVenda, estoqueRestante, fornecedor, codComercial, formComercial, quantCaixa, minimo, multiplo, estoque, promocao FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec()) { throw RuntimeException("Erro na busca do produto: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados do produto com id: '" + ui->itemBoxProduto->getId().toString() + "'"); }

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
    const int selectedIdProduto = ui->itemBoxProduto->getId().toInt();
    double alreadyCommitted = 0.;
    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (row == currentRowItem) { continue; }
      if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; }
      if (modelItem.data(row, "estoque").toBool() and modelItem.data(row, "idProduto").toInt() == selectedIdProduto) { alreadyCommitted += modelItem.data(row, "quant").toDouble(); }
    }
    const double available = qMax(0., query.value("estoqueRestante").toDouble() - alreadyCommitted);
    ui->doubleSpinBoxCaixas->setMaximum(available / quantCaixa);
    ui->doubleSpinBoxQuant->setMaximum(available);
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

  // -------------------------------------------------------------------------

  itemFormState.stepQt = ui->doubleSpinBoxQuant->singleStep();
  itemFormState.stepCx = ui->doubleSpinBoxCaixas->singleStep();
  itemFormState.prcUn = query.value("precoVenda").toDouble();
}

void Orcamento::on_itemBoxProfissional_idChanged() {
  const auto idProfissional = ui->itemBoxProfissional->getId();

  SqlQuery query;
  query.prepare("SELECT comissao FROM profissional WHERE idProfissional = :id");
  query.bindValue(":id", idProfissional);

  if (not query.exec()) { throw RuntimeException("Erro buscando dados do profissional: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados do profissional não encontrados!"); }

  ui->itemBoxProfissional->setStyleSheet(query.value("comissao").toDouble() > 5 ? "background-color: rgb(255, 255, 127); color: rgb(0, 0, 0);" : "");
}

void Orcamento::on_itemBoxCliente_textChanged() {
  const QString idCliente = QString::number(ui->itemBoxCliente->getId().toInt());
  ui->itemBoxEndereco->setFilter("(idCliente = " + idCliente + " OR idEndereco = 1) AND desativado = FALSE");

  SqlQuery queryCliente;
  queryCliente.prepare("SELECT idProfissionalRel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not queryCliente.exec()) { throw RuntimeException("Erro ao buscar cliente: " + queryCliente.lastError().text()); }

  if (not queryCliente.first()) { throw RuntimeException("Dados do cliente não encontrados!"); }

  ui->itemBoxProfissional->setId(queryCliente.value("idProfissionalRel"));
  ui->itemBoxEndereco->setEnabled(true);
  ui->itemBoxEndereco->clear();
}

void Orcamento::on_itemBoxEndereco_idChanged() {
  if (User::isGerente()) { freteMinimoAtual = 0.; }
  canChangeFrete = false;
  ui->checkBoxFreteManual->setChecked(false);
  ui->checkBoxFreteManual->setEnabled(true);

  if (not ui->checkBoxRepresentacao->isChecked()) { ui->doubleSpinBoxFrete->setMinimum(0); }

  if (const auto resultado = calcularFrete()) { aplicarFreteCalculado(*resultado); }

  if (not ui->checkBoxRepresentacao->isChecked()) {
    if (qFuzzyIsNull(freteMinimoAtual)) { freteMinimoAtual = ui->doubleSpinBoxFrete->value(); }
    renderTotais();
  }

  const QString disclaimer = "O VALOR CALCULADO PARA O FRETE É VÁLIDO APENAS PARA AS REGIÕES DE SÃO PAULO, BARUERI E JUNDIAÍ.";
  QString observacao = ui->plainTextEditObs->toPlainText();

  if (ui->itemBoxEndereco->text() == "NÃO HÁ/RETIRA") {
    if (!observacao.contains(disclaimer, Qt::CaseInsensitive)) { observacao += disclaimer; }
  } else {
    observacao.remove(disclaimer, Qt::CaseInsensitive);
  }

  ui->plainTextEditObs->setPlainText(observacao);
}

bool Orcamento::verificaServicosEspeciais() {
  QStringList fornecedores;

  for (int row = 0; row < modelItem.rowCount(); ++row) { fornecedores << modelItem.data(row, "fornecedor").toString(); }

  fornecedores.removeDuplicates();

  return fornecedores.size() == 1 and fornecedores.first() == "STACCATO SERVIÇOS ESPECIAIS (SSE)";
}

std::optional<FreteResultado> Orcamento::calcularFrete() {
  if (ui->checkBoxFreteManual->isChecked()) { return std::nullopt; }
  if (verificaServicosEspeciais()) { return FreteResultado{0., 0., true}; }
  if (replicando) { return std::nullopt; }

  double fretePorcentagem = ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100.;
  double freteMaior = qMax(fretePorcentagem, minimoFrete);
  double minimoGerenteNovo = freteMinimoAtual; // preserva o valor anterior se o bloco de endereço abaixo não recalcular

  if (!ui->itemBoxEndereco->text().isEmpty() and ui->itemBoxEndereco->text() != "NÃO HÁ/RETIRA") {
    double pesoSul = 0.;
    double pesoTotal = 0.;

    QStringList idProdutos;
    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; }
      idProdutos << modelItem.data(row, "idProduto").toString();
    }

    const auto pesos = Sql::pesosProdutos(idProdutos);

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

      const QString idProduto = modelItem.data(row, "idProduto").toString();

      if (not pesos.contains(idProduto)) { throw RuntimeException("Produto não encontrado com id: " + idProduto); }

      const auto &p = pesos.value(idProduto);
      const double peso = modelItem.data(row, "caixas").toDouble() * p.kgcx;

      if (p.vemDoSul) { pesoSul += peso; }

      pesoTotal += peso;
    }

    // --------------------------------------------

    CalculoFrete calculoFrete;
    calculoFrete.setOrcamento(ui->itemBoxEndereco->getId(), pesoSul, pesoTotal);

    double freteQualp = 0.;

    try {
      freteQualp = calculoFrete.getFrete();
    } catch (std::exception &e) {
      Log::createLog("Exceção", e.what());
      qApp->enqueueInformation("Frete não pôde ser calculado automaticamente, verifique o valor manualmente!", this);
    }

    freteMaior = qMax(freteMaior, freteQualp);

    if (User::isGerente()) {
      const double freteMenor = qMin(freteQualp, freteMaior);
      minimoGerenteNovo = qFuzzyIsNull(freteMenor) ? freteMaior : freteMenor * 0.8;
    }
  }

  return FreteResultado{freteMaior, User::isGerente() ? minimoGerenteNovo : freteMaior, false};
}

void Orcamento::on_checkBoxFreteManual_clicked(const bool checked) {
  Q_UNUSED(checked)

  if (not canChangeFrete) {
    if (User::temPermissao("ajusteFrete")) {
      canChangeFrete = true;
    } else {
      qApp->enqueueInformation("Necessário autorização do administrativo!", this);

      LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

      if (dialog.exec() != QDialog::Accepted) {
        ui->checkBoxFreteManual->setChecked(false);
        return;
      }

      canChangeFrete = true;
      ui->checkBoxFreteManual->setDisabled(true);
    }
  }

  if (User::temPermissao("ajusteFrete")) {
    freteMinimoAtual = 0;
    ui->doubleSpinBoxFrete->setMinimum(0);
  } else {
    freteMinimoAtual = User::valorMinimoFrete;
    ui->doubleSpinBoxFrete->setMinimum(User::valorMinimoFrete);
    ui->doubleSpinBoxFrete->setValue(User::valorMinimoFrete);
    User::valorMinimoFrete = -1;
  }
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
  queryEstoque.prepare("SELECT 0 FROM produto WHERE idProduto = :idProduto AND estoqueRestante >= :quant LIMIT 1");

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    const bool isEstoque = modelItem.data(row, "estoque").toBool();

    if (not isEstoque) {
      queryProduto.bindValue(":idProduto", modelItem.data(row, "idProduto"));

      if (not queryProduto.exec()) { throw RuntimeException("Erro verificando validade dos produtos: " + queryProduto.lastError().text()); }

      if (not queryProduto.first()) { throw RuntimeException("Dados do produto não encontrado!"); }

      if (queryProduto.value("invalido").toBool()) {
        queryEquivalente.bindValue(":fornecedor", modelItem.data(row, "fornecedor"));
        queryEquivalente.bindValue(":codComercial", modelItem.data(row, "codComercial"));

        if (not queryEquivalente.exec()) { throw RuntimeException("Erro procurando produto equivalente: " + queryEquivalente.lastError().text()); }

        if (queryEquivalente.first()) {
          modelItem.setData(row, "idProduto", queryEquivalente.value("idProduto"));
        } else {
          produtos << "Linha " + QString::number(row + 1) + " - " + modelItem.data(row, "produto").toString() + " - Cód. " + modelItem.data(row, "codComercial").toString();
          skipRows << row;
        }
      }
    }

    if (isEstoque) {
      queryEstoque.bindValue(":idProduto", modelItem.data(row, "idProduto"));
      queryEstoque.bindValue(":quant", modelItem.data(row, "quant"));

      if (not queryEstoque.exec()) { throw RuntimeException("Erro verificando estoque: " + queryEstoque.lastError().text()); }

      if (not queryEstoque.first()) {
        estoques << "Linha " + QString::number(row + 1) + " - " + modelItem.data(row, "produto").toString() + " - Cód. " + modelItem.data(row, "codComercial").toString();
        skipRows << row;
      }
    }
  }

  if (not produtos.isEmpty()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Os seguintes itens estão descontinuados e serão removidos da réplica:\n    " + produtos.join("\n    "), QMessageBox::Yes | QMessageBox::No,
                       this);
    msgBox.button(QMessageBox::Yes)->setText("Continuar");
    msgBox.button(QMessageBox::No)->setText("Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  if (not estoques.isEmpty()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!",
                       "Os seguintes produtos de estoque não estão mais disponíveis na quantidade selecionada e serão removidos da réplica:\n    " + estoques.join("\n    "),
                       QMessageBox::Yes | QMessageBox::No, this);
    msgBox.button(QMessageBox::Yes)->setText("Continuar");
    msgBox.button(QMessageBox::No)->setText("Voltar");

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

  replica->replicando = true;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (skipRows.contains(row)) { continue; }

    replica->ui->itemBoxProduto->setId(modelItem.data(row, "idProduto"));
    replica->ui->doubleSpinBoxQuant->setValue(modelItem.data(row, "quant").toDouble());
    replica->ui->doubleSpinBoxDesconto->setValue(modelItem.data(row, "desconto").toDouble());
    replica->ui->lineEditObs->setText(modelItem.data(row, "obs").toString());
    replica->adicionarItem();
  }

  replica->replicando = false;
  replica->calcPrecoGlobalTotal();

  replica->show();
}

void Orcamento::cadastrar() {
  try {
    qApp->startTransaction("Orcamento::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    unsetConnections();
    savingProcedures();
    setConnections();

    model.submitAll();

    primaryId = ui->lineEditOrcamento->text();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    modelItem.submitAll();

    Sql::updateFornecedoresOrcamento(primaryId);

    qApp->endTransaction();

    backupItem.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelItem.setFilter(primaryKey + " = '" + primaryId + "'");
  } catch (std::exception &e) {
    qApp->rollbackTransaction(e.what());
    model.select();
    modelItem.select();

    for (auto &record : backupItem) { modelItem.insertRecord(-1, record); }

    if (tipo == Tipo::Cadastrar) { ui->lineEditOrcamento->setText("Auto gerado"); }

    throw;
  }
}

void Orcamento::verificaCadastroCliente() {
  SqlQuery queryCadastro;
  queryCadastro.prepare("SELECT incompleto FROM cliente WHERE idCliente = :id");
  queryCadastro.bindValue(":id", ui->itemBoxCliente->getId());

  if (not queryCadastro.exec()) { throw RuntimeException("Erro verificando se cadastro do cliente está completo: " + queryCadastro.lastError().text()); }

  if (not queryCadastro.first()) { throw RuntimeException("Dados do cliente não encontrado!"); }

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
  ui->checkBoxFreteManual->setHidden(checked);

  const double frete = qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete);
  ui->doubleSpinBoxFrete->setMinimum(checked ? 0 : frete);

  ui->itemBoxProduto->setRepresentacao(checked);
  novoItem();
}

void Orcamento::on_doubleSpinBoxDesconto_valueChanged(const double desconto) {
  itemFormState.prcUn = ui->doubleSpinBoxPrecoUn->value();

  itemFormState = reduceSetDesconto(itemFormState, desconto);

  renderItemForm();
}

void Orcamento::on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) {
  aplicarTotais("setDescontoReais", Log::dinheiro(descontoReais), reduceSetDescontoReais(totais, descontoReais));
  renderTotais();
}

void Orcamento::on_doubleSpinBoxFrete_valueChanged(const double frete) {
  aplicarTotais("setFrete", Log::dinheiro(frete), reduceSetFrete(totais, frete));
  renderTotais();
}

void Orcamento::on_itemBoxVendedor_textChanged() {
  if (ui->itemBoxVendedor->text().isEmpty()) { return; }

  buscarParametrosFrete();

  if (not ui->checkBoxFreteManual->isChecked()) {
    if (const auto resultado = calcularFrete()) { aplicarFreteCalculado(*resultado); }
  }
}

void Orcamento::buscarParametrosFrete() {
  const int idLoja = User::fromLoja("usuario.idLoja", ui->itemBoxVendedor->getId().toString()).toInt();

  if (idLoja == 0) { throw RuntimeException("Erro buscando idLoja!"); }

  SqlQuery queryFrete;
  queryFrete.prepare("SELECT valorMinimoFrete, porcentagemFrete FROM loja WHERE idLoja = :idLoja");
  queryFrete.bindValue(":idLoja", idLoja);

  if (not queryFrete.exec() or not queryFrete.next()) { throw RuntimeException("Erro buscando parâmetros do frete: " + queryFrete.lastError().text()); }

  minimoFrete = queryFrete.value("valorMinimoFrete").toDouble();
  porcFrete = queryFrete.value("porcentagemFrete").toDouble();
}

void Orcamento::on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) {
  aplicarTotais("setDescontoPorc", Log::dinheiro(descontoPorc), reduceSetDescontoPorc(totais, descontoPorc));
  renderTotais();
}

void Orcamento::on_doubleSpinBoxTotal_valueChanged(const double total) {
  aplicarTotais("setTotal", Log::dinheiro(total), reduceSetTotal(totais, total));
  renderTotais();
}

void Orcamento::on_doubleSpinBoxTotalItem_valueChanged() {
  if (ui->itemBoxProduto->text().isEmpty()) { return; }

  itemFormState.prcUn = ui->doubleSpinBoxPrecoUn->value();

  itemFormState = reduceSetTotalItem(itemFormState, ui->doubleSpinBoxTotalItem->value());

  renderItemForm();
}

void Orcamento::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Orçamento cadastrado com sucesso!", this); }

void Orcamento::on_pushButtonCalculadora_clicked() { QDesktopServices::openUrl(QUrl::fromLocalFile(R"(C:\Windows\System32\calc.exe)")); }

void Orcamento::on_dataEmissao_dateChanged(const QDate date) { ui->spinBoxValidade->setMaximum(date.daysInMonth() - date.day()); }

void Orcamento::verificaDisponibilidadeEstoque() {
  SqlQuery query;

  QStringList produtos;
  QMap<int, double> totalPorProduto;
  QMap<int, QString> nomePorProduto;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; }
    if (modelItem.data(row, "estoque").toInt() != 1) { continue; }

    const int idProduto = modelItem.data(row, "idProduto").toInt();
    totalPorProduto[idProduto] += modelItem.data(row, "quant").toDouble();
    nomePorProduto[idProduto] = modelItem.data(row, "produto").toString();
  }

  for (auto it = totalPorProduto.cbegin(); it != totalPorProduto.cend(); ++it) {
    query.prepare("SELECT 0 FROM produto WHERE idProduto = :id AND estoqueRestante >= :quant LIMIT 1");
    query.bindValue(":id", it.key());
    query.bindValue(":quant", it.value());

    if (not query.exec()) { throw RuntimeException("Erro verificando a disponibilidade do estoque: " + query.lastError().text()); }

    if (not query.first()) { produtos << nomePorProduto[it.key()]; }
  }

  if (not produtos.isEmpty()) {
    throw RuntimeError("Os seguintes produtos de estoque não estão mais disponíveis na quantidade selecionada:\n    -" + produtos.join("\n    -") + "\n\nRemova ou diminua a quant. para prosseguir!");
  }
}

// TODO: esse código está repetido em venda e searchDialog, refatorar
void Orcamento::on_pushButtonModelo3d_clicked() {
  const auto selection = ui->tableProdutos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const int row = selection.first().row();

  const QString ip = qApp->getWebDavIp();
  const QString fornecedor = modelItem.data(row, "fornecedor").toString();
  const QString codComercial = modelItem.data(row, "codComercial").toString();

  const QString url = "https://" + ip + "/webdav/SISTEMA/MODELOS 3D/" + fornecedor + "/" + codComercial + ".skp";

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    authenticator->setUser(User::usuario);
    authenticator->setPassword(User::senha);
  });

  auto *reply = manager->get(QNetworkRequest(QUrl(url)));

  QPointer<Orcamento> self(this);

  connect(reply, &QNetworkReply::finished, this, [=] {
    if (not self) { return; }

    if (reply->error() != QNetworkReply::NoError) {
      const QString msg = reply->error() == QNetworkReply::ContentNotFoundError ? "Produto não possui modelo 3D!" : "Erro ao baixar arquivo: " + reply->errorString();
      qApp->enqueueInformation(msg, self);
      return;
    }

    const QString filename = QDir::currentPath() + "/arquivos/" + url.split("/").last();

    File file(filename);

    if (not file.open(QFile::WriteOnly)) {
      qApp->enqueueInformation("Erro abrindo arquivo para escrita: " + file.errorString(), self);
      return;
    }

    file.write(reply->readAll());

    file.close();

    if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filename))) { qApp->enqueueInformation("Não foi possível abrir o arquivo 3D!", self); }
  });
}

double Orcamento::calcularPeso() {
  SqlQuery queryProduto;
  queryProduto.prepare("SELECT kgcx FROM produto WHERE idProduto = :id");
  queryProduto.bindValue(":id", ui->itemBoxProduto->getId());

  if (not queryProduto.exec()) { throw RuntimeException("Erro buscando kgcx: " + queryProduto.lastError().text()); }

  if (not queryProduto.first()) { throw RuntimeException("Peso não encontrado do produto com id: '" + ui->itemBoxProduto->getId().toString() + "'"); }

  return ui->doubleSpinBoxCaixas->value() * queryProduto.value("kgcx").toDouble();
}

void Orcamento::calcularPesoTotal() {
  QStringList idProdutos;
  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion
    idProdutos << modelItem.data(row, "idProduto").toString();
  }

  const auto pesos = Sql::pesosProdutos(idProdutos);

  double total = 0;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.headerData(row, Qt::Vertical) == "!") { continue; } // skip item pending deletion

    const QString idProduto = modelItem.data(row, "idProduto").toString();

    if (not pesos.contains(idProduto)) { throw RuntimeException("Peso não encontrado do produto com id: '" + idProduto + "'"); }

    total += modelItem.data(row, "caixas").toDouble() * pesos.value(idProduto).kgcx;
  }

  // TODO: implicit conversion double -> int
  ui->spinBoxPesoTotal->setValue(total);
}

void Orcamento::verificaSeFoiAlterado() {
  if (ui->lineEditOrcamento->text() == "Auto gerado") { return; }

  SqlQuery query;
  query.prepare("SELECT lastUpdated FROM orcamento WHERE idOrcamento = :id");
  query.bindValue(":id", data("idOrcamento"));

  if (not query.exec()) { throw RuntimeException("Erro verificando se orçamento foi alterado: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Erro verificando se orçamento foi alterado!"); }

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

void Orcamento::connectLineEditsToDirty() {
  const auto children = ui->tabWidget->findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &Orcamento::marcarDirty); }
}

void Orcamento::on_pushButtonAbrirReplicaDe_clicked() {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);
  orcamento->viewRegisterById(ui->lineEditReplicaDe->text());

  orcamento->show();
}

void Orcamento::on_pushButtonAbrirReplicadoEm_clicked() {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);
  orcamento->viewRegisterById(ui->lineEditReplicadoEm->text());

  orcamento->show();
}

void Orcamento::on_pushButtonAbrirVenda_clicked() {
  auto *venda = new Venda(this);
  venda->setAttribute(Qt::WA_DeleteOnClose);
  venda->viewRegisterById(ui->lineEditVenda->text());

  venda->show();
}

void Orcamento::buscarIdVenda() {
  SqlQuery query;
  query.prepare("SELECT idVenda FROM venda WHERE idOrcamento = :id AND status NOT IN ('CANCELADO')");
  query.bindValue(":id", ui->lineEditOrcamento->text());

  if (not query.exec()) { throw RuntimeException("Erro buscando venda: " + query.lastError().text()); }

  if (query.first()) {
    ui->lineEditVenda->setText(query.value("idVenda").toString());

    ui->labelVenda->show();
    ui->pushButtonAbrirVenda->show();
    ui->lineEditVenda->show();
  }
}

bool Orcamento::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::Wheel and (obj->inherits("QSpinBox") or obj->inherits("QDoubleSpinBox"))) {
    event->ignore();
    return true;
  }

  return QObject::eventFilter(obj, event);
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
// TODO: antes de gerar excel/pdf salvar o arquivo para não ficar dados divergentes
