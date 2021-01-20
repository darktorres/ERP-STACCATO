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
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCNPJ_textEdited);
  connect(ui->lineEditCNPJBancario, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCNPJBancario_textEdited);
  connect(ui->lineEditCPF, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCPF_textEdited);
  connect(ui->lineEditCPFBancario, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCPFBancario_textEdited);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditContatoCPF_textEdited);
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

  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

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
  // dados
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditCPF->setInputMask("999.999.999-99;_");
  ui->lineEditContatoCPF->setInputMask("999.999.999-99;_");
  ui->lineEditContatoRG->setInputMask("99.999.999-9;_");
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditUF->setInputMask(">AA;_");

  // endereco
  ui->lineEditCEP->setInputMask("99999-999;_");

  // bancario
  ui->lineEditAgencia->setInputMask("9999-9;_");
  ui->lineEditCNPJBancario->setInputMask("99.999.999/9999-99;_");
  ui->lineEditCPFBancario->setInputMask("999.999.999-99;_");
}

void CadastroProfissional::setupMapper() {
  addMapping(ui->comboBoxTipo, "tipoProf");
  addMapping(ui->doubleSpinBoxComissao, "comissao");
  addMapping(ui->itemBoxLoja, "idLoja", "id");
  addMapping(ui->itemBoxVendedor, "idUsuarioRel", "id");
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditCPF, "cpf");
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
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditProfissional, "nome_razao");
  addMapping(ui->lineEditTel_Cel, "telCel");
  addMapping(ui->lineEditTel_Com, "telCom");
  addMapping(ui->lineEditTel_Res, "tel");

  addMapping(ui->lineEditNomeBancario, "nomeBanco");
  addMapping(ui->lineEditCPFBancario, "cpfBanco");
  addMapping(ui->lineEditCNPJBancario, "cnpjBanco");
  addMapping(ui->lineEditBanco, "banco");
  addMapping(ui->lineEditAgencia, "agencia");
  addMapping(ui->lineEditCC, "cc");
  addMapping(ui->checkBoxPoupanca, "poupanca");

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

bool CadastroProfissional::verificaVinculo() {
  SqlQuery query;

  if (not query.exec("SELECT 0 FROM venda WHERE idProfissional = " + data("idProfissional").toString())) {
    throw RuntimeException("Erro verificando se existe pedidos vinculados: " + query.lastError().text());
  }

  return query.size() > 0;
}

bool CadastroProfissional::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idProfissional = " + data("idProfissional").toString() + (inativos ? "" : " AND desativado = FALSE"));

  modelEnd.select();

  //---------------------------------------------------

  tipoPFPJ = data("pfpj").toString();

  (tipoPFPJ == "PF") ? ui->radioButtonPF->setChecked(true) : ui->radioButtonPJ->setChecked(true);

  const bool existeVinculo = verificaVinculo();

  const bool administrativo = UserSession::tipoUsuario == "ADMINISTRADOR" or UserSession::tipoUsuario == "ADMINISTRATIVO" or UserSession::tipoUsuario == "DIRETOR";

  const bool bloquear = (existeVinculo and not administrativo);

  ui->lineEditProfissional->setReadOnly(bloquear);
  ui->lineEditCPF->setReadOnly(bloquear);
  ui->lineEditCNPJ->setReadOnly(bloquear);

  ui->groupBoxPFPJ->setDisabled(bloquear);
  ui->pushButtonRemover->setDisabled(bloquear);

  return true;
}

void CadastroProfissional::cadastrar() {
  try {
    qApp->startTransaction("CadastroProfissional::cadastrar");

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
  } catch (std::exception &e) {
    qApp->rollbackTransaction();
    model.select();
    modelEnd.select();

    for (auto &record : backupEndereco) { modelEnd.insertRecord(-1, record); }

    throw;
  }
}

void CadastroProfissional::verifyFields() {
  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { verifyRequiredField(*line); }

  if (ui->radioButtonPF->isChecked()) { validaCPF(ui->lineEditCPF->text()); }

  if (ui->radioButtonPJ->isChecked()) { validaCNPJ(ui->lineEditCNPJ->text()); }
}

