#include "tabgalpao.h"
#include "ui_tabgalpao.h"

TabGalpao::TabGalpao(QWidget *parent) : QWidget(parent), ui(new Ui::TabGalpao) {
  ui->setupUi(this);
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
  if (currentTab == "Peso") { ui->widgetEstoquePeso->updateTables(); }
}

void TabGalpao::resetTables() {
  ui->widgetGalpao->resetTables();
  ui->widgetEstoquePeso->resetTables();
}

void TabGalpao::on_tabWidget_currentChanged(const int) { updateTables(); }
