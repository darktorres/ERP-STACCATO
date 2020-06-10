#include "cadastroprofissional.h"
#include "ui_cadastroprofissional.h"

#include "application.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "itembox.h"
#include "usersession.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

CadastroProfissional::CadastroProfissional(QWidget *parent) : RegisterAddressDialog("profissional", "idProfissional", parent), ui(new Ui::CadastroProfissional) {
  ui->setupUi(this);

  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroProfissional::on_checkBoxMostrarInativos_clicked);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroProfissional::on_lineEditCEP_textChanged);
  connect(ui->lineEditCNPJ, &QLineEdit::editingFinished, this, &CadastroProfissional::on_lineEditCNPJ_editingFinished);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCNPJ_textEdited);
  connect(ui->lineEditCNPJBancario, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCNPJBancario_textEdited);
  connect(ui->lineEditCPF, &QLineEdit::editingFinished, this, &CadastroProfissional::on_lineEditCPF_editingFinished);
  connect(ui->lineEditCPF, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCPF_textEdited);
  connect(ui->lineEditCPFBancario, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCPFBancario_textEdited);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditContatoCPF_textEdited);
  connect(ui->lineEditProfissional, &QLineEdit::editingFinished, this, &CadastroProfissional::on_lineEditProfissional_editingFinished);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonAdicionarEnd_clicked);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonAtualizarEnd_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonRemoverEnd_clicked);
  connect(ui->radioButtonPF, &QRadioButton::toggled, this, &CadastroProfissional::on_radioButtonPF_toggled);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroProfissional::on_tableEndereco_clicked);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  setWindowModality(Qt::NonModal);

  setupTables();
  setupUi();
  setupMapper();
  newRegister();

  sdProfissional = SearchDialog::profissional(false, this);
  connect(sdProfissional, &SearchDialog::itemSelected, this, &CadastroProfissional::viewRegisterById);

  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));
  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));

  if (UserSession::tipoUsuario() != "ADMINISTRADOR" and UserSession::tipoUsuario() != "ADMINISTRATIVO") {
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabBancario), false);
    ui->pushButtonRemover->setDisabled(true);
  }
}

CadastroProfissional::~CadastroProfissional() { delete ui; }

void CadastroProfissional::setupTables() {
  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idProfissional");
  ui->tableEndereco->hideColumn("codUF");
  ui->tableEndereco->hideColumn("created");
  ui->tableEndereco->hideColumn("lastUpdated");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroProfissional::setupUi() {
  ui->lineEditAgencia->setInputMask("9999-9;_");
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditCNPJBancario->setInputMask("99.999.999/9999-99;_");
  ui->lineEditContatoCPF->setInputMask("999.999.999-99;_");
  ui->lineEditContatoRG->setInputMask("99.999.999-9;_");
  ui->lineEditCPF->setInputMask("999.999.999-99;_");
  ui->lineEditCPFBancario->setInputMask("999.999.999-99;_");
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditUF->setInputMask(">AA;_");
}

void CadastroProfissional::setupMapper() {
  addMapping(ui->comboBoxTipo, "tipoProf");
  addMapping(ui->doubleSpinBoxComissao, "comissao");
  addMapping(ui->itemBoxLoja, "idLoja", "id");
  addMapping(ui->itemBoxVendedor, "idUsuarioRel", "id");
  addMapping(ui->lineEditAgencia, "agencia");
  addMapping(ui->lineEditBanco, "banco");
  addMapping(ui->lineEditCC, "cc");
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditCNPJBancario, "cnpjBanco");
  addMapping(ui->lineEditCPF, "cpf");
  addMapping(ui->lineEditCPFBancario, "cpfBanco");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoCPF, "contatoCPF");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditEmail, "email");
  addMapping(ui->lineEditIdNextel, "idNextel");
  addMapping(ui->lineEditInscEstadual, "inscEstadual");
  addMapping(ui->lineEditNextel, "nextel");
  addMapping(ui->lineEditNomeBancario, "nomeBanco");
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditProfissional, "nome_razao");
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

void CadastroProfissional::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

  ui->pushButtonRemoverEnd->hide();
}

void CadastroProfissional::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

