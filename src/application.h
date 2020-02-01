#pragma once

#include <QApplication>
#include <QDateTime>
#include <QPalette>
#include <QSqlDatabase>

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application *>(QCoreApplication::instance()))

class Application : public QApplication {
  Q_OBJECT

public:
  Application(int &argc, char **argv, int = ApplicationFlags);
  auto darkTheme() -> void;
  auto dbConnect() -> bool;
  auto dbReconnect(const bool silent = false) -> bool;
  auto endTransaction() -> bool;
  auto enqueueError(const QString &error, QWidget *parent = nullptr) -> void;
  auto enqueueError(const bool boolean, const QString &error, QWidget *parent = nullptr) -> bool;
  auto enqueueInformation(const QString &information, QWidget *parent = nullptr) -> void;
  auto enqueueWarning(const QString &warning, QWidget *parent = nullptr) -> void;
  auto getWebDavIp() const -> QString;
  auto getInTransaction() const -> bool;
  auto getIsConnected() const -> bool;
  auto getMapLojas() const -> QMap<QString, QString>;
  auto getShowingErrors() const -> bool;
  auto getUpdating() const -> bool;
  auto lightTheme() -> void;
  auto rollbackTransaction() -> void;
  auto serverDate() -> QDate;
  auto serverDateTime() -> QDateTime;
  auto setInTransaction(const bool value) -> void;
  auto setUpdating(const bool value) -> void;
  auto showMessages() -> void;
  auto startTransaction(const bool delayMessages = true) -> bool;
  auto updater() -> void;

signals:
  void verifyDb(const bool conectado);

private:
  struct Message {
    QString message;
    QWidget *widget = nullptr;
  };

  // attributes
  QMap<QString, QString> mapLojas;
  QSqlDatabase db;
  QVector<Message> errorQueue;
  QVector<Message> informationQueue;
  QVector<Message> warningQueue;
  bool updaterOpen = false;
  bool delayMessages = false;
  bool inTransaction = false;
  bool isConnected = false;
  bool showingErrors = false;
  bool updating = false;
  const QPalette defaultPalette = palette();
  QDateTime serverDateCache;
  QDate systemDate = QDate::currentDate();
  // methods
  auto readSettingsFile() -> void;
  auto runSqlJobs() -> bool;
  auto setDatabase() -> bool;
  auto startSqlPing() -> void;
  auto startUpdaterPing() -> void;
  auto storeSelection() -> void;
};
