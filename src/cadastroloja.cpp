#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>

#include "cadastroloja.h"
#include "cepcompleter.h"
#include "porcentagemdelegate.h"
#include "searchdialog.h"
#include "ui_cadastroloja.h"
#include "usersession.h"

CadastroLoja::CadastroLoja(QWidget *parent) : RegisterAddressDialog("loja", "idLoja", parent), ui(new Ui::CadastroLoja) {
  ui->setupUi(this);

  Q_FOREACH (const auto &line, findChildren<QLineEdit *>()) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

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
  connect(ui->pushButtonContaLimpar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonContaLimpar_clicked);
  connect(ui->pushButtonEndLimpar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonEndLimpar_clicked);
  connect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonLimparSelecao_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemoveAssociacao, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoveAssociacao_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverConta_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverEnd_clicked);
  connect(ui->pushButtonRemoverPagamento, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverPagamento_clicked);
  connect(ui->tableConta, &TableView::clicked, this, &CadastroLoja::on_tableConta_clicked);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroLoja::on_tableEndereco_clicked);
  connect(ui->tableEndereco, &TableView::entered, this, &CadastroLoja::on_tableEndereco_entered);
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
  modelAssocia1.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelAssocia1.setHeaderData("pagamento", "Pagamento");
  modelAssocia1.setHeaderData("parcelas", "Parcelas");

  if (not modelAssocia1.select()) emit errorSignal("Erro lendo tabela forma_pagamento: " + modelAssocia1.lastError().text());

  ui->tableAssocia1->setModel(&modelAssocia1);
  ui->tableAssocia1->hideColumn("idPagamento");

  //

  modelAssocia2.setTable("view_pagamento_loja");
  modelAssocia2.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelAssocia2.setHeaderData("pagamento", "Pagamento");

  if (not modelAssocia2.select()) emit errorSignal("Erro lendo tabela view_pagamento_loja: " + modelAssocia2.lastError().text());

  ui->tableAssocia2->setModel(&modelAssocia2);
  ui->tableAssocia2->hideColumn("idLoja");
  ui->tableAssocia2->hideColumn("idPagamento");
  //

  modelPermissoes.setTable("usuario_has_permissao");
  modelPermissoes.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (not modelPermissoes.select()) {
    emit errorSignal("Erro lendo tabela de permissões: " + modelPermissoes.lastError().text());
    return;
  }

  //

  ui->tableEndereco->setModel(&modelEnd);
  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("desativado");
  ui->tableEndereco->hideColumn("idLoja");
  ui->tableEndereco->hideColumn("codUF");

  //

  modelConta.setTable("loja_has_conta");
  modelConta.setEditStrategy(QSqlTableModel::OnManualSubmit);
  modelConta.setFilter("idLoja = " + QString::number(UserSession::idLoja()));

  modelConta.setHeaderData("banco", "Banco");
  modelConta.setHeaderData("agencia", "Agência");
  modelConta.setHeaderData("conta", "Conta");

  ui->tableConta->setModel(&modelConta);
  ui->tableConta->hideColumn("idConta");
  ui->tableConta->hideColumn("idLoja");
  ui->tableConta->hideColumn("desativado");

  //
  modelPagamentos.setTable("forma_pagamento");
  modelPagamentos.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelPagamentos.setHeaderData("pagamento", "Pagamento");
  modelPagamentos.setHeaderData("parcelas", "Parcelas");

  ui->tablePagamentos->setModel(&modelPagamentos);
  ui->tablePagamentos->hideColumn("idPagamento");

  modelTaxas.setTable("forma_pagamento_has_taxa");
  modelTaxas.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelTaxas.setHeaderData("parcela", "Quant. Parcelas");
  modelTaxas.setHeaderData("taxa", "Taxa");

  ui->tableTaxas->setModel(&modelTaxas);
  ui->tableTaxas->hideColumn("idTaxa");
  ui->tableTaxas->hideColumn("idPagamento");
  ui->tableTaxas->setItemDelegateForColumn("taxa", new PorcentagemDelegate(this));
  //
}

void CadastroLoja::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  novaConta();
  setupUi();
}

bool CadastroLoja::verifyFields() {
  Q_FOREACH (const auto &line, ui->groupBoxCadastro->findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) return false;
  }

  return true;
}

