#include "tabgalpao.h"
#include "ui_tabgalpao.h"

#include "permissao.h"

TabGalpao::TabGalpao(QWidget *parent) : QWidget(parent), ui(new Ui::TabGalpao) {
  ui->setupUi(this);

  // Marca as telas-filhas antes de aplicar: a poda de aplicarTela para nessas subarvores
  // (cada uma tem prefixo proprio), entao aqui so as paginas do QTabWidget sao gateadas.
  ui->widgetGalpao->setProperty("prefixoPermissao", "galpao.galpao");
  ui->widgetGalpaoPeso->setProperty("prefixoPermissao", "galpao.peso");

  Permissao::aplicarTela(this, "galpao");

  setConnections();
}

TabGalpao::~TabGalpao() { delete ui; }

void TabGalpao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &TabGalpao::on_tabWidget_currentChanged, connectionType);
}

void TabGalpao::updateTables() {
  const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentTab == "Galpão") { ui->widgetGalpao->updateTables(); }
  if (currentTab == "Peso") { ui->widgetGalpaoPeso->updateTables(); }
}

void TabGalpao::resetTables() {
  ui->widgetGalpao->resetTables();
  ui->widgetGalpaoPeso->resetTables();
}

void TabGalpao::on_tabWidget_currentChanged() { updateTables(); }
