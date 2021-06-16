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

WidgetOrcamento::WidgetOrcamento(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetOrcamento) {
  ui->setupUi(this);
  timer.setSingleShot(true);
}

WidgetOrcamento::~WidgetOrcamento() { delete ui; }

void WidgetOrcamento::setPermissions() {
  unsetConnections();

  try {
    [&] {
      listarLojas();

      if (User::isAdministrativo()) { ui->groupBoxMes->setChecked(true); }

      if (User::isGerente()) { ui->groupBoxLojas->hide(); }

      if (User::isVendedorOrEspecial()) {
        ui->checkBoxValido->setChecked(true);
        ui->checkBoxExpirado->setChecked(true);

        ui->groupBoxVendedores->hide();
      }

      // TODO: remove this?
      if (User::nome == "VIVIANE") { ui->groupBoxVendedores->show(); }

      (User::isVendedorOrEspecial()) ? ui->radioButtonProprios->setChecked(true) : ui->radioButtonTodos->setChecked(true);

      ui->comboBoxLojas->setCurrentValue(User::idLoja);

      fillComboBoxVendedor();

      ui->dateEdit->setDate(qApp->serverDate());
    }();
  } catch (std::exception &) {}

  setConnections();
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
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxExpirado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxFechado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxPerdido, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxReplicado, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->checkBoxValido, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxFollowup, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->comboBoxLojas, qOverload<int>(&ComboBox::currentIndexChanged), this, &WidgetOrcamento::on_comboBoxLojas_currentIndexChanged, connectionType);
  connect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetOrcamento::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetOrcamento::delayFiltro, connectionType);
  connect(ui->pushButtonCriarOrc, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonCriarOrc_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->radioButtonProprios, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->radioButtonTodos, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetOrcamento::on_table_activated, connectionType);
}

void WidgetOrcamento::unsetConnections() {
  blockingSignals.push(0);

  disconnect(&timer, &QTimer::timeout, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxCancelado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxExpirado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxFechado, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxPerdido, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxReplicado, &QCheckBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->checkBoxValido, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxFollowup, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxLojas, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->comboBoxLojas, qOverload<int>(&ComboBox::currentIndexChanged), this, &WidgetOrcamento::on_comboBoxLojas_currentIndexChanged);
  disconnect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetOrcamento::on_groupBoxStatus_toggled);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetOrcamento::delayFiltro);
  disconnect(ui->pushButtonCriarOrc, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonCriarOrc_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetOrcamento::on_pushButtonFollowup_clicked);
  disconnect(ui->radioButtonProprios, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->radioButtonTodos, &QAbstractButton::toggled, this, &WidgetOrcamento::montaFiltro);
  disconnect(ui->table, &TableView::activated, this, &WidgetOrcamento::on_table_activated);
}

void WidgetOrcamento::delayFiltro() { timer.start(500); }

void WidgetOrcamento::fillComboBoxVendedor() {
  ui->comboBoxVendedores->clear();

  const QString filtroLoja = ui->comboBoxLojas->currentText().isEmpty() ? "" : " AND idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();

  SqlQuery query;

  if (not query.exec("SELECT idUsuario, nome FROM usuario WHERE tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')" + filtroLoja + " ORDER BY nome")) {
    throw RuntimeException("Erro: " + query.lastError().text());
  }

  ui->comboBoxVendedores->addItem("");

  while (query.next()) { ui->comboBoxVendedores->addItem(query.value("nome").toString(), query.value("idUsuario")); }
}

void WidgetOrcamento::updateTables() {
  if (not isSet) {
    setPermissions();
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewOrcamento.select();
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
  if (not User::isGerente() and not User::isVendedorOrEspecial() and not ui->lineEditBusca->text().isEmpty()) { return montaFiltroTexto(); }

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  // para gerente o combobox é marcado com a loja e escondido então a regra abaixo se aplica
  const QString filtroLoja = (ui->comboBoxLojas->currentText().isEmpty()) ? "" : "idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();

  if (not filtroLoja.isEmpty()) { filtros << filtroLoja; }

  //-------------------------------------

  const QString filtroData = (ui->groupBoxMes->isChecked()) ? "DATE_FORMAT(Data, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";
  if (not filtroData.isEmpty()) { filtros << filtroData; }

  //-------------------------------------

  const QString idVendedor = ui->comboBoxVendedores->getCurrentValue().toString();
  const QString filtroVendedor = (ui->comboBoxVendedores->currentText().isEmpty()) ? "" : "(idUsuario = " + idVendedor + " OR idUsuarioConsultor = " + idVendedor + ")";
  if (not filtroVendedor.isEmpty()) { filtros << filtroVendedor; }

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

  modelViewOrcamento.setFilter(filtros.join(" AND "));
}

void WidgetOrcamento::montaFiltroTexto() {
  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(Código LIKE '%" + textoBusca + "%' OR Vendedor LIKE '%" + textoBusca + "%' OR Cliente LIKE '%" + textoBusca + "%' OR Profissional LIKE '%" + textoBusca + "%')";

  modelViewOrcamento.setFilter(filtroBusca);
}

void WidgetOrcamento::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString codigo = modelViewOrcamento.data(list.first().row(), "Código").toString();

  FollowUp *followup = new FollowUp(codigo, FollowUp::Tipo::Orcamento, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetOrcamento::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  try {
    [&] {
      const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

      for (const auto &child : children) {
        child->setEnabled(true);
        child->setChecked(enabled);
      }
    }();
  } catch (std::exception &) {}

  setConnections();

  montaFiltro();
}

void WidgetOrcamento::on_comboBoxLojas_currentIndexChanged() {
  unsetConnections();

  try {
    [&] {
      fillComboBoxVendedor();

      // -------------------------------------------------------------------------

      if (User::isVendedor()) {
        if (ui->comboBoxLojas->getCurrentValue() != User::idLoja) {
          ui->radioButtonTodos->setDisabled(true);
          ui->radioButtonProprios->setChecked(true);
        } else {
          ui->radioButtonTodos->setEnabled(true);
        }
      }

      montaFiltro();
    }();
  } catch (std::exception &) {}

  setConnections();
}

void WidgetOrcamento::listarLojas() {
  SqlQuery query;

  if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE ORDER BY descricao")) { throw RuntimeException("Erro: " + query.lastError().text()); }

  while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }
}
