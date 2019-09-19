#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QTransposeProxyModel>

#include "application.h"
#include "cadastrousuario.h"
#include "checkboxdelegate.h"
#include "searchdialog.h"
#include "ui_cadastrousuario.h"
#include "usersession.h"

CadastroUsuario::CadastroUsuario(QWidget *parent) : RegisterDialog("usuario", "idUsuario", parent), ui(new Ui::CadastroUsuario) {
  ui->setupUi(this);

  ui->labelEspecialidade->hide();
  ui->comboBoxEspecialidade->hide();

  connect(ui->comboBoxTipo, &QComboBox::currentTextChanged, this, &CadastroUsuario::on_comboBoxTipo_currentTextChanged);
  connect(ui->lineEditUser, &QLineEdit::textEdited, this, &CadastroUsuario::on_lineEditUser_textEdited);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonRemover_clicked);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") { ui->table->hide(); }

  setupTables();
  fillCombobox();
  setupMapper();
  newRegister();

  sdUsuario = SearchDialog::usuario(this);
  connect(sdUsuario, &SearchDialog::itemSelected, this, &CadastroUsuario::viewRegisterById);
}

CadastroUsuario::~CadastroUsuario() { delete ui; }

void CadastroUsuario::setupTables() {
  modelPermissoes.setTable("usuario_has_permissao");

  modelPermissoes.setHeaderData("view_tab_orcamento", "Ver Orçamentos?");
  modelPermissoes.setHeaderData("view_tab_venda", "Ver Vendas?");
  modelPermissoes.setHeaderData("view_tab_compra", "Ver Compras?");
  modelPermissoes.setHeaderData("view_tab_logistica", "Ver Logística?");
  modelPermissoes.setHeaderData("view_tab_nfe", "Ver NFe?");
  modelPermissoes.setHeaderData("view_tab_estoque", "Ver Estoque?");
  modelPermissoes.setHeaderData("view_tab_financeiro", "Ver Financeiro?");
  modelPermissoes.setHeaderData("view_tab_relatorio", "Ver Relatório?");
  modelPermissoes.setHeaderData("view_tab_rh", "Ver RH?");
}

void CadastroUsuario::modificarUsuario() {
  ui->pushButtonBuscar->hide();
  ui->pushButtonNovoCad->hide();
  ui->pushButtonRemover->hide();

  ui->lineEditNome->setDisabled(true);
  ui->lineEditUser->setDisabled(true);
  ui->comboBoxLoja->setDisabled(true);
  ui->comboBoxTipo->setDisabled(true);
}

bool CadastroUsuario::verifyFields() { // TODO: deve marcar uma loja?
  const auto children = ui->tab->findChildren<QLineEdit *>();

  for (const auto &line : children) {
    if (not verifyRequiredField(line)) { return false; }
  }

  if (ui->lineEditPasswd->text() != ui->lineEditPasswd_2->text()) {
    qApp->enqueueError("As senhas não batem!", this);
    ui->lineEditPasswd->setFocus();
    return false;
  }

  return true;
}

void CadastroUsuario::clearFields() { RegisterDialog::clearFields(); }

void CadastroUsuario::setupMapper() {
  addMapping(ui->comboBoxLoja, "idLoja", "currentValue");
  addMapping(ui->comboBoxTipo, "tipo");
  addMapping(ui->lineEditEmail, "email");
  addMapping(ui->lineEditNome, "nome");
  addMapping(ui->lineEditUser, "user");
}

void CadastroUsuario::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();
}

void CadastroUsuario::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

bool CadastroUsuario::savingProcedures() {
  if (not setData("nome", ui->lineEditNome->text())) { return false; }
  if (not setData("idLoja", ui->comboBoxLoja->getCurrentValue())) { return false; }
  if (not setData("tipo", ui->comboBoxTipo->currentText())) { return false; }
  if (not setData("user", ui->lineEditUser->text())) { return false; }
  if (not setData("email", ui->lineEditEmail->text())) { return false; }
  if (not setData("user", ui->lineEditUser->text())) { return false; }

  // NOTE: change this when upgrading for MySQL 8
  if (ui->lineEditPasswd->text() != "********") {
    QSqlQuery query;

    if (not query.exec("SELECT PASSWORD('" + ui->lineEditPasswd->text() + "')") or not query.first()) { return false; }

    if (not setData("passwd", query.value(0))) { return false; }
  }

  if (ui->comboBoxTipo->currentText() == "VENDEDOR ESPECIAL" and not setData("especialidade", ui->comboBoxEspecialidade->currentText().left(1).toInt())) { return false; }

  return true;
}

