#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QTimer>
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
  QTimer timer;
  QTimer timer2;
  SqlQueryModel model;
  SqlTableModel modelProdutos;
  Ui::WidgetEstoque *ui;
  // methods
  auto delayFiltro() -> void;
  auto delayFiltro2() -> void;
  auto escolheFiltro() -> void;
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) -> void;
  auto getMatch() const -> QString;
  auto montaFiltro() -> void;
  auto montaFiltro2() -> void;
  auto montaFiltroContabil() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_radioButtonEstoque_toggled(bool checked) -> void;
  auto on_radioButtonStaccatoOFF_toggled(bool checked) -> void;
  auto on_radioButtonTodos_toggled(bool checked) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setHeaderData() -> void;
  auto setupTables() -> void;
};
