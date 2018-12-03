#ifndef WIDGETLOGISTICAENTREGAS_H
#define WIDGETLOGISTICAENTREGAS_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaEntregas;
}

class WidgetLogisticaEntregas final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaEntregas(QWidget *parent = nullptr);
  ~WidgetLogisticaEntregas();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelCalendario;
  SqlRelationalTableModel modelCarga;
  SqlRelationalTableModel modelProdutos;
  Ui::WidgetLogisticaEntregas *ui;
  // methods
  auto cancelarEntrega(const QModelIndexList &list) -> bool;
  auto confirmarEntrega(const QDateTime &dataRealEnt, const QString &entregou, const QString &recebeu) -> bool;
  auto consultarNFe(const int idNFe, const QString &xml) -> bool;
  auto montaFiltro() -> void;
  auto on_lineEditBuscar_textChanged(const QString &) -> void;
  auto on_pushButtonCancelarEntrega_clicked() -> void;
  auto on_pushButtonConfirmarEntrega_clicked() -> void;
  auto on_pushButtonConsultarNFe_clicked() -> void;
  auto on_pushButtonGerarNFeEntregar_clicked() -> void;
  auto on_pushButtonImprimirDanfe_clicked() -> void;
  auto on_pushButtonProtocoloEntrega_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_tableCalendario_clicked(const QModelIndex &index) -> void;
  auto on_tableCarga_clicked(const QModelIndex &index) -> void;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevEnt) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICAENTREGAS_H
