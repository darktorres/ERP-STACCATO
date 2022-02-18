#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetLogisticaColeta;
}

class WidgetLogisticaColeta final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaColeta(QWidget *parent);
  ~WidgetLogisticaColeta();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelColeta;
  SqlTableModel modelFornecedor;
  Ui::WidgetLogisticaColeta *ui;
  // methods
  auto cadastrar(const QModelIndexList &list, const QDate dataColeta, const QDate dataPrevReceb) -> void;
  auto cancelar(const QModelIndexList &list) -> void;
  auto montaFiltro() -> void;
  auto on_checkBoxMarcarTodos_clicked(const bool checked) -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_pushButtonMarcarColetado_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_tableForn_selectionChanged() -> void;
  auto on_table_doubleClicked(const QModelIndex &index) -> void;
  auto reagendar(const QModelIndexList &list, const QDate dataPrevColeta) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
