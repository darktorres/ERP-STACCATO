#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraResumo;
}

class WidgetCompraResumo : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraResumo(QWidget *parent);
  ~WidgetCompraResumo();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  Ui::WidgetCompraResumo *ui;
  SqlTableModel modelResumo;
  // methods
  auto setupTables() -> void;
};
