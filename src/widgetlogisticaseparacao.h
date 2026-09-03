#pragma once

#include "sqlquerymodel.h"

#include <QStack>
#include <QWidget>

namespace Ui {
class WidgetLogisticaSeparacao;
}

class WidgetLogisticaSeparacao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaSeparacao(QWidget *parent);
  ~WidgetLogisticaSeparacao();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  QStack<int> blockingSignals;
  SqlQueryModel modelPedido;
  SqlQueryModel modelProdutoLocal;
  Ui::WidgetLogisticaSeparacao *ui;
  // methods
  auto buscarPedido() -> void;
  auto buscarProdutoLocal() -> void;
  auto on_pushButtonRomaneio_clicked() -> void;
  auto on_pushButtonSeparar_clicked() -> void;
  auto on_tablePedido_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableProdutoLocal_doubleClicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
