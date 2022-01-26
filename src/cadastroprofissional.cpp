#include "cadastroprofissional.h"
#include "ui_cadastroprofissional.h"

#include "application.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "user.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

CadastroProfissional::CadastroProfissional(QWidget *parent) : RegisterAddressDialog("profissional", "idProfissional", parent), ui(new Ui::CadastroProfissional) {
  ui->setupUi(this);

  connectLineEditsToDirty();
  setItemBoxes();
  setupUi();
  setupTables();
  setupMapper();
  newRegister();

  ui->lineEditProfissional->setFocus();

  on_radioButtonPF_toggled(true);

  setConnections();
}

CadastroProfissional::~CadastroProfissional() { delete ui; }

void CadastroProfissional::setItemBoxes() {
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));
  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
}

void CadastroProfissional::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(sdProfissional, &SearchDialog::itemSelected, this, &CadastroProfissional::viewRegisterById, connectionType);
  connect(ui->checkBoxDataNasc, &QCheckBox::stateChanged, this, &CadastroProfissional::on_checkBoxDataNasc_stateChanged, connectionType);
  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroProfissional::on_checkBoxMostrarInativos_clicked, connectionType);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroProfissional::on_lineEditCEP_textChanged, connectionType);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCNPJ_textEdited, connectionType);
  connect(ui->lineEditCNPJBancario, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCNPJBancario_textEdited, connectionType);
  connect(ui->lineEditCPF, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCPF_textEdited, connectionType);
  connect(ui->lineEditCPFBancario, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditCPFBancario_textEdited, connectionType);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroProfissional::on_lineEditContatoCPF_textEdited, connectionType);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonAdicionarEnd_clicked, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonAtualizar_clicked, connectionType);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonAtualizarEnd_clicked, connectionType);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonDesativar, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonDesativar_clicked, connectionType);
  connect(ui->pushButtonDesativarEnd, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonDesativarEnd_clicked, connectionType);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroProfissional::on_pushButtonNovoCad_clicked, connectionType);
  connect(ui->radioButtonPF, &QRadioButton::toggled, this, &CadastroProfissional::on_radioButtonPF_toggled, connectionType);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroProfissional::on_tableEndereco_clicked, connectionType);
}

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
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditUF->setInputMask(">AA;_");

  // endereco
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");

  // bancario
  ui->lineEditAgencia->setInputMask("9999-9;_");
  ui->lineEditCNPJBancario->setInputMask("99.999.999/9999-99;_");
  ui->lineEditCPFBancario->setInputMask("999.999.999-99;_");
}

void CadastroProfissional::setupMapper() {
  addMapping(ui->comboBoxTipo, "tipoProf");
  addMapping(ui->dateEditDataNasc, "aniversario");
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
  addMapping(ui->lineEditTel, "tel");
  addMapping(ui->lineEditTel_Cel, "telCel");
  addMapping(ui->lineEditTel_Com, "telCom");

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
  mapperEnd.addMapping(ui->lineEditComplemento, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNumero, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroProfissional::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonDesativar->hide();

  ui->pushButtonDesativarEnd->hide();
}

void CadastroProfissional::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonDesativar->show();

  if (readOnly) {
    ui->pushButtonBuscar->hide();
    ui->pushButtonNovoCad->hide();
  }
}

bool CadastroProfissional::verificaVinculo() {
  SqlQuery query;

  if (not query.exec("SELECT 0 FROM venda WHERE status NOT IN ('CANCELADO') AND idProfissional = " + data("idProfissional").toString() + " LIMIT 1")) {
    throw RuntimeException("Erro verificando se existe pedidos vinculados: " + query.lastError().text());
  }

  return query.size() > 0;
}

bool CadastroProfissional::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const double comissao = data("comissao").toDouble();

  if (comissao > 5) {
    ui->doubleSpinBoxComissao->setMaximum(comissao);
    ui->doubleSpinBoxComissao->setValue(comissao);
  }

  if (not User::isAdmin() and comissao <= 5) { ui->doubleSpinBoxComissao->setMaximum(5); }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idProfissional = " + data("idProfissional").toString() + (inativos ? "" : " AND desativado = FALSE"));

  modelEnd.select();

  //---------------------------------------------------

  tipoPFPJ = data("pfpj").toString();

  (tipoPFPJ == "PF") ? ui->radioButtonPF->setChecked(true) : ui->radioButtonPJ->setChecked(true);

  if (not data("aniversario").isNull()) {
    ui->checkBoxDataNasc->setChecked(true);
    ui->dateEditDataNasc->setEnabled(true);
  }

  const bool existeVinculo = verificaVinculo();
  const bool administrativo = User::isAdministrativo();
  const bool bloquear = (existeVinculo and not administrativo);

  ui->lineEditProfissional->setReadOnly(bloquear);
  ui->lineEditCPF->setReadOnly(bloquear);
  ui->lineEditCNPJ->setReadOnly(bloquear);

  ui->groupBoxPFPJ->setDisabled(bloquear);
  ui->pushButtonDesativar->setDisabled(bloquear);

  return true;
}

