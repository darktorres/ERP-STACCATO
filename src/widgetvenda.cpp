#include <QDate>
#include <QDebug>
#include <QSqlError>

#include "application.h"
#include "followup.h"
#include "reaisdelegate.h"
#include "ui_widgetvenda.h"
#include "usersession.h"
#include "venda.h"
#include "vendaproxymodel.h"
#include "widgetvenda.h"

WidgetVenda::WidgetVenda(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetVenda) {
  ui->setupUi(this);
  ui->groupBoxStatusFinanceiro->hide();
}

WidgetVenda::~WidgetVenda() { delete ui; }

void WidgetVenda::setupTables() {
  modelViewVenda.setTable("view_venda");

  modelViewVenda.setHeaderData("statusFinanceiro", "Financeiro");
  modelViewVenda.setHeaderData("dataFinanceiro", "Data Financ.");

  ui->table->setModel(new VendaProxyModel(&modelViewVenda, this));
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idUsuario");
  ui->table->hideColumn("idUsuarioConsultor");
  ui->table->setItemDelegateForColumn("Total R$", new ReaisDelegate(this));
}

void WidgetVenda::montaFiltro() {
  QStringList filtros;

  QString filtroLoja;

  if (const auto tipoUsuario = UserSession::tipoUsuario(); not ui->comboBoxLojas->currentText().isEmpty()) {
    filtroLoja = "idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();
  } else if (tipoUsuario == "ADMINISTRADOR" or tipoUsuario == "ADMINISTRATIVO" or tipoUsuario == "DIRETOR" or tipoUsuario == "GERENTE DEPARTAMENTO" or tipoUsuario == "VENDEDOR ESPECIAL") {
    filtroLoja = "";
  } else if (tipoUsuario == "GERENTE LOJA" or tipoUsuario == "VENDEDOR") {
    const auto siglaLoja = UserSession::fromLoja("sigla");
    const auto sigla = siglaLoja ? siglaLoja.value().toString() : "";
    filtroLoja = "(Código LIKE '%" + sigla + "%')";
  }

  if (not filtroLoja.isEmpty()) { filtros << filtroLoja; }

  //-------------------------------------

  const QString filtroData = ui->groupBoxMes->isChecked() ? "DATE_FORMAT(Data, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";
  if (not filtroData.isEmpty()) { filtros << filtroData; }

  //-------------------------------------

  const QString idVendedor = ui->comboBoxVendedores->getCurrentValue().toString();
  const QString filtroVendedor = ui->comboBoxVendedores->currentText().isEmpty() ? "" : "(idUsuario = " + idVendedor + " OR idUsuarioConsultor = " + idVendedor + ")";
  if (not filtroVendedor.isEmpty()) { filtros << filtroVendedor; }

  //-------------------------------------

  const QString filtroRadio = ui->radioButtonTodos->isChecked() ? "" : "(Vendedor = '" + UserSession::nome() + "'" + " OR Consultor = '" + UserSession::nome() + "')";
  if (not filtroRadio.isEmpty()) { filtros << filtroRadio; }

  //-------------------------------------

  QStringList filtroCheck;

  if (financeiro) {
    Q_FOREACH (const auto &child, ui->groupBoxStatusFinanceiro->findChildren<QCheckBox *>()) {
      if (child->isChecked()) { filtroCheck << "statusFinanceiro = '" + child->text().toUpper() + "'"; }
    }
  } else {
    Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
      if (child->isChecked()) { filtroCheck << "status = '" + child->text().toUpper() + "'"; }
    }
  }

  if (not filtroCheck.isEmpty()) { filtros << "(" + filtroCheck.join(" OR ") + ")"; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();

  const QString filtroBusca =
      textoBusca.isEmpty() ? "" : "(Código LIKE '%" + textoBusca + "%' OR Vendedor LIKE '%" + textoBusca + "%' OR Cliente LIKE '%" + textoBusca + "%' OR Profissional LIKE '%" + textoBusca + "%')";
  if (not filtroBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelViewVenda.setFilter(filtros.join(" AND "));

  if (not modelViewVenda.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetVenda::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    child->setEnabled(true);
    child->setChecked(enabled);
  }

  setConnections();

  montaFiltro();
}

void WidgetVenda::setPermissions() {
  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario == "ADMINISTRADOR" or tipoUsuario == "DIRETOR") {
    QSqlQuery query("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE");

    while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }

    ui->comboBoxLojas->setCurrentValue(UserSession::idLoja());
  }

  if (tipoUsuario == "GERENTE LOJA") {
    ui->groupBoxLojas->hide();

    QSqlQuery query("SELECT idUsuario, user FROM usuario WHERE desativado = FALSE AND idLoja = " + QString::number(UserSession::idLoja()));

    ui->comboBoxVendedores->addItem("");

    while (query.next()) { ui->comboBoxVendedores->addItem(query.value("user").toString(), query.value("idUsuario")); }
  }

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") {
    QSqlQuery query("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE");

    while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }

    ui->radioButtonProprios->click();

    ui->groupBoxVendedores->hide();
  } else {
    ui->radioButtonTodos->click();
  }

  ui->dateEdit->setDate(QDate::currentDate());
}

