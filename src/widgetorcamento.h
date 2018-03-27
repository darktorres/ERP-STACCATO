#ifndef WIDGETORCAMENTO_H
#define WIDGETORCAMENTO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetOrcamento;
}

class WidgetOrcamento final : public Widget {
  Q_OBJECT

public:
  explicit WidgetOrcamento(QWidget *parent = nullptr);
  ~WidgetOrcamento();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelViewOrcamento;
  Ui::WidgetOrcamento *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_comboBoxLojas_currentIndexChanged(const int) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCriarOrc_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setPermissions() -> void;
  auto setupConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETORCAMENTO_H
