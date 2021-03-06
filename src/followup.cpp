#include "followup.h"
#include "ui_followup.h"

#include "application.h"
#include "followupproxymodel.h"
#include "user.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

FollowUp::FollowUp(const QString &id, const Tipo tipo, QWidget *parent) : QDialog(parent), id(id), tipo(tipo), ui(new Ui::FollowUp) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  setWindowTitle((tipo == Tipo::Orcamento ? "Orçamento: " : "Pedido: ") + id);

  ui->dateFollowup->setDateTime(qApp->serverDateTime());
  ui->dateProxFollowup->setDateTime(qApp->serverDateTime().addDays(1));

  if (tipo == Tipo::Venda) { ui->frameOrcamento->hide(); }

  setConnections();
}

FollowUp::~FollowUp() { delete ui; }

void FollowUp::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateFollowup, &QDateTimeEdit::dateChanged, this, &FollowUp::on_dateFollowup_dateChanged, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &FollowUp::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &FollowUp::on_pushButtonSalvar_clicked, connectionType);
}

void FollowUp::on_pushButtonCancelar_clicked() { close(); }

void FollowUp::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  SqlQuery query;
  if (tipo == Tipo::Orcamento) {
    query.prepare("INSERT INTO orcamento_has_followup (idOrcamento, idOrcamentoBase, idLoja, idUsuario, semaforo, observacao, dataFollowup, dataProxFollowup) VALUES (:idOrcamento, :idOrcamentoBase, "
                  ":idLoja, :idUsuario, :semaforo, :observacao, :dataFollowup, :dataProxFollowup)");
    query.bindValue(":idOrcamento", id);
    query.bindValue(":idOrcamentoBase", id.left(11));
    query.bindValue(":idLoja", User::idLoja);
    query.bindValue(":idUsuario", User::idUsuario);
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
    query.bindValue(":idLoja", User::idLoja);
    query.bindValue(":idUsuario", User::idUsuario);
    query.bindValue(":observacao", ui->plainTextEdit->toPlainText());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
  }

  if (not query.exec()) { throw RuntimeException("Erro salvando followup: " + query.lastError().text(), this); }

  qApp->enqueueInformation("Followup salvo com sucesso!", this);
  close();
}

bool FollowUp::verifyFields() {
  if (tipo == Tipo::Orcamento and not ui->radioButtonQuente->isChecked() and not ui->radioButtonMorno->isChecked() and not ui->radioButtonFrio->isChecked()) {
    throw RuntimeError("Deve selecionar uma temperatura!", this);
  }

  if (ui->plainTextEdit->toPlainText().isEmpty()) { throw RuntimeError("Deve escrever uma observação!", this); }

  return true;
}

void FollowUp::setupTables() {
  modelViewFollowup.setTable("view_followup_" + QString(tipo == Tipo::Orcamento ? "orcamento" : "venda"));

  if (tipo == Tipo::Orcamento) {
    modelViewFollowup.setHeaderData("idOrcamento", "Orçamento");
    modelViewFollowup.setHeaderData("dataProxFollowup", "Próx. Data");
  }

  if (tipo == Tipo::Venda) { modelViewFollowup.setHeaderData("idVenda", "Venda"); }

  modelViewFollowup.setHeaderData("nome", "Usuário");
  modelViewFollowup.setHeaderData("observacao", "Observação");
  modelViewFollowup.setHeaderData("dataFollowup", "Data");

  modelViewFollowup.setFilter(tipo == Tipo::Orcamento ? "idOrcamento LIKE '" + id.left(12) + "%'" : "idVenda LIKE '" + id.left(11) + "%'");

  modelViewFollowup.setSort("dataFollowup");

  modelViewFollowup.select();

  modelViewFollowup.proxyModel = new FollowUpProxyModel(&modelViewFollowup, this);

  ui->table->setModel(&modelViewFollowup);

  if (tipo == Tipo::Orcamento) { ui->table->hideColumn("semaforo"); }

  if (tipo == Tipo::Orcamento) {
    modelOrcamento.setTable("orcamento");

    modelOrcamento.setFilter("idOrcamento = '" + id + "'");

    modelOrcamento.select();

    ui->plainTextEditBaixa->setPlainText(modelOrcamento.data(0, "motivoCancelamento").toString() + "\n\n" + modelOrcamento.data(0, "observacaoCancelamento").toString());
  }
}

void FollowUp::on_dateFollowup_dateChanged(const QDate &date) {
  if (ui->dateProxFollowup->date() < date) { ui->dateProxFollowup->setDate(date); }
}
