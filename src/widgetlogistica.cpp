#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "ui_widgetlogistica.h"
#include "widgetlogistica.h"

WidgetLogistica::WidgetLogistica(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogistica) {
  ui->setupUi(this);

  ui->splitter_6->setStretchFactor(0, 0);
  ui->splitter_6->setStretchFactor(1, 1);

  setConnections();

  ui->tableForn->setModel(&modelViewLogistica);
}

WidgetLogistica::~WidgetLogistica() { delete ui; }

void WidgetLogistica::setConnections() {
  connect(ui->tableForn, &TableView::clicked, this, &WidgetLogistica::on_tableForn_clicked);
  connect(ui->tabWidgetLogistica, &QTabWidget::currentChanged, this, &WidgetLogistica::on_tabWidgetLogistica_currentChanged);
}

void WidgetLogistica::resetTables() {
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

void WidgetLogistica::updateTables() {
  const QString currentText = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  ui->frameForn->hide();

  if (currentText == "Agendar Coleta") { modelViewLogistica.setTable("view_fornecedor_logistica_agendar_coleta"); }
  if (currentText == "Coleta") { modelViewLogistica.setTable("view_fornecedor_logistica_coleta"); }
  if (currentText == "Recebimento") { modelViewLogistica.setTable("view_fornecedor_logistica_recebimento"); }
  if (currentText == "Representação") { modelViewLogistica.setTable("view_fornecedor_logistica_representacao"); }

  modelViewLogistica.setFilter("");

  if (currentText == "Agendar Coleta" or currentText == "Coleta" or currentText == "Recebimento" or currentText == "Representação") {
    ui->frameForn->show();

    if (not modelViewLogistica.select()) { return; }

    ui->tableForn->resizeColumnsToContents();
  }

  //--------------------------------------------------------

  if (currentText == "Agendar Coleta") { ui->widgetAgendarColeta->updateTables(); }
  if (currentText == "Coleta") { ui->widgetColeta->updateTables(); }
  if (currentText == "Recebimento") { ui->widgetRecebimento->updateTables(); }
  if (currentText == "Agendar Entrega") { ui->widgetAgendaEntrega->updateTables(); }
  if (currentText == "Entregas") { ui->widgetCalendarioEntrega->updateTables(); }
  if (currentText == "Caminhões") { ui->widgetCaminhao->updateTables(); }
  if (currentText == "Representação") { ui->widgetRepresentacao->updateTables(); }
  if (currentText == "Entregues") { ui->widgetEntregues->updateTables(); }
  if (currentText == "Calendário") { ui->widgetCalendario->updateTables(); }
}

void WidgetLogistica::on_tableForn_clicked(const QModelIndex &index) {
  const QString fornecedor = index.isValid() ? modelViewLogistica.data(index.row(), "fornecedor").toString() : "";

  const QString currentText = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentText == "Agendar Coleta") { ui->widgetAgendarColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentText == "Coleta") { ui->widgetColeta->tableFornLogistica_clicked(fornecedor); }
  if (currentText == "Recebimento") { ui->widgetRecebimento->tableFornLogistica_clicked(fornecedor); }
  if (currentText == "Representação") { ui->widgetRepresentacao->tableFornLogistica_clicked(fornecedor); }
}

void WidgetLogistica::on_tabWidgetLogistica_currentChanged(const int) { updateTables(); }

// NOTE: tela para guardar imagens (fotos/documentos scaneados)
// TODO: 1followup das entregas (no lugar de followup colocar campo observacao no inputDialog?)
// TODO: 5colocar aba para fazer cotacao frete, puxar os orcamentos abertos com o peso das caixas para calcular frete
// TODO: 0verificar nos cancelamentos se estou removendo as datas/previsoes corretamente
// TODO: colocar botao para importar nota de saida gerada por fora e fazer as associacoes
