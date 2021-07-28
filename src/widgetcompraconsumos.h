#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetCompraConsumos;
}

class WidgetCompraConsumos final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraConsumos(QWidget *parent);
  ~WidgetCompraConsumos();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelPedido;
  SqlTableModel modelProduto;
  Ui::WidgetCompraConsumos *ui;
  // methods
  auto delayFiltro() -> void;
  auto desfazerConsumo(const QModelIndexList &list) -> void;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonDesfazerConsumo_clicked() -> void;
  auto on_tablePedido_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