bool CadastroLoja::savingProcedures() {
  if (not setData("descricao", ui->lineEditDescricao->text())) return false;
  if (not setData("razaoSocial", ui->lineEditRazaoSocial->text())) return false;
  if (not setData("sigla", ui->lineEditSIGLA->text())) return false;
  if (not setData("nomeFantasia", ui->lineEditNomeFantasia->text())) return false;
  if (not setData("cnpj", ui->lineEditCNPJ->text())) return false;
  if (not setData("inscEstadual", ui->lineEditInscEstadual->text())) return false;
  if (not setData("tel", ui->lineEditTel->text())) return false;
  if (not setData("tel2", ui->lineEditTel2->text())) return false;
  if (not setData("valorMinimoFrete", ui->doubleSpinBoxValorMinimoFrete->value())) return false;
  if (not setData("porcentagemFrete", ui->doubleSpinBoxPorcFrete->value())) return false;
  if (not setData("custoTransporteTon", ui->doubleSpinBoxCustoTransportePorTon->value())) return false;
  if (not setData("custoTransporte1", ui->doubleSpinBoxCustoTransporte2Ton->value())) return false;
  if (not setData("custoTransporte2", ui->doubleSpinBoxCustoTransporteAcima2Ton->value())) return false;
  if (not setData("custoFuncionario", ui->doubleSpinBoxCustoFuncionario->value())) return false;

  return true;
}

void CadastroLoja::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

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

void CadastroLoja::on_pushButtonBuscar_clicked() { sdLoja->show(); }

void CadastroLoja::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "background-color: rgb(255, 255, 127)"
                                                                                                : "background-color: rgb(255, 255, 127);color: rgb(255, 0, 0)");
}

void CadastroLoja::on_pushButtonAdicionarEnd_clicked() {
  if (not cadastrarEndereco()) {
    emit errorSignal("Não foi possível cadastrar este endereço.");
    return;
  }

  novoEndereco();
}

void CadastroLoja::on_pushButtonAtualizarEnd_clicked() {
  if (not cadastrarEndereco(true)) {
    emit errorSignal("Não foi possível atualizar este endereço.");
    return;
  }

  novoEndereco();
}

void CadastroLoja::on_pushButtonEndLimpar_clicked() { novoEndereco(); }

void CadastroLoja::on_pushButtonRemoverEnd_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not setDataEnd("desativado", true)) {
      emit errorSignal("Erro marcando desativado!");
      return;
    }

    if (not modelEnd.submitAll()) {
      emit errorSignal("Não foi possível remover este item: " + modelEnd.lastError().text());
      return;
    }

    novoEndereco();
  }
}

void CadastroLoja::on_checkBoxMostrarInativos_clicked(const bool checked) {
  modelEnd.setFilter("idLoja = " + data("idLoja").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) emit errorSignal("Erro lendo tabela endereço: " + modelEnd.lastError().text());

  ui->tableEndereco->resizeColumnsToContents();
}

bool CadastroLoja::cadastrarEndereco(const bool isUpdate) {
  Q_FOREACH (const auto &line, ui->groupBoxEndereco->findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) return false;
  }

  if (not ui->lineEditCEP->isValid()) {
    ui->lineEditCEP->setFocus();
    emit errorSignal("CEP inválido!");
    return false;
  }

  currentRowEnd = isUpdate ? mapperEnd.currentIndex() : modelEnd.rowCount();

  if (not isUpdate) modelEnd.insertRow(currentRowEnd);

  if (not setDataEnd("descricao", ui->comboBoxTipoEnd->currentText())) return false;
  if (not setDataEnd("cep", ui->lineEditCEP->text())) return false;
  if (not setDataEnd("logradouro", ui->lineEditLogradouro->text())) return false;
  if (not setDataEnd("numero", ui->lineEditNro->text())) return false;
  if (not setDataEnd("complemento", ui->lineEditComp->text())) return false;
  if (not setDataEnd("bairro", ui->lineEditBairro->text())) return false;
  if (not setDataEnd("cidade", ui->lineEditCidade->text())) return false;
  if (not setDataEnd("uf", ui->lineEditUF->text())) return false;
  if (not setDataEnd("codUF", getCodigoUF(ui->lineEditUF->text()))) return false;
  if (not setDataEnd("desativado", false)) return false;

  ui->tableEndereco->resizeColumnsToContents();

  isDirty = true;

  return true;
}

void CadastroLoja::novoEndereco() {
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonAdicionarEnd->show();
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
  if (not ui->lineEditCEP->isValid()) return;

  ui->lineEditNro->clear();
  ui->lineEditComp->clear();

  CepCompleter cc;

  if (not cc.buscaCEP(cep)) {
    emit warningSignal("CEP não encontrado!");
    return;
  }

  ui->lineEditUF->setText(cc.getUf());
  ui->lineEditCidade->setText(cc.getCidade());
  ui->lineEditLogradouro->setText(cc.getEndereco());
  ui->lineEditBairro->setText(cc.getBairro());
}

