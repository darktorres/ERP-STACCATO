#pragma once

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraResumo;
}

class WidgetCompraResumo : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraResumo(QWidget *parent = nullptr);
  ~WidgetCompraResumo();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool modelIsSet = false;
  Ui::WidgetCompraResumo *ui;
  SqlRelationalTableModel modelResumo;
  // methods
  auto setupTables() -> void;
};
