#pragma once

#include "sqlrelationaltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraConsumos;
}

class WidgetCompraConsumos final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraConsumos(QWidget *parent = nullptr);
  ~WidgetCompraConsumos();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelPedido;
  SqlRelationalTableModel modelProduto;
  Ui::WidgetCompraConsumos *ui;
  // methods
  auto desfazerConsumo(const int row) -> bool;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonDesfazerConsumo_clicked() -> void;
  auto on_tablePedido_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
