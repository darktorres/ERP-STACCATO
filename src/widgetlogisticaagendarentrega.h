#ifndef WIDGETLOGISTICAAGENDARENTREGA_H
#define WIDGETLOGISTICAAGENDARENTREGA_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaAgendarEntrega;
}

class WidgetLogisticaAgendarEntrega final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarEntrega(QWidget *parent = nullptr);
  ~WidgetLogisticaAgendarEntrega();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelTranspAtual;
  SqlRelationalTableModel modelTranspAgend;
  SqlRelationalTableModel modelVendas;
  SqlQueryModel modelViewProdutos;
  Ui::WidgetLogisticaAgendarEntrega *ui;
  // methods
  auto adicionarProduto(const QModelIndexList &list) -> bool;
  auto adicionarProdutoParcial(const int row, const int quantAgendar, const int quantTotal) -> bool;
  auto calcularDisponivel() -> void;
  auto calcularPeso() -> void;
  auto montaFiltro() -> void;
  auto on_dateTimeEdit_dateChanged(const QDate &date) -> void;
  auto on_itemBoxVeiculo_textChanged(const QString &) -> void;
  auto on_pushButtonAdicionarParcial_clicked() -> void;
  auto on_pushButtonAdicionarProduto_clicked() -> void;
  auto on_pushButtonAgendarCarga_clicked() -> void;
  auto on_pushButtonGerarNFeFutura_clicked() -> void;
  auto on_pushButtonReagendarPedido_clicked() -> void;
  auto on_pushButtonRemoverProduto_clicked() -> void;
  auto on_tableProdutos_entered(const QModelIndex &) -> void;
  auto on_tableTransp2_entered(const QModelIndex &) -> void;
  auto on_tableVendas_clicked(const QModelIndex &index) -> void;
  auto on_tableVendas_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableVendas_entered(const QModelIndex &) -> void;
  auto processRows() -> bool;
  auto quebrarConsumo(const int row, const double proporcao, const double proporcaoNovo, const int idVendaProduto) -> bool;
  auto quebrarProduto(const int row, const int quantAgendar, const int quantTotal) -> bool;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao) -> bool;
  auto setupTables() -> void;
  auto setConnections() -> void;
};

#endif // WIDGETLOGISTICAAGENDARENTREGA_H
