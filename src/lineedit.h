#pragma once

#include <QLineEdit>
#include <QTimer>

class LineEdit final : public QLineEdit {
  Q_OBJECT

public:
  explicit LineEdit(QWidget *parent);

  auto resizeToContent() -> void;
  auto setDelayed() -> void;
  auto setResizeToText() -> void;

signals:
  void delayedTextChanged(const QString &text);

private:
  QTimer timer;
};
