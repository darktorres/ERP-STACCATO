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
  void darkTheme();
  void lightTheme();
  bool getUpdating() const;
  void setUpdating(const bool value);
  bool getInTransaction() const;
  void setInTransaction(const bool value);
  void showErrors();
  bool dbConnect();
  void updater();
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
  void storeSelection();
  void readSettingsFile();
  void startSqlPing();
};

#endif // APPLICATION_H
