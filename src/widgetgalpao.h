#pragma once

#include "palletitem.h"
#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QGraphicsScene>
#include <QStack>
#include <QWidget>

namespace Ui {
class WidgetGalpao;
}

class WidgetGalpao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGalpao(QWidget *parent = nullptr);
  ~WidgetGalpao();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QStack<int> blockingSignals;
  QGraphicsScene *scene = nullptr;
  SqlTableModel modelTranspAgend;
  SqlQueryModel modelPallet;
  QHash<QString, PalletItem *> palletsHash;
  Ui::WidgetGalpao *ui;
  // methods
  auto carregarBloco(const QString &idBloco, const QString &label) -> void;
  auto carregarPallets() -> void;
  auto hidePalletFrame() -> void;
  auto on_checkBoxConteudo_toggled(bool checked) -> void;
  auto on_checkBoxCriarApagar_toggled(bool checked) -> void;
  auto on_checkBoxMover_toggled(bool checked) -> void;
  auto on_dateTimeEdit_dateChanged() -> void;
  auto on_groupBoxEdicao_toggled(const bool checked) -> void;
  auto on_itemBoxVeiculo_textChanged() -> void;
  auto on_pushButtonCriarPallet_clicked() -> void;
  auto on_pushButtonRemoverPallet_clicked() -> void;
  auto on_pushButtonSalvarMover_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_table_selectionChanged() -> void;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto salvarPallets() -> void;
  auto setConnections() -> void;
  auto setFilter() -> void;
  auto setupTables() -> void;
  auto unselectOthers() -> void;
  auto unsetConnections() -> void;
};
