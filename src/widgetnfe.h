#ifndef WIDGETNFE_H
#define WIDGETNFE_H

#include "widget.h"

namespace Ui {
class WidgetNfe;
}

class WidgetNfe final : public Widget {
  Q_OBJECT

public:
  explicit WidgetNfe(QWidget *parent = nullptr);
  ~WidgetNfe();
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetNfe *ui;
  // methods
  auto setConnections() -> void;
  auto on_tabWidgetNfe_currentChanged(const int) -> void;
};

#endif // WIDGETNFE_H
