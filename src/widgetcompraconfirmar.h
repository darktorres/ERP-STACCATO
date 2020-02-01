#pragma once

#include "sqlrelationaltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraConfirmar;
}

class WidgetCompraConfirmar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraConfirmar(QWidget *parent = nullptr);
  ~WidgetCompraConfirmar();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewCompras;
  SqlRelationalTableModel modelResumo;
  Ui::WidgetCompraConfirmar *ui;
  // methods
  auto confirmarCompra(const QString &idCompra, const QDate &dataPrevista, const QDate &dataConf) -> bool;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonConfirmarCompra_clicked() -> void;
  auto setupTables() -> void;
  auto setConnections() -> void;
};
