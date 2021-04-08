#pragma once

#include "sqltablemodel.h"

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
  bool modelIsSet = false;
  bool financeiro = false;
  QTimer timer;
  SqlTableModel modelViewVenda;
  Ui::WidgetVenda *ui;
  // methods
  auto delayFiltro() -> void;
  auto fillComboBoxVendedor() -> void;
  auto listarLojas() -> void;
  auto montaFiltro() -> void;
  auto on_comboBoxLojas_currentIndexChanged() -> void;
  auto on_groupBoxStatusFinanceiro_toggled(const bool enabled) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_table_activated(const QModelIndex index) -> void;
  auto setComboBoxFornecedores() -> void;
  auto setConnections() -> void;
  auto setPermissions() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
