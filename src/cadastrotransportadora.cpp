#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "cadastrotransportadora.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "searchdialog.h"
#include "ui_cadastrotransportadora.h"
#include "usersession.h"

CadastroTransportadora::CadastroTransportadora(QWidget *parent) : RegisterAddressDialog("transportadora", "idTransportadora", parent), ui(new Ui::CadastroTransportadora) {
  ui->setupUi(this);

  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroTransportadora::on_checkBoxMostrarInativos_clicked);
  connect(ui->checkBoxMostrarInativosVeiculo, &QCheckBox::toggled, this, &CadastroTransportadora::on_checkBoxMostrarInativosVeiculo_toggled);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroTransportadora::on_lineEditCEP_textChanged);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroTransportadora::on_lineEditCNPJ_textEdited);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAdicionarEnd_clicked);
  connect(ui->pushButtonAdicionarVeiculo, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAdicionarVeiculo_clicked);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAtualizarEnd_clicked);
  connect(ui->pushButtonAtualizarVeiculo, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAtualizarVeiculo_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonRemoverEnd_clicked);
  connect(ui->pushButtonRemoverVeiculo, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonRemoverVeiculo_clicked);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroTransportadora::on_tableEndereco_clicked);
  connect(ui->tableVeiculo, &TableView::clicked, this, &CadastroTransportadora::on_tableVeiculo_clicked);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  setupUi();
  setupTables();
  setupMapper();
  newRegister();

  sdTransportadora = SearchDialog::transportadora(this);
  connect(sdTransportadora, &SearchDialog::itemSelected, this, &CadastroTransportadora::viewRegisterById);

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") {
    ui->pushButtonRemover->setDisabled(true);
    ui->pushButtonRemoverEnd->setDisabled(true);
  }
}

CadastroTransportadora::~CadastroTransportadora() { delete ui; }

void CadastroTransportadora::setupTables() {
  modelVeiculo.setTable("transportadora_has_veiculo");

  modelVeiculo.setHeaderData("modelo", "Modelo");
  modelVeiculo.setHeaderData("capacidade", "Carga Kg");
  modelVeiculo.setHeaderData("placa", "Placa");
  modelVeiculo.setHeaderData("ufPlaca", "UF Placa");
  modelVeiculo.setHeaderData("desativado", "Desativado");

  ui->tableVeiculo->setModel(&modelVeiculo);

  ui->tableVeiculo->hideColumn("idVeiculo");
  ui->tableVeiculo->hideColumn("idTransportadora");

  ui->tableVeiculo->setItemDelegateForColumn("desativado", new CheckBoxDelegate(this, true));

  ui->tableVeiculo->setPersistentColumns({"desativado"});

  // -------------------------------------------------------------------------

  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idTransportadora");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(this, true));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroTransportadora::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  novoVeiculo();
  setupUi();
}

bool CadastroTransportadora::verifyFields() {
  const auto children = ui->groupBox_7->findChildren<QLineEdit *>();

  for (const auto &line : children) {
    if (not verifyRequiredField(line)) { return false; }
  }

  return true;
}

bool CadastroTransportadora::savingProcedures() {
  if (not setData("cnpj", ui->lineEditCNPJ->text())) { return false; }
  if (not setData("razaoSocial", ui->lineEditRazaoSocial->text())) { return false; }
  if (not setData("nomeFantasia", ui->lineEditNomeFantasia->text())) { return false; }
  if (not setData("inscEstadual", ui->lineEditInscEstadual->text())) { return false; }
  if (not setData("tel", ui->lineEditTel->text())) { return false; }
  if (not setData("antt", ui->lineEditANTT->text())) { return false; }

  return true;
}

void CadastroTransportadora::setupMapper() {
  addMapping(ui->lineEditANTT, "antt");
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditInscEstadual, "inscEstadual");
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditRazaoSocial, "razaoSocial");
  addMapping(ui->lineEditTel, "tel");

  mapperVeiculo.setModel(&modelVeiculo);
  mapperVeiculo.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
  mapperVeiculo.addMapping(ui->lineEditModelo, modelVeiculo.fieldIndex("modelo"));
  mapperVeiculo.addMapping(ui->lineEditCarga, modelVeiculo.fieldIndex("capacidade"));
  mapperVeiculo.addMapping(ui->lineEditPlaca, modelVeiculo.fieldIndex("placa"));

  mapperEnd.addMapping(ui->comboBoxTipoEnd, modelEnd.fieldIndex("descricao"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditComp, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNro, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroTransportadora::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

  ui->pushButtonRemoverVeiculo->hide();

  ui->pushButtonRemoverEnd->hide();
}

void CadastroTransportadora::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

void CadastroTransportadora::on_pushButtonCadastrar_clicked() { save(); }

void CadastroTransportadora::on_pushButtonAtualizar_clicked() { save(); }

void CadastroTransportadora::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroTransportadora::on_pushButtonRemover_clicked() { remove(); }

void CadastroTransportadora::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdTransportadora->show();
}

void CadastroTransportadora::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "background-color: rgb(255, 255, 127);color: rgb(0, 190, 0)"
                                                                                                : "background-color: rgb(255, 255, 127);color: rgb(255, 0, 0)");
}

void CadastroTransportadora::on_pushButtonAdicionarEnd_clicked() { cadastrarEndereco() ? novoEndereco() : qApp->enqueueError("Não foi possível cadastrar este endereço!", this); }

void CadastroTransportadora::on_pushButtonAtualizarEnd_clicked() { cadastrarEndereco(Tipo::Atualizar) ? novoEndereco() : qApp->enqueueError("Não foi possível cadastrar este endereço!", this); }

