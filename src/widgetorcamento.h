#pragma once

#include "sqltablemodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetOrcamento;
}

class WidgetOrcamento final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetOrcamento(QWidget *parent);
  ~WidgetOrcamento() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  QStack<int> blockingSignals;
  SqlTableModel modelOrcamento;
  Ui::WidgetOrcamento *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto fillComboBoxFollowup() -> void;
  auto fillComboBoxFornecedor() -> void;
  auto fillComboBoxLoja() -> void;
  auto fillComboBoxVendedor() -> void;
  auto montaFiltro() -> void;
  auto on_comboBoxLojas_currentIndexChanged() -> void;
  auto on_dateEditMes_dateChanged() -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCriarOrc_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_radioButton_toggled(const bool checked) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setWidgets() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
