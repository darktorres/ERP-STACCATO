#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetFinanceiroCompra;
}

class WidgetFinanceiroCompra final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetFinanceiroCompra(QWidget *parent);
  ~WidgetFinanceiroCompra();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel model;
  Ui::WidgetFinanceiroCompra *ui;
  // methods
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
