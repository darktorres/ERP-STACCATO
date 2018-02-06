#ifndef WIDGETCOMPRAGERAR_H
#define WIDGETCOMPRAGERAR_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetCompraGerar;
}

class WidgetCompraGerar final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompraGerar(QWidget *parent = nullptr);
  ~WidgetCompraGerar();
  auto updateTables() -> bool;

private:
  // attributes
  int oc = 0;
  SqlRelationalTableModel modelResumo;
  SqlRelationalTableModel modelProdutos;
  Ui::WidgetCompraGerar *ui;
  // methods
  auto calcularPreco() -> void;
  auto cancelar(const QModelIndexList &list) -> bool;
  auto gerarCompra(const QList<int> &lista, const QDateTime &dataCompra, const QDateTime &dataPrevista) -> bool;
  auto gerarExcel(const QList<int> &lista, QString &anexo, const bool isRepresentacao) -> bool;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_checkBoxMostrarSul_toggled(bool checked) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonGerarCompra_clicked() -> void;
  auto on_tableProdutos_entered(const QModelIndex &) -> void;
  auto on_tableResumo_activated(const QModelIndex &index) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETCOMPRAGERAR_H
