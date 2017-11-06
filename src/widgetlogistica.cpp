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

  ui->tableForn->setModel(&model);
}

WidgetLogistica::~WidgetLogistica() { delete ui; }

void WidgetLogistica::setConnections() {
  connect(ui->widgetCalendarioEntrega, &WidgetLogisticaEntregas::errorSignal, this, &WidgetLogistica::errorSignal);
  connect(ui->widgetAgendarColeta, &WidgetLogisticaAgendarColeta::errorSignal, this, &WidgetLogistica::errorSignal);
  connect(ui->widgetRecebimento, &WidgetLogisticaRecebimento::errorSignal, this, &WidgetLogistica::errorSignal);
  connect(ui->widgetAgendaEntrega, &WidgetLogisticaAgendarEntrega::errorSignal, this, &WidgetLogistica::errorSignal);

  connect(ui->widgetCalendarioEntrega, &WidgetLogisticaEntregas::transactionStarted, this, &WidgetLogistica::transactionStarted);
  connect(ui->widgetAgendarColeta, &WidgetLogisticaAgendarColeta::transactionStarted, this, &WidgetLogistica::transactionStarted);
  connect(ui->widgetRecebimento, &WidgetLogisticaRecebimento::transactionStarted, this, &WidgetLogistica::transactionStarted);
  connect(ui->widgetAgendaEntrega, &WidgetLogisticaAgendarEntrega::transactionStarted, this, &WidgetLogistica::transactionStarted);

  connect(ui->widgetCalendarioEntrega, &WidgetLogisticaEntregas::transactionEnded, this, &WidgetLogistica::transactionEnded);
  connect(ui->widgetAgendarColeta, &WidgetLogisticaAgendarColeta::transactionEnded, this, &WidgetLogistica::transactionEnded);
  connect(ui->widgetRecebimento, &WidgetLogisticaRecebimento::transactionEnded, this, &WidgetLogistica::transactionEnded);
  connect(ui->widgetAgendaEntrega, &WidgetLogisticaAgendarEntrega::transactionEnded, this, &WidgetLogistica::transactionEnded);
}

bool WidgetLogistica::updateTables() {
  const QString currentText = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentText == "Agendar Coleta") {
    ui->frameForn->show();

    model.setTable("view_fornecedor_logistica_agendar_coleta");

    if (not model.select()) {
      emit errorSignal("Erro lendo tabela: " + model.lastError().text());
      return false;
    }

    ui->tableForn->resizeColumnsToContents();
    return ui->widgetAgendarColeta->updateTables();
  }

  if (currentText == "Coleta") {
    ui->frameForn->show();

    model.setTable("view_fornecedor_logistica_coleta");

    if (not model.select()) {
      emit errorSignal("Erro lendo tabela: " + model.lastError().text());
      return false;
    }

    ui->tableForn->resizeColumnsToContents();
    return ui->widgetColeta->updateTables();
  }

  if (currentText == "Recebimento") {
    ui->frameForn->show();

    model.setTable("view_fornecedor_logistica_recebimento");

    if (not model.select()) {
      emit errorSignal("Erro lendo tabela: " + model.lastError().text());
      return false;
    }

    ui->tableForn->resizeColumnsToContents();
    return ui->widgetRecebimento->updateTables();
  }

  if (currentText == "Agendar Entrega") {
    ui->frameForn->hide();
    return ui->widgetAgendaEntrega->updateTables();
  }

  if (currentText == "Entregas") {
    ui->frameForn->hide();
    return ui->widgetCalendarioEntrega->updateTables();
  }

  if (currentText == "Caminhões") {
    ui->frameForn->hide();
    return ui->widgetCaminhao->updateTables();
  }

  if (currentText == "Representação") {
    ui->frameForn->show();

    model.setTable("view_fornecedor_logistica_representacao");

    if (not model.select()) {
      emit errorSignal("Erro lendo tabela: " + model.lastError().text());
      return false;
    }

    ui->tableForn->resizeColumnsToContents();
    return ui->widgetRepresentacao->updateTables();
  }

  if (currentText == "Entregues") {
    ui->frameForn->hide();
    return ui->widgetEntregues->updateTables();
  }

  if (currentText == "Calendário") {
    ui->frameForn->hide();
    return ui->widgetCalendario->updateTables();
  }

  return true;
}

void WidgetLogistica::on_tableForn_activated(const QModelIndex &index) {
  const QString fornecedor = model.data(index.row(), "fornecedor").toString();

  const QString currentText = ui->tabWidgetLogistica->tabText(ui->tabWidgetLogistica->currentIndex());

  if (currentText == "Agendar Coleta") ui->widgetAgendarColeta->tableFornLogistica_activated(fornecedor);
  if (currentText == "Coleta") ui->widgetColeta->tableFornLogistica_activated(fornecedor);
  if (currentText == "Recebimento") ui->widgetRecebimento->tableFornLogistica_activated(fornecedor);
  if (currentText == "Representação") ui->widgetRepresentacao->tableFornLogistica_activated(fornecedor);
}

void WidgetLogistica::on_tabWidgetLogistica_currentChanged(const int) { updateTables(); }

// NOTE: tela para guardar imagens (fotos/documentos scaneados)
// TODO: 1followup das entregas (no lugar de followup colocar campo observacao no inputDialog?)
// TODO: 5colocar aba para fazer cotacao frete, puxar os orcamentos abertos com o peso das caixas para calcular frete
// TODO: 0verificar nos cancelamentos se estou removendo as datas/previsoes corretamente
