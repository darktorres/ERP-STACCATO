#ifndef LINEEDITTEL_H
#define LINEEDITTEL_H

#include <QLineEdit>

class LineEditTel final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditTel(QWidget *parent);
  ~LineEditTel() = default;

private:
  void processTel(const QString &value);
};

#endif // LINEEDITTEL_H