bool CadastroUsuario::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  ui->lineEditPasswd->setText("********");
  ui->lineEditPasswd_2->setText("********");

  modelPermissoes.setFilter("idUsuario = " + data("idUsuario").toString());

  if (not modelPermissoes.select()) { return false; }

  auto *transpose = new QTransposeProxyModel(this);
  transpose->setSourceModel(&modelPermissoes);
  ui->table->setModel(transpose);

  ui->table->hideRow(0);                                  // idUsuario
  ui->table->hideRow(ui->table->model()->rowCount() - 1); // created
  ui->table->hideRow(ui->table->model()->rowCount() - 2); // lastUpdated

  ui->table->setItemDelegate(new CheckBoxDelegate(this));

  ui->table->horizontalHeader()->setVisible(false);

  for (int row = 0; row < ui->table->model()->rowCount(); ++row) { ui->table->openPersistentEditor(row, 0); }

  if (ui->comboBoxTipo->currentText() == "VENDEDOR ESPECIAL") { ui->comboBoxEspecialidade->setCurrentIndex(data("especialidade").toString().left(1).toInt()); }

  return true;
}

void CadastroUsuario::fillCombobox() {
  QSqlQuery query;

  if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE ORDER BY descricao")) { return; }

  while (query.next()) { ui->comboBoxLoja->addItem(query.value("descricao").toString(), query.value("idLoja")); }

  ui->comboBoxLoja->setCurrentValue(UserSession::idLoja());
}

void CadastroUsuario::on_pushButtonCadastrar_clicked() { save(); }

void CadastroUsuario::on_pushButtonAtualizar_clicked() { save(); }

void CadastroUsuario::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroUsuario::on_pushButtonRemover_clicked() { remove(); }

void CadastroUsuario::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdUsuario->show();
}

bool CadastroUsuario::cadastrar() {
  const bool success = [&] {
    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    if (not savingProcedures()) { return false; }

    if (not columnsToUpper(model, currentRow)) { return false; }

    if (not model.submitAll()) { return false; }

    primaryId = (tipo == Tipo::Atualizar) ? data(currentRow, primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

    if (tipo == Tipo::Cadastrar) {
      QFile file("mysql.txt");

      if (not file.open(QFile::ReadOnly)) { return qApp->enqueueError(false, "Erro lendo mysql.txt: " + file.errorString()); }

      const QString password = file.readAll();

      // NOTE: those query's below commit transaction so rollback won't work
      QSqlQuery query;
      query.prepare("CREATE USER :user@'%' IDENTIFIED BY '" + password + "'");
      query.bindValue(":user", ui->lineEditUser->text().toLower());

      if (not query.exec()) { return qApp->enqueueError(false, "Erro criando usuário do banco de dados: " + query.lastError().text(), this); }

      query.prepare("GRANT ALL PRIVILEGES ON *.* TO :user@'%' WITH GRANT OPTION");
      query.bindValue(":user", ui->lineEditUser->text().toLower());

      if (not query.exec()) { return qApp->enqueueError(false, "Erro guardando privilégios do usuário do banco de dados: " + query.lastError().text(), this); }

      if (not QSqlQuery("FLUSH PRIVILEGES").exec()) { return false; }

      // -------------------------------------------------------------------------

      const int row2 = modelPermissoes.insertRowAtEnd();

      if (not modelPermissoes.setData(row2, "idUsuario", primaryId)) { return false; }
      if (not modelPermissoes.setData(row2, "view_tab_orcamento", true)) { return false; }
      if (not modelPermissoes.setData(row2, "view_tab_venda", true)) { return false; }
      if (not modelPermissoes.setData(row2, "view_tab_relatorio", true)) { return false; }
    }

    if (not modelPermissoes.submitAll()) { return false; }

    return true;
  }();

  if (not success) {
    qApp->rollbackTransaction();
    void(model.select());
    void(modelPermissoes.select());
  }

  return success;
}

void CadastroUsuario::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Usuário cadastrado com sucesso!", this); }

void CadastroUsuario::on_lineEditUser_textEdited(const QString &text) {
  QSqlQuery query;
  query.prepare("SELECT idUsuario FROM usuario WHERE user = :user");
  query.bindValue(":user", text);

  if (not query.exec()) { return qApp->enqueueError("Erro buscando usuário: " + query.lastError().text(), this); }

  if (query.first()) { return qApp->enqueueError("Nome de usuário já existe!", this); }
}

void CadastroUsuario::on_comboBoxTipo_currentTextChanged(const QString &text) {
  if (text == "VENDEDOR ESPECIAL") {
    ui->labelEspecialidade->show();
    ui->comboBoxEspecialidade->show();
  } else {
    ui->labelEspecialidade->hide();
    ui->comboBoxEspecialidade->hide();
  }
}

// TODO: 1colocar permissoes padroes para cada tipo de usuario
// TODO: colocar uma coluna 'ultimoAcesso' no BD (para saber quais usuarios nao estao mais ativos e desativar depois de x dias)
// FIXME: quando o usuario é alterado o usuario do MySql não é, fazendo com que o login não funcione mais
