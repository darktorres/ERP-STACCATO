#pragma once

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogistica;
}

class WidgetLogistica final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogistica(QWidget *parent = nullptr);
  ~WidgetLogistica();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  SqlRelationalTableModel modelViewLogistica;
  Ui::WidgetLogistica *ui;
  // methods
  auto setConnections() -> void;
  auto on_tableForn_clicked(const QModelIndex &index) -> void;
  auto on_tabWidgetLogistica_currentChanged(const int) -> void;
};
