#include <QMessageBox>
#include <QSqlError>

#include "baixaorcamento.h"
#include "ui_baixaorcamento.h"

BaixaOrcamento::BaixaOrcamento(const QString &idOrcamento, QWidget *parent) : Dialog(parent), ui(new Ui::BaixaOrcamento) {
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

  if (not modelOrcamento.select()) { emit errorSignal("Erro lendo tabela orcamento: " + modelOrcamento.lastError().text()); }
}

void BaixaOrcamento::on_pushButtonCancelar_clicked() { close(); }

void BaixaOrcamento::on_pushButtonSalvar_clicked() {
  if (ui->plainTextEditObservacao->toPlainText().isEmpty()) {
    emit errorSignal("Deve preencher a observação!");
    return;
  }

  QString motivo;

  Q_FOREACH (const auto &child, ui->groupBox->findChildren<QRadioButton *>()) {
    if (child->isChecked()) { motivo = child->text(); }
  }

  if (motivo.isEmpty()) {
    emit errorSignal("Deve escolher um motivo!");
    return;
  }

  if (not modelOrcamento.setData(0, "status", "PERDIDO")) { return; }
  if (not modelOrcamento.setData(0, "motivoCancelamento", motivo)) { return; }
  if (not modelOrcamento.setData(0, "observacaoCancelamento", ui->plainTextEditObservacao->toPlainText())) { return; }

  if (not modelOrcamento.submitAll()) { return; }

  // TODO: exibir mensagem de confirmacao

  close();
  parentWidget()->close();
}
