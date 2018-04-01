#ifndef LINEEDITCEP_H
#define LINEEDITCEP_H

#include <QLineEdit>

class LineEditCEP final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditCEP(QWidget *parent);
  ~LineEditCEP() = default;
  auto isValid() const -> bool;

private:
  auto getValue() const -> QString;
  auto setValue(const QString &value) -> void;
};

#endif // LINEEDITCEP_H
