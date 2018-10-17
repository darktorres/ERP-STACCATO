#ifndef WIDGETVENDA_H
#define WIDGETVENDA_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetVenda;
}

class WidgetVenda final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetVenda(QWidget *parent = nullptr);
  ~WidgetVenda() final;
  auto resetTables() -> void;
  auto setFinanceiro() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  // REFAC: convert this to constructor enum/bool?
  bool financeiro = false;
  SqlRelationalTableModel modelViewVenda;
  Ui::WidgetVenda *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_comboBoxLojas_currentIndexChanged(const int) -> void;
  auto on_groupBoxStatusFinanceiro_toggled(const bool enabled) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_radioButtonProprios_toggled(const bool checked) -> void;
  auto on_table_activated(const QModelIndex index) -> void;
  auto on_table_entered(const QModelIndex) -> void;
  auto setConnections() -> void;
  auto setPermissions() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETVENDA_H
