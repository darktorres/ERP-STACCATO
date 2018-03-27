#ifndef WIDGETNFESAIDA_H
#define WIDGETNFESAIDA_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetNfeSaida;
}

class WidgetNfeSaida final : public Widget {
  Q_OBJECT

public:
  explicit WidgetNfeSaida(QWidget *parent = nullptr);
  ~WidgetNfeSaida();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelViewNFeSaida;
  Ui::WidgetNfeSaida *ui;
  // methods
  auto atualizarNFe(const int idNFe, const QString &xml) -> bool;
  auto montaFiltro() -> bool;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCancelarNFe_clicked() -> void;
  auto on_pushButtonConsultarNFe_clicked() -> void;
  auto on_pushButtonExportar_clicked() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETNFESAIDA_H
