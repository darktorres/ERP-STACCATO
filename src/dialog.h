#ifndef DIALOG_H
#define DIALOG_H

#include "QDialog"

class Dialog : public QDialog {
  Q_OBJECT

public:
  explicit Dialog(QWidget *parent);

  Dialog(Dialog const &) = delete;
  Dialog(Dialog &&) = delete;
  Dialog &operator=(Dialog const &) = delete;
  Dialog &operator=(Dialog &&) = delete;

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();
};

#endif // DIALOG_H
