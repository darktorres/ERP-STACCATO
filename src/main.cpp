#include "application.h"
#include "logindialog.h"
#include "mainwindow.h"

#include <QMessageBox>
#include <QSharedMemory>

int main(int argc, char *argv[]) {
#ifdef _WIN32
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
  }
#endif

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

// TODO: replace insert querys with model so values can be correctly rounded (Application::roundDouble)
// TODO: test changing table header to resizeToContents
// TODO: evitar divisoes por zero
// TODO: criar um delegate unidade para concatenar a unidade na coluna quant?
// TODO: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// TODO: use initializer lists?
