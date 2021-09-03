#include "cadastrousuario.h"
#include "ui_cadastrousuario.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "file.h"
#include "user.h"

#include <QDebug>
#include <QSqlError>
#include <QTransposeProxyModel>

CadastroUsuario::CadastroUsuario(QWidget *parent) : RegisterDialog("usuario", "idUsuario", parent), ui(new Ui::CadastroUsuario) {
  ui->setupUi(this);

  connectLineEditsToDirty();
  fillComboBoxLoja();
  setupTables();
  setupMapper();
  newRegister();

  ui->labelEspecialidade->hide();
  ui->comboBoxEspecialidade->hide();

  if (not User::isAdmin()) { ui->table->hide(); }

  setConnections();
}

CadastroUsuario::~CadastroUsuario() { delete ui; }

void CadastroUsuario::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(sdUsuario, &SearchDialog::itemSelected, this, &CadastroUsuario::viewRegisterById, connectionType);
  connect(ui->comboBoxTipo, &QComboBox::currentTextChanged, this, &CadastroUsuario::on_comboBoxTipo_currentTextChanged, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonAtualizar_clicked, connectionType);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonDesativar, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonDesativar_clicked, connectionType);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroUsuario::on_pushButtonNovoCad_clicked, connectionType);
}

void CadastroUsuario::setupTables() {
  modelPermissoes.setTable("usuario_has_permissao");

  modelPermissoes.setHeaderData("view_tab_orcamento", "Ver Orçamentos?");
  modelPermissoes.setHeaderData("view_tab_venda", "Ver Vendas?");
  modelPermissoes.setHeaderData("view_tab_compra", "Ver Compras?");
  modelPermissoes.setHeaderData("view_tab_logistica", "Ver Logística?");
  modelPermissoes.setHeaderData("view_tab_nfe", "Ver NFe?");
  modelPermissoes.setHeaderData("view_tab_estoque", "Ver Estoque?");
  modelPermissoes.setHeaderData("view_tab_galpao", "Ver Galpão?");
  modelPermissoes.setHeaderData("view_tab_financeiro", "Ver Financeiro?");
  modelPermissoes.setHeaderData("view_tab_relatorio", "Ver Relatório?");
  modelPermissoes.setHeaderData("view_tab_grafico", "Ver Gráfico?");
  modelPermissoes.setHeaderData("view_tab_rh", "Ver RH?");

  modelPermissoes.setHeaderData("webdav_documentos", "Rede - Documentos");
  modelPermissoes.setHeaderData("webdav_compras", "Rede - Compras");
  modelPermissoes.setHeaderData("webdav_financeiro", "Rede - Financeiro");
  modelPermissoes.setHeaderData("webdav_obras", "Rede - Obras");
  modelPermissoes.setHeaderData("webdav_logistica", "Rede - Logística");

  auto *transposeProxyModel = new QTransposeProxyModel(this);
  transposeProxyModel->setSourceModel(&modelPermissoes);

  ui->table->setModel(transposeProxyModel);

  ui->table->hideRow(0);                                  // idUsuario
  ui->table->hideRow(ui->table->model()->rowCount() - 2); // created
  ui->table->hideRow(ui->table->model()->rowCount() - 1); // lastUpdated

  for (int row = 1; row < modelPermissoes.columnCount() - 2; ++row) { ui->table->setItemDelegateForRow(row, new CheckBoxDelegate(this)); }

  ui->table->horizontalHeader()->hide();
}

void CadastroUsuario::modificarUsuario() {
  limitado = true;

  ui->pushButtonBuscar->hide();
  ui->pushButtonNovoCad->hide();
  ui->pushButtonDesativar->hide();

  ui->lineEditNome->setDisabled(true);
  ui->lineEditUser->setDisabled(true);
  ui->comboBoxLoja->setDisabled(true);
  ui->comboBoxTipo->setDisabled(true);
}

void CadastroUsuario::verifyFields() {
  // TODO: deve marcar uma loja?
  // quando não é marcado nenhuma loja o banco de dados salva como 1 - Alphaville

  if (tipo == Tipo::Cadastrar) { verificaUsuarioDisponivel(); }

  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { verifyRequiredField(*line); }

  if (ui->lineEditPasswd->text() != ui->lineEditPasswd_2->text()) { throw RuntimeError("As senhas não batem!"); }

  if (ui->comboBoxTipo->currentText().isEmpty()) { throw RuntimeError("Não escolheu o tipo de usuário!"); }
}

