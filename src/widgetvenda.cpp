#include "widgetvenda.h"
#include "ui_widgetvenda.h"

#include "application.h"
#include "followup.h"
#include "reaisdelegate.h"
#include "user.h"
#include "venda.h"
#include "vendaproxymodel.h"

#include <QDate>
#include <QDebug>
#include <QSqlError>

WidgetVenda::WidgetVenda(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetVenda) {
  ui->setupUi(this);
  ui->groupBoxStatusFinanceiro->hide();
  timer.setSingleShot(true);
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

void WidgetVenda::montaFiltro() {
  if (not ui->lineEditBusca->text().isEmpty()) { return montaFiltroTexto(); }

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  // para gerente o combobox é marcado com a loja e escondido então a regra abaixo se aplica
  const QString filtroLoja = (ui->comboBoxLojas->currentText().isEmpty()) ? "" : "idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();

  if (not filtroLoja.isEmpty()) { filtros << filtroLoja; }

  //-------------------------------------

  const QString filtroMes = (ui->groupBoxMes->isChecked()) ? "DATE_FORMAT(Data, '%Y-%m') = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'" : "";
  if (not filtroMes.isEmpty()) { filtros << filtroMes; }

  //-------------------------------------

  const QString filtroDia = (ui->groupBoxDia->isChecked()) ? "DATE_FORMAT(Data, '%Y-%m-%d') = '" + ui->dateEditDia->date().toString("yyyy-MM-dd") + "'" : "";
  if (not filtroDia.isEmpty()) { filtros << filtroDia; }

  //-------------------------------------

  const QString idVendedor = ui->comboBoxVendedores->getCurrentValue().toString();
  const QString filtroVendedor = (ui->comboBoxVendedores->currentText().isEmpty()) ? "" : "(idUsuario = " + idVendedor + " OR idUsuarioConsultor = " + idVendedor + ")";
  if (not filtroVendedor.isEmpty()) { filtros << filtroVendedor; }

  //-------------------------------------

  const QString fornecedor = qApp->sanitizeSQL(ui->comboBoxFornecedores->currentText());
  const QString filtroFornecedor = (fornecedor.isEmpty()) ? "" : "(fornecedores LIKE '%" + fornecedor + "%')";
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

  modelViewVenda.setFilter(filtros.join(" AND "));
}

void WidgetVenda::montaFiltroTexto() {
  if (ui->lineEditBusca->text().isEmpty()) { return montaFiltro(); }

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  QString filtroLoja;

  if (User::isGerente()) { filtroLoja = "idLoja = " + User::idLoja; }

  if (User::isVendedorOrEspecial()) { filtroLoja = "(Vendedor = '" + User::nome + "'" + " OR Consultor = '" + User::nome + "')"; }

  if (not filtroLoja.isEmpty()) { filtros << filtroLoja; }

  //-------------------------------------

  const QString filtroStatus = "status NOT IN ('CANCELADO')";

  filtros << filtroStatus;

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(Código LIKE '%" + textoBusca + "%' OR Vendedor LIKE '%" + textoBusca + "%' OR Cliente LIKE '%" + textoBusca + "%' OR Profissional LIKE '%" + textoBusca +
                              "%' OR `OC Rep` LIKE '%" + textoBusca + "%')";

  filtros << filtroBusca;

  //-------------------------------------

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
  } catch (std::exception &) {}

  setConnections();

  montaFiltro();
}

void WidgetVenda::setPermissions() {
  unsetConnections();

  try {
    listarLojas();

    if (User::isAdministrativo()) { ui->groupBoxMes->setChecked(true); }

    if (User::isGerente()) { ui->groupBoxLojas->hide(); }

    if (User::isVendedorOrEspecial()) { ui->groupBoxVendedores->hide(); }

    // TODO: remove this?
    if (User::nome == "VIVIANE") { ui->groupBoxVendedores->show(); }

    (User::isVendedorOrEspecial()) ? ui->radioButtonProprios->setChecked(true) : ui->radioButtonTodos->setChecked(true);

    ui->comboBoxLojas->setCurrentValue(User::idLoja);

    fillComboBoxVendedor();

    ui->dateEditMes->setDate(qApp->serverDate());
    ui->dateEditDia->setDate(qApp->serverDate());
  } catch (std::exception &) {}

  setConnections();
}

void WidgetVenda::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetVenda::montaFiltroTexto, connectionType);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxConferido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxDevolvido, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
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
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxPendente2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxRepoEntrega, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->checkBoxRepoReceb, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->comboBoxFornecedores, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->comboBoxLojas, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetVenda::on_comboBoxLojas_currentIndexChanged, connectionType);
  connect(ui->comboBoxVendedores, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->dateEditDia, &QDateEdit::dateChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->groupBoxDia, &QGroupBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatus_toggled, connectionType);
  connect(ui->groupBoxStatusFinanceiro, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatusFinanceiro_toggled, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetVenda::delayFiltro, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetVenda::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetVenda::on_table_activated, connectionType);
}

