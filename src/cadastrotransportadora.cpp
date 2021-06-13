#include "cadastrotransportadora.h"
#include "ui_cadastrotransportadora.h"

#include "application.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "searchdialog.h"
#include "user.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

CadastroTransportadora::CadastroTransportadora(QWidget *parent) : RegisterAddressDialog("transportadora", "idTransportadora", parent), ui(new Ui::CadastroTransportadora) {
  ui->setupUi(this);

  connectLineEditsToDirty();
  setupUi();
  setupTables();
  setupMapper();
  newRegister();

  if (not User::isAdministrativo()) {
    ui->pushButtonRemover->setDisabled(true);
    ui->pushButtonRemoverEnd->setDisabled(true);
  }

  setConnections();
}

CadastroTransportadora::~CadastroTransportadora() { delete ui; }

void CadastroTransportadora::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(sdTransportadora, &SearchDialog::itemSelected, this, &CadastroTransportadora::viewRegisterById, connectionType);
  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroTransportadora::on_checkBoxMostrarInativos_clicked, connectionType);
  connect(ui->checkBoxMostrarInativosVeiculo, &QCheckBox::toggled, this, &CadastroTransportadora::on_checkBoxMostrarInativosVeiculo_toggled, connectionType);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroTransportadora::on_lineEditCEP_textChanged, connectionType);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroTransportadora::on_lineEditCNPJ_textEdited, connectionType);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAdicionarEnd_clicked, connectionType);
  connect(ui->pushButtonAdicionarVeiculo, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAdicionarVeiculo_clicked, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAtualizar_clicked, connectionType);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAtualizarEnd_clicked, connectionType);
  connect(ui->pushButtonAtualizarVeiculo, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonAtualizarVeiculo_clicked, connectionType);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonNovoCad_clicked, connectionType);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonRemover_clicked, connectionType);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonRemoverEnd_clicked, connectionType);
  connect(ui->pushButtonRemoverVeiculo, &QPushButton::clicked, this, &CadastroTransportadora::on_pushButtonRemoverVeiculo_clicked, connectionType);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroTransportadora::on_tableEndereco_clicked, connectionType);
  connect(ui->tableVeiculo, &TableView::clicked, this, &CadastroTransportadora::on_tableVeiculo_clicked, connectionType);
}

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

  ui->tableVeiculo->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableVeiculo->setPersistentColumns({"desativado"});

  // -------------------------------------------------------------------------

  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idTransportadora");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroTransportadora::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  novoVeiculo();
  setupUi();
}

void CadastroTransportadora::verifyFields() {
  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { verifyRequiredField(*line); }
}

void CadastroTransportadora::savingProcedures() {
  setData("cnpj", ui->lineEditCNPJ->text());
  setData("razaoSocial", ui->lineEditRazaoSocial->text());
  setData("nomeFantasia", ui->lineEditNomeFantasia->text());
  setData("inscEstadual", ui->lineEditInscEstadual->text());
  setData("tel", ui->lineEditTel->text());
  setData("antt", ui->lineEditANTT->text());
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
  mapperVeiculo.addMapping(ui->lineEditUfPlaca, modelVeiculo.fieldIndex("ufPlaca"));

  mapperEnd.addMapping(ui->comboBoxTipoEnd, modelEnd.fieldIndex("descricao"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditComplemento, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNumero, modelEnd.fieldIndex("numero"));
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

// TODO: antes de remover verificar se existem agendamentos em aberto e avisar usuario
// TODO: ao desativar uma transportadora desativar cada um de seus veiculos senao eles continuam a aparecer no SearchDialog/Calendario
void CadastroTransportadora::on_pushButtonRemover_clicked() { remove(); }

void CadastroTransportadora::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdTransportadora->show();
}

void CadastroTransportadora::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(text) ? "background-color: rgb(255, 255, 127);color: rgb(0, 190, 0)" : "background-color: rgb(255, 255, 127);color: rgb(255, 0, 0)");
}

void CadastroTransportadora::on_pushButtonAdicionarEnd_clicked() {
  if (cadastrarEndereco()) { novoEndereco(); }
}

void CadastroTransportadora::on_pushButtonAtualizarEnd_clicked() {
  if (cadastrarEndereco(Tipo::Atualizar)) { novoEndereco(); }
}

void CadastroTransportadora::on_pushButtonRemoverEnd_clicked() {
  if (removeBox() == QMessageBox::Yes) {
    setDataEnd("desativado", true);

    modelEnd.submitAll();

    novoEndereco();
  }
}

void CadastroTransportadora::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idTransportadora = " + data("idTransportadora").toString() + (checked ? "" : " AND desativado = FALSE"));
}

bool CadastroTransportadora::cadastrarEndereco(const Tipo tipoEndereco) {
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
  ui->lineEditComplemento->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNumero->clear();
  ui->lineEditUF->clear();
}

void CadastroTransportadora::on_lineEditCEP_textChanged(const QString &cep) {
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

void CadastroTransportadora::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  currentRowEnd = index.row();

  const bool desativado = dataEnd("desativado").toBool();

  ui->pushButtonAtualizarEnd->setEnabled(desativado);
  ui->pushButtonRemoverEnd->setDisabled(desativado);

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();

  //------------------------------------------
  disconnect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroTransportadora::on_lineEditCEP_textChanged);
  mapperEnd.setCurrentModelIndex(index);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroTransportadora::on_lineEditCEP_textChanged);
  //------------------------------------------
}

