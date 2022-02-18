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
  ~WidgetLogisticaRepresentacao() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelFornecedor;
  SqlTableModel modelViewLogisticaRepresentacao;
  Ui::WidgetLogisticaRepresentacao *ui;
  // methods
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_pushButtonMarcarEntregue_clicked() -> void;
  auto on_tableForn_selectionChanged() -> void;
  auto on_table_doubleClicked(const QModelIndex &index) -> void;
  auto processRows(const QModelIndexList &list, const QDate dataEntrega, const QString &recebeu) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
