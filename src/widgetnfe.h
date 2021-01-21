#pragma once

#include <QWidget>

namespace Ui {
class WidgetNfe;
}

class WidgetNfe final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfe(QWidget *parent);
  ~WidgetNfe();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetNfe *ui;
  // methods
  auto on_tabWidgetNfe_currentChanged(const int) -> void;
  auto setConnections() -> void;
};
