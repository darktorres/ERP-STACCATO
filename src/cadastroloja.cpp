#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "cadastroloja.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "porcentagemdelegate.h"
#include "searchdialog.h"
#include "ui_cadastroloja.h"
#include "usersession.h"

CadastroLoja::CadastroLoja(QWidget *parent) : RegisterAddressDialog("loja", "idLoja", parent), ui(new Ui::CadastroLoja) {
  ui->setupUi(this);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  setupUi();
  setupTables();
  setupMapper();
  newRegister();

  sdLoja = SearchDialog::loja(this);
  connect(sdLoja, &SearchDialog::itemSelected, this, &CadastroLoja::viewRegisterById);

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonLimparSelecao->hide();

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") {
    ui->pushButtonRemover->setDisabled(true);
    ui->pushButtonRemoverEnd->setDisabled(true);
  }

  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroLoja::on_checkBoxMostrarInativos_clicked);
  connect(ui->checkBoxMostrarInativosConta, &QCheckBox::clicked, this, &CadastroLoja::on_checkBoxMostrarInativosConta_clicked);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroLoja::on_lineEditCEP_textChanged);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroLoja::on_lineEditCNPJ_textEdited);
  connect(ui->pushButtonAdicionaAssociacao, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAdicionaAssociacao_clicked);
  connect(ui->pushButtonAdicionarConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAdicionarConta_clicked);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAdicionarEnd_clicked);
  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAdicionarPagamento_clicked);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonAtualizarConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizarConta_clicked);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizarEnd_clicked);
  connect(ui->pushButtonAtualizarPagamento, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizarPagamento_clicked);
  connect(ui->pushButtonAtualizarTaxas, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizarTaxas_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonLimparSelecao_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemoveAssociacao, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoveAssociacao_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverConta_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverEnd_clicked);
  connect(ui->pushButtonRemoverPagamento, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverPagamento_clicked);
  connect(ui->tableConta, &TableView::clicked, this, &CadastroLoja::on_tableConta_clicked);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroLoja::on_tableEndereco_clicked);
  connect(ui->tablePagamentos, &TableView::clicked, this, &CadastroLoja::on_tablePagamentos_clicked);
}

CadastroLoja::~CadastroLoja() { delete ui; }

void CadastroLoja::setupUi() {
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditSIGLA->setInputMask(">AANN;_");
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
}

void CadastroLoja::setupTables() {
  modelAssocia1.setTable("forma_pagamento");

  modelAssocia1.setHeaderData("pagamento", "Pagamento");
  modelAssocia1.setHeaderData("parcelas", "Parcelas");

  ui->tableAssocia1->setModel(&modelAssocia1);

  ui->tableAssocia1->hideColumn("idPagamento");

  // -------------------------------------------------------------------------

  modelAssocia2.setTable("view_pagamento_loja");

  modelAssocia2.setHeaderData("pagamento", "Pagamento");

  ui->tableAssocia2->setModel(&modelAssocia2);

  ui->tableAssocia2->hideColumn("idLoja");
  ui->tableAssocia2->hideColumn("idPagamento");

  // -------------------------------------------------------------------------

  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idLoja");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(this, true));

  ui->tableEndereco->setPersistentColumns({"desativado"});

  // -------------------------------------------------------------------------

  modelConta.setTable("loja_has_conta");

  modelConta.setFilter("idLoja = " + QString::number(UserSession::idLoja())); // ????????

  modelConta.setHeaderData("banco", "Banco");
  modelConta.setHeaderData("agencia", "Agência");
  modelConta.setHeaderData("conta", "Conta");
  modelConta.setHeaderData("desativado", "Desativado");

  ui->tableConta->setModel(&modelConta);

  ui->tableConta->hideColumn("idConta");
  ui->tableConta->hideColumn("idLoja");

  ui->tableConta->setItemDelegateForColumn("desativado", new CheckBoxDelegate(this, true));

  ui->tableConta->setPersistentColumns({"desativado"});

  // -------------------------------------------------------------------------

  modelPagamentos.setTable("forma_pagamento");

  modelPagamentos.setHeaderData("pagamento", "Pagamento");
  modelPagamentos.setHeaderData("parcelas", "Parcelas");

  ui->tablePagamentos->setModel(&modelPagamentos);

  ui->tablePagamentos->hideColumn("idPagamento");

  // -------------------------------------------------------------------------

  modelTaxas.setTable("forma_pagamento_has_taxa");

  modelTaxas.setHeaderData("parcela", "Quant. Parcelas");
  modelTaxas.setHeaderData("taxa", "Taxa");

  ui->tableTaxas->setModel(&modelTaxas);

  ui->tableTaxas->hideColumn("idTaxa");
  ui->tableTaxas->hideColumn("idPagamento");

  ui->tableTaxas->setItemDelegateForColumn("taxa", new PorcentagemDelegate(this));
}

