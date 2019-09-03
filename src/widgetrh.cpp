#include "widgetrh.h"
#include "application.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "ui_widgetrh.h"

WidgetRh::WidgetRh(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetRh) { ui->setupUi(this); }

WidgetRh::~WidgetRh() { delete ui; }

void WidgetRh::resetTables() { modelIsSet = false; }

void WidgetRh::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(QDate::currentDate());
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not modelRelatorio.select()) { qApp->enqueueError("Erro atualizando tabela: " + modelRelatorio.lastError().text(), this); }

  if (not modelTotal.select()) { qApp->enqueueError("Erro atualizando tabela total: " + modelTotal.lastError().text(), this); }
}

void WidgetRh::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetRh::on_dateEdit_dateChanged, connectionType);
  connect(ui->pushButtonSalvarMes, &QPushButton::clicked, this, &WidgetRh::on_pushButtonSalvarMes_clicked, connectionType);
}

void WidgetRh::setupTables() {
  modelRelatorio.setTable("comissao");

  modelRelatorio.setHeaderData("idVenda", "Venda");

  ui->table->setModel(&modelRelatorio);

  ui->table->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("%", new PorcentagemDelegate(this));

  ui->table->hideColumn("idComissao");
  ui->table->hideColumn("Mês");
  ui->table->hideColumn("idUsuario");

  // -------------------------------------------------------------------------

  modelTotal.setTable("view_relatorio_vendedor2");

  ui->tableTotal->setModel(&modelTotal);

  ui->tableTotal->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotal->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableTotal->setItemDelegateForColumn("%", new PorcentagemDelegate(this));

  ui->tableTotal->hideColumn("idUsuario");
  ui->tableTotal->hideColumn("Mês");
}

void WidgetRh::on_pushButtonSalvarMes_clicked() {
  const QDate mes = ui->dateEdit->date();

  QSqlQuery queryVerifica;
  queryVerifica.prepare("SELECT * FROM comissao WHERE Mês = :mes");
  queryVerifica.bindValue(":mes", mes.toString("yyyy-MM"));

  if (not queryVerifica.exec()) { return qApp->enqueueError("Erro buscando dados da comissão: " + queryVerifica.lastError().text(), this); }

  if (queryVerifica.first()) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Este mês já está cadastrado. Deseja substituir os dados?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Substituir");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }

    QSqlQuery queryRemove;
    queryRemove.prepare("DELETE FROM comissao WHERE Mês = :mes");
    queryRemove.bindValue(":mes", mes.toString("yyyy-MM"));

    if (not queryRemove.exec()) { return qApp->enqueueError("Erro removendo dados: " + queryRemove.lastError().text()); }
  }

  QSqlQuery queryRelatorio;
  queryRelatorio.prepare("SELECT * FROM view_relatorio WHERE Mês = :mes");
  queryRelatorio.bindValue(":mes", mes.toString("yyyy-MM"));

  // get data from view_relatorio
  if (not queryRelatorio.exec()) { return qApp->enqueueError("Erro buscando dados da comissão: " + queryRelatorio.lastError().text(), this); }

  if (not queryRelatorio.first()) { return qApp->enqueueInformation("Não há dados para este mês!", this); }

  // insert last month data into 'comissao'
  for (int i = 0; i < queryRelatorio.size(); ++i, queryRelatorio.next()) {
    int newRow = modelRelatorio.insertRowAtEnd();

    if (not modelRelatorio.setData(newRow, "Loja", queryRelatorio.value("Loja"))) { return; }
    if (not modelRelatorio.setData(newRow, "Vendedor", queryRelatorio.value("Vendedor"))) { return; }
    if (not modelRelatorio.setData(newRow, "idUsuario", queryRelatorio.value("idUsuario"))) { return; }
    if (not modelRelatorio.setData(newRow, "idVenda", queryRelatorio.value("idVenda"))) { return; }
    if (not modelRelatorio.setData(newRow, "Mês", queryRelatorio.value("Mês"))) { return; }
    if (not modelRelatorio.setData(newRow, "Faturamento", queryRelatorio.value("Faturamento"))) { return; }
    if (not modelRelatorio.setData(newRow, "Comissão", queryRelatorio.value("Comissão"))) { return; }
    if (not modelRelatorio.setData(newRow, "%", queryRelatorio.value("%"))) { return; }
  }

  if (not modelRelatorio.submitAll()) { return qApp->enqueueError("Erro salvando dados: " + modelRelatorio.lastError().text(), this); }

  // -------------------------------------------------------------------------

  updateTables();

  qApp->enqueueInformation("Dados salvos com sucesso!", this);
}

void WidgetRh::on_dateEdit_dateChanged(const QDate &date) {
  modelRelatorio.setFilter("Mês = '" + date.toString("yyyy-MM") + "'");

  if (not modelRelatorio.select()) { return qApp->enqueueError("Erro filtrando tabela: " + modelRelatorio.lastError().text(), this); }

  // -------------------------------------------------------------------------

  modelTotal.setFilter("Mês = '" + date.toString("yyyy-MM") + "'");

  if (not modelTotal.select()) { return qApp->enqueueError("Erro filtrando tabela total: " + modelTotal.lastError().text()); }
}
