#include "importatabelaibpt.h"

#include "application.h"

#include <QFileDialog>
#include <QProgressDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

ImportaTabelaIBPT::ImportaTabelaIBPT(QWidget *parent) : QDialog(parent) { setupTables(); }

void ImportaTabelaIBPT::importar() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Impotar tabela IBPT", QDir::currentPath(), "Excel (*.csv)");

  if (filePath.isEmpty()) { return; }

  QFileInfo fileInfo(filePath);

  if (not fileInfo.completeBaseName().contains("SP")) { return qApp->enqueueError("Tabela escolhida não é de SP!", this); }

  const QString versao = fileInfo.completeBaseName().right(6);

  QSqlQuery queryVersao;

  if (not queryVersao.exec("SELECT versao FROM ibpt WHERE versao = '" + versao + "' LIMIT 1")) { return qApp->enqueueException("Erro verificando versão: " + queryVersao.lastError().text(), this); }

  if (queryVersao.size() == 1) { return qApp->enqueueInformation("Tabela já cadastrada!", this); }

  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueException("Erro abrindo arquivo para leitura: " + file.errorString(), this); }

  QProgressDialog progressDialog;
  progressDialog.reset();
  progressDialog.setCancelButton(nullptr);
  progressDialog.setLabelText("Processando...");
  progressDialog.setWindowTitle("ERP Staccato");
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.setMinimum(0);
  progressDialog.setMaximum(file.size() + 1);

  progressDialog.show();

  QTextStream stream(&file);

  const QString line1 = stream.readLine();

  if (line1 != "codigo;ex;tipo;descricao;nacionalfederal;importadosfederal;estadual;municipal;vigenciainicio;vigenciafim;chave;versao;fonte") {
    return qApp->enqueueError("Arquivo incompatível!", this);
  }

  const auto size = file.size();

  while (not stream.atEnd()) {
    QStringList line = stream.readLine().split(";");

    const int row = model.insertRowAtEnd();

    if (not model.setData(row, "codigo", line.takeFirst())) { return; }
    const QString excecao = line.takeFirst();
    if (not model.setData(row, "excecao", excecao.isEmpty() ? 0 : excecao.toInt())) { return; }
    if (not model.setData(row, "tipo", line.takeFirst())) { return; }
    if (not model.setData(row, "descricao", line.takeFirst())) { return; }
    if (not model.setData(row, "nacionalfederal", line.takeFirst())) { return; }
    if (not model.setData(row, "importadosfederal", line.takeFirst())) { return; }
    if (not model.setData(row, "estadual", line.takeFirst())) { return; }
    if (not model.setData(row, "municipal", line.takeFirst())) { return; }
    if (not model.setData(row, "vigenciainicio", QDate::fromString(line.takeFirst(), "dd/MM/yyyy"))) { return; }
    if (not model.setData(row, "vigenciafim", QDate::fromString(line.takeFirst(), "dd/MM/yyyy"))) { return; }
    if (not model.setData(row, "chave", line.takeFirst())) { return; }
    if (not model.setData(row, "versao", line.takeFirst())) { return; }

    const auto remains = file.bytesAvailable();
    progressDialog.setValue(size - remains);
  }

  if (not qApp->startTransaction("ImportaTabelaIBPT::importar")) { return; }

  if (not model.submitAll()) {
    qApp->rollbackTransaction();
    return qApp->enqueueException("Erro salvando dados: " + model.lastError().text(), this);
  }

  if (not qApp->endTransaction()) { return; }

  progressDialog.cancel();

  qApp->enqueueInformation("Tabela cadastrada com sucesso!", this);
}

void ImportaTabelaIBPT::setupTables() { model.setTable("ibpt"); }