void CadastroLoja::on_tableEndereco_clicked(const QModelIndex &index) {
  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  mapperEnd.setCurrentModelIndex(index);
}

bool CadastroLoja::viewRegister() {
  if (not RegisterDialog::viewRegister()) return false;

  modelEnd.setFilter("idLoja = " + primaryId + " AND desativado = FALSE");

  if (not modelEnd.select()) {
    emit errorSignal("Erro lendo tabela endereço da loja: " + modelEnd.lastError().text());
    return false;
  }

  ui->tableEndereco->resizeColumnsToContents();

  //

  modelConta.setFilter("idLoja = " + primaryId + " AND desativado = FALSE");

  if (not modelConta.select()) {
    emit errorSignal("Erro lendo tabela conta: " + modelConta.lastError().text());
    return false;
  }

  ui->tableConta->resizeColumnsToContents();

  //  modelPagamentos.setFilter("idLoja = " + primaryId);

  if (not modelPagamentos.select()) {
    emit errorSignal("Erro lendo tabela pagamento: " + modelPagamentos.lastError().text());
    return false;
  }

  modelTaxas.setFilter("0");

  if (not modelTaxas.select()) {
    emit errorSignal("Erro lendo tabela taxas: " + modelTaxas.lastError().text());
    return false;
  }

  ui->tabWidget->setTabEnabled(1, true);
  ui->tabWidget->setTabEnabled(2, true);
  ui->tabWidget->setTabEnabled(3, true);

  //

  modelAssocia2.setFilter("idLoja = " + data("idLoja").toString());

  if (not modelAssocia2.select()) emit errorSignal("Erro lendo tabela view_pagamento_loja: " + modelAssocia2.lastError().text());

  //

  return true;
}

void CadastroLoja::successMessage() { emit informationSignal(tipo == Tipo::Atualizar ? "Cadastro atualizado!" : "Loja cadastrada com sucesso!"); }

void CadastroLoja::on_tableEndereco_entered(const QModelIndex &) { ui->tableEndereco->resizeColumnsToContents(); }

bool CadastroLoja::cadastrar() {
  currentRow = tipo == Tipo::Atualizar ? mapper.currentIndex() : model.rowCount();

  if (currentRow == -1) {
    emit errorSignal("Erro linha -1");
    return false;
  }

  if (tipo == Tipo::Cadastrar and not model.insertRow(currentRow)) {
    emit errorSignal("Erro inserindo linha na tabela: " + model.lastError().text());
    return false;
  }

  if (not savingProcedures()) return false;

  for (int column = 0; column < model.rowCount(); ++column) {
    const QVariant dado = model.data(currentRow, column);
    if (dado.type() == QVariant::String) {
      if (not model.setData(currentRow, column, dado.toString().toUpper())) return false;
    }
  }

  if (not model.submitAll()) {
    emit errorSignal("Erro: " + model.lastError().text());
    return false;
  }

  primaryId = data(currentRow, primaryKey).isValid() ? data(currentRow, primaryKey).toString() : getLastInsertId().toString();

  if (primaryId.isEmpty()) {
    emit errorSignal("Id vazio!");
    return false;
  }

  //

  for (int row = 0, rowCount = modelEnd.rowCount(); row < rowCount; ++row) {
    if (not modelEnd.setData(row, primaryKey, primaryId)) return false;
  }

  for (int column = 0; column < modelEnd.rowCount(); ++column) {
    const QVariant dado = modelEnd.data(currentRow, column);
    if (dado.type() == QVariant::String) {
      if (not modelEnd.setData(currentRow, column, dado.toString().toUpper())) return false;
    }
  }

  if (not modelEnd.submitAll()) {
    emit errorSignal("Erro: " + modelEnd.lastError().text());
    return false;
  }

  //

  for (int row = 0; row < modelConta.rowCount(); ++row) {
    if (not modelConta.setData(row, primaryKey, primaryId)) return false;
  }

  if (not modelConta.submitAll()) {
    emit errorSignal("Erro: " + modelConta.lastError().text());
    return false;
  }

  //

  return true;
}

void CadastroLoja::on_tableConta_clicked(const QModelIndex &index) {
  ui->pushButtonAtualizarConta->show();
  ui->pushButtonAdicionarConta->hide();
  mapperConta.setCurrentModelIndex(index);
}

