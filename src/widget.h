#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class Widget : public QWidget {
  Q_OBJECT

public:
  explicit Widget(QWidget *parent = nullptr);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();
};

#endif // WIDGET_H
