#include "application.h"
#include "logindialog.h"
#include "mainwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QSharedMemory>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
  }
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  Application app(argc, argv);

  QTranslator translator;

  if (translator.load(QLocale(), "qt", "_", "translations")) { app.installTranslator(&translator); }

#ifdef Q_OS_WIN
  QSharedMemory sharedMemory;
  sharedMemory.setKey("staccato-erp");

  if (not sharedMemory.create(1)) {
    QMessageBox::critical(nullptr, "Erro!", "ERP já rodando!");
    app.exit();
    return 0;
  }
#endif

  try {
    LoginDialog dialog;

    if (dialog.exec() == QDialog::Rejected) { exit(1); }

    auto *window = new MainWindow;
#ifdef DEPLOY
    window->showMaximized();
#else
    window->show();
#endif
  } catch (std::exception &e) {
    app.rollbackTransaction(e.what());
    exit(1);
  }

  return app.exec();
}

// ----------------- SERVIDOR ----------------------

// -------------------------------------------------

// TODO: replace insert querys with model so values can be correctly rounded (Application::roundDouble)
// TODO: test changing table header to resizeToContents
// TODO: evitar divisoes por zero
// TODO: criar um delegate unidade para concatenar a unidade na coluna quant?
// TODO: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// TODO: use initializer lists?