void CadastroUsuario::clearFields() { RegisterDialog::clearFields(); }

void CadastroUsuario::setupMapper() {
  addMapping(ui->comboBoxLoja, "idLoja", "currentValue");
  addMapping(ui->comboBoxTipo, "tipo");
  addMapping(ui->lineEditEmail, "email");
  addMapping(ui->lineEditNome, "nome");
  addMapping(ui->lineEditUser, "user");

  addMapping(ui->lineEditNomeBancario, "nomeBanco");
  addMapping(ui->lineEditCPFBancario, "cpfBanco");
  addMapping(ui->lineEditCNPJBancario, "cnpjBanco");
  addMapping(ui->lineEditBanco, "banco");
  addMapping(ui->lineEditAgencia, "agencia");
  addMapping(ui->lineEditCC, "cc");
  addMapping(ui->checkBoxPoupanca, "poupanca");
}

void CadastroUsuario::registerMode() {
  if (limitado) { return; }

  ui->pushButtonCadastrar->show();

  ui->pushButtonAtualizar->hide();
  ui->pushButtonDesativar->hide();
}

void CadastroUsuario::updateMode() {
  if (limitado) { return; }

  ui->pushButtonCadastrar->hide();

  ui->pushButtonAtualizar->show();
  ui->pushButtonDesativar->show();

  if (readOnly) {
    ui->pushButtonBuscar->hide();
    ui->pushButtonNovoCad->hide();
  }
}

void CadastroUsuario::savingProcedures() {
  setData("nome", ui->lineEditNome->text());
  setData("idLoja", ui->comboBoxLoja->getCurrentValue());
  setData("tipo", ui->comboBoxTipo->currentText());
  setData("user", ui->lineEditUser->text());
  setData("email", ui->lineEditEmail->text());
  setData("user", ui->lineEditUser->text());

  if (ui->lineEditPasswd->text() != "********") {
    SqlQuery query;

    if (not query.exec("SELECT SHA_PASSWORD('" + ui->lineEditPasswd->text() + "'), SHA1_PASSWORD('" + ui->lineEditPasswd->text() + "')")) {
      throw RuntimeException("Erro criptografando senha: " + query.lastError().text());
    }

    if (not query.first()) { throw RuntimeException("Erro criptografando senha!"); }

    setData("password", query.value(0), false);
    setData("passwd", query.value(1), false);
  }

  if (ui->comboBoxTipo->currentText() == "VENDEDOR ESPECIAL") { setData("especialidade", ui->comboBoxEspecialidade->currentText().left(1).toInt()); }

  // Dados bancários

  setData("nomeBanco", ui->lineEditNomeBancario->text());

  if (not ui->lineEditCPFBancario->text().remove(".").remove("-").isEmpty()) { setData("cpfBanco", ui->lineEditCPFBancario->text()); }
  if (not ui->lineEditCNPJBancario->text().remove(".").remove("/").remove("-").isEmpty()) { setData("cnpjBanco", ui->lineEditCNPJBancario->text()); }

  setData("banco", ui->lineEditBanco->text());

  if (not ui->lineEditAgencia->text().remove("-").isEmpty()) { setData("agencia", ui->lineEditAgencia->text()); }

  setData("cc", ui->lineEditCC->text());
  setData("poupanca", ui->checkBoxPoupanca->isChecked());
}

bool CadastroUsuario::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  ui->lineEditPasswd->setText("********");
  ui->lineEditPasswd_2->setText("********");

  modelPermissoes.setFilter("idUsuario = " + data("idUsuario").toString());

  modelPermissoes.select();

  for (int row = 0; row < ui->table->model()->rowCount(); ++row) { ui->table->openPersistentEditor(ui->table->model()->index(row, 0)); }

  if (ui->comboBoxTipo->currentText() == "VENDEDOR ESPECIAL") { ui->comboBoxEspecialidade->setCurrentIndex(data("especialidade").toString().left(1).toInt()); }

  return true;
}

