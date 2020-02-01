#pragma once

#include "sqlquerymodel.h"

#include <QWidget>

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent = nullptr);
  ~WidgetEstoque();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlQueryModel model;
  Ui::WidgetEstoque *ui;
  // methods
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) -> bool;
  auto montaFiltro() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables(const bool checked = true) -> void;
};
