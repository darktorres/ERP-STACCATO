#pragma once

#include <QWidget>

namespace Ui {
class WidgetFinanceiro;
}

class WidgetFinanceiro final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetFinanceiro(QWidget *parent);
  ~WidgetFinanceiro();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetFinanceiro *ui;
  // methods
  auto setConnections() -> void;
};
