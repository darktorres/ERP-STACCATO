#pragma once

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetFinanceiroCompra;
}

class WidgetHistoricoCompra final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Compra, Financeiro };
  explicit WidgetHistoricoCompra(QWidget *parent = nullptr);
  ~WidgetHistoricoCompra();
  auto resetTables() -> void;
  auto updateTables() -> void;
  auto setTipo(const Tipo novotipo) -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  Tipo tipo;
  SqlRelationalTableModel modelViewComprasFinanceiro;
  Ui::WidgetFinanceiroCompra *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
