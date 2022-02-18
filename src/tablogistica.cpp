#include "tablogistica.h"
#include "ui_tablogistica.h"

#include <QDebug>
#include <QSqlError>

TabLogistica::TabLogistica(QWidget *parent) : QWidget(parent), ui(new Ui::TabLogistica) { ui->setupUi(this); }

TabLogistica::~TabLogistica() { delete ui; }

void TabLogistica::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tabWidgetLogistica, &QTabWidget::currentChanged, this, &TabLogistica::on_tabWidgetLogistica_currentChanged, connectionType);
}

void TabLogistica::resetTables() {
  ui->widgetAgendarColeta->resetTables();
  ui->widgetColeta->resetTables();
  ui->widgetRecebimento->resetTables();
  ui->widgetAgendaEntrega->resetTables();
  ui->widgetCalendarioEntrega->resetTables();
  ui->widgetCaminhao->resetTables();
  ui->widgetRepresentacao->resetTables();
  ui->widgetEntregues->resetTables();
  ui->widgetCalendario->resetTables();
}

void TabLogistica::updateTables() {
  if (not isSet) {
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 0);
    setConnections();
    isSet = true;
  }

  const QString currentTab = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentTab == "Agendar Coleta") { ui->widgetAgendarColeta->updateTables(); }
  if (currentTab == "Coleta") { ui->widgetColeta->updateTables(); }
  if (currentTab == "Recebimento") { ui->widgetRecebimento->updateTables(); }
  if (currentTab == "Agendar Entrega") { ui->widgetAgendaEntrega->updateTables(); }
  if (currentTab == "Entregas") { ui->widgetCalendarioEntrega->updateTables(); }
  if (currentTab == "Caminhões") { ui->widgetCaminhao->updateTables(); }
  if (currentTab == "Representação") { ui->widgetRepresentacao->updateTables(); }
  if (currentTab == "Entregues") { ui->widgetEntregues->updateTables(); }
  if (currentTab == "Calendário") { ui->widgetCalendario->updateTables(); }
  if (currentTab == "Devolução") { ui->widgetDevolucao->updateTables(); }
}

void TabLogistica::on_tabWidgetLogistica_currentChanged() { updateTables(); }

// TODO: 1followup das entregas (no lugar de followup colocar campo observacao no inputDialog?)
// TODO: 5colocar aba para fazer cotacao frete, puxar os orcamentos abertos com o peso das caixas para calcular frete
// TODO: 0verificar nos cancelamentos se estou removendo as datas/previsoes corretamente
