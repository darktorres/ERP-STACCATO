#pragma once

#include "sqlquerymodel.h"

#include <QWidget>

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent);
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
  auto escolheFiltro() -> void;
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) -> bool;
  auto getMatch() const -> QString;
  auto montaFiltro() -> void;
  auto montaFiltroContabil() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setHeaderData() -> void;
  auto setupTables() -> void;
};
