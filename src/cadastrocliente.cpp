#include <QDebug>
#include <QMessageBox>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "cadastrocliente.h"
#include "cadastroprofissional.h"
#include "cepcompleter.h"
#include "ui_cadastrocliente.h"
#include "usersession.h"

CadastroCliente::CadastroCliente(QWidget *parent) : RegisterAddressDialog("cliente", "idCliente", parent), ui(new Ui::CadastroCliente) {
  ui->setupUi(this);

  Q_FOREACH (const auto &line, findChildren<QLineEdit *>()) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(this));
  ui->itemBoxProfissional->setRegisterDialog(new CadastroProfissional(this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));

  setupTables();
  setupMapper();
  newRegister();

  sdCliente = SearchDialog::cliente(this);
  connect(sdCliente, &SearchDialog::itemSelected, this, &CadastroCliente::viewRegisterById);

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") {
    ui->pushButtonRemover->setDisabled(true);
    ui->pushButtonRemoverEnd->setDisabled(true);
  }

  ui->lineEditCliente->setFocus();

  setConnections();

  on_radioButtonPF_toggled(true);
}

CadastroCliente::~CadastroCliente() { delete ui; }

void CadastroCliente::setConnections() {
  connect(ui->checkBoxInscEstIsento, &QCheckBox::toggled, this, &CadastroCliente::on_checkBoxInscEstIsento_toggled);
  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroCliente::on_checkBoxMostrarInativos_clicked);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditCNPJ_textEdited);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditContatoCPF_textEdited);
  connect(ui->lineEditCPF, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditCPF_textEdited);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonAdicionarEnd_clicked);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonAtualizarEnd_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonEndLimpar, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonEndLimpar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroCliente::on_pushButtonRemoverEnd_clicked);
  connect(ui->radioButtonPF, &QRadioButton::toggled, this, &CadastroCliente::on_radioButtonPF_toggled);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroCliente::on_tableEndereco_clicked);
}

void CadastroCliente::setupUi() {
  ui->lineEditCPF->setInputMask("999.999.999-99;_");
  ui->lineEditContatoCPF->setInputMask("999.999.999-99;_");
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditInscEstadual->setValidator(new QRegExpValidator(QRegExp(R"([0-9]\d{0,15})"), this));
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
}

void CadastroCliente::setupTables() {
  ui->tableEndereco->setModel(&modelEnd);
  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("desativado");
  ui->tableEndereco->hideColumn("idCliente");
  ui->tableEndereco->hideColumn("codUF");
}

bool CadastroCliente::verifyFields() {
  Q_FOREACH (const auto &line, ui->frame->findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) { return false; }
  }

  if (ui->radioButtonPF->isChecked() and ui->lineEditCPF->styleSheet().contains("color: rgb(255, 0, 0)")) { return qApp->enqueueError(false, "CPF inválido!", this); }

  if (ui->radioButtonPJ->isChecked() and ui->lineEditCNPJ->styleSheet().contains("color: rgb(255, 0, 0)")) { return qApp->enqueueError(false, "CNPJ inválido!", this); }

  if (tipo == Tipo::Cadastrar) {
    QSqlQuery query;
    query.prepare("SELECT cpf, cnpj FROM cliente WHERE cpf = :cpf OR cnpj = :cnpj");
    query.bindValue(":cpf", ui->lineEditCPF->text());
    query.bindValue(":cnpj", ui->lineEditCNPJ->text());

    if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se CPF/CNPJ já cadastrado!", this); }

    if (query.first()) { return qApp->enqueueError(false, "CPF/CNPJ já cadastrado!", this); }
  }

  return true;
}

