#include "widgetorcamento.h"
#include "ui_widgetorcamento.h"

#include "application.h"
#include "followup.h"
#include "orcamento.h"
#include "orcamentoproxymodel.h"
#include "reaisdelegate.h"
#include "user.h"

#include <QDebug>
#include <QSqlError>

WidgetOrcamento::WidgetOrcamento(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetOrcamento) { ui->setupUi(this); }

WidgetOrcamento::~WidgetOrcamento() { delete ui; }

void WidgetOrcamento::setWidgets() {
  unsetConnections();

  try {
    fillComboBoxFornecedor();
    fillComboBoxFollowup();
    fillComboBoxLoja();

    const QString lojaGeral = "1";

    if (not User::isAdmin() and not User::isAdministrativo() and User::idLoja != lojaGeral) { ui->comboBoxLojas->setCurrentText(User::fromLoja("descricao").toString()); }

    fillComboBoxVendedor();

    if (User::isAdministrativo()) {
      ui->checkBoxMes->setChecked(true);
      ui->dateEditMes->setEnabled(true);
    }

    if (User::isGerente()) { ui->frameLojas->hide(); }

    if (User::isVendedorOrEspecial()) {
      ui->checkBoxValido->setChecked(true);
      ui->checkBoxExpirado->setChecked(true);

      ui->frameVendedores->hide();
    }

    (User::isVendedorOrEspecial()) ? ui->radioButtonProprios->setChecked(true) : ui->radioButtonTodos->setChecked(true);

    ui->dateEditMes->setDate(qApp->serverDate());

    ui->lineEditBusca->setDelayed();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetOrcamento::setupTables() {
  modelOrcamento.setTable("view_orcamento");

  modelOrcamento.proxyModel = new OrcamentoProxyModel(&modelOrcamento, this);

  ui->table->setModel(&modelOrcamento);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idUsuario");
  ui->table->hideColumn("idUsuarioConsultor");
  ui->table->hideColumn("semaforo");
  ui->table->hideColumn("fornecedores");
}

void WidgetOrcamento::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxCancelado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxExpirado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxFechado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxMes, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxPerdido, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxReplicado, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxValido, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxFollowup, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxFornecedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxLojas, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetOrcamento::on_comboBoxLojas_currentIndexChanged, connectionType);
  connect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetOrcamento::on_dateEditMes_dateChanged, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetOrcamento::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->pushButtonCriarOrc, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonCriarOrc_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->radioButtonProprios, &QAbstractButton::toggled, this, &WidgetOrcamento::on_radioButton_toggled, connectionType);
  connect(ui->radioButtonTodos, &QAbstractButton::toggled, this, &WidgetOrcamento::on_radioButton_toggled, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetOrcamento::on_table_activated, connectionType);
}

void WidgetOrcamento::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxCancelado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxExpirado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxFechado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxMes, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxPerdido, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxReplicado, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxValido, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxFollowup, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxFornecedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxLojas, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxLojas, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetOrcamento::on_comboBoxLojas_currentIndexChanged);
  disconnect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetOrcamento::on_dateEditMes_dateChanged);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetOrcamento::on_groupBoxStatus_toggled);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->pushButtonCriarOrc, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonCriarOrc_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonFollowup_clicked);
  disconnect(ui->radioButtonProprios, &QAbstractButton::toggled, this, &WidgetOrcamento::on_radioButton_toggled);
  disconnect(ui->radioButtonTodos, &QAbstractButton::toggled, this, &WidgetOrcamento::on_radioButton_toggled);
  disconnect(ui->table, &TableView::activated, this, &WidgetOrcamento::on_table_activated);
}

