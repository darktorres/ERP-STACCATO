#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetLogisticaAgendarEntrega;
}

class WidgetLogisticaAgendarEntrega final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarEntrega(QWidget *parent);
  ~WidgetLogisticaAgendarEntrega();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  QString selectedIdVenda;
  SqlTableModel modelTranspAtual;
  SqlTableModel modelTranspAgend;
  SqlQueryModel modelVendas;
  SqlQueryModel modelProdutos;
  Ui::WidgetLogisticaAgendarEntrega *ui;
  // methods
  auto adicionaProdutoNoModel(const int row, const double caixas) -> void;
  auto adicionarProduto(const QModelIndexList &list) -> void;
  auto adicionarProdutoParcial(const int row, const double caixasAgendar, const double caixasTotal, const int novoIdVendaProduto2) -> void;
  auto calcularDisponivel() -> void;
  auto calcularPeso() -> void;
  auto delayFiltro() -> void;
  auto dividirCompra(const int row, const double caixasAgendar, const double caixasTotal, const int novoIdVendaProduto2) -> void;
  auto dividirConsumo(const int row, const double proporcao, const double proporcaoNovo, const int idVendaProduto2) -> void;
  auto dividirVenda(const int row, const double caixasAgendar, const double caixasTotal, const int novoIdVendaProduto2) -> void;
  auto filtroProdutos() -> void;
  auto montaFiltro() -> void;
  auto on_dateTimeEdit_dateChanged(const QDate &date) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_itemBoxVeiculo_textChanged(const QString &) -> void;
  auto on_pushButtonAdicionarParcial_clicked() -> void;
  auto on_pushButtonAdicionarProduto_clicked() -> void;
  auto on_pushButtonAgendarCarga_clicked() -> void;
  auto on_pushButtonGerarNFeFutura_clicked() -> void;
  auto on_pushButtonImportarNFe_clicked() -> void;
  auto on_pushButtonReagendarPedido_clicked() -> void;
  auto on_pushButtonRemoverProduto_clicked() -> void;
  auto on_tableVendas_clicked(const QModelIndex &index) -> void;
  auto on_tableVendas_doubleClicked(const QModelIndex &index) -> void;
  auto processRows() -> void;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
