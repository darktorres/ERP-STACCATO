#include "cadastrocliente.h"
#include "ui_cadastrocliente.h"

#include "application.h"
#include "cadastroprofissional.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "user.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

CadastroCliente::CadastroCliente(QWidget *parent) : RegisterAddressDialog("cliente", "idCliente", parent), ui(new Ui::CadastroCliente) {
  ui->setupUi(this);

  connectLineEditsToDirty();
  setItemBoxes();
  setupTables();
  setupMapper();
  newRegister();

  ui->lineEditCliente->setFocus();

  on_radioButtonPF_toggled(true);

  setConnections();
}

CadastroCliente::~CadastroCliente() { delete ui; }

void CadastroCliente::setItemBoxes() {
  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(true, this));
  ui->itemBoxProfissional->setRegisterDialog(new CadastroProfissional(this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));
}

void CadastroCliente::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(sdCliente, &SearchDialog::itemSelected, this, &CadastroCliente::viewRegisterById, connectionType);
  connect(ui->checkBoxDataNasc, &QCheckBox::stateChanged, this, &CadastroCliente::on_checkBoxDataNasc_stateChanged, connectionType);
  connect(ui->checkBoxInscEstIsento, &QCheckBox::toggled, this, &CadastroCliente::on_checkBoxInscEstIsento_toggled, connectionType);
  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroCliente::on_checkBoxMostrarInativos_clicked, connectionType);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged, connectionType);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditCNPJ_textEdited, connectionType);
  connect(ui->lineEditCPF, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditCPF_textEdited, connectionType);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditContatoCPF_textEdited, connectionType);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonAdicionarEnd_clicked, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonAtualizar_clicked, connectionType);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonAtualizarEnd_clicked, connectionType);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonNovoCad_clicked, connectionType);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonRemover_clicked, connectionType);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonRemoverEnd_clicked, connectionType);
  connect(ui->radioButtonPF, &QRadioButton::toggled, this, &CadastroCliente::on_radioButtonPF_toggled, connectionType);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroCliente::on_tableEndereco_clicked, connectionType);
}

void CadastroCliente::setupUi() {
  // dados
  ui->lineEditCPF->setInputMask("999.999.999-99;_");
  ui->lineEditContatoCPF->setInputMask("999.999.999-99;_");
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditInscEstadual->setValidator(new QRegExpValidator(QRegExp(R"([0-9]\d{0,15})"), this));

  // endereco
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
}

void CadastroCliente::setupTables() {
  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idCliente");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroCliente::verifyFields() {
  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { verifyRequiredField(*line); }

  if (ui->radioButtonPF->isChecked()) { validaCPF(ui->lineEditCPF->text()); }

  if (ui->radioButtonPJ->isChecked()) { validaCNPJ(ui->lineEditCNPJ->text()); }

  if (not ui->lineEditContatoCPF->text().isEmpty()) { validaCPF(ui->lineEditContatoCPF->text()); }

  if (tipo == Tipo::Cadastrar) {
    SqlQuery query;
    query.prepare("SELECT cpf, cnpj FROM cliente WHERE cpf = :cpf OR cnpj = :cnpj");
    query.bindValue(":cpf", ui->lineEditCPF->text());
    query.bindValue(":cnpj", ui->lineEditCNPJ->text());

    if (not query.exec()) { throw RuntimeException("Erro verificando se CPF/CNPJ já cadastrado!"); }

    if (query.first()) { throw RuntimeError("CPF/CNPJ já cadastrado!"); }
  }
}

