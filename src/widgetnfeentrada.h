#pragma once

#include "sqlpaginatedmodel.h"
#include "sqlquerymodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetNfeEntrada;
}

class WidgetNfeEntrada final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfeEntrada(QWidget *parent);
  ~WidgetNfeEntrada();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool carregandoPagina = false;
  QStack<int> blockingSignals;
  SqlPaginatedModel model;
  SqlQueryModel modelResumo;
  Ui::WidgetNfeEntrada *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto ajustarGroupBoxUtilizada() -> void;
  auto inutilizar(const QVariant &idNFe) -> void;
  auto montaFiltro() -> void;
  auto onTableScrolled(const int value) -> void;
  auto on_dateEditDe_dateChanged(const QDate date) -> void;
  auto on_groupBoxMes_toggled(const bool enabled) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_groupBoxUtilizada_toggled(const bool enabled) -> void;
  auto on_pushButtonExportarExcel_clicked() -> void;
  auto on_pushButtonExportarMes_clicked() -> void;
  auto on_pushButtonExportar_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonInutilizarNFe_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
