#pragma once

#include <QWidget>

namespace Ui {
class TabEstoque;
}

class TabEstoque final : public QWidget {
  Q_OBJECT

public:
  explicit TabEstoque(QWidget *parent);
  ~TabEstoque();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::TabEstoque *ui;
  // methods
  auto on_tabWidget_currentChanged(const int &) -> void;
  auto setConnections() -> void;
};