void CadastroCliente::savingProcedures() {
  if (ui->checkBoxDataNasc->isChecked()) { setData("dataNasc", ui->dateEditDataNasc->date()); }

  if (tipoPFPJ == "PF") { setData("cnpj", ""); }
  if (tipoPFPJ == "PJ") { setData("cpf", ""); }

  setData("contatoCPF", (ui->lineEditContatoCPF->text() == "..-") ? "" : ui->lineEditContatoCPF->text());
  setData("cpf", (ui->lineEditCPF->text() == "..-") ? "" : ui->lineEditCPF->text());
  setData("cnpj", (ui->lineEditCNPJ->text() == "../-") ? "" : ui->lineEditCNPJ->text());
  setData("nome_razao", ui->lineEditCliente->text().remove("\r").remove("\n"));
  setData("nomeFantasia", ui->lineEditNomeFantasia->text());
  setData("contatoNome", ui->lineEditContatoNome->text());
  setData("contatoApelido", ui->lineEditContatoApelido->text());
  setData("contatoRG", ui->lineEditContatoRG->text());
  setData("inscEstadual", ui->lineEditInscEstadual->text());
  setData("tel", ui->lineEditTel_Res->text());
  setData("telCel", ui->lineEditTel_Cel->text());
  setData("telCom", ui->lineEditTel_Com->text());
  setData("nextel", ui->lineEditNextel->text());
  setData("email", ui->lineEditEmail->text());
  setData("idCadastroRel", ui->itemBoxCliente->getId());
  setData("idProfissionalRel", ui->itemBoxProfissional->getId());
  setData("idUsuarioRel", ui->itemBoxVendedor->getId());
  setData("pfpj", tipoPFPJ);
  setData("credito", ui->doubleSpinBoxCredito->value());

  const bool incompleto = (modelEnd.rowCount() == 0 or ui->lineEditTel_Res->text().isEmpty() or ui->lineEditEmail->text().isEmpty());

  setData("incompleto", incompleto);
}

void CadastroCliente::clearFields() {
  RegisterDialog::clearFields();

  ui->radioButtonPF->setChecked(true);
  novoEndereco();
  setupUi();
}

void CadastroCliente::setupMapper() {
  addMapping(ui->dateEditDataNasc, "dataNasc");
  addMapping(ui->doubleSpinBoxCredito, "credito");
  addMapping(ui->itemBoxCliente, "idCadastroRel", "id");
  addMapping(ui->itemBoxProfissional, "idProfissionalRel", "id");
  addMapping(ui->itemBoxVendedor, "idUsuarioRel", "id");
  addMapping(ui->lineEditCliente, "nome_razao");
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoCPF, "contatoCPF");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditCPF, "cpf");
  addMapping(ui->lineEditEmail, "email");
  addMapping(ui->lineEditIdNextel, "idNextel");
  addMapping(ui->lineEditInscEstadual, "inscEstadual");
  addMapping(ui->lineEditNextel, "nextel");
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditTel_Cel, "telCel");
  addMapping(ui->lineEditTel_Com, "telCom");
  addMapping(ui->lineEditTel_Res, "tel");

  mapperEnd.addMapping(ui->comboBoxTipoEnd, modelEnd.fieldIndex("descricao"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNumero, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditComplemento, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroCliente::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

  ui->pushButtonRemoverEnd->hide();

  ui->lineEditCliente->setReadOnly(false);
  ui->lineEditCPF->setReadOnly(false);
  ui->lineEditCNPJ->setReadOnly(false);

  ui->pushButtonAtualizarEnd->setDisabled(false);
  ui->groupBoxPFPJ->setDisabled(false);
}

void CadastroCliente::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

bool CadastroCliente::verificaVinculo() {
  SqlQuery query;

  if (not query.exec("SELECT 0 FROM venda WHERE idCliente = " + data("idCliente").toString())) { throw RuntimeException("Erro verificando se existe pedidos vinculados: " + query.lastError().text()); }

  return query.size() > 0;
}

bool CadastroCliente::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idCliente = " + data("idCliente").toString() + (inativos ? "" : " AND desativado = FALSE"));

  modelEnd.select();

  //---------------------------------------------------

  ui->itemBoxCliente->setFilter("idCliente NOT IN (" + data("idCliente").toString() + ")");

  tipoPFPJ = data("pfpj").toString();

  (tipoPFPJ == "PF") ? ui->radioButtonPF->setChecked(true) : ui->radioButtonPJ->setChecked(true);

  ui->checkBoxInscEstIsento->setChecked(data("inscEstadual").toString() == "ISENTO");

  if (not data("dataNasc").isNull()) {
    ui->checkBoxDataNasc->setChecked(true);
    ui->dateEditDataNasc->setEnabled(true);
  }

  const bool bloquear = verificaVinculo();

  ui->lineEditCliente->setReadOnly(bloquear);
  ui->lineEditCPF->setReadOnly(bloquear);
  ui->lineEditCNPJ->setReadOnly(bloquear);

  ui->pushButtonAtualizarEnd->setDisabled(bloquear);
  ui->groupBoxPFPJ->setDisabled(bloquear);

  //---------------------------------------------------

  if (User::usuario == "zelante") {
    ui->doubleSpinBoxCredito->setReadOnly(false);
    ui->doubleSpinBoxCredito->setButtonSymbols(QDoubleSpinBox::UpDownArrows);
  }

  return true;
}