bool CadastroLoja::newRegister() {
  if (not RegisterAddressDialog::newRegister()) return false;

  modelConta.setFilter("0");

  if (not modelConta.select()) {
    emit errorSignal("Erro lendo tabela conta: " + modelConta.lastError().text());
    return false;
  }

  return true;
}

bool CadastroLoja::cadastrarConta(const bool isUpdate) {
  Q_FOREACH (const auto &line, ui->groupBoxConta->findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) return false;
  }

  const int rowConta = isUpdate ? mapperConta.currentIndex() : modelConta.rowCount();

  if (not isUpdate) modelConta.insertRow(rowConta);

  if (not modelConta.setData(rowConta, "banco", ui->lineEditBanco->text())) return false;
  if (not modelConta.setData(rowConta, "agencia", ui->lineEditAgencia->text())) return false;
  if (not modelConta.setData(rowConta, "conta", ui->lineEditConta->text())) return false;

  ui->tableConta->resizeColumnsToContents();

  isDirty = true;

  return true;
}

void CadastroLoja::novaConta() {
  ui->pushButtonAtualizarConta->hide();
  ui->pushButtonAdicionarConta->show();
  ui->tableConta->clearSelection();
  clearConta();
}

void CadastroLoja::clearConta() {
  ui->lineEditBanco->clear();
  ui->lineEditAgencia->clear();
  ui->lineEditConta->clear();
}

void CadastroLoja::on_pushButtonAdicionarConta_clicked() {
  if (not cadastrarConta()) {
    emit errorSignal("Não foi possível cadastrar esta conta.");
    return;
  }

  novaConta();
}

void CadastroLoja::on_pushButtonAtualizarConta_clicked() {
  if (not cadastrarConta(true)) {
    emit errorSignal("Não foi possível cadastrar esta conta.");
    return;
  }

  novaConta();
}

void CadastroLoja::on_pushButtonContaLimpar_clicked() { novaConta(); }

void CadastroLoja::on_pushButtonRemoverConta_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not modelConta.setData(mapperConta.currentIndex(), "desativado", true)) {
      emit errorSignal("Erro marcando desativado!");
      return;
    }

    if (not modelConta.submitAll()) {
      emit errorSignal("Não foi possível remover este item: " + modelConta.lastError().text());
      return;
    }

    novaConta();
  }
}

void CadastroLoja::on_checkBoxMostrarInativosConta_clicked(bool checked) {
  modelConta.setFilter("idLoja = " + data("idLoja").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelConta.select()) emit errorSignal("Erro lendo tabela contas: " + modelConta.lastError().text());

  ui->tableEndereco->resizeColumnsToContents();
}

bool CadastroLoja::adicionarPagamento() {
  const int row = modelPagamentos.rowCount();
  if (not modelPagamentos.insertRow(row)) {
    emit errorSignal("Erro inserindo linha na tabela: " + model.lastError().text());
    return false;
  }

  if (not modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text())) return false;
  if (not modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value())) return false;

  if (not modelPagamentos.submitAll()) {
    emit errorSignal("Erro salvando dados pagamentos: " + modelPagamentos.lastError().text());
    return false;
  }

  const int id = getLastInsertId().toInt();

  for (int i = 0; i < ui->spinBoxParcelas->value(); ++i) {
    const int rowTaxas = modelTaxas.rowCount();
    if (not modelTaxas.insertRow(rowTaxas)) {
      emit errorSignal("Erro inserindo linha na tabela: " + modelTaxas.lastError().text());
      return false;
    }

    if (not modelTaxas.setData(rowTaxas, "idPagamento", id)) return false;
    if (not modelTaxas.setData(rowTaxas, "parcela", i + 1)) return false;
    if (not modelTaxas.setData(rowTaxas, "taxa", 0)) return false;
  }

  if (not modelTaxas.submitAll()) {
    emit errorSignal("Erro salvando dados taxas: " + modelTaxas.lastError().text());
    return false;
  }

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  if (not modelTaxas.select()) {
    emit errorSignal("Erro lendo tabela taxas: " + modelTaxas.lastError().text());
    return false;
  }

  return true;
}

void CadastroLoja::on_pushButtonAdicionarPagamento_clicked() {
  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not adicionarPagamento()) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();
}

void CadastroLoja::on_tablePagamentos_clicked(const QModelIndex &index) {
  const int id = modelPagamentos.data(index.row(), "idPagamento").toInt();

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  if (not modelTaxas.select()) emit errorSignal("Erro lendo tabela taxas: " + modelTaxas.lastError().text());

  mapperPagamento.setCurrentModelIndex(index);

  ui->pushButtonAdicionarPagamento->hide();
  ui->pushButtonAtualizarPagamento->show();
  ui->pushButtonLimparSelecao->show();
}

