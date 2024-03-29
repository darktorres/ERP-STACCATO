#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraFaturar;
}

class WidgetCompraFaturar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraFaturar(QWidget *parent);
  ~WidgetCompraFaturar();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelFaturamento;
  SqlTableModel modelResumo;
  Ui::WidgetCompraFaturar *ui;
  // methods
  auto faturarRepresentacao(const QDate dataReal, const QStringList &idsCompra) -> void;
  auto montaFiltro() -> void;
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_pushButtonMarcarFaturado_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_tableResumo_clicked(const QModelIndex &index) -> void;
  auto on_table_doubleClicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