void CadastroCliente::on_pushButtonCadastrar_clicked() { save(); }

void CadastroCliente::on_pushButtonAtualizar_clicked() { save(); }

void CadastroCliente::on_pushButtonRemover_clicked() { remove(); }

void CadastroCliente::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroCliente::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdCliente->show();
}

void CadastroCliente::on_lineEditCPF_textEdited(const QString &text) {
  const bool valido = validaCPF(text);

  ui->lineEditCPF->setStyleSheet(valido ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");

  if (valido) {
    SqlQuery query;
    query.prepare("SELECT idCliente FROM cliente WHERE cpf = :cpf");
    query.bindValue(":cpf", text);

    if (not query.exec()) { throw RuntimeException("Erro buscando CPF: " + query.lastError().text(), this); }

    if (query.first()) {
      viewRegisterById(query.value("idCliente"));
      throw RuntimeError("CPF já cadastrado!", this);
    }
  }
}

void CadastroCliente::on_lineEditCNPJ_textEdited(const QString &text) {
  const bool valido = validaCNPJ(text);

  ui->lineEditCNPJ->setStyleSheet(valido ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");

  if (valido) {
    SqlQuery query;
    query.prepare("SELECT idCliente FROM cliente WHERE cnpj = :cnpj");
    query.bindValue(":cnpj", text);

    if (not query.exec()) { throw RuntimeException("Erro buscando CNPJ: " + query.lastError().text(), this); }

    if (query.first()) {
      viewRegisterById(query.value("idCliente"));
      throw RuntimeError("CNPJ já cadastrado!", this);
    }
  }
}

bool CadastroCliente::cadastrarEndereco(const Tipo tipoEndereco) {
  verificaEndereco();

  if (tipoEndereco == Tipo::Cadastrar) { currentRowEnd = modelEnd.insertRowAtEnd(); }

  setDataEnd("descricao", ui->comboBoxTipoEnd->currentText());
  setDataEnd("cep", ui->lineEditCEP->text());
  setDataEnd("logradouro", ui->lineEditLogradouro->text());
  setDataEnd("numero", ui->lineEditNumero->text());
  setDataEnd("complemento", ui->lineEditComplemento->text());
  setDataEnd("bairro", ui->lineEditBairro->text());
  setDataEnd("cidade", ui->lineEditCidade->text());
  setDataEnd("uf", ui->lineEditUF->text());
  setDataEnd("codUF", getCodigoUF(ui->lineEditUF->text()));
  setDataEnd("desativado", false);

  if (tipoEndereco == Tipo::Cadastrar) { backupEndereco.append(modelEnd.record(currentRowEnd)); }

  isDirty = true;

  if (tipo == Tipo::Atualizar) { save(true); }

  return true;
}

void CadastroCliente::cadastrar() {
  try {
    qApp->startTransaction("CadastroCliente::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    // -------------------------------------------------------------------------

    setForeignKey(modelEnd);

    modelEnd.submitAll();

    qApp->endTransaction();

    backupEndereco.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelEnd.setFilter(primaryKey + " = '" + primaryId + "'");
  } catch (std::exception &) {
    qApp->rollbackTransaction();
    model.select();
    modelEnd.select();

    for (auto &record : backupEndereco) { modelEnd.insertRecord(-1, record); }

    throw;
  }
}

void CadastroCliente::on_pushButtonAdicionarEnd_clicked() {
  if (cadastrarEndereco()) { novoEndereco(); }
}

void CadastroCliente::on_pushButtonAtualizarEnd_clicked() {
  if (cadastrarEndereco(Tipo::Atualizar)) { novoEndereco(); }
}

void CadastroCliente::on_lineEditCEP_textChanged(const QString &cep) {
  if (not ui->lineEditCEP->isValid()) { return; }

  ui->lineEditNumero->clear();
  ui->lineEditComplemento->clear();

  CepCompleter cc;
  cc.buscaCEP(cep, this);

  ui->lineEditLogradouro->setText(cc.getEndereco());
  ui->lineEditComplemento->setText(cc.getComplemento());
  ui->lineEditBairro->setText(cc.getBairro());
  ui->lineEditCidade->setText(cc.getCidade());
  ui->lineEditUF->setText(cc.getUf());
}

void CadastroCliente::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComplemento->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNumero->clear();
  ui->lineEditUF->clear();
}

void CadastroCliente::novoEndereco() {
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonRemoverEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
  setEnderecoReadOnly(false);
}

void CadastroCliente::setEnderecoReadOnly(const bool readOnly) {
  ui->lineEditCEP->setReadOnly(readOnly);
  ui->lineEditLogradouro->setReadOnly(readOnly);
  ui->lineEditNumero->setReadOnly(readOnly);
  ui->lineEditComplemento->setReadOnly(readOnly);
  ui->lineEditBairro->setReadOnly(readOnly);
  ui->lineEditCidade->setReadOnly(readOnly);
  ui->lineEditUF->setReadOnly(readOnly);
}

void CadastroCliente::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  currentRowEnd = index.row();

  const bool desativado = dataEnd("desativado").toBool();

  ui->pushButtonAtualizarEnd->setEnabled(desativado);
  ui->pushButtonRemoverEnd->setDisabled(desativado);

  setEnderecoReadOnly(verificaVinculo());

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();

  //------------------------------------------
  disconnect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged);
  mapperEnd.setCurrentModelIndex(index);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged);
  //------------------------------------------
}

