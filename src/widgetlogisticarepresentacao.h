#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetLogisticaRepresentacao;
}

class WidgetLogisticaRepresentacao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRepresentacao(QWidget *parent);
  ~WidgetLogisticaRepresentacao();
  auto resetTables() -> void;
  auto tableFornLogistica_clicked(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelViewLogisticaRepresentacao;
  Ui::WidgetLogisticaRepresentacao *ui;
  // methods
  auto delayFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonMarcarEntregue_clicked() -> void;
  auto processRows(const QModelIndexList &list, const QDate &dataEntrega, const QString &recebeu) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
