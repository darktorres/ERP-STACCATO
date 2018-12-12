#include <QSqlError>

#include "application.h"
#include "baixaorcamento.h"
#include "ui_baixaorcamento.h"

BaixaOrcamento::BaixaOrcamento(const QString &idOrcamento, QWidget *parent) : QDialog(parent), ui(new Ui::BaixaOrcamento) {
  ui->setupUi(this);

  setupTables(idOrcamento);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &BaixaOrcamento::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &BaixaOrcamento::on_pushButtonSalvar_clicked);
}

BaixaOrcamento::~BaixaOrcamento() { delete ui; }

void BaixaOrcamento::setupTables(const QString &idOrcamento) {
  modelOrcamento.setTable("orcamento");
  modelOrcamento.setEditStrategy(QSqlTableModel::OnManualSubmit);
  modelOrcamento.setFilter("idOrcamento = '" + idOrcamento + "'");

  if (not modelOrcamento.select()) { return; }
}

void BaixaOrcamento::on_pushButtonCancelar_clicked() { close(); }

void BaixaOrcamento::on_pushButtonSalvar_clicked() {
  if (ui->plainTextEditObservacao->toPlainText().isEmpty()) { return qApp->enqueueError("Deve preencher a observação!", this); }

  QString motivo;

  Q_FOREACH (const auto &child, ui->groupBox->findChildren<QRadioButton *>()) {
    if (child->isChecked()) { motivo = child->text(); }
  }

  if (motivo.isEmpty()) { return qApp->enqueueError("Deve escolher um motivo!", this); }

  if (not modelOrcamento.setData(0, "status", "PERDIDO")) { return; }
  if (not modelOrcamento.setData(0, "motivoCancelamento", motivo)) { return; }
  if (not modelOrcamento.setData(0, "observacaoCancelamento", ui->plainTextEditObservacao->toPlainText())) { return; }

  if (not modelOrcamento.submitAll()) { return; }

  // TODO: exibir mensagem de confirmacao

  close();
  parentWidget()->close();
}
