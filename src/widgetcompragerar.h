#ifndef WIDGETCOMPRAGERAR_H
#define WIDGETCOMPRAGERAR_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraGerar;
}

class WidgetCompraGerar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraGerar(QWidget *parent = nullptr);
  ~WidgetCompraGerar();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelResumo;
  SqlRelationalTableModel modelProdutos;
  Ui::WidgetCompraGerar *ui;
  // methods
  auto calcularPreco() -> void;
  auto cancelar(const QModelIndexList &list) -> bool;
  auto enviarEmail(const QString &razaoSocial, const QString &anexo) -> void;
  auto gerarCompra(const QList<QModelIndex> &list, const QDateTime &dataCompra, const QDateTime &dataPrevista, const int oc) -> bool;
  auto gerarExcel(const QList<QModelIndex> &list, const int oc, const bool isRepresentacao) -> std::optional<QString>;
  auto getDates(const QList<QModelIndex> &list) -> std::optional<std::tuple<QDateTime, QDateTime>>;
  auto getOrdemCompra() -> std::optional<int>;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonGerarCompra_clicked() -> void;
  auto on_tableResumo_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto verificaRepresentacao(const QList<QModelIndex> &list) -> std::optional<bool>;
};

#endif // WIDGETCOMPRAGERAR_H
