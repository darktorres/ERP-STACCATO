#include "cadastrocliente.h"
#include "ui_cadastrocliente.h"

#include "application.h"
#include "cadastroprofissional.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "usersession.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

CadastroCliente::CadastroCliente(QWidget *parent) : RegisterAddressDialog("cliente", "idCliente", parent), ui(new Ui::CadastroCliente) {
  ui->setupUi(this);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(true, this));
  ui->itemBoxProfissional->setRegisterDialog(new CadastroProfissional(this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));

  setupTables();
  setupMapper();
  newRegister();

  sdCliente = SearchDialog::cliente(this);
  connect(sdCliente, &SearchDialog::itemSelected, this, &CadastroCliente::viewRegisterById);

  ui->lineEditCliente->setFocus();

  setConnections();

  on_radioButtonPF_toggled(true);
}

CadastroCliente::~CadastroCliente() { delete ui; }

void CadastroCliente::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxInscEstIsento, &QCheckBox::toggled, this, &CadastroCliente::on_checkBoxInscEstIsento_toggled, connectionType);
  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroCliente::on_checkBoxMostrarInativos_clicked, connectionType);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged, connectionType);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditCNPJ_textEdited, connectionType);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditContatoCPF_textEdited, connectionType);
  connect(ui->lineEditCPF, &QLineEdit::textEdited, this, &CadastroCliente::on_lineEditCPF_textEdited, connectionType);
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
  ui->tableEndereco->hideColumn("idCliente");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroCliente::verifyFields() {
  const auto children = ui->frame->findChildren<QLineEdit *>();

  for (const auto &line : children) { verifyRequiredField(*line); }

  if (ui->radioButtonPF->isChecked() and ui->lineEditCPF->styleSheet().contains("color: rgb(255, 0, 0)")) { throw RuntimeError("CPF inválido!"); }

  if (ui->radioButtonPJ->isChecked() and ui->lineEditCNPJ->styleSheet().contains("color: rgb(255, 0, 0)")) { throw RuntimeError("CNPJ inválido!"); }

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
  const QDate aniversario = ui->dateEdit->date();
  if (aniversario.toString("yyyy-MM-dd") != "1900-01-01") { setData("dataNasc", aniversario); }

  if (not ui->lineEditContatoCPF->text().remove(".").remove("-").isEmpty()) { setData("contatoCPF", ui->lineEditContatoCPF->text()); }

  if (tipoPFPJ == "PF") { setData("cnpj", ""); }
  if (tipoPFPJ == "PJ") { setData("cpf", ""); }

  if (not ui->lineEditCPF->text().remove(".").remove("-").isEmpty()) { setData("cpf", ui->lineEditCPF->text()); }
  if (not ui->lineEditCNPJ->text().remove(".").remove("/").remove("-").isEmpty()) { setData("cnpj", ui->lineEditCNPJ->text()); }

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
  ui->checkBoxInscEstIsento->setChecked(false);
  novoEndereco();

  const auto children = findChildren<ItemBox *>();

  for (const auto &box : children) { box->clear(); }

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
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNro, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditComp, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroCliente::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

  ui->pushButtonRemoverEnd->hide();
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

  if (data("dataNasc").isNull()) { ui->dateEdit->setDate(QDate(1900, 1, 1)); }

  const bool existeVinculo = verificaVinculo();

  const bool administrativo = UserSession::tipoUsuario == "ADMINISTRADOR" or UserSession::tipoUsuario == "ADMINISTRATIVO" or UserSession::tipoUsuario == "DIRETOR";

  const bool bloquear = (existeVinculo and not administrativo);

  ui->lineEditCliente->setReadOnly(bloquear);
  ui->lineEditCPF->setReadOnly(bloquear);
  ui->lineEditCNPJ->setReadOnly(bloquear);

  ui->pushButtonRemover->setDisabled(bloquear);
  ui->pushButtonRemoverEnd->setDisabled(bloquear);
  ui->pushButtonAtualizarEnd->setDisabled(bloquear);
  ui->groupBoxPFPJ->setDisabled(bloquear);

  if (bloquear and UserSession::tipoUsuario != "GERENTE LOJA" and UserSession::tipoUsuario != "DIRETOR" and UserSession::tipoUsuario != "ADMINISTRADOR") {
    if (not data("contatoNome").toString().isEmpty()) { ui->lineEditContatoNome->setReadOnly(true); }
    if (not data("contatoApelido").toString().isEmpty()) { ui->lineEditContatoApelido->setReadOnly(true); }
    if (not data("contatoRG").toString().isEmpty()) { ui->lineEditContatoRG->setReadOnly(true); }
    if (not data("inscEstadual").toString().isEmpty()) { ui->lineEditInscEstadual->setReadOnly(true); }
    if (not data("tel").toString().isEmpty()) { ui->lineEditTel_Res->setReadOnly(true); }
    if (not data("telCel").toString().isEmpty()) { ui->lineEditTel_Cel->setReadOnly(true); }
    if (not data("telCom").toString().isEmpty()) { ui->lineEditTel_Com->setReadOnly(true); }
    if (not data("nextel").toString().isEmpty()) { ui->lineEditNextel->setReadOnly(true); }
    if (not data("email").toString().isEmpty()) { ui->lineEditEmail->setReadOnly(true); }
    if (not data("idCadastroRel").toString().isEmpty()) { ui->itemBoxCliente->setReadOnly(true); }
    if (not data("idProfissionalRel").toString().isEmpty()) { ui->itemBoxProfissional->setReadOnly(true); }
    if (not data("idUsuarioRel").toString().isEmpty()) { ui->itemBoxVendedor->setReadOnly(true); }
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
  ui->lineEditCPF->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");

  if (not ui->lineEditCPF->styleSheet().contains("color: rgb(255, 0, 0)")) {
    SqlQuery query;
    query.prepare("SELECT idCliente FROM cliente WHERE cpf = :cpf");
    query.bindValue(":cpf", text);

    if (not query.exec()) { throw RuntimeException("Erro buscando CPF: " + query.lastError().text(), this); }

    if (query.first()) {
      throw RuntimeError("CPF já cadastrado!", this);
      viewRegisterById(query.value("idCliente"));
    }
  }
}

void CadastroCliente::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");

  if (not ui->lineEditCNPJ->styleSheet().contains("color: rgb(255, 0, 0)")) {
    SqlQuery query;
    query.prepare("SELECT idCliente FROM cliente WHERE cnpj = :cnpj");
    query.bindValue(":cnpj", text);

    if (not query.exec()) { throw RuntimeException("Erro buscando CNPJ: " + query.lastError().text(), this); }

    if (query.first()) {
      throw RuntimeError("CNPJ já cadastrado!", this);
      viewRegisterById(query.value("idCliente"));
    }
  }
}

