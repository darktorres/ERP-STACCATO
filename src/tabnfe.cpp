#include "tabnfe.h"
#include "ui_tabnfe.h"

#include "permissao.h"

TabNFe::TabNFe(QWidget *parent) : QWidget(parent), ui(new Ui::TabNFe) {
  ui->setupUi(this);

  // Marca as telas-filhas antes de aplicar: a poda de aplicarTela para nessas subarvores
  // (cada uma tem prefixo proprio), entao aqui so as paginas do QTabWidget sao gateadas.
  ui->widgetEntrada->setProperty("prefixoPermissao", "nfe.entrada");
  ui->widgetSaida->setProperty("prefixoPermissao", "nfe.saida");
  ui->widgetDistribuicao->setProperty("prefixoPermissao", "nfe.distribuicao");

  Permissao::aplicarTela(this, "nfe");

  setConnections();
}

TabNFe::~TabNFe() { delete ui; }

void TabNFe::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tabWidgetNfe, &QTabWidget::currentChanged, this, &TabNFe::on_tabWidgetNfe_currentChanged, connectionType);
}

void TabNFe::updateTables() {
  const QString currentTab = ui->tabWidgetNfe->tabText(ui->tabWidgetNfe->currentIndex());

  if (currentTab == "Entrada") { ui->widgetEntrada->updateTables(); }
  if (currentTab == "Saída") { ui->widgetSaida->updateTables(); }
  if (currentTab == "Distribuição") { ui->widgetDistribuicao->updateTables(); }
}

void TabNFe::resetTables() {
  ui->widgetEntrada->resetTables();
  ui->widgetSaida->resetTables();
  ui->widgetDistribuicao->resetTables();
}

void TabNFe::on_tabWidgetNfe_currentChanged() { updateTables(); }

// TODO: colocar uma terceira aba para NF-es de produtos comprados em lojas de terceiro para reposicao (não devem entrar na tela de NF-es da staccato pois não entram no financeiro/não devem ter
// impostos calculados/não devem ser enviadas para a contabilidade) não deve criar estoque??