void CadastroLoja::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  novaConta();
  setupUi();
}

bool CadastroLoja::verifyFields() {
  const auto children = ui->groupBoxCadastro->findChildren<QLineEdit *>();

  for (const auto &line : children) {
    if (not verifyRequiredField(line)) { return false; }
  }

  return true;
}

bool CadastroLoja::savingProcedures() {
  if (not setData("descricao", ui->lineEditDescricao->text())) { return false; }
  if (not setData("razaoSocial", ui->lineEditRazaoSocial->text())) { return false; }
  if (not setData("sigla", ui->lineEditSIGLA->text())) { return false; }
  if (not setData("nomeFantasia", ui->lineEditNomeFantasia->text())) { return false; }
  if (not setData("cnpj", ui->lineEditCNPJ->text())) { return false; }
  if (not setData("inscEstadual", ui->lineEditInscEstadual->text())) { return false; }
  if (not setData("tel", ui->lineEditTel->text())) { return false; }
  if (not setData("tel2", ui->lineEditTel2->text())) { return false; }
  if (not setData("valorMinimoFrete", ui->doubleSpinBoxValorMinimoFrete->value())) { return false; }
  if (not setData("porcentagemFrete", ui->doubleSpinBoxPorcFrete->value())) { return false; }
  if (not setData("custoTransporteTon", ui->doubleSpinBoxCustoTransportePorTon->value())) { return false; }
  if (not setData("custoTransporte1", ui->doubleSpinBoxCustoTransporte2Ton->value())) { return false; }
  if (not setData("custoTransporte2", ui->doubleSpinBoxCustoTransporteAcima2Ton->value())) { return false; }
  if (not setData("custoFuncionario", ui->doubleSpinBoxCustoFuncionario->value())) { return false; }

  return true;
}

void CadastroLoja::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

  ui->pushButtonRemoverEnd->hide();

  ui->pushButtonRemoverConta->hide();

  ui->tabWidget->setTabEnabled(1, false);
  ui->tabWidget->setTabEnabled(2, false);
  ui->tabWidget->setTabEnabled(3, false);
}

void CadastroLoja::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

void CadastroLoja::setupMapper() {
  addMapping(ui->doubleSpinBoxPorcFrete, "porcentagemFrete");
  addMapping(ui->doubleSpinBoxValorMinimoFrete, "valorMinimoFrete");
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditDescricao, "descricao");
  addMapping(ui->lineEditInscEstadual, "inscEstadual");
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditRazaoSocial, "razaoSocial");
  addMapping(ui->lineEditSIGLA, "sigla");
  addMapping(ui->lineEditTel, "tel");
  addMapping(ui->lineEditTel2, "tel2");
  addMapping(ui->doubleSpinBoxCustoTransportePorTon, "custoTransporteTon");
  addMapping(ui->doubleSpinBoxCustoTransporte2Ton, "custoTransporte1");
  addMapping(ui->doubleSpinBoxCustoTransporteAcima2Ton, "custoTransporte2");
  addMapping(ui->doubleSpinBoxCustoFuncionario, "custoFuncionario");

  mapperEnd.addMapping(ui->comboBoxTipoEnd, modelEnd.fieldIndex("descricao"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditComp, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNro, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));

  mapperConta.setModel(&modelConta);
  mapperConta.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
  mapperConta.addMapping(ui->lineEditBanco, modelConta.fieldIndex("banco"));
  mapperConta.addMapping(ui->lineEditAgencia, modelConta.fieldIndex("agencia"));
  mapperConta.addMapping(ui->lineEditConta, modelConta.fieldIndex("conta"));

  mapperPagamento.setModel(&modelPagamentos);
  mapperPagamento.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
  mapperPagamento.addMapping(ui->lineEditPagamento, modelPagamentos.fieldIndex("pagamento"));
  mapperPagamento.addMapping(ui->spinBoxParcelas, modelPagamentos.fieldIndex("parcelas"));
}

void CadastroLoja::on_pushButtonCadastrar_clicked() { save(); }

void CadastroLoja::on_pushButtonAtualizar_clicked() { save(); }

void CadastroLoja::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroLoja::on_pushButtonRemover_clicked() { remove(); }

void CadastroLoja::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdLoja->show();
}