bool CadastroCliente::savingProcedures() {
  if (not setData("nome_razao", ui->lineEditCliente->text())) { return false; }
  if (not setData("nomeFantasia", ui->lineEditNomeFantasia->text())) { return false; }
  if (not setData("cpf", ui->lineEditCPF->text())) { return false; }
  if (not setData("contatoNome", ui->lineEditContatoNome->text())) { return false; }
  if (not setData("contatoCPF", ui->lineEditContatoCPF->text())) { return false; }
  if (not setData("contatoApelido", ui->lineEditContatoApelido->text())) { return false; }
  if (not setData("contatoRG", ui->lineEditContatoRG->text())) { return false; }
  if (not setData("cnpj", ui->lineEditCNPJ->text())) { return false; }
  if (not setData("inscEstadual", ui->lineEditInscEstadual->text())) { return false; }
  if (not setData("dataNasc", ui->dateEdit->date())) { return false; }
  if (not setData("tel", ui->lineEditTel_Res->text())) { return false; }
  if (not setData("telCel", ui->lineEditTel_Cel->text())) { return false; }
  if (not setData("telCom", ui->lineEditTel_Com->text())) { return false; }
  if (not setData("nextel", ui->lineEditNextel->text())) { return false; }
  if (not setData("email", ui->lineEditEmail->text())) { return false; }
  if (not setData("idCadastroRel", ui->itemBoxCliente->getId())) { return false; }
  if (not setData("idProfissionalRel", ui->itemBoxProfissional->getId())) { return false; }
  if (not setData("idUsuarioRel", ui->itemBoxVendedor->getId())) { return false; }
  if (not setData("pfpj", tipoPFPJ)) { return false; }
  if (not setData("incompleto", modelEnd.rowCount() == 0)) { return false; }
  if (not setData("credito", ui->doubleSpinBoxCredito->value())) { return false; }

  return true;
}

void CadastroCliente::clearFields() {
  RegisterDialog::clearFields();

  ui->radioButtonPF->setChecked(true);
  ui->checkBoxInscEstIsento->setChecked(false);
  novoEndereco();

  Q_FOREACH (const auto &box, findChildren<ItemBox *>()) { box->clear(); }

  setupUi();
}

void CadastroCliente::setupMapper() {
  addMapping(ui->dateEdit, "dataNasc");
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
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditComp, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNro, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroCliente::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();
}

void CadastroCliente::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

bool CadastroCliente::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  if (data("idCliente").toString().isEmpty()) { return qApp->enqueueError(false, "idCliente vazio!", this); }

  modelEnd.setFilter("idCliente = " + data("idCliente").toString() + " AND desativado = FALSE");

  if (not modelEnd.select()) { return false; }

  ui->itemBoxCliente->setFilter("idCliente NOT IN (" + data("idCliente").toString() + ")");

  tipoPFPJ = data("pfpj").toString();

  tipoPFPJ == "PF" ? ui->radioButtonPF->setChecked(true) : ui->radioButtonPJ->setChecked(true);

  ui->checkBoxInscEstIsento->setChecked(data("inscEstadual").toString() == "ISENTO");

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
  ui->lineEditCPF->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");

  if (not ui->lineEditCPF->styleSheet().contains("color: rgb(255, 0, 0)")) {
    QSqlQuery query;
    query.prepare("SELECT idCliente FROM cliente WHERE cpf = :cpf");
    query.bindValue(":cpf", text);

    if (not query.exec()) { return qApp->enqueueError("Erro buscando CPF: " + query.lastError().text(), this); }

    if (query.first()) {
      qApp->enqueueError("CPF já cadastrado!", this);
      viewRegisterById(query.value("idCliente"));
    }
  }
}

void CadastroCliente::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");

  if (not ui->lineEditCNPJ->styleSheet().contains("color: rgb(255, 0, 0)")) {
    QSqlQuery query;
    query.prepare("SELECT idCliente FROM cliente WHERE cnpj = :cnpj");
    query.bindValue(":cnpj", text);

    if (not query.exec()) { return qApp->enqueueError("Erro buscando CNPJ: " + query.lastError().text(), this); }

    if (query.first()) {
      qApp->enqueueError("CNPJ já cadastrado!", this);
      viewRegisterById(query.value("idCliente"));
    }
  }
}

bool CadastroCliente::cadastrarEndereco(const Tipo tipo) {
  Q_FOREACH (const auto &line, ui->groupBoxEndereco->findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) { return false; }
  }

  if (not ui->lineEditCEP->isValid()) {
    ui->lineEditCEP->setFocus();
    return qApp->enqueueError(false, "CEP inválido!", this);
  }

  if (ui->lineEditNro->text().isEmpty()) {
    ui->lineEditNro->setFocus();
    return qApp->enqueueError(false, "Número vazio!", this);
  }

  currentRowEnd = (tipo == Tipo::Atualizar) ? mapperEnd.currentIndex() : modelEnd.rowCount();

  if (tipo == Tipo::Cadastrar) { modelEnd.insertRow(currentRowEnd); }

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

  isDirty = true;

  return true;
}

