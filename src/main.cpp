#include "application.h"
#include "logindialog.h"
#include "mainwindow.h"

#include <QMessageBox>
#include <QSharedMemory>

int main(int argc, char *argv[]) {
  Application app(argc, argv);

#ifdef Q_OS_WIN
  QSharedMemory sharedMemory;
  sharedMemory.setKey("staccato-erp");

  if (sharedMemory.create(1) == false) {
    QMessageBox::critical(nullptr, "Erro!", "ERP já rodando!");
    app.exit();
    return 0;
  }
#endif

  LoginDialog dialog;

  if (dialog.exec() == QDialog::Rejected) { exit(1); }

  MainWindow window;
#ifdef Q_OS_WIN
  window.showMaximized();
#else
  window.show();
#endif

  return app.exec();
}

// ----------------- SERVIDOR ----------------------

// TODO: set ftp server on each server to replace ssh on backup
// TODO: ubuntu 14.04 parou de receber atualizações, atualizar para a versão mais recente
// TODO: colocar pelo menos as ultimas copias do backup do mysql na nuvem

// -------------------------------------------------

// TODO: test changing table header to resizeToContents
// TODO: criar uma branch 'audit' para criar um log para cada operacao feita pelo usuario, com data e hora, um para cada funcao basicamente e salvar em um arquivo audit.log
// REFAC: evitar divisoes por zero
// REFAC: pesquisar selects/submits sem verificacao
// REFAC: criar um delegate unidade para concatenar a unidade na coluna quant?
// REFAC: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// REFAC: use initializer lists?
