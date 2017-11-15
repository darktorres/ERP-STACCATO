#ifndef DIALOG_H
#define DIALOG_H

#include "QDialog"

class Dialog : public QDialog {
  Q_OBJECT

public:
  explicit Dialog(QWidget *parent);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();
};

#endif // DIALOG_H
