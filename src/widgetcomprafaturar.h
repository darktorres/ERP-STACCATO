#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraFaturar;
}

class WidgetCompraFaturar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraFaturar(QWidget *parent = nullptr);
  ~WidgetCompraFaturar();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelViewFaturamento;
  SqlTableModel modelResumo;
  Ui::WidgetCompraFaturar *ui;
  // methods
  auto faturarRepresentacao(const QDate &dataReal, const QStringList &idsCompra) -> bool;
  auto montaFiltro() -> void;
  auto on_checkBoxRepresentacao_toggled(bool checked) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonMarcarFaturado_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto setupTables() -> void;
  auto setConnections() -> void;
};
