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
  ~WidgetOrcamento();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QStack<int> blockingSignals;
  QTimer timer;
  SqlTableModel modelViewOrcamento;
  Ui::WidgetOrcamento *ui;
  // methods
  auto delayFiltro() -> void;
  auto fillComboBoxFollowup() -> void;
  auto fillComboBoxFornecedor() -> void;
  auto fillComboBoxLoja() -> void;
  auto fillComboBoxVendedor() -> void;
  auto montaFiltro() -> void;
  auto montaFiltroTexto() -> void;
  auto on_comboBoxLojas_currentIndexChanged() -> void;
  auto on_dateEditMes_dateChanged(const QDate) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCriarOrc_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_radioButtonProprios_toggled(bool checked) -> void;
  auto on_radioButtonTodos_toggled(bool checked) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setWidgets() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
