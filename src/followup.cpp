#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "followup.h"
#include "followupproxymodel.h"
#include "ui_followup.h"
#include "usersession.h"

FollowUp::FollowUp(const QString &id, const Tipo tipo, QWidget *parent) : QDialog(parent), id(id), tipo(tipo), ui(new Ui::FollowUp) {
  ui->setupUi(this);

  connect(ui->dateFollowup, &QDateTimeEdit::dateChanged, this, &FollowUp::on_dateFollowup_dateChanged);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &FollowUp::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &FollowUp::on_pushButtonSalvar_clicked);

  setWindowFlags(Qt::Window);

  setupTables();

  setWindowTitle((tipo == Tipo::Orcamento ? "Orçamento: " : "Pedido: ") + id);

  ui->dateFollowup->setDateTime(QDateTime::currentDateTime());
  ui->dateProxFollowup->setDateTime(QDateTime::currentDateTime().addDays(1));

  if (tipo == Tipo::Venda) { ui->frameOrcamento->hide(); }
}

FollowUp::~FollowUp() { delete ui; }

void FollowUp::on_pushButtonCancelar_clicked() { close(); }

void FollowUp::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  QSqlQuery query;
  if (tipo == Tipo::Orcamento) {
    query.prepare("INSERT INTO orcamento_has_followup (idOrcamento, idOrcamentoBase, idLoja, idUsuario, semaforo, observacao, dataFollowup, dataProxFollowup) VALUES (:idOrcamento, :idOrcamentoBase, "
                  ":idLoja, :idUsuario, :semaforo, :observacao, :dataFollowup, :dataProxFollowup)");
    query.bindValue(":idOrcamento", id);
    query.bindValue(":idOrcamentoBase", id.left(11));
    query.bindValue(":idLoja", UserSession::idLoja());
    query.bindValue(":idUsuario", UserSession::idUsuario());
    query.bindValue(":semaforo", ui->radioButtonQuente->isChecked() ? 1 : ui->radioButtonMorno->isChecked() ? 2 : ui->radioButtonFrio->isChecked() ? 3 : 0);
    query.bindValue(":observacao", ui->plainTextEdit->toPlainText());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
    query.bindValue(":dataProxFollowup", ui->dateProxFollowup->dateTime());
  }

  if (tipo == Tipo::Venda) {
    query.prepare(
        "INSERT INTO venda_has_followup (idVenda, idVendaBase, idLoja, idUsuario, observacao, dataFollowup) VALUES (:idVenda, :idVendaBase, :idLoja, :idUsuario, :observacao, :dataFollowup)");
    query.bindValue(":idVenda", id);
    query.bindValue(":idVendaBase", id.left(11));
    query.bindValue(":idLoja", UserSession::idLoja());
    query.bindValue(":idUsuario", UserSession::idUsuario());
    query.bindValue(":observacao", ui->plainTextEdit->toPlainText());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
  }

  if (not query.exec()) { return qApp->enqueueError("Erro salvando followup: " + query.lastError().text(), this); }

  qApp->enqueueInformation("Followup salvo com sucesso!", this);
  close();
}

bool FollowUp::verifyFields() {
  if (tipo == Tipo::Orcamento and not ui->radioButtonQuente->isChecked() and not ui->radioButtonMorno->isChecked() and not ui->radioButtonFrio->isChecked()) {
    return qApp->enqueueError(false, "Deve selecionar uma temperatura!", this);
  }

  if (ui->plainTextEdit->toPlainText().isEmpty()) { return qApp->enqueueError(false, "Deve escrever uma observação!", this); }

  return true;
}

void FollowUp::setupTables() {
  modelViewFollowup.setTable("view_followup_" + QString(tipo == Tipo::Orcamento ? "orcamento" : "venda"));

  modelViewFollowup.setHeaderData("idOrcamento", "Orçamento");
  modelViewFollowup.setHeaderData("idVenda", "Venda");
  modelViewFollowup.setHeaderData("nome", "Usuário");
  modelViewFollowup.setHeaderData("observacao", "Observação");
  modelViewFollowup.setHeaderData("dataFollowup", "Data");
  modelViewFollowup.setHeaderData("dataProxFollowup", "Próx. Data");

  modelViewFollowup.setFilter(tipo == Tipo::Orcamento ? "idOrcamento LIKE '" + id.left(12) + "%'" : "idVenda LIKE '" + id.left(11) + "%'");

  if (not modelViewFollowup.select()) { return; }

  modelViewFollowup.proxyModel = new FollowUpProxyModel(&modelViewFollowup, this);

  ui->table->setModel(&modelViewFollowup);

  ui->table->hideColumn("semaforo");
}

void FollowUp::on_dateFollowup_dateChanged(const QDate &date) {
  if (ui->dateProxFollowup->date() < date) { ui->dateProxFollowup->setDate(date); }
}
