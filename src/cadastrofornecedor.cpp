#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "cadastrofornecedor.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "searchdialog.h"
#include "ui_cadastrofornecedor.h"
#include "usersession.h"

CadastroFornecedor::CadastroFornecedor(QWidget *parent) : RegisterAddressDialog("fornecedor", "idFornecedor", parent), ui(new Ui::CadastroFornecedor) {
  ui->setupUi(this);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  setupUi();
  setupTables();
  setupMapper();
  newRegister();

  sdFornecedor = SearchDialog::fornecedor(this);
  connect(sdFornecedor, &SearchDialog::itemSelected, this, &CadastroFornecedor::viewRegisterById);

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") {
    ui->pushButtonRemover->setDisabled(true);
    ui->pushButtonRemoverEnd->setDisabled(true);
  }

  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroFornecedor::on_checkBoxMostrarInativos_clicked);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroFornecedor::on_lineEditCEP_textChanged);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroFornecedor::on_lineEditCNPJ_textEdited);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroFornecedor::on_lineEditContatoCPF_textEdited);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonAdicionarEnd_clicked);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonAtualizarEnd_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonRemover_clicked);
  connect(ui->pushButtonRemoverEnd, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonRemoverEnd_clicked);
  connect(ui->pushButtonValidade, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonValidade_clicked);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroFornecedor::on_tableEndereco_clicked);
}

CadastroFornecedor::~CadastroFornecedor() { delete ui; }

void CadastroFornecedor::setupTables() {
  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idFornecedor");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(this, true));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroFornecedor::setupUi() {
  ui->lineEditContatoCPF->setInputMask("999.999.999-99;_");
  ui->lineEditContatoRG->setInputMask("99.999.999-9;_");
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");
}

void CadastroFornecedor::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComp->clear();
  ui->lineEditEndereco->clear();
  ui->lineEditNro->clear();
  ui->lineEditUF->clear();
}

void CadastroFornecedor::novoEndereco() {
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonRemoverEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

bool CadastroFornecedor::verifyFields() {
  const auto children = ui->frameCadastro->findChildren<QLineEdit *>();

  for (const auto &line : children) {
    if (not verifyRequiredField(line)) { return false; }
  }

  if (ui->lineEditCNPJ->styleSheet().contains("color: rgb(255, 0, 0)")) { return qApp->enqueueError(false, "CNPJ inválido!", this); }

  return true;
}

bool CadastroFornecedor::savingProcedures() {
  if (not setData("razaoSocial", ui->lineEditFornecedor->text())) { return false; }
  if (not setData("nomeFantasia", ui->lineEditNomeFantasia->text())) { return false; }
  if (not setData("contatoNome", ui->lineEditContatoNome->text())) { return false; }
  if (not setData("contatoCPF", ui->lineEditContatoCPF->text().remove(".").remove("-"))) { return false; }
  if (not setData("contatoApelido", ui->lineEditContatoApelido->text())) { return false; }
  if (not setData("contatoRG", ui->lineEditContatoRG->text().remove(".").remove("-"))) { return false; }
  if (not setData("cnpj", ui->lineEditCNPJ->text().remove(".").remove("/").remove("-"))) { return false; }
  if (not setData("inscEstadual", ui->lineEditInscEstadual->text())) { return false; }
  if (not setData("tel", ui->lineEditTel_Res->text())) { return false; }
  if (not setData("telCel", ui->lineEditTel_Cel->text())) { return false; }
  if (not setData("telCom", ui->lineEditTel_Com->text())) { return false; }
  if (not setData("nextel", ui->lineEditNextel->text())) { return false; }
  if (not setData("email", ui->lineEditEmail->text())) { return false; }
  if (not setData("aliquotaSt", ui->doubleSpinBoxAliquotaSt->value())) { return false; }
  if (not setData("st", ui->comboBoxSt->currentText())) { return false; }
  if (not setData("comissao1", ui->doubleSpinBoxComissao->value())) { return false; }
  if (not setData("especialidade", ui->comboBoxEspecialidade->currentText().left(1).toInt())) { return false; }

  return true;
}

void CadastroFornecedor::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  setupUi();
}

void CadastroFornecedor::setupMapper() {
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoCPF, "contatoCPF");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditEmail, "email");
  addMapping(ui->lineEditFornecedor, "razaoSocial");
  addMapping(ui->lineEditIdNextel, "idNextel");
  addMapping(ui->lineEditInscEstadual, "inscEstadual");
  addMapping(ui->lineEditNextel, "nextel");
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditTel_Cel, "telCel");
  addMapping(ui->lineEditTel_Com, "telCom");
  addMapping(ui->lineEditTel_Res, "tel");
  addMapping(ui->checkBoxRepresentacao, "representacao", "checked");
  addMapping(ui->comboBoxSt, "st");
  addMapping(ui->doubleSpinBoxAliquotaSt, "aliquotaSt");
  addMapping(ui->doubleSpinBoxComissao, "comissao1");

  mapperEnd.addMapping(ui->comboBoxTipoEnd, modelEnd.fieldIndex("descricao"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditComp, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditEndereco, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNro, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroFornecedor::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();
  ui->pushButtonValidade->hide();

  ui->pushButtonRemoverEnd->hide();
}

void CadastroFornecedor::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

bool CadastroFornecedor::cadastrar() {
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

  return modelEnd.submitAll();
}

void CadastroFornecedor::on_pushButtonCadastrar_clicked() { save(); }

void CadastroFornecedor::on_pushButtonAtualizar_clicked() { save(); }

void CadastroFornecedor::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdFornecedor->show();
}

