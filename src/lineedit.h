#pragma once

#include <QLineEdit>

class LineEdit final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEdit(QWidget *parent);

  auto resizeToContent() -> void;
};
