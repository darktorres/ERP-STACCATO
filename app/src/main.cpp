#include "application.h"
#include "logindialog.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {

#ifdef QT_DEBUG
  qSetMessagePattern("%{function}:%{file}:%{line} - %{message}");
#else
  qSetMessagePattern("%{message}");
#endif
  Application app(argc, argv);

  LoginDialog dialog;

  if (dialog.exec() == QDialog::Rejected) exit(1);

  MainWindow window;
  window.showMaximized();

  return app.exec();
}

// REFAC: evitar divisoes por zero
// REFAC: pesquisar selects/submits sem verificacao
// REFAC: criar um delegate unidade para concatenar a unidade na coluna quant?
// REFAC: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// REFAC: use initializer lists?
