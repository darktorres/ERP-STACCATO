#pragma once

#include <QScrollArea>

class ScrollArea final : public QScrollArea {
  Q_OBJECT

public:
  explicit ScrollArea(QWidget *parent);

  auto sizeHint() const -> QSize override;
};
