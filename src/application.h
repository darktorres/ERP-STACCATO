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
  auto dbConnect() -> bool;
  auto getInTransaction() const -> bool;
  auto getIsConnected() const -> bool;
  auto getMapLojas() const -> QMap<QString, QString>;
  auto getShowingErrors() const -> bool;
  auto getUpdating() const -> bool;
  auto lightTheme() -> void;
  auto setInTransaction(const bool value) -> void;
  auto setUpdating(const bool value) -> void;
  auto showMessages() -> void;
  auto updater() -> void;

signals:
  void verifyDb();

public slots:
  void endTransaction();
  void enqueueWarning(const QString &warning);
  void enqueueInformation(const QString &information);
  void enqueueError(const QString &error);
  void startTransaction();

private:
  // attributes
  QMap<QString, QString> mapLojas;
  QStringList errorQueue;
  QStringList informationQueue;
  QStringList warningQueue;
  bool inTransaction = false;
  bool isConnected = false;
  bool showingErrors = false;
  bool updating = false;
  const QPalette defaultPalette = palette();
  // methods
  auto storeSelection() -> void;
  auto readSettingsFile() -> void;
  auto startSqlPing() -> void;
};

#endif // APPLICATION_H
