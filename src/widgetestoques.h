#pragma once

#include "sqlquerymodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetEstoques;
}

class WidgetEstoques : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoques(QWidget *parent);
  ~WidgetEstoques();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlQueryModel model;
  Ui::WidgetEstoques *ui;
  // methods
  auto delayFiltro() -> void;
  auto escolheFiltro() -> void;
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) -> void;
  auto getMatch() const -> QString;
  auto montaFiltro() -> void;
  auto montaFiltroContabil() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setHeaderData() -> void;
  auto setupTables() -> void;
};