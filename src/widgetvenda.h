#pragma once

#include "sqltablemodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetVenda;
}

class WidgetVenda final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetVenda(QWidget *parent);
  ~WidgetVenda() final;

  auto resetTables() -> void;
  auto setFinanceiro() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool financeiro = false;
  QStack<int> blockingSignals;
  SqlTableModel modelVenda;
  Ui::WidgetVenda *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto fillComboBoxFornecedor() -> void;
  auto fillComboBoxLoja() -> void;
  auto fillComboBoxVendedor() -> void;
  auto montaFiltro() -> void;
  auto on_comboBoxLojas_currentIndexChanged() -> void;
  auto on_dateEditDia_dateChanged() -> void;
  auto on_dateEditMes_dateChanged() -> void;
  auto on_groupBoxStatusFinanceiro_toggled(const bool enabled) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_radioButton_toggled(const bool checked) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setWidgets() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
