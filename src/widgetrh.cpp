#include "widgetrh.h"
#include "ui_widgetrh.h"

//#include "application.h"
//#include "porcentagemdelegate.h"
//#include "reaisdelegate.h"
//#include "sqlquery.h"

#include <QMessageBox>
#include <QSqlError>

WidgetRh::WidgetRh(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetRh) { ui->setupUi(this); }

WidgetRh::~WidgetRh() { delete ui; }

// void WidgetRh::resetTables() { setupTables(); }

// void WidgetRh::updateTables() {
//  if (not isSet) {
//    ui->dateEdit->setDate(qApp->serverDate());
//    setupTables();
//    setConnections();
//    isSet = true;
//  }

//  modelRelatorio.select();

//  modelTotal.select();
//}

// void WidgetRh::setConnections() {
//  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

//  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetRh::on_dateEdit_dateChanged, connectionType);
//  connect(ui->pushButtonSalvarMes, &QPushButton::clicked, this, &WidgetRh::on_pushButtonSalvarMes_clicked, connectionType);
//}

// void WidgetRh::setupTables() {
//  modelRelatorio.setTable("comissao");

//  modelRelatorio.setHeaderData("idVenda", "Venda");

//  ui->table->setModel(&modelRelatorio);

//  ui->table->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
//  ui->table->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
//  ui->table->setItemDelegateForColumn("porcentagem", new PorcentagemDelegate(false, this));

//  ui->table->hideColumn("idVendaProduto1");
//  ui->table->hideColumn("idComissao");
//  ui->table->hideColumn("Mês");
//  ui->table->hideColumn("idUsuario");

//  // -------------------------------------------------------------------------

//  modelTotal.setTable("view_relatorio_vendedor");

//  ui->tableTotal->setModel(&modelTotal);

//  ui->tableTotal->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
//  ui->tableTotal->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
//  ui->tableTotal->setItemDelegateForColumn("porcentagem", new PorcentagemDelegate(false, this));

//  ui->tableTotal->hideColumn("idUsuario");
//  ui->tableTotal->hideColumn("Mês");
//}

// void WidgetRh::on_pushButtonSalvarMes_clicked() {
//  const QDate mes = ui->dateEdit->date();

//  SqlQuery queryVerifica;
//  queryVerifica.prepare("SELECT * FROM comissao WHERE Mês = :mes");
//  queryVerifica.bindValue(":mes", mes.toString("yyyy-MM"));

//  if (not queryVerifica.exec()) { throw RuntimeException("Erro buscando dados da comissão: " + queryVerifica.lastError().text(), this); }

//  if (queryVerifica.first()) {
//    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Este mês já está cadastrado. Deseja substituir os dados?", QMessageBox::Yes | QMessageBox::No, this);
//    msgBox.setButtonText(QMessageBox::Yes, "Substituir");
//    msgBox.setButtonText(QMessageBox::No, "Voltar");

//    if (msgBox.exec() == QMessageBox::No) { return; }

//    SqlQuery queryRemove;
//    queryRemove.prepare("DELETE FROM comissao WHERE Mês = :mes");
//    queryRemove.bindValue(":mes", mes.toString("yyyy-MM"));

//    if (not queryRemove.exec()) { throw RuntimeException("Erro removendo dados: " + queryRemove.lastError().text(), this); }
//  }

//  SqlQuery queryRelatorio;
//  queryRelatorio.prepare("SELECT * FROM view_relatorio WHERE Mês = :mes");
//  queryRelatorio.bindValue(":mes", mes.toString("yyyy-MM"));

//  // get data from view_relatorio
//  if (not queryRelatorio.exec()) { throw RuntimeException("Erro buscando dados da comissão: " + queryRelatorio.lastError().text(), this); }

//  if (not queryRelatorio.first()) { return qApp->enqueueInformation("Não há dados para este mês!", this); }

//  // insert last month data into 'comissao'
//  for (int i = 0; i < queryRelatorio.size(); ++i, queryRelatorio.next()) {
//    int newRow = modelRelatorio.insertRowAtEnd();

//    modelRelatorio.setData(newRow, "Loja", queryRelatorio.value("Loja"));
//    modelRelatorio.setData(newRow, "Vendedor", queryRelatorio.value("Vendedor"));
//    modelRelatorio.setData(newRow, "idUsuario", queryRelatorio.value("idUsuario"));
//    modelRelatorio.setData(newRow, "idVenda", queryRelatorio.value("idVenda"));
//    modelRelatorio.setData(newRow, "Mês", queryRelatorio.value("Mês"));
//    modelRelatorio.setData(newRow, "Faturamento", queryRelatorio.value("Faturamento"));
//    modelRelatorio.setData(newRow, "Comissão", queryRelatorio.value("Comissão"));
//    modelRelatorio.setData(newRow, "porcentagem", queryRelatorio.value("porcentagem"));
//  }

//  modelRelatorio.submitAll();

//  // -------------------------------------------------------------------------

//  updateTables();

//  qApp->enqueueInformation("Dados salvos com sucesso!", this);
//}

// void WidgetRh::on_dateEdit_dateChanged(const QDate &date) {
//  modelRelatorio.setFilter("Mês = '" + date.toString("yyyy-MM") + "'");

//  modelRelatorio.select();

//  // -------------------------------------------------------------------------

//  modelTotal.setFilter("Mês = '" + date.toString("yyyy-MM") + "'");

//  modelTotal.select();
//}
