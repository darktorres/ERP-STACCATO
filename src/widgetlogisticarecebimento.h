#ifndef WIDGETLOGISTICARECEBIMENTO_H
#define WIDGETLOGISTICARECEBIMENTO_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaRecebimento;
}

class WidgetLogisticaRecebimento final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRecebimento(QWidget *parent = nullptr);
  ~WidgetLogisticaRecebimento();
  auto resetTables() -> void;
  auto tableFornLogistica_activated(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewRecebimento;
  Ui::WidgetLogisticaRecebimento *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto montaFiltro() -> void;
  auto on_checkBoxMarcarTodos_clicked(const bool) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonMarcarRecebido_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto processRows(const QModelIndexList &list, const QDateTime &dataReceb, const QString &recebidoPor) -> bool;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICARECEBIMENTO_H