void CadastroProfissional::savingProcedures() {
  if (tipoPFPJ == "PF") { setData("cnpj", ""); }
  if (tipoPFPJ == "PJ") { setData("cpf", ""); }
  if (not ui->lineEditCPF->text().remove(".").remove("-").isEmpty()) { setData("cpf", ui->lineEditCPF->text()); }
  if (not ui->lineEditCNPJ->text().remove(".").remove("/").remove("-").isEmpty()) { setData("cnpj", ui->lineEditCNPJ->text()); }
  if (not ui->lineEditContatoCPF->text().remove(".").remove("-").isEmpty()) { setData("contatoCPF", ui->lineEditContatoCPF->text()); }
  if (not ui->lineEditContatoRG->text().remove(".").remove("-").isEmpty()) { setData("contatoRG", ui->lineEditContatoRG->text()); }

  setData("nome_razao", ui->lineEditProfissional->text());
  setData("nomeFantasia", ui->lineEditNomeFantasia->text());
  setData("contatoNome", ui->lineEditContatoNome->text());
  setData("contatoApelido", ui->lineEditContatoApelido->text());
  setData("inscEstadual", ui->lineEditInscEstadual->text());
  setData("tel", ui->lineEditTel_Res->text());
  setData("telCel", ui->lineEditTel_Cel->text());
  setData("telCom", ui->lineEditTel_Com->text());
  setData("nextel", ui->lineEditNextel->text());
  setData("email", ui->lineEditEmail->text());
  setData("pfpj", tipoPFPJ);
  setData("tipoProf", ui->comboBoxTipo->currentText());
  setData("idUsuarioRel", ui->itemBoxVendedor->getId());
  setData("idLoja", ui->itemBoxLoja->getId());
  setData("comissao", ui->doubleSpinBoxComissao->value());

  // Dados bancários

  setData("nomeBanco", ui->lineEditNomeBancario->text());

  if (not ui->lineEditCPFBancario->text().remove(".").remove("-").isEmpty()) { setData("cpfBanco", ui->lineEditCPFBancario->text()); }
  if (not ui->lineEditCNPJBancario->text().remove(".").remove("/").remove("-").isEmpty()) { setData("cnpjBanco", ui->lineEditCNPJBancario->text()); }

  setData("banco", ui->lineEditBanco->text());

  if (not ui->lineEditAgencia->text().remove("-").isEmpty()) { setData("agencia", ui->lineEditAgencia->text()); }

  setData("cc", ui->lineEditCC->text());
  setData("poupanca", ui->checkBoxPoupanca->isChecked());
}

void CadastroProfissional::on_pushButtonCadastrar_clicked() { save(); }

void CadastroProfissional::on_pushButtonAtualizar_clicked() { save(); }

void CadastroProfissional::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroProfissional::on_pushButtonRemover_clicked() { remove(); }

void CadastroProfissional::clearFields() {
  RegisterDialog::clearFields();

  ui->comboBoxTipo->setCurrentIndex(0);
  ui->radioButtonPF->setChecked(true);
  novoEndereco();
  setupUi();
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

void CadastroProfissional::on_lineEditCPF_textEdited(const QString &text) { ui->lineEditCPF->setStyleSheet(validaCPF(text) ? "" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::on_lineEditCNPJ_textEdited(const QString &text) { ui->lineEditCNPJ->setStyleSheet(validaCNPJ(text) ? "" : "color: rgb(255, 0, 0)"); }

bool CadastroProfissional::cadastrarEndereco(const Tipo tipoEndereco) {
  verificaEndereco();

  if (tipoEndereco == Tipo::Cadastrar) { currentRowEnd = modelEnd.insertRowAtEnd(); }

  setDataEnd("descricao", ui->comboBoxTipoEnd->currentText());
  setDataEnd("CEP", ui->lineEditCEP->text());
  setDataEnd("logradouro", ui->lineEditLogradouro->text());
  setDataEnd("numero", ui->lineEditNro->text());
  setDataEnd("complemento", ui->lineEditComp->text());
  setDataEnd("bairro", ui->lineEditBairro->text());
  setDataEnd("cidade", ui->lineEditCidade->text());
  setDataEnd("uf", ui->lineEditUF->text());
  setDataEnd("codUF", getCodigoUF(ui->lineEditUF->text()));
  setDataEnd("desativado", false);

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

  CepCompleter cc;
  cc.buscaCEP(cep, this);

  ui->lineEditUF->setText(cc.getUf());
  ui->lineEditCidade->setText(cc.getCidade());
  ui->lineEditLogradouro->setText(cc.getEndereco());
  ui->lineEditBairro->setText(cc.getBairro());
}

void CadastroProfissional::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();
  mapperEnd.setCurrentModelIndex(index);
  currentRowEnd = index.row();
}

void CadastroProfissional::on_lineEditContatoCPF_textEdited(const QString &text) { ui->lineEditContatoCPF->setStyleSheet(validaCPF(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idProfissional = " + data("idProfissional").toString() + (checked ? "" : " AND desativado = FALSE"));

  modelEnd.select();
}

void CadastroProfissional::on_pushButtonRemoverEnd_clicked() {
  // TODO: se já estiver desativado apenas retornar

  if (removeBox() == QMessageBox::Yes) {
    setDataEnd("desativado", true);

    modelEnd.submitAll();

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

void CadastroProfissional::on_lineEditCPFBancario_textEdited(const QString &text) { ui->lineEditCPFBancario->setStyleSheet(validaCPF(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::on_lineEditCNPJBancario_textEdited(const QString &text) { ui->lineEditCNPJBancario->setStyleSheet(validaCNPJ(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::successMessage() {
  qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Profissional cadastrado com sucesso!", this);
  emit registerUpdated(primaryId);
}

void CadastroProfissional::verificaEndereco() {
  RegisterAddressDialog::verificaEndereco(ui->lineEditCidade->text(), ui->lineEditUF->text());

  if (not ui->lineEditCEP->isValid()) { throw RuntimeError("CEP inválido!", this); }

  if (ui->lineEditCidade->text().isEmpty()) { throw RuntimeError("Cidade vazio!", this); }

  if (ui->lineEditUF->text().isEmpty()) { throw RuntimeError("UF vazio!", this); }
}
