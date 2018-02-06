#ifndef WIDGETLOGISTICAENTREGAS_H
#define WIDGETLOGISTICAENTREGAS_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaEntregas;
}

class WidgetLogisticaEntregas final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaEntregas(QWidget *parent = nullptr);
  ~WidgetLogisticaEntregas();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelCalendario;
  SqlRelationalTableModel modelCarga;
  SqlRelationalTableModel modelProdutos;
  Ui::WidgetLogisticaEntregas *ui;
  // methods
  auto cancelarEntrega(const QModelIndexList &list) -> bool;
  auto confirmarEntrega(const QDateTime &dataRealEnt, const QString &entregou, const QString &recebeu) -> bool;
  auto consultarNFe(const int idNFe, const QString &xml) -> bool;
  auto on_lineEditBuscar_textChanged(const QString &text) -> void;
  auto on_pushButtonCancelarEntrega_clicked() -> void;
  auto on_pushButtonConfirmarEntrega_clicked() -> void;
  auto on_pushButtonConsultarNFe_clicked() -> void;
  auto on_pushButtonGerarNFeEntregar_clicked() -> void;
  auto on_pushButtonImprimirDanfe_clicked() -> void;
  auto on_pushButtonProtocoloEntrega_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_tableCalendario_clicked(const QModelIndex &index) -> void;
  auto on_tableCarga_clicked(const QModelIndex &index) -> void;
  auto on_tableCarga_entered(const QModelIndex &) -> void;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevEnt) -> bool;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICAENTREGAS_H
