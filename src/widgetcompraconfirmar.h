#ifndef WIDGETCOMPRACONFIRMAR_H
#define WIDGETCOMPRACONFIRMAR_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetCompraConfirmar;
}

class WidgetCompraConfirmar final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompraConfirmar(QWidget *parent = nullptr);
  ~WidgetCompraConfirmar();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelViewCompras;
  SqlRelationalTableModel modelResumo;
  Ui::WidgetCompraConfirmar *ui;
  // methods
  auto cancelar(const int row) -> bool;
  auto confirmarCompra(const QString &idCompra, const QDateTime &dataPrevista, const QDateTime &dataConf) -> bool;
  auto on_checkBoxMostrarSul_toggled(bool checked) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonConfirmarCompra_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETCOMPRACONFIRMAR_H
