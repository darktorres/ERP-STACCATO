#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraGerar;
}

class WidgetCompraGerar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraGerar(QWidget *parent);
  ~WidgetCompraGerar();

  auto resetTables() -> void;
  auto updateTables() -> void;

signals:
  void finished();

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelResumo;
  SqlTableModel modelProdutos;
  Ui::WidgetCompraGerar *ui;
  // methods
  auto calcularPreco() -> void;
  auto cancelar(const QModelIndexList &list) -> void;
  auto enviarEmail(const QString &razaoSocial, const QString &anexo) -> void;
  auto gerarCompra(const QModelIndexList &list, const QDate dataCompra, const QDate dataPrevista, const int ordemCompra) -> void;
  auto gerarExcel(const QModelIndexList &list, const int ordemCompra, const bool isRepresentacao) -> QString;
  auto getDates(const QModelIndexList &list) -> std::tuple<QDate, QDate>;
  auto getOrdemCompra() -> int;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonGerarCompra_clicked() -> void;
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_tableProdutos_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableResumo_selectionChanged() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto verificaRepresentacao(const QModelIndexList &list) -> bool;
};