void CadastroProfissional::on_checkBoxDataNasc_stateChanged(const int state) { ui->dateEditDataNasc->setEnabled(state); }

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
  } catch (std::exception &) {
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
  setData("aniversario", (ui->checkBoxDataNasc->isChecked()) ? ui->dateEditDataNasc->date() : QVariant());

  if (tipoPFPJ == "PF") { setData("cnpj", ""); }
  if (tipoPFPJ == "PJ") { setData("cpf", ""); }

  setData("cpf", (ui->lineEditCPF->text() == "..-") ? "" : ui->lineEditCPF->text());
  setData("cnpj", (ui->lineEditCNPJ->text() == "../-") ? "" : ui->lineEditCNPJ->text());
  setData("contatoCPF", (ui->lineEditContatoCPF->text() == "..-") ? "" : ui->lineEditContatoCPF->text());
  setData("contatoRG", ui->lineEditContatoRG->text());
  setData("nome_razao", ui->lineEditProfissional->text());
  setData("nomeFantasia", ui->lineEditNomeFantasia->text());
  setData("contatoNome", ui->lineEditContatoNome->text());
  setData("contatoApelido", ui->lineEditContatoApelido->text());
  setData("inscEstadual", ui->lineEditInscEstadual->text());
  setData("tel", ui->lineEditTel->text());
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

  setData("cpfBanco", (ui->lineEditCPFBancario->text() == "..-") ? "" : ui->lineEditCPFBancario->text());
  setData("cnpjBanco", (ui->lineEditCNPJBancario->text() == "../-") ? "" : ui->lineEditCNPJBancario->text());
  setData("banco", ui->lineEditBanco->text());
  setData("agencia", (ui->lineEditAgencia->text() == "-") ? "" : ui->lineEditAgencia->text());
  setData("cc", ui->lineEditCC->text());
  setData("poupanca", ui->checkBoxPoupanca->isChecked());
}

void CadastroProfissional::on_pushButtonCadastrar_clicked() { save(); }

void CadastroProfissional::on_pushButtonAtualizar_clicked() { save(); }

void CadastroProfissional::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroProfissional::on_pushButtonDesativar_clicked() { remove(); }

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
  ui->pushButtonDesativarEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroProfissional::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComplemento->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNumero->clear();
  ui->lineEditUF->clear();
}

void CadastroProfissional::on_pushButtonBuscar_clicked() {
  if (not askSaveBeforeClosing()) { return; }

  sdProfissional->show();
}

void CadastroProfissional::on_lineEditCPF_textEdited(const QString &text) { ui->lineEditCPF->setStyleSheet(validaCPF(text) ? "" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::on_lineEditCNPJ_textEdited(const QString &text) { ui->lineEditCNPJ->setStyleSheet(validaCNPJ(text) ? "" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::cadastrarEndereco(const Tipo tipoEndereco) {
  verificaEndereco();

  if (tipoEndereco == Tipo::Cadastrar) { currentRowEnd = modelEnd.insertRowAtEnd(); }

  setDataEnd("descricao", ui->comboBoxTipoEnd->currentText());
  setDataEnd("CEP", ui->lineEditCEP->text());
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
}

void CadastroProfissional::on_pushButtonAdicionarEnd_clicked() {
  cadastrarEndereco();
  novoEndereco();
}

void CadastroProfissional::on_pushButtonAtualizarEnd_clicked() {
  cadastrarEndereco(Tipo::Atualizar);
  novoEndereco();
}

void CadastroProfissional::on_lineEditCEP_textChanged(const QString &cep) {
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

void CadastroProfissional::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  currentRowEnd = index.row();

  const bool desativado = dataEnd("desativado").toBool();

  ui->pushButtonAtualizarEnd->setEnabled(desativado);
  ui->pushButtonDesativarEnd->setDisabled(desativado);

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonDesativarEnd->show();

  //------------------------------------------
  disconnect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroProfissional::on_lineEditCEP_textChanged);
  mapperEnd.setCurrentModelIndex(index);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroProfissional::on_lineEditCEP_textChanged);
  //------------------------------------------
}

void CadastroProfissional::on_lineEditContatoCPF_textEdited(const QString &text) { ui->lineEditContatoCPF->setStyleSheet(validaCPF(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroProfissional::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idProfissional = " + data("idProfissional").toString() + (checked ? "" : " AND desativado = FALSE"));

  modelEnd.select();
}

void CadastroProfissional::on_pushButtonDesativarEnd_clicked() {
  // TODO: se já estiver desativado apenas retornar

  if (removeBox() == QMessageBox::Yes) {
    setDataEnd("desativado", true);

    modelEnd.submitAll();

    novoEndereco();
  }
}

void CadastroProfissional::on_radioButtonPF_toggled(const bool checked) {
  tipoPFPJ = (checked) ? "PF" : "PJ";

  ui->lineEditCNPJ->setHidden(checked);
  ui->labelCNPJ->setHidden(checked);
  ui->lineEditCPF->setVisible(checked);
  ui->labelCPF->setVisible(checked);
  ui->lineEditInscEstadual->setHidden(checked);
  ui->labelInscricaoEstadual->setHidden(checked);

  ui->dateEditDataNasc->setVisible(checked);
  ui->checkBoxDataNasc->setVisible(checked);

  if (not checked) { ui->checkBoxDataNasc->setChecked(false); }

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
  if (not ui->lineEditCEP->isValid()) { throw RuntimeError("CEP inválido!", this); }

  if (ui->lineEditNumero->text().isEmpty()) { throw RuntimeError(R"(Número vazio! Se necessário coloque "S/N"!)", this); }

  if (ui->lineEditCidade->text().isEmpty()) { throw RuntimeError("Cidade vazio!", this); }

  if (ui->lineEditUF->text().isEmpty()) { throw RuntimeError("UF vazio!", this); }

  RegisterAddressDialog::verificaEndereco(ui->lineEditCidade->text(), ui->lineEditUF->text());
}

void CadastroProfissional::connectLineEditsToDirty() {
  const auto children = ui->frame->findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &CadastroProfissional::marcarDirty); }
}

bool CadastroProfissional::newRegister() {
  if (not RegisterAddressDialog::newRegister()) { return false; }

  if (not User::isAdmin()) { ui->doubleSpinBoxComissao->setMaximum(5); }

  return true;
}
