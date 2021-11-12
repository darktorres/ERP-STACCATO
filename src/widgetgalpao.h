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

  friend class GraphicsView;

public:
  explicit WidgetGalpao(QWidget *parent);
  ~WidgetGalpao();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isDirty = false;
  bool isSet = false;
  bool modelIsSet = false;
  PalletItem *selectedIdBloco = nullptr;
  QGraphicsScene *scene = nullptr;
  //  QHash<QString, PalletItem *> palletsHash;
  QStack<int> blockingSignals;
  SqlQueryModel modelPallet;
  SqlTableModel modelTranspAgend;
  Ui::WidgetGalpao *ui;
  // methods
  auto atualizarPallet(PalletItem *pallet) -> void;
  auto carregarBloco() -> void;
  auto carregarPallets() -> void;
  auto inserirPallet(PalletItem *pallet) -> void;
  auto on_checkBoxConteudo_toggled(bool checked) -> void;
  auto on_checkBoxCriarPallet_toggled(bool checked) -> void;
  auto on_checkBoxEdicao_toggled(const bool checked) -> void;
  auto on_checkBoxMoverPallet_toggled(bool checked) -> void;
  auto on_comboBoxPalletAtual_currentTextChanged(const QString &text) -> void;
  auto on_dateTimeEdit_dateChanged() -> void;
  auto on_itemBoxVeiculo_textChanged() -> void;
  auto on_lineEditMoverParaPallet_textChanged(const QString &text) -> void;
  auto on_lineEditNomePallet_textChanged(const QString &text) -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonMover_clicked() -> void;
  auto on_pushButtonRemoverPallet_clicked() -> void;
  auto on_pushButtonSalvarPallets_clicked() -> void;
  auto on_table_selectionChanged() -> void;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto salvarPallets() -> void;
  auto setConnections() -> void;
  auto setFilter() -> void;
  auto setupTables() -> void;
  auto unselectBloco() -> void;
  auto unselectOthers() -> void;
  auto unsetConnections() -> void;
};
