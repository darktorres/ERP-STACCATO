#pragma once

#include <QWidget>

namespace Ui {
class TabFinanceiro;
}

class TabFinanceiro final : public QWidget {
  Q_OBJECT

public:
  explicit TabFinanceiro(QWidget *parent);
  ~TabFinanceiro();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::TabFinanceiro *ui;
  // methods
  auto setConnections() -> void;
};
