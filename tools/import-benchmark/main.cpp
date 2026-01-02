/**
 * ImportaProdutos CLI Benchmark Tool
 *
 * Uses the REAL importaprodutos.cpp code to measure actual performance.
 * Requires database connection.
 *
 * Usage:
 *   import-benchmark <excel-file> --host <db-host> --user <db-user> --pass <db-pass> [--validade <days>]
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

#include "application.h"
#include "importaprodutos.h"

int main(int argc, char *argv[]) {
  // Need QApplication for widgets (ImportaProdutos is a QDialog)
  Application app(argc, argv);
  app.setApplicationName("import-benchmark");
  app.setApplicationVersion("2.0.0");

  QCommandLineParser parser;
  parser.setApplicationDescription("ImportaProdutos CLI Benchmark - uses real code");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addPositionalArgument("file", "Excel file to import");

  QCommandLineOption hostOption(QStringList() << "H" << "host", "Database host", "host", "localhost");
  parser.addOption(hostOption);

  QCommandLineOption userOption(QStringList() << "u" << "user", "Database user", "user");
  parser.addOption(userOption);

  QCommandLineOption passOption(QStringList() << "p" << "pass", "Database password", "pass");
  parser.addOption(passOption);

  QCommandLineOption validadeOption(QStringList() << "d" << "validade", "Validade in days", "days", "30");
  parser.addOption(validadeOption);

  QCommandLineOption tipoOption(QStringList() << "t" << "tipo", "Tipo: 0=Normal, 1=Promocao", "tipo", "0");
  parser.addOption(tipoOption);

  QCommandLineOption dryRunOption(QStringList() << "n" << "dry-run", "Don't commit changes (rollback at end)");
  parser.addOption(dryRunOption);

  parser.process(app);

  QStringList args = parser.positionalArguments();
  if (args.isEmpty()) {
    qCritical() << "Error: No input file specified";
    parser.showHelp(1);
  }

  QString filePath = args.first();
  if (!QFile::exists(filePath)) {
    qCritical() << "Error: File not found:" << filePath;
    return 1;
  }

  if (!parser.isSet(userOption)) {
    qCritical() << "Error: Database user required (--user)";
    return 1;
  }

  QString dbHost = parser.value(hostOption);
  QString dbUser = parser.value(userOption);
  QString dbPass = parser.value(passOption);
  int validade = parser.value(validadeOption).toInt();
  int tipo = parser.value(tipoOption).toInt();
  bool dryRun = parser.isSet(dryRunOption);

  QTextStream out(stdout);
  out << "=== ImportaProdutos CLI Benchmark ===" << Qt::endl;
  out << "File: " << filePath << Qt::endl;
  out << "Database: " << dbUser << "@" << dbHost << Qt::endl;
  out << "Validade: " << validade << " days" << Qt::endl;
  out << "Tipo: " << (tipo == 0 ? "Normal" : "Promocao") << Qt::endl;
  out << "Dry run: " << (dryRun ? "Yes" : "No") << Qt::endl;
  out << Qt::endl;

  // Connect to database
  out << "Connecting to database..." << Qt::endl;
  QElapsedTimer totalTimer;
  totalTimer.start();

  try {
    app.dbConnect(dbHost, dbUser, dbPass);
    out << "Connected in " << totalTimer.elapsed() << " ms" << Qt::endl;
  } catch (const std::exception &e) {
    qCritical() << "Database connection failed:" << e.what();
    return 1;
  }

  // Create ImportaProdutos instance
  out << Qt::endl << "Starting import..." << Qt::endl;
  out.flush();

  QElapsedTimer importTimer;
  importTimer.start();

  try {
    auto tipoEnum = static_cast<ImportaProdutos::Tipo>(tipo);
    ImportaProdutos importador(tipoEnum, nullptr);

    // Use CLI method that bypasses file/validade dialogs
    importador.importarTabelaCLI(filePath, validade);

    out << "Import completed." << Qt::endl;

  } catch (const std::exception &e) {
    qCritical() << "Import failed:" << e.what();
    if (dryRun || app.getInTransaction()) {
      out << "Rolling back transaction..." << Qt::endl;
      app.rollbackTransaction("");
    }
    return 1;
  }

  qint64 importTime = importTimer.elapsed();
  qint64 totalTime = totalTimer.elapsed();

  out << Qt::endl;
  out << "=== Results ===" << Qt::endl;
  out << "Import time: " << importTime << " ms" << Qt::endl;
  out << "Total time:  " << totalTime << " ms" << Qt::endl;

  if (dryRun) {
    out << Qt::endl << "Rolling back (dry run)..." << Qt::endl;
    app.rollbackTransaction("");
  }

  return 0;
}
