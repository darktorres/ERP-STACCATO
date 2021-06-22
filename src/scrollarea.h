#pragma once

#include <QScrollArea>

class ScrollArea final : public QScrollArea {

public:
  explicit ScrollArea(QWidget *parent);

  virtual QSize sizeHint() const override;
};
