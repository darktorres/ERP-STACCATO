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
  bool updateTables();

private:
  // attributes
  Ui::WidgetFinanceiro *ui;
  // methods
  void setConnections();
};

#endif // WIDGETFINANCEIRO_H
