#ifndef WIDGETFINANCEIRO_H
#define WIDGETFINANCEIRO_H

#include "widget.h"

namespace Ui {
class WidgetFinanceiro;
}

class WidgetFinanceiro final : public Widget {
  Q_OBJECT

public:
  explicit WidgetFinanceiro(QWidget *parent = nullptr);
  ~WidgetFinanceiro();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetFinanceiro *ui;
  // methods
  auto setConnections() -> void;
};

#endif // WIDGETFINANCEIRO_H
