#pragma once

#include <QScrollArea>

class ScrollArea final : public QScrollArea {
  Q_OBJECT

public:
  explicit ScrollArea(QWidget *parent);

  auto minimumSizeHint() const -> QSize final;
  auto sizeHint() const -> QSize final;
};
