#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetConsistencia;
}

class WidgetConsistencia : public QWidget {
  Q_OBJECT

public:
  explicit WidgetConsistencia(QWidget *parent);
  ~WidgetConsistencia();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  bool modelIsSet = false;
  SqlTableModel model1;
  SqlTableModel model2;
  SqlTableModel model3;
  SqlTableModel model4;
  SqlTableModel model5;
  SqlTableModel model6;
  SqlTableModel model7;
  SqlTableModel model8;

private:
  // attributes
  Ui::WidgetConsistencia *ui;
  // methods
  auto setConnections() -> void;
  auto setupTables() -> void;
};
