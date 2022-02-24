#include "followup.h"
#include "ui_followup.h"

#include "application.h"
#include "followupproxymodel.h"
#include "user.h"

#include <QDebug>
#include <QSqlError>

FollowUp::FollowUp(const QString &id, const Tipo tipo, QWidget *parent) : QDialog(parent), id(id), tipo(tipo), ui(new Ui::FollowUp) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  if (tipo == Tipo::Orcamento) { setWindowTitle("Orçamento: " + id); }
  if (tipo == Tipo::Venda) { setWindowTitle("Pedido: " + id); }
  if (tipo == Tipo::Compra) { setWindowTitle("O.C.: " + id); }
  if (tipo == Tipo::Estoque) { setWindowTitle("Estoque: " + id); }
  if (tipo == Tipo::NFe) { setWindowTitle("NFe: " + id); }

  ui->dateFollowup->setDateTime(qApp->serverDateTime());
  ui->dateProxFollowup->setDateTime(qApp->serverDateTime().addDays(1));

  if (tipo != Tipo::Orcamento) { ui->frameOrcamento->hide(); }

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
  verifyFields();

  SqlQuery query;

  if (tipo == Tipo::Orcamento) {
    query.prepare("INSERT INTO orcamento_has_followup (idOrcamento, idOrcamentoBase, idLoja, idUsuario, semaforo, observacao, dataFollowup, dataProxFollowup) VALUES (:idOrcamento, :idOrcamentoBase, "
                  ":idLoja, :idUsuario, :semaforo, :observacao, :dataFollowup, :dataProxFollowup)");
    query.bindValue(":idOrcamento", id);
    query.bindValue(":idOrcamentoBase", id.left(11));
    query.bindValue(":idLoja", User::idLoja);
    query.bindValue(":idUsuario", User::idUsuario);
    query.bindValue(":semaforo", ui->radioButtonQuente->isChecked() ? 1 : ui->radioButtonMorno->isChecked() ? 2 : ui->radioButtonFrio->isChecked() ? 3 : 0);
    query.bindValue(":observacao", ui->lineEditObservacao->text().toUpper());
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
    query.bindValue(":observacao", ui->lineEditObservacao->text().toUpper());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
  }

  if (tipo == Tipo::Compra) {
    query.prepare("INSERT INTO pedido_fornecedor_has_followup (ordemCompra, idLoja, idUsuario, observacao, dataFollowup) VALUES (:ordemCompra, :idLoja, :idUsuario, :observacao, :dataFollowup)");
    query.bindValue(":ordemCompra", id);
    query.bindValue(":idLoja", User::idLoja);
    query.bindValue(":idUsuario", User::idUsuario);
    query.bindValue(":observacao", ui->lineEditObservacao->text().toUpper());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
  }

  if (tipo == Tipo::Estoque) {
    query.prepare("INSERT INTO estoque_has_followup (idEstoque, idLoja, idUsuario, observacao, dataFollowup) VALUES (:idEstoque, :idLoja, :idUsuario, :observacao, :dataFollowup)");
    query.bindValue(":idEstoque", id);
    query.bindValue(":idLoja", User::idLoja);
    query.bindValue(":idUsuario", User::idUsuario);
    query.bindValue(":observacao", ui->lineEditObservacao->text().toUpper());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
  }

  if (tipo == Tipo::NFe) {
    query.prepare("INSERT INTO nfe_has_followup (idNFe, idLoja, idUsuario, observacao, dataFollowup) VALUES (:idNFe, :idLoja, :idUsuario, :observacao, :dataFollowup)");
    query.bindValue(":idNFe", id);
    query.bindValue(":idLoja", User::idLoja);
    query.bindValue(":idUsuario", User::idUsuario);
    query.bindValue(":observacao", ui->lineEditObservacao->text().toUpper());
    query.bindValue(":dataFollowup", ui->dateFollowup->dateTime());
  }

  if (not query.exec()) { throw RuntimeException("Erro salvando followup: " + query.lastError().text(), this); }

  qApp->enqueueInformation("Followup salvo com sucesso!", this);
  close();
}

void FollowUp::verifyFields() {
  if (tipo == Tipo::Orcamento and not ui->radioButtonQuente->isChecked() and not ui->radioButtonMorno->isChecked() and not ui->radioButtonFrio->isChecked()) {
    throw RuntimeError("Deve selecionar uma temperatura!", this);
  }

  if (ui->lineEditObservacao->text().isEmpty()) { throw RuntimeError("Deve escrever uma observação!", this); }
}

void FollowUp::setupTables() {
  if (tipo == Tipo::Orcamento) { modelFollowup.setTable("view_followup_orcamento"); }
  if (tipo == Tipo::Venda) { modelFollowup.setTable("view_followup_venda"); }
  if (tipo == Tipo::Compra) { modelFollowup.setTable("view_followup_pedido_fornecedor"); }
  if (tipo == Tipo::Estoque) { modelFollowup.setTable("view_followup_estoque"); }
  if (tipo == Tipo::NFe) { modelFollowup.setTable("view_followup_nfe"); }

  if (tipo == Tipo::Orcamento) {
    modelFollowup.setHeaderData("idOrcamento", "Orçamento");
    modelFollowup.setHeaderData("dataProxFollowup", "Próx. Data");
  }

  if (tipo == Tipo::Venda) { modelFollowup.setHeaderData("idVenda", "Venda"); }
  if (tipo == Tipo::Compra) { modelFollowup.setHeaderData("ordemCompra", "O.C."); }
  if (tipo == Tipo::Estoque) { modelFollowup.setHeaderData("idEstoque", "Estoque"); }
  if (tipo == Tipo::NFe) { modelFollowup.setHeaderData("idNFe", "NF-e"); }

  modelFollowup.setHeaderData("nome", "Usuário");
  modelFollowup.setHeaderData("observacao", "Observação");
  modelFollowup.setHeaderData("dataFollowup", "Data");

  if (tipo == Tipo::Orcamento) { modelFollowup.setFilter("idOrcamento LIKE '" + id.left(12) + "%'"); }
  if (tipo == Tipo::Venda) { modelFollowup.setFilter("idVenda LIKE '" + id.left(11) + "%'"); }
  if (tipo == Tipo::Compra) { modelFollowup.setFilter("ordemCompra = " + id); }
  if (tipo == Tipo::Estoque) { modelFollowup.setFilter("idEstoque = " + id); }
  if (tipo == Tipo::NFe) { modelFollowup.setFilter("idNFe = " + id); }

  modelFollowup.setSort("dataFollowup");

  modelFollowup.select();

  modelFollowup.proxyModel = new FollowUpProxyModel(&modelFollowup, this);

  ui->table->setModel(&modelFollowup);

  if (tipo == Tipo::Orcamento) {
    modelOrcamento.setTable("orcamento");

    modelOrcamento.setFilter("idOrcamento = '" + id + "'");

    modelOrcamento.select();

    ui->table->hideColumn("semaforo");

    const QString motivoCancelamento = modelOrcamento.data(0, "motivoCancelamento").toString();
    const QString observacaoCancelamento = modelOrcamento.data(0, "observacaoCancelamento").toString();

    ui->lineEditBaixa->setText(motivoCancelamento + "\n\n" + observacaoCancelamento);

    if (motivoCancelamento.isEmpty() and observacaoCancelamento.isEmpty()) {
      ui->labelBaixa->hide();
      ui->lineEditBaixa->hide();
    }
  }
}

void FollowUp::on_dateFollowup_dateChanged(const QDate date) {
  if (ui->dateProxFollowup->date() < date) { ui->dateProxFollowup->setDate(date); }
}