void CadastroFornecedor::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroFornecedor::on_pushButtonRemover_clicked() { remove(); }

void CadastroFornecedor::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(QString(text).remove(".").remove("/").remove("-")) ? "background-color: rgb(255, 255, 127);color: rgb(0, 190, 0)"
                                                                                                : "background-color: rgb(255, 255, 127);color: rgb(255, 0, 0)");
}

void CadastroFornecedor::on_lineEditContatoCPF_textEdited(const QString &text) {
  ui->lineEditContatoCPF->setStyleSheet(validaCPF(QString(text).remove(".").remove("-")) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)");
}

void CadastroFornecedor::on_pushButtonAdicionarEnd_clicked() { cadastrarEndereco() ? novoEndereco() : qApp->enqueueError("Não foi possível cadastrar este endereço.", this); }

bool CadastroFornecedor::cadastrarEndereco(const Tipo tipo) { //TODO: V688 http://www.viva64.com/en/V688 The 'tipo' function argument possesses the same name as one of the class members, which can result in a confusion.bool CadastroFornecedor::cadastrarEndereco(const Tipo tipo) {
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
  if (not setDataEnd("cep", ui->lineEditCEP->text())) { return false; }
  if (not setDataEnd("logradouro", ui->lineEditEndereco->text())) { return false; }
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

void CadastroFornecedor::on_lineEditCEP_textChanged(const QString &cep) {
  if (not ui->lineEditCEP->isValid()) { return; }

  ui->lineEditNro->clear();
  ui->lineEditComp->clear();

  if (CepCompleter cc; cc.buscaCEP(cep)) {
    ui->lineEditUF->setText(cc.getUf());
    ui->lineEditCidade->setText(cc.getCidade());
    ui->lineEditEndereco->setText(cc.getEndereco());
    ui->lineEditBairro->setText(cc.getBairro());
  }
}

void CadastroFornecedor::on_pushButtonAtualizarEnd_clicked() { cadastrarEndereco(Tipo::Atualizar) ? novoEndereco() : qApp->enqueueError("Não foi possível atualizar este endereço!", this); }

void CadastroFornecedor::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonRemoverEnd->show();
  mapperEnd.setCurrentModelIndex(index);
  currentRowEnd = index.row();
}

bool CadastroFornecedor::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idFornecedor = " + data("idFornecedor").toString() + (inativos ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return false; }

  //---------------------------------------------------

  ui->pushButtonValidade->show();

  ui->comboBoxEspecialidade->setCurrentIndex(data("especialidade").toString().left(1).toInt());

  return true;
}

void CadastroFornecedor::on_pushButtonRemoverEnd_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not setDataEnd("desativado", true)) { return; }

    if (not modelEnd.submitAll()) { return; }

    novoEndereco();
  }
}

void CadastroFornecedor::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Fornecedor cadastrado com sucesso!", this); }

bool CadastroFornecedor::ajustarValidade(const int novaValidade) {
  const QString fornecedor = data("razaoSocial").toString();

  QSqlQuery query;
  query.prepare("UPDATE produto SET validade = :novaValidade, descontinuado = 0 WHERE fornecedor = :fornecedor AND validade = :oldValidade");
  query.bindValue(":novaValidade", QDate::currentDate().addDays(novaValidade));
  query.bindValue(":fornecedor", fornecedor);
  query.bindValue(":oldValidade", data("validadeProdutos"));

  if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando validade nos produtos: " + query.lastError().text(), this); }

  query.prepare(
      "UPDATE produto_has_preco php, produto p SET php.validadeFim = :novaValidade, expirado = FALSE WHERE php.idProduto = p.idProduto AND php.preco = p.precoVenda AND p.fornecedor = :fornecedor");
  query.bindValue(":novaValidade", QDate::currentDate().addDays(novaValidade));
  query.bindValue(":fornecedor", fornecedor);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando validade no preço/produto: " + query.lastError().text(), this); }

  query.prepare("UPDATE fornecedor SET validadeProdutos = :novaValidade WHERE razaoSocial = :fornecedor");
  query.bindValue(":novaValidade", QDate::currentDate().addDays(novaValidade));
  query.bindValue(":fornecedor", fornecedor);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando validade no fornecedor: " + query.lastError().text(), this); }

  if (not query.exec("CALL invalidar_produtos_expirados()")) { return qApp->enqueueError(false, "Erro executando InvalidarExpirados: " + query.lastError().text(), this); }

  return true;
}

void CadastroFornecedor::on_pushButtonValidade_clicked() {
  bool ok;

  const int novaValidade = QInputDialog::getInt(this, "Validade", "Quantos dias de validade para os produtos: ", 0, -1, 1000, 1, &ok);

  if (not ok) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not ajustarValidade(novaValidade)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Validade alterada com sucesso!", this);
}

void CadastroFornecedor::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idFornecedor = " + data("idFornecedor").toString() + (checked ? "" : " AND desativado = FALSE"));

  if (not modelEnd.select()) { return; }
}

// TODO: 5criar um tipo 'serviço' para atelier (fluxo pagamento é loja mas segue como representacao)
// TODO: 5poder alterar na tela 'comissao'
