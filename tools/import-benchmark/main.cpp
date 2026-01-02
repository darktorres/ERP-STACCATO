/**
 * ImportaProdutos CLI Benchmark Tool
 *
 * Uses the REAL importaprodutos.cpp code to measure actual performance.
 * Supports regression testing via golden file comparison.
 *
 * Usage:
 *   import-benchmark <excel-file> --host <db-host> --user <db-user> --pass <db-pass> [options]
 *
 * Regression Testing:
 *   --generate-golden <file>  Save results to golden file
 *   --verify <file>           Compare results against golden file
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlRecord>
#include <QStandardItemModel>
#include <QTextStream>

#include "application.h"
#include "importaprodutos.h"

// Fields to capture for regression testing (order matters for consistency)
static const QStringList REGRESSION_FIELDS = {
    "idProduto",    "idFornecedor", "fornecedor",    "descricao",   "un",       "un2",         "colecao",
    "m2cx",         "pccx",         "kgcx",          "formComercial", "codComercial", "codBarras",
    "ncm",          "qtdPallet",    "custo",         "precoVenda",  "ui",       "minimo",      "mva",
    "st",           "sticms",       "quantCaixa",    "markup",      "validade", "descontinuado"
};

// Fields used for row identification (for matching rows between runs)
// NOTE: idProduto is required because duplicate products exist with same fornecedor+codComercial+ui
static const QStringList KEY_FIELDS = {"idProduto", "fornecedor", "codComercial", "ui"};

struct RegressionResult {
  int itensImported = 0;
  int itensUpdated = 0;
  int itensNotChanged = 0;
  int itensExpired = 0;
  int itensError = 0;
  int modelProdutoRowCount = 0;
  int modelErroRowCount = 0;
  QJsonArray modelProdutoData;
  QJsonArray modelErroData;

  QJsonObject toJson() const {
    QJsonObject obj;
    obj["itensImported"] = itensImported;
    obj["itensUpdated"] = itensUpdated;
    obj["itensNotChanged"] = itensNotChanged;
    obj["itensExpired"] = itensExpired;
    obj["itensError"] = itensError;
    obj["modelProdutoRowCount"] = modelProdutoRowCount;
    obj["modelErroRowCount"] = modelErroRowCount;
    obj["modelProdutoData"] = modelProdutoData;
    obj["modelErroData"] = modelErroData;
    return obj;
  }

  static RegressionResult fromJson(const QJsonObject &obj) {
    RegressionResult r;
    r.itensImported = obj["itensImported"].toInt();
    r.itensUpdated = obj["itensUpdated"].toInt();
    r.itensNotChanged = obj["itensNotChanged"].toInt();
    r.itensExpired = obj["itensExpired"].toInt();
    r.itensError = obj["itensError"].toInt();
    r.modelProdutoRowCount = obj["modelProdutoRowCount"].toInt();
    r.modelErroRowCount = obj["modelErroRowCount"].toInt();
    r.modelProdutoData = obj["modelProdutoData"].toArray();
    r.modelErroData = obj["modelErroData"].toArray();
    return r;
  }
};

QJsonObject captureRowData(QStandardItemModel &model, int row, const QStringList &columns) {
  QJsonObject rowObj;
  for (const QString &field : REGRESSION_FIELDS) {
    int col = columns.indexOf(field);
    if (col == -1) continue;

    QStandardItem *item = model.item(row, col);
    if (!item) continue;

    QVariant value = item->data(Qt::DisplayRole);
    if (value.isNull()) {
      rowObj[field] = QJsonValue::Null;
    } else if (value.userType() == QMetaType::Double) {
      rowObj[field] = value.toDouble();
    } else if (value.userType() == QMetaType::Int || value.userType() == QMetaType::LongLong) {
      rowObj[field] = value.toInt();
    } else if (value.userType() == QMetaType::Bool) {
      rowObj[field] = value.toBool();
    } else {
      rowObj[field] = value.toString();
    }
  }
  return rowObj;
}

QString getRowKey(const QJsonObject &row) {
  QStringList keyParts;
  for (const QString &field : KEY_FIELDS) {
    keyParts << row[field].toVariant().toString();
  }
  return keyParts.join("|");
}

RegressionResult captureResults(ImportaProdutos &importador) {
  RegressionResult result;

  result.itensImported = importador.getItensImported();
  result.itensUpdated = importador.getItensUpdated();
  result.itensNotChanged = importador.getItensNotChanged();
  result.itensExpired = importador.getItensExpired();
  result.itensError = importador.getItensError();

  // Use new optimized models
  QStandardItemModel &previewModel = importador.getPreviewModel();
  QStandardItemModel &errorModel = importador.getErrorModel();
  const QStringList &columns = ImportaProdutos::getPreviewColumns();

  result.modelProdutoRowCount = previewModel.rowCount();
  result.modelErroRowCount = errorModel.rowCount();

  // Capture model data
  for (int row = 0; row < previewModel.rowCount(); ++row) {
    result.modelProdutoData.append(captureRowData(previewModel, row, columns));
  }

  for (int row = 0; row < errorModel.rowCount(); ++row) {
    result.modelErroData.append(captureRowData(errorModel, row, columns));
  }

  return result;
}

bool saveGoldenFile(const RegressionResult &result, const QString &filePath) {
  QJsonObject root;
  root["version"] = 1;
  root["description"] = "ImportaProdutos regression test golden file";
  root["result"] = result.toJson();

  QJsonDocument doc(root);
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    qCritical() << "Failed to open golden file for writing:" << filePath;
    return false;
  }

  file.write(doc.toJson(QJsonDocument::Indented));
  return true;
}

RegressionResult loadGoldenFile(const QString &filePath, bool *ok) {
  *ok = false;
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    qCritical() << "Failed to open golden file:" << filePath;
    return {};
  }

  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError) {
    qCritical() << "Failed to parse golden file:" << error.errorString();
    return {};
  }

  *ok = true;
  return RegressionResult::fromJson(doc.object()["result"].toObject());
}

struct ComparisonResult {
  bool passed = true;
  QStringList differences;
};

ComparisonResult compareResults(const RegressionResult &expected, const RegressionResult &actual) {
  ComparisonResult cmp;

  // Compare counters
  auto checkCounter = [&](const QString &name, int exp, int act) {
    if (exp != act) {
      cmp.passed = false;
      cmp.differences << QString("%1: expected %2, got %3").arg(name).arg(exp).arg(act);
    }
  };

  checkCounter("itensImported", expected.itensImported, actual.itensImported);
  checkCounter("itensUpdated", expected.itensUpdated, actual.itensUpdated);
  checkCounter("itensNotChanged", expected.itensNotChanged, actual.itensNotChanged);
  checkCounter("itensExpired", expected.itensExpired, actual.itensExpired);
  checkCounter("itensError", expected.itensError, actual.itensError);
  checkCounter("modelProdutoRowCount", expected.modelProdutoRowCount, actual.modelProdutoRowCount);
  checkCounter("modelErroRowCount", expected.modelErroRowCount, actual.modelErroRowCount);

  // Build lookup maps by key for row comparison
  auto buildRowMap = [](const QJsonArray &rows) {
    QMap<QString, QJsonObject> map;
    for (const QJsonValue &v : rows) {
      QJsonObject row = v.toObject();
      map[getRowKey(row)] = row;
    }
    return map;
  };

  // Compare modelProduto rows
  QMap<QString, QJsonObject> expectedRows = buildRowMap(expected.modelProdutoData);
  QMap<QString, QJsonObject> actualRows = buildRowMap(actual.modelProdutoData);

  // Check for missing rows
  for (const QString &key : expectedRows.keys()) {
    if (!actualRows.contains(key)) {
      cmp.passed = false;
      cmp.differences << QString("Missing row in modelProduto: %1").arg(key);
    }
  }

  // Check for extra rows
  for (const QString &key : actualRows.keys()) {
    if (!expectedRows.contains(key)) {
      cmp.passed = false;
      cmp.differences << QString("Extra row in modelProduto: %1").arg(key);
    }
  }

  // Compare field values for matching rows
  for (const QString &key : expectedRows.keys()) {
    if (!actualRows.contains(key)) continue;

    QJsonObject expRow = expectedRows[key];
    QJsonObject actRow = actualRows[key];

    for (const QString &field : REGRESSION_FIELDS) {
      if (!expRow.contains(field) && !actRow.contains(field)) continue;

      QJsonValue expVal = expRow[field];
      QJsonValue actVal = actRow[field];

      bool match = false;

      // Convert to QVariant for flexible comparison
      QVariant expVar = expVal.toVariant();
      QVariant actVar = actVal.toVariant();

      // Handle null values
      if (expVal.isNull() && actVal.isNull()) {
        match = true;
      } else if (expVal.isNull() || actVal.isNull()) {
        // One is null, one is not - check for null-equivalent values (0, "", false)
        QVariant nonNullVal = expVal.isNull() ? actVar : expVar;
        if (nonNullVal.toString().isEmpty() || nonNullVal.toDouble() == 0.0) {
          match = true;  // Null and empty/zero are considered equivalent
        } else {
          match = false;
        }
      } else {
        // Try numeric comparison via QVariant (handles string "123" vs number 123)
        bool expNumOk = false, actNumOk = false;
        double expNum = expVar.toDouble(&expNumOk);
        double actNum = actVar.toDouble(&actNumOk);

        if (expNumOk && actNumOk) {
          match = qAbs(expNum - actNum) < 0.0001;
        } else {
          // Fall back to string comparison
          match = (expVar.toString() == actVar.toString());
        }
      }

      if (!match) {
        cmp.passed = false;
        cmp.differences << QString("Row %1, field %2: expected %3, got %4")
                               .arg(key)
                               .arg(field)
                               .arg(expVal.toVariant().toString())
                               .arg(actVal.toVariant().toString());
      }
    }
  }

  return cmp;
}

int main(int argc, char *argv[]) {
  Application app(argc, argv);
  app.setApplicationName("import-benchmark");
  app.setApplicationVersion("2.1.0");

  QCommandLineParser parser;
  parser.setApplicationDescription("ImportaProdutos CLI Benchmark with regression testing");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addPositionalArgument("file", "Excel file to import");

  QCommandLineOption hostOption(QStringList() << "H"
                                              << "host",
                                "Database host", "host", "localhost");
  parser.addOption(hostOption);

  QCommandLineOption userOption(QStringList() << "u"
                                              << "user",
                                "Database user", "user");
  parser.addOption(userOption);

  QCommandLineOption passOption(QStringList() << "p"
                                              << "pass",
                                "Database password", "pass");
  parser.addOption(passOption);

  QCommandLineOption validadeOption(QStringList() << "d"
                                                  << "validade",
                                    "Validade in days", "days", "30");
  parser.addOption(validadeOption);

  QCommandLineOption tipoOption(QStringList() << "t"
                                              << "tipo",
                                "Tipo: 0=Normal, 1=Promocao", "tipo", "0");
  parser.addOption(tipoOption);

  QCommandLineOption dryRunOption(QStringList() << "n"
                                                << "dry-run",
                                  "Don't commit changes (rollback at end)");
  parser.addOption(dryRunOption);

  QCommandLineOption generateGoldenOption(QStringList() << "g"
                                                        << "generate-golden",
                                          "Generate golden file for regression testing", "file");
  parser.addOption(generateGoldenOption);

  QCommandLineOption verifyOption(QStringList() << "V"
                                                << "verify",
                                  "Verify results against golden file", "file");
  parser.addOption(verifyOption);

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
  QString goldenFile = parser.value(generateGoldenOption);
  QString verifyFile = parser.value(verifyOption);

  // If verifying, force dry-run
  if (!verifyFile.isEmpty()) {
    dryRun = true;
  }

  QTextStream out(stdout);
  out << "=== ImportaProdutos CLI Benchmark ===" << Qt::endl;
  out << "File: " << filePath << Qt::endl;
  out << "Database: " << dbUser << "@" << dbHost << Qt::endl;
  out << "Validade: " << validade << " days" << Qt::endl;
  out << "Tipo: " << (tipo == 0 ? "Normal" : "Promocao") << Qt::endl;
  out << "Dry run: " << (dryRun ? "Yes" : "No") << Qt::endl;
  if (!goldenFile.isEmpty()) {
    out << "Generate golden: " << goldenFile << Qt::endl;
  }
  if (!verifyFile.isEmpty()) {
    out << "Verify against: " << verifyFile << Qt::endl;
  }
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

  int exitCode = 0;

  try {
    auto tipoEnum = static_cast<ImportaProdutos::Tipo>(tipo);
    ImportaProdutos importador(tipoEnum, nullptr);

    // Use CLI method that bypasses file/validade dialogs
    importador.importarTabelaCLI(filePath, validade);

    qint64 importTime = importTimer.elapsed();

    out << "Import completed in " << importTime << " ms" << Qt::endl;
    out << Qt::endl;

    out << "=== Results ===" << Qt::endl;
    out << "Imported:    " << importador.getItensImported() << Qt::endl;
    out << "Updated:     " << importador.getItensUpdated() << Qt::endl;
    out << "Not changed: " << importador.getItensNotChanged() << Qt::endl;
    out << "Expired:     " << importador.getItensExpired() << Qt::endl;
    out << "Errors:      " << importador.getItensError() << Qt::endl;
    out << "Model rows:  " << importador.getPreviewModel().rowCount() << Qt::endl;
    out << "Error rows:  " << importador.getErrorModel().rowCount() << Qt::endl;
    out << Qt::endl;

    // Only capture full results when needed for regression testing
    if (!goldenFile.isEmpty() || !verifyFile.isEmpty()) {
      RegressionResult result = captureResults(importador);

      // Generate golden file if requested
      if (!goldenFile.isEmpty()) {
        out << "Generating golden file: " << goldenFile << Qt::endl;
        if (saveGoldenFile(result, goldenFile)) {
          out << "Golden file saved successfully." << Qt::endl;
        } else {
          exitCode = 1;
        }
      }

      // Verify against golden file if requested
      if (!verifyFile.isEmpty()) {
        out << "Verifying against golden file: " << verifyFile << Qt::endl;
        bool loadOk;
        RegressionResult expected = loadGoldenFile(verifyFile, &loadOk);

        if (!loadOk) {
          exitCode = 1;
        } else {
          ComparisonResult cmp = compareResults(expected, result);

          if (cmp.passed) {
            out << Qt::endl << "=== VERIFICATION PASSED ===" << Qt::endl;
          } else {
            out << Qt::endl << "=== VERIFICATION FAILED ===" << Qt::endl;
            out << "Differences found: " << cmp.differences.size() << Qt::endl;
            for (const QString &diff : cmp.differences) {
              out << "  - " << diff << Qt::endl;
            }
            exitCode = 1;
          }
        }
      }
    }

    qint64 totalTime = totalTimer.elapsed();
    out << Qt::endl << "Total time: " << totalTime << " ms" << Qt::endl;

  } catch (const std::exception &e) {
    qCritical() << "Import failed:" << e.what();
    exitCode = 1;
  }

  if (dryRun || app.getInTransaction()) {
    out << "Rolling back transaction..." << Qt::endl;
    app.rollbackTransaction("");
  }

  return exitCode;
}
