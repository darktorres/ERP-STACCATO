#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetEstoqueProduto;
}

class WidgetEstoqueProduto : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoqueProduto(QWidget *parent = nullptr);
  ~WidgetEstoqueProduto();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelProdutos;
  Ui::WidgetEstoqueProduto *ui;
  // methods
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto on_radioButtonEstoque_toggled(const bool checked) -> void;
  auto on_radioButtonStaccatoOFF_toggled(const bool checked) -> void;
  auto on_radioButtonTodos_toggled(const bool checked) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
