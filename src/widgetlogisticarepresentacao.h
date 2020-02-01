#pragma once

#include "sqlrelationaltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetLogisticaRepresentacao;
}

class WidgetLogisticaRepresentacao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRepresentacao(QWidget *parent = nullptr);
  ~WidgetLogisticaRepresentacao();
  auto resetTables() -> void;
  auto tableFornLogistica_clicked(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewLogisticaRepresentacao;
  Ui::WidgetLogisticaRepresentacao *ui;
  // methods
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonMarcarEntregue_clicked() -> void;
  auto processRows(const QModelIndexList &list, const QDate &dataEntrega, const QString &recebeu) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
