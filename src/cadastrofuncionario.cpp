#include "cadastrofuncionario.h"
#include "ui_cadastrofuncionario.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

CadastroFuncionario::CadastroFuncionario(QWidget *parent) : QDialog(parent), ui(new Ui::CadastroFuncionario) {
  ui->setupUi(this);
  setupTables();
  fillComboBoxRegime();
  setConnections();
  ui->comboBoxRegime->setCurrentText("CLT");
}

CadastroFuncionario::~CadastroFuncionario() { delete ui; }

void CadastroFuncionario::setupTables() {
  model.setTable("usuario");

  model.setFilter("desativado = FALSE");

  model.select();

  model.setHeaderData("nome", "Nome");
  model.setHeaderData("regime", "Regime");
  model.setHeaderData("banco", "Banco");
  model.setHeaderData("agencia", "Agência");
  model.setHeaderData("cc", "CC");
  model.setHeaderData("poupanca", "Poupança");
  model.setHeaderData("nomeBanco", "Nome no banco");
  model.setHeaderData("cpfBanco", "CPF");
  model.setHeaderData("cnpjBanco", "CNPJ");

  model.proxyModel = new SortFilterProxyModel(&model, this);

  ui->table->setModel(&model);

  ui->table->sortByColumn("nome");

  ui->table->hideColumn("idUsuario");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("user");
  ui->table->hideColumn("passwd");
  ui->table->hideColumn("password");
  ui->table->hideColumn("tipo");
  ui->table->hideColumn("email");
  ui->table->hideColumn("telefone");
  ui->table->hideColumn("especialidade");
  ui->table->hideColumn("desativado");

  ui->table->setItemDelegateForColumn("poupanca", new CheckBoxDelegate(false, this));

  ui->table->setPersistentColumns({"poupanca"});
}

void CadastroFuncionario::fillComboBoxRegime() {
  SqlQuery query;

  if (not query.exec("SELECT DISTINCT(regime) AS regime FROM usuario")) { throw RuntimeException("Erro buscando regimes: " + query.lastError().text()); }

  while (query.next()) { ui->comboBoxRegime->addItem(query.value("regime").toString()); }
}

void CadastroFuncionario::on_comboBoxRegime_currentTextChanged(const QString &text) {
  const QString desativado = (ui->checkBoxDesativado->isChecked()) ? "TRUE" : "FALSE";

  model.setFilter("regime = '" + text + "' AND desativado = " + desativado);
}

void CadastroFuncionario::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxDesativado, &QCheckBox::toggled, this, &CadastroFuncionario::on_checkBoxDesativado_toggled, connectionType);
  connect(ui->comboBoxRegime, &QComboBox::currentTextChanged, this, &CadastroFuncionario::on_comboBoxRegime_currentTextChanged, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CadastroFuncionario::on_pushButtonSalvar_clicked, connectionType);
}

void CadastroFuncionario::on_checkBoxDesativado_toggled(const bool checked) {
  Q_UNUSED(checked)
  on_comboBoxRegime_currentTextChanged(ui->comboBoxRegime->currentText());
}

void CadastroFuncionario::on_pushButtonSalvar_clicked() {
  qApp->startTransaction("CadastroFuncionario::on_pushButtonSalvar_clicked");

  model.submitAll();

  qApp->endTransaction();

  qApp->enqueueInformation("Dados salvos com sucesso!", this);

  close();
}