void CadastroCliente::on_radioButtonPF_toggled(const bool checked) {
  tipoPFPJ = (checked) ? "PF" : "PJ";

  if (checked) {
    ui->lineEditCNPJ->clear();

    ui->lineEditCNPJ->setHidden(true);
    ui->labelCNPJ->setHidden(true);
    ui->lineEditInscEstadual->setHidden(true);
    ui->labelInscricaoEstadual->setHidden(true);
    ui->checkBoxInscEstIsento->setHidden(true);

    ui->lineEditCPF->setVisible(true);
    ui->labelCPF->setVisible(true);
    ui->dateEditDataNasc->setVisible(true);
    ui->checkBoxDataNasc->setVisible(true);
  } else {
    ui->lineEditCPF->clear();

    ui->lineEditCPF->setVisible(false);
    ui->labelCPF->setVisible(false);
    ui->dateEditDataNasc->setVisible(false);
    ui->checkBoxDataNasc->setVisible(false);

    ui->lineEditCNPJ->setHidden(false);
    ui->labelCNPJ->setHidden(false);
    ui->lineEditInscEstadual->setHidden(false);
    ui->labelInscricaoEstadual->setHidden(false);
    ui->checkBoxInscEstIsento->setHidden(false);

    ui->checkBoxDataNasc->setChecked(false);
  }

  adjustSize();
}

void CadastroCliente::on_lineEditContatoCPF_textEdited(const QString &text) { ui->lineEditContatoCPF->setStyleSheet(validaCPF(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroCliente::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idCliente = " + data("idCliente").toString() + (checked ? "" : " AND desativado = FALSE"));

  modelEnd.select();
}

void CadastroCliente::on_pushButtonRemoverEnd_clicked() {
  if (removeBox() == QMessageBox::Yes) {
    setDataEnd("desativado", true);

    modelEnd.submitAll();

    novoEndereco();
  }
}

void CadastroCliente::successMessage() {
  qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Cliente cadastrado com sucesso!", this);
  emit registerUpdated(primaryId);
}

void CadastroCliente::on_checkBoxInscEstIsento_toggled(bool checked) {
  if (checked) {
    ui->lineEditInscEstadual->setValidator(nullptr);
    ui->lineEditInscEstadual->setText("ISENTO");
    ui->lineEditInscEstadual->setReadOnly(true);
  } else {
    ui->lineEditInscEstadual->setValidator(new QRegExpValidator(QRegExp(R"([0-9]\d{0,15})"), this));
    ui->lineEditInscEstadual->clear();
    ui->lineEditInscEstadual->setReadOnly(false);
  }
}

void CadastroCliente::on_checkBoxDataNasc_stateChanged(const int state) { ui->dateEditDataNasc->setEnabled(state); }

void CadastroCliente::verificaEndereco() {
  RegisterAddressDialog::verificaEndereco(ui->lineEditCidade->text(), ui->lineEditUF->text());

  if (not ui->lineEditCEP->isValid()) { throw RuntimeError("CEP inválido!", this); }

  if (ui->lineEditNumero->text().isEmpty()) { throw RuntimeError("Número vazio! Se necessário coloque \"S/N\"!", this); }

  if (ui->lineEditCidade->text().isEmpty()) { throw RuntimeError("Cidade vazio!", this); }

  if (ui->lineEditUF->text().isEmpty()) { throw RuntimeError("UF vazio!", this); }
}

// TODO: 0ao trocar de cadastro nao esta limpando observacao (esta fazendo append)
// TODO: limitar complemento de endereco a 60 caracteres