void CadastroUsuario::fillComboBoxLoja() {
  SqlQuery query;

  if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE ORDER BY descricao")) { return; }

  while (query.next()) { ui->comboBoxLoja->addItem(query.value("descricao").toString(), query.value("idLoja")); }
}

void CadastroUsuario::on_pushButtonCadastrar_clicked() { save(); }

void CadastroUsuario::on_pushButtonAtualizar_clicked() { save(); }

void CadastroUsuario::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroUsuario::on_pushButtonDesativar_clicked() {
  // TODO: encerrar conexoes no SQL do usuario desativado

  remove();
}

void CadastroUsuario::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdUsuario->show();
}

void CadastroUsuario::cadastrar() {
  try {
    qApp->startTransaction("CadastroUsuario::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    if (tipo == Tipo::Cadastrar) { modelPermissoes.setData(0, "idUsuario", primaryId); }

    modelPermissoes.submitAll();

    qApp->endTransaction();

    if (tipo == Tipo::Cadastrar) { criarUsuarioMySQL(); }
  } catch (std::exception &) {
    qApp->rollbackTransaction();
    model.select();
    modelPermissoes.select();

    throw;
  }
}

void CadastroUsuario::criarUsuarioMySQL() {
  // TODO: verificar se usuário existe antes de criar

  File file("mysql.txt");

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo mysql.txt: " + file.errorString(), this); }

  const QString password = file.readAll().trimmed();

  // those query's below commit transaction so have to be done outside transaction
  SqlQuery query;
  query.prepare("CREATE USER :user@'%' IDENTIFIED WITH mysql_native_password BY '" + password + "'");
  query.bindValue(":user", ui->lineEditUser->text().toLower());

  if (not query.exec()) { throw RuntimeException("Erro criando usuário do banco de dados: " + query.lastError().text(), this); }

  query.prepare("GRANT ALL PRIVILEGES ON *.* TO :user@'%' WITH GRANT OPTION");
  query.bindValue(":user", ui->lineEditUser->text().toLower());

  if (not query.exec()) { throw RuntimeException("Erro guardando privilégios do usuário do banco de dados: " + query.lastError().text(), this); }

  if (not query.exec("FLUSH PRIVILEGES")) { throw RuntimeException("Erro FLUSH: " + query.lastError().text()); }
}

void CadastroUsuario::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Usuário cadastrado com sucesso!", this); }

void CadastroUsuario::verificaUsuarioDisponivel() {
  const QString text = ui->lineEditUser->text();

  if (text.isEmpty()) { return; }

  SqlQuery query;
  query.prepare("SELECT idUsuario FROM usuario WHERE user = :user");
  query.bindValue(":user", text);

  if (not query.exec()) { throw RuntimeException("Erro buscando usuário: " + query.lastError().text(), this); }

  if (query.first()) { throw RuntimeError("Nome de usuário já existe!", this); }
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

bool CadastroUsuario::newRegister() {
  if (not RegisterDialog::newRegister()) { return false; }

  modelPermissoes.setFilter("0");

  modelPermissoes.select();

  const int row = modelPermissoes.insertRowAtEnd();

  modelPermissoes.setData(row, "view_tab_orcamento", 1);
  modelPermissoes.setData(row, "view_tab_venda", 1);
  modelPermissoes.setData(row, "view_tab_estoque", 1);
  modelPermissoes.setData(row, "view_tab_relatorio", 1);
  modelPermissoes.setData(row, "webdav_documentos", 1);

  for (int row = 0; row < ui->table->model()->rowCount(); ++row) { ui->table->openPersistentEditor(ui->table->model()->index(row, 0)); }

  return true;
}

void CadastroUsuario::connectLineEditsToDirty() {
  const auto children = ui->tabWidget->findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &CadastroUsuario::marcarDirty); }
}

// TODO: 1colocar permissoes padroes para cada tipo de usuario
// TODO: colocar uma coluna 'ultimoAcesso' no BD (para saber quais usuarios nao estao mais ativos e desativar depois de x dias)
// FIXME: quando o usuario é alterado o usuario do MySql não é, fazendo com que o login não funcione mais
// FIXME: nao está mostrando mensagem de confirmacao apos desativar usuario
// TODO: colocar combobox para escolher regime CLT/PJ/Outros
