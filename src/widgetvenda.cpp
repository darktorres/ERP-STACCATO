#include "widgetvenda.h"
#include "ui_widgetvenda.h"

#include "application.h"
#include "followup.h"
#include "reaisdelegate.h"
#include "user.h"
#include "venda.h"
#include "vendaproxymodel.h"

#include <QDebug>
#include <QSqlError>

WidgetVenda::WidgetVenda(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetVenda) {
  ui->setupUi(this);
  ui->groupBoxStatusFinanceiro->hide();
}

WidgetVenda::~WidgetVenda() { delete ui; }

void WidgetVenda::setupTables() {
  modelViewVenda.setTable("view_venda");

  modelViewVenda.setHeaderData("statusFinanceiro", "Financeiro");
  modelViewVenda.setHeaderData("dataFinanceiro", "Data Financ.");

  modelViewVenda.proxyModel = new VendaProxyModel(&modelViewVenda, this);

  ui->table->setModel(&modelViewVenda);

  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idUsuario");
  ui->table->hideColumn("idUsuarioConsultor");
  ui->table->hideColumn("fornecedores");

  ui->table->setItemDelegateForColumn("Total R$", new ReaisDelegate(this));
}

void WidgetVenda::ajustarGroupBoxStatus() {
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

void WidgetVenda::montaFiltro() {
  ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  // para gerente o combobox é marcado com a loja e escondido então a regra abaixo se aplica
  const QString filtroLoja = (ui->comboBoxLojas->currentText() == "Lojas") ? "" : "idLoja = " + ui->comboBoxLojas->currentData().toString();

  if (not filtroLoja.isEmpty()) { filtros << filtroLoja; }

  //-------------------------------------

  const QString filtroMes = (ui->checkBoxMes->isChecked()) ? "DATE_FORMAT(Data, '%Y-%m') = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'" : "";
  if (not filtroMes.isEmpty()) { filtros << filtroMes; }

  //-------------------------------------

  const QString filtroDia = (ui->checkBoxDia->isChecked()) ? "DATE_FORMAT(Data, '%Y-%m-%d') = '" + ui->dateEditDia->date().toString("yyyy-MM-dd") + "'" : "";
  if (not filtroDia.isEmpty()) { filtros << filtroDia; }

  //-------------------------------------

  const QString idVendedor = ui->comboBoxVendedores->currentData().toString();
  const QString filtroVendedor = (ui->comboBoxVendedores->currentText() == "Vendedores") ? "" : "(idUsuario = " + idVendedor + " OR idUsuarioConsultor = " + idVendedor + ")";
  if (not filtroVendedor.isEmpty()) { filtros << filtroVendedor; }

  //-------------------------------------

  const QString fornecedor = qApp->sanitizeSQL(ui->comboBoxFornecedores->currentText());
  const QString filtroFornecedor = (fornecedor == "Fornecedores") ? "" : "(fornecedores LIKE '%" + fornecedor + "%')";
  if (not filtroFornecedor.isEmpty()) { filtros << filtroFornecedor; }

  //-------------------------------------

  const QString filtroRadio = (ui->radioButtonTodos->isChecked()) ? "" : "(Vendedor = '" + qApp->sanitizeSQL(User::nome) + "'" + " OR Consultor = '" + qApp->sanitizeSQL(User::nome) + "')";
  if (not filtroRadio.isEmpty()) { filtros << filtroRadio; }

  //-------------------------------------

  QStringList filtroCheck;

  const auto children1 = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children1) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  if (financeiro) {
    QStringList filtroCheck2;

    const auto children2 = ui->groupBoxStatusFinanceiro->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

    for (const auto &child : children2) {
      if (child->isChecked()) { filtroCheck2 << "'" + child->text().toUpper() + "'"; }
    }

    if (not filtroCheck2.isEmpty()) { filtros << "statusFinanceiro IN (" + filtroCheck2.join(", ") + ")"; }
  }

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(Código LIKE '%" + textoBusca + "%' OR Vendedor LIKE '%" + textoBusca + "%' OR Cliente LIKE '%" + textoBusca + "%' OR Profissional LIKE '%" + textoBusca +
                              "%' OR `OC Rep` LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  // TODO: sanitizar SQL aqui em vez de fazer picado?

  modelViewVenda.setFilter(filtros.join(" AND "));
}

void WidgetVenda::on_groupBoxStatus_toggled(const bool enabled) {
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

void WidgetVenda::setWidgets() {
  unsetConnections();

  try {
    fillComboBoxFornecedor();
    fillComboBoxLoja();

    const QString lojaGeral = "1";

    if (not User::isAdmin() and not User::isAdministrativo() and User::idLoja != lojaGeral) { ui->comboBoxLojas->setCurrentText(User::fromLoja("descricao").toString()); }

    fillComboBoxVendedor();

    if (User::isAdministrativo()) {
      ui->checkBoxMes->setChecked(true);
      ui->dateEditMes->setEnabled(true);
    }

    if (User::isGerente()) { ui->frameLojas->hide(); }

    if (User::isVendedorOrEspecial()) { ui->frameVendedores->hide(); }

    (User::isVendedorOrEspecial()) ? ui->radioButtonProprios->setChecked(true) : ui->radioButtonTodos->setChecked(true);

    ui->dateEditMes->setDate(qApp->serverDate());
    ui->dateEditDia->setDate(qApp->serverDate());

    ui->lineEditBusca->setDelayed();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetVenda::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxConferido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxDevolvido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxDia, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEmColeta, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEmCompra, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEmEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEmFaturamento, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEmRecebimento, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEntregaAgend, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEntregue, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxIniciado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxLiberado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxMes, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxPendente2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxRepoEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxRepoReceb, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->comboBoxFornecedores, &QComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->comboBoxLojas, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetVenda::on_comboBoxLojas_currentIndexChanged, connectionType);
  connect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->dateEditDia, &QDateEdit::dateChanged, this, &WidgetVenda::on_dateEditDia_dateChanged, connectionType);
  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetVenda::on_dateEditMes_dateChanged, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatus_toggled, connectionType);
  connect(ui->groupBoxStatusFinanceiro, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatusFinanceiro_toggled, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetVenda::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::on_radioButton_toggled, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetVenda::on_radioButton_toggled, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetVenda::on_table_activated, connectionType);
}

