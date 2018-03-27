#include "widget.h"
#include "application.h"

Widget::Widget(QWidget *parent) : QWidget(parent) {
  connect(this, &Widget::informationSignal, qApp, &Application::enqueueInformation);
  connect(this, &Widget::warningSignal, qApp, &Application::enqueueWarning);
  connect(this, &Widget::errorSignal, qApp, &Application::enqueueError);
  connect(this, &Widget::transactionStarted, qApp, &Application::startTransaction);
  connect(this, &Widget::transactionEnded, qApp, &Application::endTransaction);
}
