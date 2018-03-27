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
  auto updateTables() -> bool;

private:
  // attributes
  Ui::WidgetFinanceiro *ui;
  // methods
  auto setConnections() -> void;
};

#endif // WIDGETFINANCEIRO_H
