#include <QDebug>
#include <QSqlError>

#include "application.h"
#include "followup.h"
#include "orcamento.h"
#include "orcamentoproxymodel.h"
#include "reaisdelegate.h"
#include "ui_widgetorcamento.h"
#include "usersession.h"
#include "widgetorcamento.h"

WidgetOrcamento::WidgetOrcamento(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetOrcamento) { ui->setupUi(this); }

WidgetOrcamento::~WidgetOrcamento() { delete ui; }

void WidgetOrcamento::setPermissions() {
  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario == "ADMINISTRADOR" or tipoUsuario == "DIRETOR") {
    QSqlQuery query;

    if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE")) { return; }

    while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }

    ui->comboBoxLojas->setCurrentValue(UserSession::idLoja());

    ui->groupBoxMes->setChecked(true);
  }

  if (tipoUsuario == "GERENTE LOJA") {
    ui->groupBoxLojas->hide();

    ui->comboBoxVendedores->clear();

    QSqlQuery query;

    if (not query.exec("SELECT idUsuario, user FROM usuario WHERE desativado = FALSE AND idLoja = " + QString::number(UserSession::idLoja()) + " ORDER BY nome")) { return; }

    ui->comboBoxVendedores->addItem("");

    while (query.next()) { ui->comboBoxVendedores->addItem(query.value("user").toString(), query.value("idUsuario")); }
  }

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") {
    QSqlQuery query;

    if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE")) { return; }

    while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }

    ui->radioButtonProprios->click();
    ui->checkBoxValido->setChecked(true);
    ui->checkBoxExpirado->setChecked(true);

    ui->groupBoxVendedores->hide();
  } else {
    ui->radioButtonTodos->click();
  }

  ui->dateEdit->setDate(QDate::currentDate());
}

void WidgetOrcamento::setupTables() {
  modelViewOrcamento.setTable("view_orcamento");

  modelViewOrcamento.proxyModel = new OrcamentoProxyModel(&modelViewOrcamento, this);

  ui->table->setModel(&modelViewOrcamento);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idUsuario");
  ui->table->hideColumn("idUsuarioConsultor");
  ui->table->hideColumn("semaforo");
}

void WidgetOrcamento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxCancelado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxExpirado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxFechado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxPerdido, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxReplicado, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxValido, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxLojas, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxLojas, QOverload<int>::of(&ComboBox::currentIndexChanged), this, &WidgetOrcamento::on_comboBoxLojas_currentIndexChanged, connectionType);
  connect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetOrcamento::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->pushButtonCriarOrc, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonCriarOrc_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->radioButtonProprios, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->radioButtonTodos, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetOrcamento::on_table_activated, connectionType);
}

void WidgetOrcamento::unsetConnections() {
  disconnect(ui->checkBoxCancelado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxExpirado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxFechado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxPerdido, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxReplicado, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxValido, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxLojas, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxLojas, QOverload<int>::of(&ComboBox::currentIndexChanged), this, &WidgetOrcamento::on_comboBoxLojas_currentIndexChanged);
  disconnect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetOrcamento::on_groupBoxStatus_toggled);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->pushButtonCriarOrc, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonCriarOrc_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonFollowup_clicked);
  disconnect(ui->radioButtonProprios, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->radioButtonTodos, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->table, &TableView::activated, this, &WidgetOrcamento::on_table_activated);
}

void WidgetOrcamento::updateTables() {
  if (not isSet) {
    on_comboBoxLojas_currentIndexChanged();
    setPermissions();
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelViewOrcamento.select()) { return; }
}

void WidgetOrcamento::resetTables() { modelIsSet = false; }

void WidgetOrcamento::on_table_activated(const QModelIndex &index) {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);
  orcamento->viewRegisterById(modelViewOrcamento.data(index.row(), "Código"));

  orcamento->show();
}

void WidgetOrcamento::on_pushButtonCriarOrc_clicked() {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);

  orcamento->show();
}

void WidgetOrcamento::montaFiltro() {
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

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();
  const QString filtroBusca = "(Código LIKE '%" + textoBusca + "%' OR Vendedor LIKE '%" + textoBusca + "%' OR Cliente LIKE '%" + textoBusca + "%' OR Profissional LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelViewOrcamento.setFilter(filtros.join(" AND "));
}

void WidgetOrcamento::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  FollowUp *followup = new FollowUp(modelViewOrcamento.data(list.first().row(), "Código").toString(), FollowUp::Tipo::Orcamento, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetOrcamento::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (const auto &child : children) {
    child->setEnabled(true);
    child->setChecked(enabled);
  }

  setConnections();

  montaFiltro();
}

void WidgetOrcamento::on_comboBoxLojas_currentIndexChanged() {
  unsetConnections();

  [&] {
    ui->comboBoxVendedores->clear();

    const QString filtroLoja = ui->comboBoxLojas->currentText().isEmpty() ? "" : " AND idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();

    QSqlQuery query;

    if (not query.exec("SELECT idUsuario, nome FROM usuario WHERE desativado = FALSE AND tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')" + filtroLoja + " ORDER BY nome")) { return; }

    ui->comboBoxVendedores->addItem("");

    while (query.next()) { ui->comboBoxVendedores->addItem(query.value("nome").toString(), query.value("idUsuario")); }

    const QString tipoUsuario = UserSession::tipoUsuario();

    if (tipoUsuario == "VENDEDOR") {
      const int currentLoja = UserSession::idLoja();

      if (currentLoja != ui->comboBoxLojas->getCurrentValue()) {
        ui->radioButtonTodos->setDisabled(true);
        ui->radioButtonProprios->setChecked(true);
      } else {
        ui->radioButtonTodos->setEnabled(true);
      }
    }
  }();

  setConnections();
}
