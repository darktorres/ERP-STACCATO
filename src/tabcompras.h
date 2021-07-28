#pragma once

#include <QWidget>

namespace Ui {
class TabCompras;
}

class TabCompras final : public QWidget {
  Q_OBJECT

public:
  explicit TabCompras(QWidget *parent);
  ~TabCompras();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::TabCompras *ui;
  // methods
  auto on_tabWidget_currentChanged(const int) -> void;
  auto setConnections() -> void;
};
