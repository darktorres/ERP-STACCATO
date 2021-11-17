#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraConfirmar;
}

class WidgetCompraConfirmar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraConfirmar(QWidget *parent);
  ~WidgetCompraConfirmar();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelViewCompras;
  SqlTableModel modelResumo;
  Ui::WidgetCompraConfirmar *ui;
  // methods
  auto confirmarCompra(const QString &idCompra, const QDate dataPrevista, const QDate dataConf) -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonConfirmarCompra_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_tableResumo_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
