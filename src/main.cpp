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
#ifdef DEPLOY
  window.showMaximized();
#else
  window.show();
#endif

  return app.exec();
}

// ----------------- SERVIDOR ----------------------

// TODO: colocar pelo menos as ultimas copias do backup do mysql na nuvem

// -------------------------------------------------

// TODO: test changing table header to resizeToContents
// REFAC: evitar divisoes por zero
// REFAC: criar um delegate unidade para concatenar a unidade na coluna quant?
// REFAC: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// REFAC: use initializer lists?