void WidgetOrcamento::fillComboBoxVendedor() {
  unsetConnections();

  try {
    ui->comboBoxVendedores->clear();

    ui->comboBoxVendedores->addItem("Vendedores");

    const QString filtroLoja = (ui->comboBoxLojas->currentText() == "Lojas") ? "" : " AND idLoja = " + ui->comboBoxLojas->currentData().toString();

    SqlQuery query;

    if (not query.exec("SELECT idUsuario, nome FROM usuario WHERE tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')" + filtroLoja + " ORDER BY nome")) {
      throw RuntimeException("Erro: " + query.lastError().text());
    }

    while (query.next()) { ui->comboBoxVendedores->addItem(query.value("nome").toString(), query.value("idUsuario")); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetOrcamento::fillComboBoxLoja() {
  unsetConnections();

  try {
    ui->comboBoxLojas->clear();

    ui->comboBoxLojas->addItem("Lojas");

    SqlQuery query;

    if (not query.exec("SELECT descricao, idLoja FROM loja WHERE descricao <> '' AND desativado = FALSE ORDER BY descricao")) { throw RuntimeException("Erro: " + query.lastError().text()); }

    while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetOrcamento::fillComboBoxFollowup() {
  unsetConnections();

  try {
    ui->comboBoxFollowup->clear();

    ui->comboBoxFollowup->addItem("Followup");
    ui->comboBoxFollowup->addItem("QUENTE");
    ui->comboBoxFollowup->addItem("MORNO");
    ui->comboBoxFollowup->addItem("FRIO");
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetOrcamento::fillComboBoxFornecedor() {
  unsetConnections();

  try {
    ui->comboBoxFornecedores->clear();

    ui->comboBoxFornecedores->addItem("Fornecedores");

    SqlQuery query;

    if (not query.exec("SELECT razaoSocial FROM fornecedor")) { throw RuntimeException("Erro buscando fornecedores: " + query.lastError().text(), this); }

    while (query.next()) { ui->comboBoxFornecedores->addItem(query.value("razaoSocial").toString()); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetOrcamento::updateTables() {
  if (not isSet) {
    setWidgets();
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelOrcamento.select();
}

void WidgetOrcamento::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetOrcamento::on_table_activated(const QModelIndex &index) {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);
  orcamento->viewRegisterById(modelOrcamento.data(index.row(), "Código"));

  orcamento->show();
}

void WidgetOrcamento::on_pushButtonCriarOrc_clicked() {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);

  orcamento->show();
}

void WidgetOrcamento::ajustarGroupBoxStatus() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (auto *checkBox : filtrosStatus) {
    if (checkBox->isChecked()) { empty = false; }
  }

  unsetConnections();

  ui->groupBoxStatus->setChecked(not empty);

  for (auto *checkBox : filtrosStatus) { checkBox->setEnabled(true); }

  setConnections();
}

void WidgetOrcamento::montaFiltro() {
  ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  // para gerente o combobox é marcado com a loja e escondido então a regra abaixo se aplica
  const QString filtroLoja = (ui->comboBoxLojas->currentText() == "Lojas") ? "" : "idLoja = " + ui->comboBoxLojas->currentData().toString();

  if (not filtroLoja.isEmpty()) { filtros << filtroLoja; }

  //-------------------------------------

  const QString filtroData = (ui->checkBoxMes->isChecked()) ? "DATE_FORMAT(Data, '%Y-%m') = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'" : "";

  if (not filtroData.isEmpty()) { filtros << filtroData; }

  //-------------------------------------

  const QString idVendedor = ui->comboBoxVendedores->currentData().toString();
  const QString filtroVendedor = (ui->comboBoxVendedores->currentText() == "Vendedores") ? "" : "(idUsuario = " + idVendedor + " OR idUsuarioConsultor = " + idVendedor + ")";

  if (not filtroVendedor.isEmpty()) { filtros << filtroVendedor; }

  //-------------------------------------

  const QString fornecedor = qApp->sanitizeSQL(ui->comboBoxFornecedores->currentText());
  const QString filtroFornecedor = (fornecedor == "Fornecedores") ? "" : "(fornecedores LIKE '%" + fornecedor + "%')";

  if (not filtroFornecedor.isEmpty()) { filtros << filtroFornecedor; }

  //-------------------------------------

  const QString filtroRadio = (ui->radioButtonTodos->isChecked()) ? "" : "(Vendedor = '" + User::nome + "'" + " OR Consultor = '" + User::nome + "')";

  if (not filtroRadio.isEmpty()) { filtros << filtroRadio; }

  //-------------------------------------

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  const QString filtroFollowup = (ui->comboBoxFollowup->currentIndex() > 0) ? "(semaforo = " + QString::number(ui->comboBoxFollowup->currentIndex()) + ")" : "";

  if (not filtroFollowup.isEmpty()) { filtros << filtroFollowup; }

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(Código LIKE '%" + textoBusca + "%' OR Vendedor LIKE '%" + textoBusca + "%' OR Cliente LIKE '%" + textoBusca + "%' OR Profissional LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelOrcamento.setFilter(filtros.join(" AND "));
}

void WidgetOrcamento::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idOrcamento = modelOrcamento.data(selection.first().row(), "Código").toString();

  auto *followup = new FollowUp(idOrcamento, FollowUp::Tipo::Orcamento, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetOrcamento::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  try {
    const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

    for (const auto &child : children) {
      child->setEnabled(true);
      child->setChecked(enabled);
    }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  montaFiltro();
}

void WidgetOrcamento::on_comboBoxLojas_currentIndexChanged() {
  unsetConnections();

  try {
    fillComboBoxVendedor();

    // -------------------------------------------------------------------------

    if (User::isVendedor()) {
      if (ui->comboBoxLojas->currentData() != User::idLoja) {
        ui->radioButtonTodos->setDisabled(true);
        ui->radioButtonProprios->setChecked(true);
      } else {
        ui->radioButtonTodos->setEnabled(true);
      }
    }

    montaFiltro();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetOrcamento::on_radioButton_toggled(const bool checked) {
  if (checked) { montaFiltro(); }
}

void WidgetOrcamento::on_dateEditMes_dateChanged() {
  if (not ui->checkBoxMes->isChecked()) { return; }

  montaFiltro();
}
