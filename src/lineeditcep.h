#pragma once

#include <QLineEdit>

class LineEditCEP final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditCEP(QWidget *parent);
  ~LineEditCEP() = default;

  auto isValid() const -> bool;
};