void CadastroLoja::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "background-color: rgb(255, 255, 127);color: rgb(0, 190, 0)"
                                                                                                : "background-color: rgb(255, 255, 127);color: rgb(255, 0, 0)");
}

void CadastroLoja::on_pushButtonAdicionarEnd_clicked() { cadastrarEndereco() ? novoEndereco() : qApp->enqueueError("Não foi possível cadastrar este endereço!", this); }

void CadastroLoja::on_pushButtonAtualizarEnd_clicked() { cadastrarEndereco(Tipo::Atualizar) ? novoEndereco() : qApp->enqueueError("Não foi possível cadastrar este endereço!", this); }

void CadastroLoja::on_pushButtonRemoverEnd_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not setDataEnd("desativado", true)) { return; }

    if (not modelEnd.submitAll()) { return; }

    novoEndereco();
  }
}

void CadastroLoja::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idLoja = " + data("idLoja").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return; }
}

bool CadastroLoja::cadastrarEndereco(const Tipo tipo) {
  if (not ui->lineEditCEP->isValid()) {
    qApp->enqueueError("CEP inválido!", this);
    ui->lineEditCEP->setFocus();
    return false;
  }

  if (tipo == Tipo::Cadastrar) {
    currentRowEnd = modelEnd.rowCount();
    modelEnd.insertRow(currentRowEnd);
  }

  if (not setDataEnd("descricao", ui->comboBoxTipoEnd->currentText())) { return false; }
  if (not setDataEnd("cep", ui->lineEditCEP->text())) { return false; }
  if (not setDataEnd("logradouro", ui->lineEditLogradouro->text())) { return false; }
  if (not setDataEnd("numero", ui->lineEditNro->text())) { return false; }
  if (not setDataEnd("complemento", ui->lineEditComp->text())) { return false; }
  if (not setDataEnd("bairro", ui->lineEditBairro->text())) { return false; }
  if (not setDataEnd("cidade", ui->lineEditCidade->text())) { return false; }
  if (not setDataEnd("uf", ui->lineEditUF->text())) { return false; }
  if (not setDataEnd("codUF", getCodigoUF(ui->lineEditUF->text()))) { return false; }
  if (not setDataEnd("desativado", false)) { return false; }

  if (not columnsToUpper(modelEnd, currentRowEnd)) { return false; }

  isDirty = true;

  return true;
}

void CadastroLoja::novoEndereco() {
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonRemoverEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroLoja::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComp->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNro->clear();
  ui->lineEditUF->clear();
}

void CadastroLoja::on_lineEditCEP_textChanged(const QString &cep) {
  if (not ui->lineEditCEP->isValid()) { return; }

  ui->lineEditNro->clear();
  ui->lineEditComp->clear();

  if (CepCompleter cc; cc.buscaCEP(cep)) {
    ui->lineEditUF->setText(cc.getUf());
    ui->lineEditCidade->setText(cc.getCidade());
    ui->lineEditLogradouro->setText(cc.getEndereco());
    ui->lineEditBairro->setText(cc.getBairro());
  }
}

void CadastroLoja::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();
  mapperEnd.setCurrentModelIndex(index);
  currentRowEnd = index.row();
}

bool CadastroLoja::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativosEnd = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idLoja = " + primaryId + (inativosEnd ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return false; }

  // -------------------------------------------------------------------------

  const bool inativosConta = ui->checkBoxMostrarInativosConta->isChecked();
  modelConta.setFilter("idLoja = " + primaryId + (inativosConta ? "" : " AND desativado = FALSE"));

  if (not modelConta.select()) { return false; }

  // -------------------------------------------------------------------------

  modelPagamentos.setFilter("");

  if (not modelPagamentos.select()) { return false; }

  // -------------------------------------------------------------------------

  modelAssocia1.setFilter("");

  if (not modelAssocia1.select()) { return false; }

  // -------------------------------------------------------------------------

  modelAssocia2.setFilter("idLoja = " + data("idLoja").toString());

  if (not modelAssocia2.select()) { return false; }

  // -------------------------------------------------------------------------

  ui->tabWidget->setTabEnabled(1, true);
  ui->tabWidget->setTabEnabled(2, true);
  ui->tabWidget->setTabEnabled(3, true);

  return true;
}

void CadastroLoja::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Loja cadastrada com sucesso!", this); }

