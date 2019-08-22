#pragma once

#include <QLineEdit>

class LineEditTel final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEditTel(QWidget *parent = nullptr);
  ~LineEditTel() = default;

private:
  auto processTel(const QString &value) -> void;
};
