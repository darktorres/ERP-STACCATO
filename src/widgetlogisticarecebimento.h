#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetLogisticaRecebimento;
}

class WidgetLogisticaRecebimento final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRecebimento(QWidget *parent);
  ~WidgetLogisticaRecebimento();

  auto resetTables() -> void;
  auto tableFornLogistica_clicked(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelViewRecebimento;
  Ui::WidgetLogisticaRecebimento *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> void;
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonMarcarRecebido_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto processRows(const QModelIndexList &list, const QDate &dataReceb, const QString &recebidoPor) -> void;
  auto reagendar(const QModelIndexList &list, const QDate &dataPrevReceb) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
