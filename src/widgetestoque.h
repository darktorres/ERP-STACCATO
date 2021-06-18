#pragma once

#include <QWidget>

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent);
  ~WidgetEstoque();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetEstoque *ui;
  // methods
  auto on_tabWidget_currentChanged(const int &) -> void;
  auto setConnections() -> void;
};
