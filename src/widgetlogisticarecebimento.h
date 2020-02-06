#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetLogisticaRecebimento;
}

class WidgetLogisticaRecebimento final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRecebimento(QWidget *parent = nullptr);
  ~WidgetLogisticaRecebimento();
  auto resetTables() -> void;
  auto tableFornLogistica_clicked(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelViewRecebimento;
  Ui::WidgetLogisticaRecebimento *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto montaFiltro() -> void;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonMarcarRecebido_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto processRows(const QModelIndexList &list, const QDate &dataReceb, const QString &recebidoPor) -> bool;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
