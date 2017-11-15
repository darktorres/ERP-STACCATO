#ifndef WIDGETFINANCEIRO_H
#define WIDGETFINANCEIRO_H

#include "widget.h"

namespace Ui {
class WidgetFinanceiro;
}

class WidgetFinanceiro final : public Widget {
  Q_OBJECT

public:
  explicit WidgetFinanceiro(QWidget *parent = 0);
  ~WidgetFinanceiro();
  bool updateTables();
  void setHasError(const bool value);

private:
  // attributes
  bool hasError = false;
  Ui::WidgetFinanceiro *ui;
  // methods
  void setConnections();
};

#endif // WIDGETFINANCEIRO_H
