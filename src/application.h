#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QPalette>

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application *>(QCoreApplication::instance()))

class Application : public QApplication {

public:
  Application(int &argc, char **argv, int = ApplicationFlags);
  void darkTheme();
  void lightTheme();
  bool getUpdating() const;
  void setUpdating(const bool value);
  bool getInTransaction() const;
  void setInTransaction(const bool value);
  void showErrors();

public slots:
  void endTransaction();
  void enqueueError(const QString &error);
  void startTransaction();

private:
  // attributes
  const QPalette defaultPalette = palette();
  bool updating = false;
  bool inTransaction = false;
  QStringList errorQueue;
  // methods
};

#endif // APPLICATION_H