void WidgetVenda::setConnections() {
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxConferido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxDevolvido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEmColeta, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEmCompra, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEmEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEmFaturamento, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEmRecebimento, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEntregaAgend, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEntregue, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxIniciado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxLiberado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxPendente2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxRepoEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->checkBoxRepoReceb, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->comboBoxLojas, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  connect(ui->comboBoxLojas, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WidgetVenda::on_comboBoxLojas_currentIndexChanged);
  connect(ui->comboBoxVendedores, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetVenda::montaFiltro);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatus_toggled);
  connect(ui->groupBoxStatusFinanceiro, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatusFinanceiro_toggled);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetVenda::montaFiltro);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetVenda::on_pushButtonFollowup_clicked);
  connect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::on_radioButtonProprios_toggled);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro);
  connect(ui->table, &TableView::activated, this, &WidgetVenda::on_table_activated);
}

void WidgetVenda::unsetConnections() {
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxConferido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxDevolvido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEmColeta, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEmCompra, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEmEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEmFaturamento, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEmRecebimento, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEntregaAgend, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEntregue, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxIniciado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxLiberado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxPendente2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxRepoEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxRepoReceb, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxLojas, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxLojas, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WidgetVenda::on_comboBoxLojas_currentIndexChanged);
  disconnect(ui->comboBoxVendedores, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatus_toggled);
  disconnect(ui->groupBoxStatusFinanceiro, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatusFinanceiro_toggled);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetVenda::on_pushButtonFollowup_clicked);
  disconnect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::on_radioButtonProprios_toggled);
  disconnect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->table, &TableView::activated, this, &WidgetVenda::on_table_activated);
}

void WidgetVenda::updateTables() {
  if (not isSet) {
    setPermissions();
    setConnections();
    on_comboBoxLojas_currentIndexChanged(0);
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelViewVenda.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetVenda::on_table_activated(const QModelIndex index) {
  auto *vendas = new Venda(this);
  vendas->setAttribute(Qt::WA_DeleteOnClose);
  if (financeiro) { vendas->setFinanceiro(); }
  vendas->viewRegisterById(modelViewVenda.data(index.row(), "Código"));
}

void WidgetVenda::on_radioButtonProprios_toggled(const bool checked) {
  if (UserSession::tipoUsuario() == "VENDEDOR") { checked ? ui->groupBoxLojas->show() : ui->groupBoxLojas->hide(); }
}

void WidgetVenda::on_comboBoxLojas_currentIndexChanged(const int) {
  unsetConnections();

  ui->comboBoxVendedores->clear();

  const QString filtroLoja = ui->comboBoxLojas->currentText().isEmpty() ? "" : " AND idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();

  QSqlQuery query("SELECT idUsuario, nome FROM usuario WHERE desativado = FALSE AND tipo = 'VENDEDOR'" + filtroLoja + " ORDER BY nome");

  ui->comboBoxVendedores->addItem("");

  while (query.next()) { ui->comboBoxVendedores->addItem(query.value("nome").toString(), query.value("idUsuario")); }

  setConnections();
}

void WidgetVenda::setFinanceiro() {
  ui->groupBoxStatusFinanceiro->show();
  ui->groupBoxStatus->hide();
  financeiro = true;
}

void WidgetVenda::resetTables() { modelIsSet = false; }

void WidgetVenda::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

  FollowUp *followup = new FollowUp(modelViewVenda.data(list.first().row(), "Código").toString(), FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetVenda::on_groupBoxStatusFinanceiro_toggled(const bool enabled) {
  unsetConnections();

  Q_FOREACH (const auto &child, ui->groupBoxStatusFinanceiro->findChildren<QCheckBox *>()) {
    child->setEnabled(true);
    child->setChecked(enabled);
  }

  setConnections();

  montaFiltro();
}

// TODO: verificar os pedidos de devolucao que estao com o status errado (update_venda_status)