bool CadastroProfissional::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idProfissional = " + data("idProfissional").toString() + (inativos ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return false; }

  //---------------------------------------------------

  ui->checkBoxPoupanca->setChecked(data("poupanca").toBool());

  tipoPFPJ = data("pfpj").toString();

  (tipoPFPJ == "PF") ? ui->radioButtonPF->setChecked(true) : ui->radioButtonPJ->setChecked(true);

  ui->groupBoxPFPJ->setDisabled(true);

  ui->lineEditProfissional->setDisabled(true);
  if (not data("cpf").toString().isEmpty()) { ui->lineEditCPF->setDisabled(true); }
  if (not data("cnpj").toString().isEmpty()) { ui->lineEditCNPJ->setDisabled(true); }

  return true;
}

bool CadastroProfissional::cadastrar() {
  if (not qApp->startTransaction("CadastroProfissional::cadastrar")) { return false; }

  const bool success = [&] {
    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    if (not savingProcedures()) { return false; }

    if (not model.submitAll()) { return false; }

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

    // -------------------------------------------------------------------------

    if (not setForeignKey(modelEnd)) { return false; }

    return modelEnd.submitAll();
  }();

  if (success) {
    if (not qApp->endTransaction()) { return false; }

    backupEndereco.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelEnd.setFilter(primaryKey + " = '" + primaryId + "'");
  } else {
    qApp->rollbackTransaction();
    void(model.select());
    void(modelEnd.select());

    for (auto &record : backupEndereco) { modelEnd.insertRecord(-1, record); }
  }

  return success;
}

bool CadastroProfissional::verifyFields() {
  const auto children = ui->frame->findChildren<QLineEdit *>();

  for (const auto &line : children) {
    if (not verifyRequiredField(*line)) { return false; }
  }

  if (ui->radioButtonPF->isChecked() and ui->lineEditCPF->styleSheet().contains("color: rgb(255, 0, 0)")) { return qApp->enqueueError(false, "CPF inválido!", this); }

  if (ui->radioButtonPJ->isChecked() and ui->lineEditCNPJ->styleSheet().contains("color: rgb(255, 0, 0)")) { return qApp->enqueueError(false, "CNPJ inválido!", this); }

  return true;
}

bool CadastroProfissional::savingProcedures() {
  if (not setData("nome_razao", ui->lineEditProfissional->text())) { return false; }
  if (not setData("nomeFantasia", ui->lineEditNomeFantasia->text())) { return false; }
  if (not setData("cpf", ui->lineEditCPF->text())) { return false; }
  if (not setData("contatoNome", ui->lineEditContatoNome->text())) { return false; }
  if (not setData("contatoCPF", ui->lineEditContatoCPF->text())) { return false; }
  if (not setData("contatoApelido", ui->lineEditContatoApelido->text())) { return false; }
  if (not setData("contatoRG", ui->lineEditContatoRG->text())) { return false; }
  if (not setData("cnpj", ui->lineEditCNPJ->text())) { return false; }
  if (not setData("inscEstadual", ui->lineEditInscEstadual->text())) { return false; }
  if (not setData("tel", ui->lineEditTel_Res->text())) { return false; }
  if (not setData("telCel", ui->lineEditTel_Cel->text())) { return false; }
  if (not setData("telCom", ui->lineEditTel_Com->text())) { return false; }
  if (not setData("nextel", ui->lineEditNextel->text())) { return false; }
  if (not setData("email", ui->lineEditEmail->text())) { return false; }
  if (not setData("pfpj", tipoPFPJ)) { return false; }
  if (not setData("tipoProf", ui->comboBoxTipo->currentText())) { return false; }
  if (not setData("idUsuarioRel", ui->itemBoxVendedor->getId())) { return false; }
  if (not setData("idLoja", ui->itemBoxLoja->getId())) { return false; }
  if (not setData("comissao", ui->doubleSpinBoxComissao->value())) { return false; }
  // Dados bancários
  if (not setData("banco", ui->lineEditBanco->text())) { return false; }
  if (not setData("agencia", ui->lineEditAgencia->text())) { return false; }
  if (not setData("cc", ui->lineEditCC->text())) { return false; }
  if (not setData("nomeBanco", ui->lineEditNomeBancario->text())) { return false; }
  if (not setData("cpfBanco", ui->lineEditCPFBancario->text())) { return false; }
  if (not setData("cnpjBanco", ui->lineEditCNPJBancario->text())) { return false; }
  if (not setData("poupanca", ui->checkBoxPoupanca->isChecked())) { return false; }

  return true;
}