void WidgetVenda::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxCancelado2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxConferido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxDevolvido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxDia, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
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
  disconnect(ui->checkBoxMes, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxPendente2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxRepoEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxRepoReceb, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxFornecedores, &QComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxLojas, &QComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxLojas, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetVenda::on_comboBoxLojas_currentIndexChanged);
  disconnect(ui->comboBoxVendedores, &QComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->dateEditDia, &QDateEdit::dateChanged, this, &WidgetVenda::on_dateEditDia_dateChanged);
  disconnect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetVenda::on_dateEditMes_dateChanged);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatus_toggled);
  disconnect(ui->groupBoxStatusFinanceiro, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatusFinanceiro_toggled);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetVenda::on_pushButtonFollowup_clicked);
  disconnect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::on_radioButton_toggled);
  disconnect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetVenda::on_radioButton_toggled);
  disconnect(ui->table, &TableView::activated, this, &WidgetVenda::on_table_activated);
}

void WidgetVenda::updateTables() {
  if (not isSet) {
    setWidgets();
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelViewVenda.select();
}

void WidgetVenda::fillComboBoxVendedor() {
  unsetConnections();

  try {
    ui->comboBoxVendedores->clear();

    ui->comboBoxVendedores->addItem("Vendedores");

    const QString filtroLoja = (ui->comboBoxLojas->currentText() == "Lojas") ? "" : " AND idLoja = " + ui->comboBoxLojas->currentData().toString();

    SqlQuery query;

    if (not query.exec("SELECT idUsuario, nome FROM usuario WHERE tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')" + filtroLoja + " ORDER BY nome")) {
      throw RuntimeException("Erro: " + query.lastError().text(), this);
    }

    while (query.next()) { ui->comboBoxVendedores->addItem(query.value("nome").toString(), query.value("idUsuario")); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetVenda::fillComboBoxLoja() {
  unsetConnections();

  try {
    ui->comboBoxLojas->clear();

    ui->comboBoxLojas->addItem("Lojas");

    SqlQuery query;

    if (not query.exec("SELECT idLoja, descricao FROM loja WHERE descricao <> '' AND desativado = FALSE ORDER BY descricao")) { throw RuntimeException("Erro: " + query.lastError().text()); }

    while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetVenda::fillComboBoxFornecedor() {
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

void WidgetVenda::on_table_activated(const QModelIndex &index) {
  auto *venda = new Venda(this);
  venda->setAttribute(Qt::WA_DeleteOnClose);
  if (financeiro) { venda->setFinanceiro(); }
  venda->viewRegisterById(modelViewVenda.data(index.row(), "Código"));

  venda->show();
}

void WidgetVenda::on_comboBoxLojas_currentIndexChanged() {
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

void WidgetVenda::setFinanceiro() {
  ui->groupBoxStatusFinanceiro->show();
  financeiro = true;
}

void WidgetVenda::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetVenda::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = modelViewVenda.data(list.first().row(), "Código").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetVenda::on_groupBoxStatusFinanceiro_toggled(const bool enabled) {
  unsetConnections();

  try {
    const auto children = ui->groupBoxStatusFinanceiro->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

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

void WidgetVenda::on_radioButton_toggled(const bool checked) {
  if (checked) { montaFiltro(); }
}

void WidgetVenda::on_dateEditDia_dateChanged() {
  if (not ui->checkBoxDia->isChecked()) { return; }

  montaFiltro();
}

void WidgetVenda::on_dateEditMes_dateChanged() {
  if (not ui->checkBoxMes->isChecked()) { return; }

  montaFiltro();
}