void CadastroTransportadora::on_pushButtonRemoverEnd_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not setDataEnd("desativado", true)) { return; }

    if (not modelEnd.submitAll()) { return; }

    novoEndereco();
  }
}

void CadastroTransportadora::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idTransportadora = " + data("idTransportadora").toString() + (checked ? "" : " AND desativado = FALSE"));
}

bool CadastroTransportadora::cadastrarEndereco(const Tipo tipo) { //TODO: V688 http://www.viva64.com/en/V688 The 'tipo' function argument possesses the same name as one of the class members, which can result in a confusion.bool CadastroTransportadora::cadastrarEndereco(const Tipo tipo) {
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
  if (not setDataEnd("CEP", ui->lineEditCEP->text())) { return false; }
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

void CadastroTransportadora::novoEndereco() {
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonRemoverEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroTransportadora::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComp->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNro->clear();
  ui->lineEditUF->clear();
}

void CadastroTransportadora::on_lineEditCEP_textChanged(const QString &cep) {
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

void CadastroTransportadora::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();
  mapperEnd.setCurrentModelIndex(index);
  currentRowEnd = index.row();
}

void CadastroTransportadora::setupUi() {
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditANTT->setInputMask("99999999;_");
  ui->lineEditPlaca->setInputMask("AAA-9999;_");
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
  ui->lineEditUfPlaca->setInputMask(">AA;_");

  ui->lineEditCarga->setValidator(new QIntValidator(this));
}

bool CadastroTransportadora::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idTransportadora = " + data("idTransportadora").toString() + (inativos ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return false; }

  //---------------------------------------------------

  const bool inativosVeiculo = ui->checkBoxMostrarInativosVeiculo->isChecked();
  modelVeiculo.setFilter("idTransportadora = " + data("idTransportadora").toString() + (inativosVeiculo ? "" : " AND desativado = FALSE"));

  if (not modelVeiculo.select()) { return false; }

  return true;
}

void CadastroTransportadora::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Transportadora cadastrada com sucesso!", this); }

bool CadastroTransportadora::cadastrarVeiculo(const Tipo tipo) { //TODO: V688 http://www.viva64.com/en/V688 The 'tipo' function argument possesses the same name as one of the class members, which can result in a confusion.bool CadastroTransportadora::cadastrarVeiculo(const Tipo tipo) {
  if (tipo == Tipo::Cadastrar) {
    currentRowVeiculo = modelVeiculo.rowCount();
    modelVeiculo.insertRow(currentRowVeiculo);
  }

  if (not modelVeiculo.setData(currentRowVeiculo, "modelo", ui->lineEditModelo->text())) { return false; }
  if (not modelVeiculo.setData(currentRowVeiculo, "capacidade", ui->lineEditCarga->text().toInt())) { return false; }

  if (ui->lineEditPlaca->text() != "-") {
    if (not modelVeiculo.setData(currentRowVeiculo, "placa", ui->lineEditPlaca->text())) { return false; }
  }

  if (not modelVeiculo.setData(currentRowVeiculo, "ufPlaca", ui->lineEditUfPlaca->text())) { return false; }

  if (not columnsToUpper(modelVeiculo, currentRowVeiculo)) { return false; }

  isDirty = true;

  return true;
}

void CadastroTransportadora::on_pushButtonAdicionarVeiculo_clicked() { cadastrarVeiculo() ? novoVeiculo() : qApp->enqueueError("Não foi possível atualizar este veículo!", this); }

void CadastroTransportadora::on_pushButtonAtualizarVeiculo_clicked() { cadastrarVeiculo(Tipo::Atualizar) ? novoVeiculo() : qApp->enqueueError("Não foi possível atualizar este veículo!", this); }

void CadastroTransportadora::novoVeiculo() {
  ui->pushButtonAtualizarVeiculo->hide();
  ui->pushButtonAdicionarVeiculo->show();
  ui->pushButtonRemoverVeiculo->hide();
  ui->tableVeiculo->clearSelection();
  clearVeiculo();
}

void CadastroTransportadora::clearVeiculo() {
  ui->lineEditModelo->clear();
  ui->lineEditCarga->clear();
  ui->lineEditPlaca->clear();
  ui->lineEditUfPlaca->clear();
}

void CadastroTransportadora::on_pushButtonRemoverVeiculo_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not modelVeiculo.setData(currentRowVeiculo, "desativado", true)) { return; }

    if (not modelVeiculo.submitAll()) { return; }

    novoVeiculo();
  }
}

bool CadastroTransportadora::cadastrar() {
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

  if (not setForeignKey(modelVeiculo)) { return false; }

  return modelVeiculo.submitAll();
}

void CadastroTransportadora::on_checkBoxMostrarInativosVeiculo_toggled(bool checked) {
  if (currentRow == -1) { return; }

  modelVeiculo.setFilter("idTransportadora = " + data("idTransportadora").toString() + (checked ? "" : " AND desativado = FALSE"));
}

void CadastroTransportadora::on_tableVeiculo_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoVeiculo(); }

  ui->pushButtonAtualizarVeiculo->show();
  ui->pushButtonAdicionarVeiculo->hide();
  ui->pushButtonRemoverVeiculo->show();
  mapperVeiculo.setCurrentModelIndex(index);
  currentRowVeiculo = index.row();
}

bool CadastroTransportadora::newRegister() {
  if (not RegisterAddressDialog::newRegister()) { return false; }

  modelVeiculo.setFilter("0");

  return true;
}
