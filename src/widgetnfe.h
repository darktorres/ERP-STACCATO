#ifndef WIDGETNFE_H
#define WIDGETNFE_H

#include <QWidget>

namespace Ui {
class WidgetNfe;
}

class WidgetNfe final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfe(QWidget *parent = nullptr);
  ~WidgetNfe();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetNfe *ui;
  // methods
  auto setConnections() -> void;
  auto on_tabWidgetNfe_currentChanged(const int) -> void;
};

#endif // WIDGETNFE_H