bool CadastroCliente::cadastrar() {
  currentRow = (tipo == Tipo::Atualizar) ? mapper.currentIndex() : model.rowCount();

  if (currentRow == -1) { return qApp->enqueueError(false, "Erro linha -1", this); }

  if (tipo == Tipo::Cadastrar and not model.insertRow(currentRow)) { return qApp->enqueueError(false, "Erro inserindo linha na tabela: " + model.lastError().text(), this); }

  if (not savingProcedures()) { return false; }

  for (int column = 0; column < model.rowCount(); ++column) {
    const QVariant dado = model.data(currentRow, column);
    if (dado.type() == QVariant::String) {
      if (not model.setData(currentRow, column, dado.toString().toUpper())) { return false; }
    }
  }

  if (not model.submitAll()) { return false; }

  primaryId = (tipo == Tipo::Atualizar) ? data(currentRow, primaryKey).toString() : model.query().lastInsertId().toString();

  if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

  for (int row = 0, rowCount = modelEnd.rowCount(); row < rowCount; ++row) {
    if (not modelEnd.setData(row, primaryKey, primaryId)) { return false; }
  }

  for (int column = 0; column < modelEnd.rowCount(); ++column) {
    const QVariant dado = modelEnd.data(currentRow, column);
    if (dado.type() == QVariant::String) {
      if (not modelEnd.setData(currentRow, column, dado.toString().toUpper())) { return false; }
    }
  }

  return modelEnd.submitAll();
}

void CadastroCliente::on_pushButtonAdicionarEnd_clicked() {
  if (not cadastrarEndereco()) { return qApp->enqueueError("Não foi possível cadastrar este endereço!", this); }

  novoEndereco();
}

void CadastroCliente::on_pushButtonAtualizarEnd_clicked() {
  if (not cadastrarEndereco(Tipo::Atualizar)) { return qApp->enqueueError("Não foi possível atualizar este endereço!", this); }

  novoEndereco();
}

void CadastroCliente::on_lineEditCEP_textChanged(const QString &cep) {
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

void CadastroCliente::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComp->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNro->clear();
  ui->lineEditUF->clear();
}

void CadastroCliente::novoEndereco() {
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonAtualizarEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroCliente::on_pushButtonEndLimpar_clicked() { novoEndereco(); }

void CadastroCliente::on_tableEndereco_clicked(const QModelIndex &index) {
  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  mapperEnd.setCurrentModelIndex(index);
}

void CadastroCliente::on_radioButtonPF_toggled(const bool checked) {
  tipoPFPJ = checked ? QString("PF") : QString("PJ");

  if (checked) {
    ui->lineEditCNPJ->clear();

    ui->lineEditCNPJ->setHidden(checked);
    ui->labelCNPJ->setHidden(checked);
    ui->lineEditInscEstadual->setHidden(checked);
    ui->labelInscricaoEstadual->setHidden(checked);
    ui->checkBoxInscEstIsento->setHidden(checked);

    ui->lineEditCPF->setVisible(checked);
    ui->labelCPF->setVisible(checked);
    ui->dateEdit->setVisible(checked);
    ui->labelDataNasc->setVisible(checked);
  } else {
    ui->lineEditCPF->clear();

    ui->lineEditCPF->setVisible(checked);
    ui->labelCPF->setVisible(checked);
    ui->dateEdit->setVisible(checked);
    ui->labelDataNasc->setVisible(checked);

    ui->lineEditCNPJ->setHidden(checked);
    ui->labelCNPJ->setHidden(checked);
    ui->lineEditInscEstadual->setHidden(checked);
    ui->labelInscricaoEstadual->setHidden(checked);
    ui->checkBoxInscEstIsento->setHidden(checked);
  }

  adjustSize();
}

void CadastroCliente::on_lineEditContatoCPF_textEdited(const QString &text) {
  ui->lineEditContatoCPF->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");
}

void CadastroCliente::on_checkBoxMostrarInativos_clicked(const bool checked) {
  modelEnd.setFilter("idCliente = " + data("idCliente").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return; }
}

void CadastroCliente::on_pushButtonRemoverEnd_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not setDataEnd("desativado", true)) { return; }

    if (not modelEnd.submitAll()) { return; }

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

// TODO: 0ao trocar de cadastro nao esta limpando observacao (esta fazendo append)
// TODO: 0nao deixar cadastrar endereco sem numero, se necessario colocar 'S/N'
// TODO: nesta tela e nas outras arrumar a ordem dos tabs
// TODO: limitar complemento de endereco a 60 caracteres