void CadastroTransportadora::setupUi() {
  // dados
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditANTT->setInputMask("99999999;_");
  ui->lineEditPlaca->setInputMask("AAA-9N99;_");
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
  ui->lineEditUfPlaca->setInputMask(">AA;_");

  // endereco
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");

  ui->lineEditCarga->setValidator(new QIntValidator(this));
}

bool CadastroTransportadora::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idTransportadora = " + data("idTransportadora").toString() + (inativos ? "" : " AND desativado = FALSE"));

  modelEnd.select();

  //---------------------------------------------------

  const bool inativosVeiculo = ui->checkBoxMostrarInativosVeiculo->isChecked();
  modelVeiculo.setFilter("idTransportadora = " + data("idTransportadora").toString() + (inativosVeiculo ? "" : " AND desativado = FALSE"));

  modelVeiculo.select();

  return true;
}

void CadastroTransportadora::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Transportadora cadastrada com sucesso!", this); }

void CadastroTransportadora::cadastrarVeiculo(const Tipo tipoVeiculo) {
  if (ui->lineEditModelo->text().isEmpty()) { throw RuntimeError("Um campo obrigatório não foi preenchido:\nModelo"); }
  if (ui->lineEditCarga->text().isEmpty()) { throw RuntimeError("Um campo obrigatório não foi preenchido:\nCarga (Kg)"); }

  if (tipoVeiculo == Tipo::Cadastrar) { currentRowVeiculo = modelVeiculo.insertRowAtEnd(); }

  modelVeiculo.setData(currentRowVeiculo, "modelo", ui->lineEditModelo->text());
  modelVeiculo.setData(currentRowVeiculo, "capacidade", ui->lineEditCarga->text().toInt());

  if (ui->lineEditPlaca->text() != "-") { modelVeiculo.setData(currentRowVeiculo, "placa", ui->lineEditPlaca->text()); }

  modelVeiculo.setData(currentRowVeiculo, "ufPlaca", ui->lineEditUfPlaca->text());

  if (tipoVeiculo == Tipo::Cadastrar) { backupVeiculo.append(modelVeiculo.record(currentRowVeiculo)); }

  isDirty = true;

  if (tipo == Tipo::Atualizar) { save(true); }
}

void CadastroTransportadora::on_pushButtonAdicionarVeiculo_clicked() {
  cadastrarVeiculo();
  novoVeiculo();
}

void CadastroTransportadora::on_pushButtonAtualizarVeiculo_clicked() {
  cadastrarVeiculo(Tipo::Atualizar);
  novoVeiculo();
}

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
  if (removeBox() == QMessageBox::Yes) {
    modelVeiculo.setData(currentRowVeiculo, "desativado", true);

    modelVeiculo.submitAll();

    novoVeiculo();
  }
}

void CadastroTransportadora::cadastrar() {
  try {
    qApp->startTransaction("CadastroTransportadora::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    // -------------------------------------------------------------------------

    setForeignKey(modelEnd);

    modelEnd.submitAll();

    // -------------------------------------------------------------------------

    setForeignKey(modelVeiculo);

    modelVeiculo.submitAll();

    qApp->endTransaction();

    backupEndereco.clear();
    backupVeiculo.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelEnd.setFilter(primaryKey + " = '" + primaryId + "'");

    modelVeiculo.setFilter(primaryKey + " = '" + primaryId + "'");
  } catch (std::exception &) {
    qApp->rollbackTransaction();
    model.select();
    modelEnd.select();
    modelVeiculo.select();

    for (auto &record : backupEndereco) { modelEnd.insertRecord(-1, record); }
    for (auto &record : backupVeiculo) { modelVeiculo.insertRecord(-1, record); }

    throw;
  }
}

void CadastroTransportadora::on_checkBoxMostrarInativosVeiculo_toggled(bool checked) {
  if (currentRow == -1) { return; }

  modelVeiculo.setFilter("idTransportadora = " + data("idTransportadora").toString() + (checked ? "" : " AND desativado = FALSE"));
}

void CadastroTransportadora::on_tableVeiculo_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoVeiculo(); }

  currentRowVeiculo = index.row();
  mapperVeiculo.setCurrentModelIndex(index);

  ui->pushButtonAtualizarVeiculo->show();
  ui->pushButtonAdicionarVeiculo->hide();
  ui->pushButtonRemoverVeiculo->show();
}

bool CadastroTransportadora::newRegister() {
  if (not RegisterAddressDialog::newRegister()) { return false; }

  modelVeiculo.setFilter("0");

  return true;
}

void CadastroTransportadora::verificaEndereco() {
  RegisterAddressDialog::verificaEndereco(ui->lineEditCidade->text(), ui->lineEditUF->text());

  if (not ui->lineEditCEP->isValid()) { throw RuntimeError("CEP inválido!", this); }

  if (ui->lineEditNumero->text().isEmpty()) { throw RuntimeError("Número vazio! Se necessário coloque \"S/N\"!", this); }

  if (ui->lineEditCidade->text().isEmpty()) { throw RuntimeError("Cidade vazio!", this); }

  if (ui->lineEditUF->text().isEmpty()) { throw RuntimeError("UF vazio!", this); }
}
