#include "cadastroloja.h"
#include "ui_cadastroloja.h"

#include "application.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "usersession.h"

#include <QDebug>
#include <QMessageBox>

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

  if (UserSession::tipoUsuario != "ADMINISTRADOR" and UserSession::tipoUsuario != "ADMINISTRATIVO") {
    ui->pushButtonRemover->setDisabled(true);
    ui->pushButtonRemoverEnd->setDisabled(true);
  }

  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroLoja::on_checkBoxMostrarInativos_clicked);
  connect(ui->checkBoxMostrarInativosConta, &QCheckBox::clicked, this, &CadastroLoja::on_checkBoxMostrarInativosConta_clicked);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroLoja::on_lineEditCEP_textChanged);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroLoja::on_lineEditCNPJ_textEdited);
  connect(ui->pushButtonAdicionarConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAdicionarConta_clicked);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAdicionarEnd_clicked);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonAtualizarConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizarConta_clicked);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonAtualizarEnd_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverConta, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverConta_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroLoja::on_pushButtonRemoverEnd_clicked);
  connect(ui->tableConta, &TableView::clicked, this, &CadastroLoja::on_tableConta_clicked);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroLoja::on_tableEndereco_clicked);
}

CadastroLoja::~CadastroLoja() { delete ui; }

void CadastroLoja::setupUi() {
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditSIGLA->setInputMask(">XXXX;_");
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
}

void CadastroLoja::setupTables() {
  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idLoja");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableEndereco->setPersistentColumns({"desativado"});

  // -------------------------------------------------------------------------

  modelConta.setTable("loja_has_conta");

  modelConta.setHeaderData("banco", "Banco");
  modelConta.setHeaderData("agencia", "Agência");
  modelConta.setHeaderData("conta", "Conta");
  modelConta.setHeaderData("desativado", "Desativado");

  ui->tableConta->setModel(&modelConta);

  ui->tableConta->hideColumn("idConta");
  ui->tableConta->hideColumn("idLoja");

  ui->tableConta->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableConta->setPersistentColumns({"desativado"});
}

void CadastroLoja::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  novaConta();
  setupUi();
}

void CadastroLoja::verifyFields() {
  const auto children = ui->groupBoxCadastro->findChildren<QLineEdit *>();

  for (const auto &line : children) { verifyRequiredField(*line); }
}

void CadastroLoja::savingProcedures() {
  setData("descricao", ui->lineEditDescricao->text());
  setData("razaoSocial", ui->lineEditRazaoSocial->text());
  setData("sigla", ui->lineEditSIGLA->text());
  setData("nomeFantasia", ui->lineEditNomeFantasia->text());
  setData("cnpj", ui->lineEditCNPJ->text());
  setData("inscEstadual", ui->lineEditInscEstadual->text());
  setData("tel", ui->lineEditTel->text());
  setData("tel2", ui->lineEditTel2->text());
  setData("valorMinimoFrete", ui->doubleSpinBoxValorMinimoFrete->value());
  setData("porcentagemFrete", ui->doubleSpinBoxPorcFrete->value());
  setData("custoTransporteTon", ui->doubleSpinBoxCustoTransportePorTon->value());
  setData("custoTransporte1", ui->doubleSpinBoxCustoTransporte2Ton->value());
  setData("custoTransporte2", ui->doubleSpinBoxCustoTransporteAcima2Ton->value());
  setData("custoFuncionario", ui->doubleSpinBoxCustoFuncionario->value());
}

void CadastroLoja::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();

  ui->pushButtonRemoverEnd->hide();

  ui->pushButtonRemoverConta->hide();

  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabParametros), false);
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

void CadastroLoja::on_pushButtonAdicionarEnd_clicked() {
  if (cadastrarEndereco()) { novoEndereco(); }
}

void CadastroLoja::on_pushButtonAtualizarEnd_clicked() {
  if (cadastrarEndereco(Tipo::Atualizar)) { novoEndereco(); }
}

