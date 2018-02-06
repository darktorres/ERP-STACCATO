#ifndef WIDGETLOGISTICARECEBIMENTO_H
#define WIDGETLOGISTICARECEBIMENTO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaRecebimento;
}

class WidgetLogisticaRecebimento final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRecebimento(QWidget *parent = nullptr);
  ~WidgetLogisticaRecebimento();
  auto tableFornLogistica_activated(const QString &fornecedor) -> void;
  auto updateTables() -> bool;

private:
  // attributes
  QString fornecedor;
  SqlRelationalTableModel model;
  Ui::WidgetLogisticaRecebimento *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto on_checkBoxMarcarTodos_clicked(const bool) -> void;
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonMarcarRecebido_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto processRows(const QModelIndexList &list, const QDateTime &dataReceb, const QString &recebidoPor) -> bool;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) -> bool;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICARECEBIMENTO_H