bool CadastroLoja::cadastrar() {
  if (tipo == Tipo::Cadastrar) {
    currentRow = model.rowCount();
    model.insertRow(currentRow);
  }

  if (not savingProcedures()) { return false; }

  if (not columnsToUpper(model, currentRow)) { return false; }

  if (not model.submitAll()) { return false; }

  primaryId = (tipo == Tipo::Atualizar) ? data(currentRow, primaryKey).toString() : model.query().lastInsertId().toString();

  if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

  // -------------------------------------------------------------------------

  if (not setForeignKey(modelEnd)) { return false; }

  if (not modelEnd.submitAll()) { return false; }

  // -------------------------------------------------------------------------

  if (not setForeignKey(modelConta)) { return false; }

  return modelConta.submitAll();
}

void CadastroLoja::on_tableConta_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novaConta(); }

  ui->pushButtonAtualizarConta->show();
  ui->pushButtonAdicionarConta->hide();
  ui->pushButtonRemoverConta->show();
  mapperConta.setCurrentModelIndex(index);
  currentRowConta = index.row();
}

bool CadastroLoja::newRegister() {
  if (not RegisterAddressDialog::newRegister()) { return false; }

  modelConta.setFilter("0");

  return true;
}

bool CadastroLoja::cadastrarConta(const Tipo tipo) {
  if (ui->lineEditBanco->text().isEmpty()) {
    qApp->enqueueError("Banco inválido!", this);
    ui->lineEditBanco->setFocus();
    return false;
  }

  if (ui->lineEditAgencia->text().isEmpty()) {
    qApp->enqueueError("Agência inválida!", this);
    ui->lineEditAgencia->setFocus();
    return false;
  }

  if (ui->lineEditConta->text().isEmpty()) {
    qApp->enqueueError("Conta inválida!", this);
    ui->lineEditConta->setFocus();
    return false;
  }

  if (tipo == Tipo::Cadastrar) {
    currentRowConta = modelConta.rowCount();
    modelConta.insertRow(currentRowConta);
  }

  if (not modelConta.setData(currentRowConta, "banco", ui->lineEditBanco->text())) { return false; }
  if (not modelConta.setData(currentRowConta, "agencia", ui->lineEditAgencia->text())) { return false; }
  if (not modelConta.setData(currentRowConta, "conta", ui->lineEditConta->text())) { return false; }

  if (not columnsToUpper(modelConta, currentRowConta)) { return false; }

  isDirty = true;

  return true;
}

void CadastroLoja::novaConta() {
  ui->pushButtonAtualizarConta->hide();
  ui->pushButtonAdicionarConta->show();
  ui->pushButtonRemoverConta->hide();
  ui->tableConta->clearSelection();
  clearConta();
}

void CadastroLoja::clearConta() {
  ui->lineEditBanco->clear();
  ui->lineEditAgencia->clear();
  ui->lineEditConta->clear();
}

void CadastroLoja::on_pushButtonAdicionarConta_clicked() { cadastrarConta() ? novaConta() : qApp->enqueueError("Não foi possível cadastrar esta conta.", this); }

void CadastroLoja::on_pushButtonAtualizarConta_clicked() { cadastrarConta(Tipo::Atualizar) ? novaConta() : qApp->enqueueError("Não foi possível cadastrar esta conta.", this); }

void CadastroLoja::on_pushButtonRemoverConta_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not modelConta.setData(mapperConta.currentIndex(), "desativado", true)) { return; }

    if (not modelConta.submitAll()) { return; }

    novaConta();
  }
}

void CadastroLoja::on_checkBoxMostrarInativosConta_clicked(bool checked) {
  if (currentRow == -1) { return; }

  modelConta.setFilter("idLoja = " + data("idLoja").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelConta.select()) { return; }
}

bool CadastroLoja::adicionarPagamento() {
  const int row = modelPagamentos.rowCount();
  if (not modelPagamentos.insertRow(row)) { return qApp->enqueueError(false, "Erro inserindo linha na tabela: " + model.lastError().text(), this); }

  if (not modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text())) { return false; }
  if (not modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value())) { return false; }

  if (not modelPagamentos.submitAll()) { return false; }

  const int id = modelPagamentos.query().lastInsertId().toInt();

  for (int i = 0; i < ui->spinBoxParcelas->value(); ++i) {
    const int rowTaxas = modelTaxas.rowCount();
    if (not modelTaxas.insertRow(rowTaxas)) { return qApp->enqueueError(false, "Erro inserindo linha na tabela: " + modelTaxas.lastError().text(), this); }

    if (not modelTaxas.setData(rowTaxas, "idPagamento", id)) { return false; }
    if (not modelTaxas.setData(rowTaxas, "parcela", i + 1)) { return false; }
    if (not modelTaxas.setData(rowTaxas, "taxa", 0)) { return false; }
  }

  if (not modelTaxas.submitAll()) { return false; }

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  return modelTaxas.select();
}