void CadastroLoja::on_pushButtonRemoverPagamento_clicked() {
  const auto list = ui->tablePagamentos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  const int id = modelPagamentos.data(list.first().row(), "idPagamento").toInt();

  QSqlQuery query;
  query.prepare("DELETE FROM forma_pagamento WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", id);

  if (not query.exec()) {
    emit errorSignal("Erro apagando pagamentos: " + query.lastError().text());
    return;
  }

  if (not modelPagamentos.select()) emit errorSignal("Erro lendo tabela pagamentos: " + modelPagamentos.lastError().text());

  modelTaxas.setFilter("0");

  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->clear();

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();
}

bool CadastroLoja::atualizarPagamento() {
  const int row = mapperPagamento.currentIndex();

  if (not modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text())) return false;
  if (not modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value())) return false;

  if (not modelTaxas.removeRows(0, modelTaxas.rowCount())) {
    emit errorSignal("Erro removendo linha: " + modelTaxas.lastError().text());
    return false;
  }

  for (int i = 0; i < ui->spinBoxParcelas->value(); ++i) {
    const int rowTaxas = modelTaxas.rowCount();
    if (not modelTaxas.insertRow(rowTaxas)) {
      emit errorSignal("Erro inserindo linha na tabela: " + modelTaxas.lastError().text());
      return false;
    }

    if (not modelTaxas.setData(rowTaxas, "idPagamento", modelPagamentos.data(rowTaxas, "idPagamento"))) return false;
    if (not modelTaxas.setData(rowTaxas, "parcela", i + 1)) return false;
    if (not modelTaxas.setData(rowTaxas, "taxa", 0)) return false;
  }

  if (not modelPagamentos.submitAll()) {
    emit errorSignal("Erro atualizando dados: " + modelPagamentos.lastError().text());
    return false;
  }

  if (not modelTaxas.submitAll()) {
    emit errorSignal("Erro atualizando taxas: " + modelTaxas.lastError().text());
    return false;
  }

  modelTaxas.setFilter("0");

  if (not modelTaxas.select()) {
    emit errorSignal("Erro lendo tabela taxas: " + modelTaxas.lastError().text());
    return false;
  }

  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->clear();

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();

  return true;
}

void CadastroLoja::on_pushButtonAtualizarPagamento_clicked() {
  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not atualizarPagamento()) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();
}

void CadastroLoja::on_pushButtonAtualizarTaxas_clicked() {
  if (not modelTaxas.submitAll()) emit errorSignal("Erro atualizando taxas: " + modelTaxas.lastError().text());

  emit informationSignal("Taxas atualizadas!");
}

void CadastroLoja::on_pushButtonAdicionaAssociacao_clicked() {
  const auto list = ui->tableAssocia1->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QSqlQuery query;
  query.prepare("INSERT INTO forma_pagamento_has_loja (idPagamento, idLoja) VALUES (:idPagamento, :idLoja)");

  for (const auto &index : list) {
    query.bindValue(":idPagamento", modelAssocia1.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", data("idLoja").toInt());

    if (not query.exec()) emit errorSignal("Erro cadastrando associacao: " + query.lastError().text());
  }

  if (not modelAssocia2.select()) emit errorSignal("Erro lendo tabela view_pagamento_loja: " + modelAssocia2.lastError().text());
}

void CadastroLoja::on_pushButtonRemoveAssociacao_clicked() {
  const auto list = ui->tableAssocia2->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QSqlQuery query;
  query.prepare("DELETE FROM forma_pagamento_has_loja WHERE idPagamento = :idPagamento AND idLoja = :idLoja");

  for (const auto &index : list) {
    query.bindValue(":idPagamento", modelAssocia2.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", modelAssocia2.data(index.row(), "idLoja"));

    if (not query.exec()) emit errorSignal("Erro removendo associacao: " + query.lastError().text());
  }

  if (not modelAssocia2.select()) emit errorSignal("Erro lendo tabela view_pagamento_loja: " + modelAssocia2.lastError().text());
}

void CadastroLoja::on_pushButtonLimparSelecao_clicked() {
  modelTaxas.setFilter("0");

  if (not modelTaxas.select()) {
    emit errorSignal("Erro lendo tabela taxas: " + modelTaxas.lastError().text());
    return;
  }

  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->clear();

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();
  ui->pushButtonLimparSelecao->hide();

  ui->tablePagamentos->clearSelection();
}

// FIXME: nao permitir associar um pagamento já associado
