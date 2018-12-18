#ifndef WIDGETLOGISTICACOLETA_H
#define WIDGETLOGISTICACOLETA_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaColeta;
}

class WidgetLogisticaColeta final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaColeta(QWidget *parent = nullptr);
  ~WidgetLogisticaColeta();
  auto resetTables() -> void;
  auto tableFornLogistica_clicked(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewColeta;
  Ui::WidgetLogisticaColeta *ui;
  // methods
  auto cadastrar(const QModelIndexList &list, const QDate &dataColeta, const QDate &dataPrevReceb) -> bool;
  auto cancelar(const QModelIndexList &list) -> bool;
  auto montaFiltro() -> void;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonMarcarColetado_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevColeta) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICACOLETA_H