bool CadastroCliente::cadastrarEndereco(const Tipo tipoEndereco) {
  if (not ui->lineEditCEP->isValid()) {
    throw RuntimeError("CEP inválido!", this);
    ui->lineEditCEP->setFocus();
    return false;
  }

  if (ui->lineEditNro->text().isEmpty()) {
    throw RuntimeError("Número vazio!", this);
    ui->lineEditNro->setFocus();
    return false;
  }

  if (tipoEndereco == Tipo::Cadastrar) { currentRowEnd = modelEnd.insertRowAtEnd(); }

  setDataEnd("descricao", ui->comboBoxTipoEnd->currentText());
  setDataEnd("cep", ui->lineEditCEP->text());
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
  } catch (std::exception &e) {
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

  ui->lineEditNro->clear();
  ui->lineEditComp->clear();

  CepCompleter cc;
  cc.buscaCEP(cep, this);

  ui->lineEditUF->setText(cc.getUf());
  ui->lineEditCidade->setText(cc.getCidade());
  ui->lineEditLogradouro->setText(cc.getEndereco());
  ui->lineEditBairro->setText(cc.getBairro());
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
  ui->pushButtonRemoverEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroCliente::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();
  //------------------------------------------
  disconnect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged);
  mapperEnd.setCurrentModelIndex(index);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroCliente::on_lineEditCEP_textChanged);
  //------------------------------------------
  currentRowEnd = index.row();
}

void CadastroCliente::on_radioButtonPF_toggled(const bool checked) {
  tipoPFPJ = checked ? "PF" : "PJ";

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
  // TODO: clicar nesse botao faz com que enderecos nao salvos sumam

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

// TODO: 0ao trocar de cadastro nao esta limpando observacao (esta fazendo append)
// TODO: 0nao deixar cadastrar endereco sem numero, se necessario colocar 'S/N'
// TODO: nesta tela e nas outras arrumar a ordem dos tabs
// TODO: limitar complemento de endereco a 60 caracteres
