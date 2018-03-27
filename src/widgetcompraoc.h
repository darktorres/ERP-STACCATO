#ifndef WIDGETCOMPRAOC_H
#define WIDGETCOMPRAOC_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetCompraOC;
}

class WidgetCompraOC final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompraOC(QWidget *parent = nullptr);
  ~WidgetCompraOC();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelNFe;
  SqlRelationalTableModel modelPedido;
  SqlRelationalTableModel modelProduto;
  Ui::WidgetCompraOC *ui;
  // methods
  auto desfazerConsumo(const int row) -> bool;
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonDanfe_clicked() -> void;
  auto on_pushButtonDesfazerConsumo_clicked() -> void;
  auto on_tableNFe_entered(const QModelIndex &) -> void;
  auto on_tablePedido_clicked(const QModelIndex &index) -> void;
  auto on_tablePedido_entered(const QModelIndex &) -> void;
  auto on_tableProduto_entered(const QModelIndex &) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETCOMPRAOC_H
