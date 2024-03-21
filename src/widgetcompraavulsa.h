#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraAvulsa;
}

class WidgetCompraAvulsa : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraAvulsa(QWidget *parent = nullptr);
  ~WidgetCompraAvulsa();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelCompra;
  Ui::WidgetCompraAvulsa *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_groupBoxProdutos_toggled(const bool enabled) -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_tableCompra_doubleClicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
