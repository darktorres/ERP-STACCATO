#include "baixaorcamento.h"
#include "ui_baixaorcamento.h"

#include "application.h"

#include <QRegularExpression>
#include <QSqlError>

BaixaOrcamento::BaixaOrcamento(const QString &idOrcamento, QWidget *parent) : QDialog(parent), ui(new Ui::BaixaOrcamento) {
  ui->setupUi(this);

  setupTables(idOrcamento);

  setConnections();
}

BaixaOrcamento::~BaixaOrcamento() { delete ui; }

void BaixaOrcamento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &BaixaOrcamento::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &BaixaOrcamento::on_pushButtonSalvar_clicked, connectionType);
}

void BaixaOrcamento::setupTables(const QString &idOrcamento) {
  modelOrcamento.setTable("orcamento");

  modelOrcamento.setFilter("idOrcamento = '" + idOrcamento + "'");

  modelOrcamento.select();
}

void BaixaOrcamento::on_pushButtonCancelar_clicked() { close(); }

void BaixaOrcamento::on_pushButtonSalvar_clicked() {
  if (ui->plainTextEditObservacao->toPlainText().isEmpty()) { throw RuntimeError("Deve preencher a observação!", this); }

  const auto children = ui->groupBox->findChildren<QRadioButton *>(QRegularExpression("radioButton"));
  QString motivo;

  for (const auto &child : children) {
    if (child->isChecked()) { motivo = child->text(); }
  }

  if (motivo.isEmpty()) { throw RuntimeError("Deve escolher um motivo!", this); }

  modelOrcamento.setData(0, "status", "PERDIDO");
  modelOrcamento.setData(0, "motivoCancelamento", motivo);
  modelOrcamento.setData(0, "observacaoCancelamento", ui->plainTextEditObservacao->toPlainText());

  modelOrcamento.submitAll();

  qApp->enqueueInformation("Baixa salva!", this);

  close();
  parentWidget()->close();
}