void WidgetVenda::unsetConnections() {
  blockingSignals.push(0);

  disconnect(&timer, &QTimer::timeout, this, &WidgetVenda::montaFiltroTexto);
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->checkBoxCancelado2, &QCheckBox::toggled, this, &WidgetVenda::montaFiltro);
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
  disconnect(ui->comboBoxFornecedores, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxLojas, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->comboBoxLojas, qOverload<int>(&QComboBox::currentIndexChanged), this, &WidgetVenda::on_comboBoxLojas_currentIndexChanged);
  disconnect(ui->comboBoxVendedores, &ComboBox::currentTextChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->dateEditDia, &QDateEdit::dateChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetVenda::montaFiltro);
  disconnect(ui->groupBoxDia, &QGroupBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatus_toggled);
  disconnect(ui->groupBoxStatusFinanceiro, &QGroupBox::toggled, this, &WidgetVenda::on_groupBoxStatusFinanceiro_toggled);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetVenda::delayFiltro);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetVenda::on_pushButtonFollowup_clicked);
  disconnect(ui->radioButtonProprios, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetVenda::montaFiltro);
  disconnect(ui->table, &TableView::activated, this, &WidgetVenda::on_table_activated);
}

void WidgetVenda::delayFiltro() { timer.start(500); }

void WidgetVenda::fillComboBoxVendedor() {
  ui->comboBoxVendedores->clear();

  const QString filtroLoja = ui->comboBoxLojas->currentText().isEmpty() ? "" : " AND idLoja = " + ui->comboBoxLojas->getCurrentValue().toString();

  SqlQuery query;

  if (not query.exec("SELECT idUsuario, nome FROM usuario WHERE tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')" + filtroLoja + " ORDER BY nome")) {
    throw RuntimeException("Erro: " + query.lastError().text(), this);
  }

  ui->comboBoxVendedores->addItem("");

  while (query.next()) { ui->comboBoxVendedores->addItem(query.value("nome").toString(), query.value("idUsuario")); }
}

void WidgetVenda::updateTables() {
  if (not isSet) {
    setComboBoxFornecedores();
    setPermissions();
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewVenda.select();
}

void WidgetVenda::setComboBoxFornecedores() {
  unsetConnections();

  try {
    ui->comboBoxFornecedores->clear();

    SqlQuery query;

    if (not query.exec("SELECT razaoSocial FROM fornecedor")) { throw RuntimeException("Erro buscando fornecedores: " + query.lastError().text(), this); }

    ui->comboBoxFornecedores->addItem("");

    while (query.next()) { ui->comboBoxFornecedores->addItem(query.value("razaoSocial").toString()); }
  } catch (std::exception &) {}

  setConnections();
}

void WidgetVenda::on_table_activated(const QModelIndex index) {
  auto *venda = new Venda(this);
  venda->setAttribute(Qt::WA_DeleteOnClose);
  if (financeiro) { venda->setFinanceiro(); }
  venda->viewRegisterById(modelViewVenda.data(index.row(), "Código"));

  venda->show();
}

void WidgetVenda::on_comboBoxLojas_currentIndexChanged() {
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

void WidgetVenda::setFinanceiro() {
  ui->groupBoxStatusFinanceiro->show();
  financeiro = true;
}

void WidgetVenda::resetTables() { modelIsSet = false; }

void WidgetVenda::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString codigo = modelViewVenda.data(list.first().row(), "Código").toString();

  FollowUp *followup = new FollowUp(codigo, FollowUp::Tipo::Venda, this);
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
  } catch (std::exception &) {}

  setConnections();

  montaFiltro();
}

void WidgetVenda::listarLojas() {
  SqlQuery query;

  if (not query.exec("SELECT descricao, idLoja FROM loja WHERE desativado = FALSE ORDER BY descricao")) { throw RuntimeException("Erro: " + query.lastError().text()); }

  while (query.next()) { ui->comboBoxLojas->addItem(query.value("descricao").toString(), query.value("idLoja")); }
}