void CadastroLoja::on_pushButtonRemoverEnd_clicked() {
  if (removeBox() == QMessageBox::Yes) {
    setDataEnd("desativado", true);

    modelEnd.submitAll();

    novoEndereco();
  }
}

void CadastroLoja::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idLoja = " + data("idLoja").toString() + (checked ? "" : " AND desativado = FALSE"));

  modelEnd.select();
}

bool CadastroLoja::cadastrarEndereco(const Tipo tipoEndereco) {
  if (not ui->lineEditCEP->isValid()) {
    throw RuntimeError("CEP inválido!", this);
    ui->lineEditCEP->setFocus();
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

  CepCompleter cc;
  cc.buscaCEP(cep, this);

  ui->lineEditUF->setText(cc.getUf());
  ui->lineEditCidade->setText(cc.getCidade());
  ui->lineEditLogradouro->setText(cc.getEndereco());
  ui->lineEditBairro->setText(cc.getBairro());
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

  modelEnd.select();

  // -------------------------------------------------------------------------

  const bool inativosConta = ui->checkBoxMostrarInativosConta->isChecked();
  modelConta.setFilter("idLoja = " + primaryId + (inativosConta ? "" : " AND desativado = FALSE"));

  modelConta.select();

  // -------------------------------------------------------------------------

  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabParametros), true);

  // -------------------------------------------------------------------------

  if (data("idLoja").toInt() == 1) { ui->groupBoxCadastro->setDisabled(true); }

  return true;
}

void CadastroLoja::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Loja cadastrada com sucesso!", this); }

void CadastroLoja::cadastrar() {
  try {
    qApp->startTransaction("CadastroLoja::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    // -------------------------------------------------------------------------

    setForeignKey(modelEnd);

    modelEnd.submitAll();

    // -------------------------------------------------------------------------

    setForeignKey(modelConta);

    modelConta.submitAll();

    qApp->endTransaction();

    backupEndereco.clear();
    backupConta.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelEnd.setFilter(primaryKey + " = '" + primaryId + "'");
  } catch (std::exception &e) {
    qApp->rollbackTransaction();
    model.select();
    modelEnd.select();
    modelConta.select();

    for (auto &record : backupEndereco) { modelEnd.insertRecord(-1, record); }
    for (auto &record : backupConta) { modelConta.insertRecord(-1, record); }

    throw;
  }
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

bool CadastroLoja::cadastrarConta(const Tipo tipoConta) {
  if (ui->lineEditBanco->text().isEmpty()) { throw RuntimeError("Banco inválido!", this); }

  if (tipoConta == Tipo::Cadastrar) { currentRowConta = modelConta.insertRowAtEnd(); }

  modelConta.setData(currentRowConta, "banco", ui->lineEditBanco->text());
  modelConta.setData(currentRowConta, "agencia", ui->lineEditAgencia->text());
  modelConta.setData(currentRowConta, "conta", ui->lineEditConta->text());

  if (tipoConta == Tipo::Cadastrar) { backupConta.append(modelConta.record(currentRowConta)); }

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

void CadastroLoja::on_pushButtonAdicionarConta_clicked() {
  // TODO: colocar flag para indicar que conta é virtual?
  if (cadastrarConta()) { novaConta(); }
}

void CadastroLoja::on_pushButtonAtualizarConta_clicked() {
  if (cadastrarConta(Tipo::Atualizar)) { novaConta(); }
}

void CadastroLoja::on_pushButtonRemoverConta_clicked() {
  if (removeBox() == QMessageBox::Yes) {
    modelConta.setData(mapperConta.currentIndex(), "desativado", true);

    modelConta.submitAll();

    novaConta();
  }
}

void CadastroLoja::on_checkBoxMostrarInativosConta_clicked(bool checked) {
  if (currentRow == -1) { return; }

  modelConta.setFilter("idLoja = " + data("idLoja").toString() + (checked ? "" : " AND desativado = FALSE"));

  modelConta.select();
}
