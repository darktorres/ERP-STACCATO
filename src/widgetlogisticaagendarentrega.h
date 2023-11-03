#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QStack>
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
  QStack<int> blockingSignals;
  QString selectedIdVenda;
  SqlQueryModel modelProdutos;
  SqlQueryModel modelVendas;
  SqlTableModel modelTranspAgend;
  SqlTableModel modelTranspAtual;
  Ui::WidgetLogisticaAgendarEntrega *ui;
  // methods
  auto adicionaProdutoNoModel(const int row, const double caixas) -> void;
  auto adicionarProduto(const QModelIndexList &list) -> void;
  auto adicionarProdutoParcial(const int row, const double caixasAgendar, const double caixasTotal, const int novoIdVendaProduto2) -> void;
  auto ajustarGroupBoxStatus() -> void;
  auto calcularDisponivel() -> void;
  auto calcularPeso() -> void;
  auto dividirCompra(const int row, const double caixasAgendar, const double caixasTotal, const int novoIdVendaProduto2) -> void;
  auto dividirConsumo(const int row, const double proporcao, const double proporcaoNovo, const int idVendaProduto2) -> void;
  auto dividirVenda(const int row, const double caixasAgendar, const double caixasTotal, const int novoIdVendaProduto2) -> void;
  auto filtroProdutos() -> void;
  auto montaFiltro() -> void;
  auto montarLike(QString textoBusca) -> QString;
  auto on_dateTimeEdit_dateChanged(const QDate date) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_itemBoxVeiculo_textChanged() -> void;
  auto on_pushButtonAdicionarParcial_clicked() -> void;
  auto on_pushButtonAdicionarProduto_clicked() -> void;
  auto on_pushButtonAgendarCarga_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonGerarNFeFutura_clicked() -> void;
  auto on_pushButtonImportarNFe_clicked() -> void;
  auto on_pushButtonMapa_clicked() -> void;
  auto on_pushButtonReagendarPedido_clicked() -> void;
  auto on_pushButtonRemoverProduto_clicked() -> void;
  auto on_tableProdutos_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableTranspAgend_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableTranspAtual_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableVendas_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableVendas_selectionChanged() -> void;
  auto processRows() -> void;
  auto reagendar(const QModelIndexList &list, const QDate dataPrev, const QString &observacao) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
