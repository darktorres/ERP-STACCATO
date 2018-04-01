#include "dialog.h"
#include "application.h"

Dialog::Dialog(QWidget *parent) : QDialog(parent) {
  connect(this, &Dialog::informationSignal, qApp, &Application::enqueueInformation);
  connect(this, &Dialog::warningSignal, qApp, &Application::enqueueWarning);
  connect(this, &Dialog::errorSignal, qApp, &Application::enqueueError);
  connect(this, &Dialog::transactionStarted, qApp, &Application::startTransaction);
  connect(this, &Dialog::transactionEnded, qApp, &Application::endTransaction);
}
