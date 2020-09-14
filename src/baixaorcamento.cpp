#include "baixaorcamento.h"
#include "ui_baixaorcamento.h"

#include "application.h"

#include <QSqlError>

BaixaOrcamento::BaixaOrcamento(const QString &idOrcamento, QWidget *parent) : QDialog(parent), ui(new Ui::BaixaOrcamento) {
  ui->setupUi(this);

  setupTables(idOrcamento);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &BaixaOrcamento::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &BaixaOrcamento::on_pushButtonSalvar_clicked);
}

BaixaOrcamento::~BaixaOrcamento() { delete ui; }

void BaixaOrcamento::setupTables(const QString &idOrcamento) {
  modelOrcamento.setTable("orcamento");

  modelOrcamento.setFilter("idOrcamento = '" + idOrcamento + "'");

  modelOrcamento.select();
}

void BaixaOrcamento::on_pushButtonCancelar_clicked() { close(); }

void BaixaOrcamento::on_pushButtonSalvar_clicked() {
  if (ui->plainTextEditObservacao->toPlainText().isEmpty()) { return qApp->enqueueError("Deve preencher a observação!", this); }

  const auto children = ui->groupBox->findChildren<QRadioButton *>();
  QString motivo;

  for (const auto &child : children) {
    if (child->isChecked()) { motivo = child->text(); }
  }

  if (motivo.isEmpty()) { return qApp->enqueueError("Deve escolher um motivo!", this); }

  modelOrcamento.setData(0, "status", "PERDIDO");
  modelOrcamento.setData(0, "motivoCancelamento", motivo);
  modelOrcamento.setData(0, "observacaoCancelamento", ui->plainTextEditObservacao->toPlainText());

  modelOrcamento.submitAll();

  qApp->enqueueInformation("Baixa salva!", this);

  close();
  parentWidget()->close();
}
