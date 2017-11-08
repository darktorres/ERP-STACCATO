#include <QApplication>

#include "logindialog.h"
#include "mainwindow.h"
#include "usersession.h"

void darkTheme() {
  qApp->setStyle("Fusion");

  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
  darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));

  darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  qApp->setPalette(darkPalette);

  qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
}

int main(int argc, char *argv[]) {

#ifdef QT_DEBUG
  qSetMessagePattern("%{function}:%{file}:%{line} - %{message}");
#else
  qSetMessagePattern("%{message}");
#endif
  QApplication app(argc, argv);
  app.setOrganizationName("Staccato");
  app.setApplicationName("ERP");
  app.setWindowIcon(QIcon("Staccato.ico"));
  app.setApplicationVersion("0.5.46");
  app.setStyle("Fusion");

  const QPalette defaultPalette = app.palette();

  if (UserSession::getSetting("User/tema").toString() == "escuro") darkTheme();

  LoginDialog dialog;

  if (dialog.exec() == QDialog::Rejected) exit(1);

  MainWindow window(defaultPalette);
  window.showMaximized();

  return app.exec();
}

// REFAC: evitar divisoes por zero
// REFAC: verificar comparacoes double: substituir por qFuzzyCompare ou floats
// REFAC: verificar todos os QSqlQuery.exec
// REFAC: pesquisar setData e selects/submits sem verificacao
// REFAC: criar um delegate unidade para concatenar a unidade na coluna quant?
// REFAC: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// REFAC: use initializer lists?
