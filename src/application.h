#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QPalette>

#if defined(qApp)
#undef qApp
#endif
#define qApp (dynamic_cast<Application *>(QCoreApplication::instance()))

class Application : public QApplication {
  Q_OBJECT

public:
  Application(int &argc, char **argv, int = ApplicationFlags);
  auto darkTheme() -> void;
  auto lightTheme() -> void;
  auto getUpdating() const -> bool;
  auto setUpdating(const bool value) -> void;
  auto getInTransaction() const -> bool;
  auto setInTransaction(const bool value) -> void;
  auto showErrors() -> void;
  auto dbConnect() -> bool;
  auto updater() -> void;
  // REFAC: make those private?
  bool showingErrors = false;
  bool isConnected = false;
  QMap<QString, QString> mapLojas;

signals:
  void verifyDb();

public slots:
  void endTransaction();
  void enqueueError(const QString &error);
  void startTransaction();

private:
  // attributes
  bool inTransaction = false;
  bool updating = false;
  const QPalette defaultPalette = palette();
  QStringList errorQueue;
  // methods
  auto storeSelection() -> void;
  auto readSettingsFile() -> void;
  auto startSqlPing() -> void;
};

#endif // APPLICATION_H