void CadastroLoja::on_pushButtonAdicionarPagamento_clicked() {
  if (not qApp->startTransaction()) { return; }

  if (not adicionarPagamento()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }
}

void CadastroLoja::on_tablePagamentos_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const int id = modelPagamentos.data(index.row(), "idPagamento").toInt();

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  if (not modelTaxas.select()) { return; }

  mapperPagamento.setCurrentModelIndex(index);

  ui->pushButtonAdicionarPagamento->hide();
  ui->pushButtonAtualizarPagamento->show();
  ui->pushButtonLimparSelecao->show();
}

void CadastroLoja::on_pushButtonRemoverPagamento_clicked() {
  const auto list = ui->tablePagamentos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  const int id = modelPagamentos.data(list.first().row(), "idPagamento").toInt();

  //--------------------------------------

  QSqlQuery query;
  query.prepare("DELETE FROM forma_pagamento WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", id);

  if (not query.exec()) { return qApp->enqueueError("Erro apagando pagamentos: " + query.lastError().text(), this); }

  //--------------------------------------

  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->clear();

  //--------------------------------------

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();
}

bool CadastroLoja::atualizarPagamento() {
  const int row = mapperPagamento.currentIndex();

  if (not modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text())) { return false; }
  if (not modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value())) { return false; }

  //--------------------------------------

  if (not modelTaxas.removeRows(0, modelTaxas.rowCount())) { return qApp->enqueueError(false, "Erro removendo linha: " + modelTaxas.lastError().text(), this); }

  //--------------------------------------

  for (int i = 0; i < ui->spinBoxParcelas->value(); ++i) {
    const int rowTaxas = modelTaxas.rowCount();
    if (not modelTaxas.insertRow(rowTaxas)) { return qApp->enqueueError(false, "Erro inserindo linha na tabela: " + modelTaxas.lastError().text(), this); }

    if (not modelTaxas.setData(rowTaxas, "idPagamento", modelPagamentos.data(rowTaxas, "idPagamento"))) { return false; }
    if (not modelTaxas.setData(rowTaxas, "parcela", i + 1)) { return false; }
    if (not modelTaxas.setData(rowTaxas, "taxa", 0)) { return false; }
  }

  //--------------------------------------

  if (not modelPagamentos.submitAll()) { return false; }

  //--------------------------------------

  if (not modelTaxas.submitAll()) { return false; }

  //--------------------------------------

  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->clear();

  //--------------------------------------

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();

  //--------------------------------------

  return true;
}

void CadastroLoja::on_pushButtonAtualizarPagamento_clicked() {
  if (not qApp->startTransaction()) { return; }

  if (not atualizarPagamento()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }
}

void CadastroLoja::on_pushButtonAtualizarTaxas_clicked() {
  if (not modelTaxas.submitAll()) { return; }

  qApp->enqueueInformation("Taxas atualizadas!", this);
}

void CadastroLoja::on_pushButtonAdicionaAssociacao_clicked() {
  const auto list = ui->tableAssocia1->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  QSqlQuery query;
  query.prepare("INSERT INTO forma_pagamento_has_loja (idPagamento, idLoja) VALUES (:idPagamento, :idLoja)");

  for (const auto &index : list) {
    query.bindValue(":idPagamento", modelAssocia1.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", data("idLoja"));

    if (not query.exec()) { qApp->enqueueError("Erro cadastrando associacao: " + query.lastError().text(), this); }
  }
}

void CadastroLoja::on_pushButtonRemoveAssociacao_clicked() {
  const auto list = ui->tableAssocia2->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  QSqlQuery query;
  query.prepare("DELETE FROM forma_pagamento_has_loja WHERE idPagamento = :idPagamento AND idLoja = :idLoja");

  for (const auto &index : list) {
    query.bindValue(":idPagamento", modelAssocia2.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", modelAssocia2.data(index.row(), "idLoja"));

    if (not query.exec()) { qApp->enqueueError("Erro removendo associacao: " + query.lastError().text(), this); }
  }
}

void CadastroLoja::on_pushButtonLimparSelecao_clicked() {
  // TODO: remove this function and use the click outside line tableView logic

  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->clear();

  //--------------------------------------

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();
  ui->pushButtonLimparSelecao->hide();

  //--------------------------------------

  ui->tablePagamentos->clearSelection();
}

// FIXME: nao permitir associar um pagamento já associado