void CadastroProfissional::on_pushButtonCadastrar_clicked() { save(); }

void CadastroProfissional::on_pushButtonAtualizar_clicked() { save(); }

void CadastroProfissional::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroProfissional::on_pushButtonRemover_clicked() { remove(); }

void CadastroProfissional::clearFields() {
  RegisterDialog::clearFields();

  ui->radioButtonPF->setChecked(true);
  novoEndereco();

  const auto children = findChildren<ItemBox *>();

  for (const auto &box : children) { box->clear(); }

  setupUi();

  ui->comboBoxTipo->setCurrentIndex(0);
}

void CadastroProfissional::novoEndereco() {
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonRemoverEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroProfissional::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComp->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNro->clear();
  ui->lineEditUF->clear();
}

void CadastroProfissional::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdProfissional->show();
}

void CadastroProfissional::on_lineEditCPF_textEdited(const QString &text) { ui->lineEditCPF->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "" : "color: rgb(255, 0, 0)");
}

bool CadastroProfissional::cadastrarEndereco(const Tipo tipoEndereco) {
  if (not ui->lineEditCEP->isValid()) {
    qApp->enqueueError("CEP inválido!", this);
    ui->lineEditCEP->setFocus();
    return false;
  }

  if (tipoEndereco == Tipo::Cadastrar) { currentRowEnd = modelEnd.insertRowAtEnd(); }

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

  if (tipoEndereco == Tipo::Cadastrar) { backupEndereco.append(modelEnd.record(currentRowEnd)); }

  isDirty = true;

  return true;
}

void CadastroProfissional::on_pushButtonAdicionarEnd_clicked() {
  if (cadastrarEndereco()) { novoEndereco(); }
}

void CadastroProfissional::on_pushButtonAtualizarEnd_clicked() {
  if (cadastrarEndereco(Tipo::Atualizar)) { novoEndereco(); }
}

void CadastroProfissional::on_lineEditCEP_textChanged(const QString &cep) {
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

void CadastroProfissional::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();
  mapperEnd.setCurrentModelIndex(index);
  currentRowEnd = index.row();
}

void CadastroProfissional::on_lineEditContatoCPF_textEdited(const QString &text) {
  ui->lineEditContatoCPF->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");
}

void CadastroProfissional::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idProfissional = " + data("idProfissional").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return; }
}

void CadastroProfissional::on_pushButtonRemoverEnd_clicked() {
  // TODO: se já estiver desativado apenas retornar

  if (removeBox() == QMessageBox::Yes) {
    if (not setDataEnd("desativado", true)) { return; }

    if (not modelEnd.submitAll()) { return; }

    novoEndereco();
  }
}

void CadastroProfissional::on_radioButtonPF_toggled(const bool checked) {
  tipoPFPJ = checked ? QString("PF") : QString("PJ");
  ui->lineEditCNPJ->setHidden(checked);
  ui->labelCNPJ->setHidden(checked);
  ui->lineEditCPF->setVisible(checked);
  ui->labelCPF->setVisible(checked);
  ui->lineEditInscEstadual->setHidden(checked);
  ui->labelInscricaoEstadual->setHidden(checked);
  checked ? ui->lineEditCNPJ->clear() : ui->lineEditCPF->clear();

  adjustSize();
}

void CadastroProfissional::on_lineEditCPFBancario_textEdited(const QString &text) {
  ui->lineEditCPFBancario->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");
}

void CadastroProfissional::on_lineEditCNPJBancario_textEdited(const QString &text) {
  ui->lineEditCNPJBancario->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("-").remove("/")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");
}

void CadastroProfissional::successMessage() {
  qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Profissional cadastrado com sucesso!", this);
  emit registerUpdated(primaryId);
}

void CadastroProfissional::on_lineEditProfissional_editingFinished() { ui->lineEditNomeBancario->setText(ui->lineEditProfissional->text()); }

void CadastroProfissional::on_lineEditCPF_editingFinished() { ui->lineEditCPFBancario->setText(ui->lineEditCPF->text()); }

void CadastroProfissional::on_lineEditCNPJ_editingFinished() { ui->lineEditCNPJBancario->setText(ui->lineEditCNPJ->text()); }
