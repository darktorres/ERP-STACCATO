#ifndef WIDGETCOMPRAOC_H
#define WIDGETCOMPRAOC_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraOC;
}

class WidgetCompraOC final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraOC(QWidget *parent = nullptr);
  ~WidgetCompraOC();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelNFe;
  SqlRelationalTableModel modelPedido;
  SqlRelationalTableModel modelProduto;
  Ui::WidgetCompraOC *ui;
  // methods
  auto desfazerConsumo(const int row) -> bool;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonDanfe_clicked() -> void;
  auto on_pushButtonDesfazerConsumo_clicked() -> void;
  auto on_tableNFe_entered(const QModelIndex &) -> void;
  auto on_tablePedido_clicked(const QModelIndex &index) -> void;
  auto on_tablePedido_entered(const QModelIndex &) -> void;
  auto on_tableProduto_entered(const QModelIndex &) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETCOMPRAOC_H
