#pragma once

#include <QApplication>
#include <QDateTime>
#include <QPalette>
#include <QSqlDatabase>

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application *>(QCoreApplication::instance()))

class RuntimeException : public std::runtime_error {
public:
  explicit RuntimeException(const QString &message, QWidget *parent = nullptr);
};

class RuntimeError : public std::runtime_error {
public:
  explicit RuntimeError(const QString &message, QWidget *parent = nullptr);
};

class Application : public QApplication {
  Q_OBJECT

public:
  Application(int &argc, char **argv, int = ApplicationFlags);

  virtual auto notify(QObject *receiver, QEvent *event) -> bool override;

  auto ajustarDiaUtil(const QDate &date) -> QDate;
  auto darkTheme() -> void;
  auto dbConnect(const QString &hostname, const QString &user, const QString &userPassword) -> bool;
  auto dbReconnect(const bool silent = false) -> bool;
  auto endTransaction() -> void;
  auto enqueueError(const QString &error, QWidget *parent = nullptr) -> void;
  auto enqueueError(const bool boolean, const QString &error, QWidget *parent = nullptr) -> bool;
  auto enqueueException(const QString &exception, QWidget *parent = nullptr) -> void;
  auto enqueueException(const bool boolean, const QString &exception, QWidget *parent = nullptr) -> bool;
  auto enqueueInformation(const QString &information, QWidget *parent = nullptr) -> void;
  auto enqueueWarning(const QString &warning, QWidget *parent = nullptr) -> void;
  auto getInTransaction() const -> bool;
  auto getIsConnected() const -> bool;
  auto getMapLojas() const -> QMap<QString, QString>;
  auto getShowingMessages() const -> bool;
  auto getSilent() const -> bool;
  auto getUpdating() const -> bool;
  auto getWebDavIp() const -> QString;
  auto lightTheme() -> void;
  auto reservarIdEstoque() -> int;
  auto reservarIdNFe() -> int;
  auto reservarIdPedido2() -> int;
  auto reservarIdVendaProduto2() -> int;
  auto rollbackTransaction() -> void;
  auto roundDouble(const double value) -> double;
  auto roundDouble(const double value, const int decimais) -> double;
  auto sanitizeSQL(const QString &string) -> QString;
  auto serverDate() -> QDate;
  auto serverDateTime() -> QDateTime;
  auto setSilent(bool value) -> void;
  auto setUpdating(const bool value) -> void;
  auto startTransaction(const QString &messageLog) -> void;
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
  QSqlDatabase db; // TODO: doc says not to store database as class member
  QVector<Message> exceptionQueue;
  QVector<Message> errorQueue;
  QVector<Message> informationQueue;
  QVector<Message> warningQueue;
  bool silent = false;
  bool updaterOpen = false;
  bool inTransaction = false;
  bool isConnected = false;
  bool showingMessages = false;
  bool updating = false;
  const QPalette defaultPalette = palette();
  QDateTime serverDateCache;
  QDate systemDate = QDate::currentDate();
  // methods
  auto genericLogin(const QString &hostname) -> void;
  auto loginError() -> void;
  auto readSettingsFile() -> void;
  auto runSqlJobs() -> void;
  auto showMessages() -> void;
  auto startSqlPing() -> void;
  auto startUpdaterPing() -> void;
  auto userLogin(const QString &user) -> void;
};
