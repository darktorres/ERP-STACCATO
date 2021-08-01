#pragma once

#include <QWidget>

namespace Ui {
class TabGalpao;
}

class TabGalpao : public QWidget {
  Q_OBJECT

public:
  explicit TabGalpao(QWidget *parent = nullptr);
  ~TabGalpao();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::TabGalpao *ui;
  // methods
  auto on_tabWidget_currentChanged() -> void;
  auto setConnections() -> void;
};
