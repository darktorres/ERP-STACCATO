#ifndef WIDGETLOGISTICACOLETA_H
#define WIDGETLOGISTICACOLETA_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaColeta;
}

class WidgetLogisticaColeta final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaColeta(QWidget *parent = nullptr);
  ~WidgetLogisticaColeta();
  auto updateTables() -> bool;
  auto tableFornLogistica_activated(const QString &fornecedor) -> void;

private:
  // attributes
  QString fornecedor;
  SqlRelationalTableModel model;
  Ui::WidgetLogisticaColeta *ui;
  // methods
  auto cadastrar(const QModelIndexList &list, const QDate &dataColeta, const QDate &dataPrevReceb) -> bool;
  auto cancelar(const QModelIndexList &list) -> bool;
  auto on_checkBoxMarcarTodos_clicked(const bool) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonMarcarColetado_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevColeta) -> bool;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICACOLETA_H
