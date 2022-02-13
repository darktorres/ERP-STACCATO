#include "tablogistica.h"
#include "ui_tablogistica.h"

#include <QDebug>
#include <QSqlError>

TabLogistica::TabLogistica(QWidget *parent) : QWidget(parent), ui(new Ui::TabLogistica) { ui->setupUi(this); }

TabLogistica::~TabLogistica() { delete ui; }

void TabLogistica::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &TabLogistica::on_pushButtonLimparFiltro_clicked, connectionType);
  connect(ui->tabWidgetLogistica, &QTabWidget::currentChanged, this, &TabLogistica::on_tabWidgetLogistica_currentChanged, connectionType);
  connect(ui->tableForn, &TableView::clicked, this, &TabLogistica::on_tableForn_clicked, connectionType);
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

    setupTables();
    setConnections();

    isSet = true;
  }

  //--------------------------------------------------------

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

  const auto selection = ui->tableForn->selectionModel()->selectedRows();

  modelViewLogistica.select();

  if (not selection.isEmpty()) { ui->tableForn->selectRow(selection.first().row()); }
}

void TabLogistica::on_tableForn_clicked(const QModelIndex &index) {
  const QString fornecedor = index.isValid() ? modelViewLogistica.data(index.row(), "fornecedor").toString() : "";

  const QString currentTab = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentTab == "Agendar Coleta") { ui->widgetAgendarColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Coleta") { ui->widgetColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Recebimento") { ui->widgetRecebimento->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Representação") { ui->widgetRepresentacao->tableFornLogistica_clicked(fornecedor); }
}

void TabLogistica::setupTables() {
  const QString currentTab = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  ui->frameForn->hide();

  if (currentTab == "Agendar Coleta") { modelViewLogistica.setTable("view_fornecedor_logistica_agendar_coleta"); }
  if (currentTab == "Coleta") { modelViewLogistica.setTable("view_fornecedor_logistica_coleta"); }
  if (currentTab == "Recebimento") { modelViewLogistica.setTable("view_fornecedor_logistica_recebimento"); }
  if (currentTab == "Representação") { modelViewLogistica.setTable("view_fornecedor_logistica_representacao"); }

  if (currentTab == "Agendar Coleta") { ui->widgetAgendarColeta->tableFornLogistica_clicked(""); }
  if (currentTab == "Coleta") { ui->widgetColeta->tableFornLogistica_clicked(""); }
  if (currentTab == "Recebimento") { ui->widgetRecebimento->tableFornLogistica_clicked(""); }
  if (currentTab == "Representação") { ui->widgetRepresentacao->tableFornLogistica_clicked(""); }

  modelViewLogistica.setFilter("");

  if (currentTab == "Agendar Coleta" or currentTab == "Coleta" or currentTab == "Recebimento" or currentTab == "Representação") {
    ui->frameForn->show();

    modelViewLogistica.select();
  }

  ui->tableForn->setModel(&modelViewLogistica);

  ui->tableForn->sortByColumn("Fornecedor");
}

void TabLogistica::on_tabWidgetLogistica_currentChanged() {
  setupTables();
  updateTables();
}

void TabLogistica::on_pushButtonLimparFiltro_clicked() {
  ui->tableForn->clearSelection();

  const QString fornecedor = "";

  const QString currentTab = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentTab == "Agendar Coleta") { ui->widgetAgendarColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Coleta") { ui->widgetColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Recebimento") { ui->widgetRecebimento->tableFornLogistica_clicked(fornecedor); }
  if (currentTab == "Representação") { ui->widgetRepresentacao->tableFornLogistica_clicked(fornecedor); }
}

// TODO: 1followup das entregas (no lugar de followup colocar campo observacao no inputDialog?)
// TODO: 5colocar aba para fazer cotacao frete, puxar os orcamentos abertos com o peso das caixas para calcular frete
// TODO: 0verificar nos cancelamentos se estou removendo as datas/previsoes corretamente
