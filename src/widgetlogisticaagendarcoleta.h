#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetLogisticaAgendarColeta;
}

class WidgetLogisticaAgendarColeta final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarColeta(QWidget *parent);
  ~WidgetLogisticaAgendarColeta();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelEstoque;
  SqlTableModel modelFornecedor;
  SqlTableModel modelTranspAgend;
  SqlTableModel modelTranspAtual;
  Ui::WidgetLogisticaAgendarColeta *ui;
  // methods
  auto adicionarProduto(const QModelIndexList &list) -> void;
  auto calcularPeso() -> void;
  auto montaFiltro() -> void;
  auto on_checkBoxEstoque_toggled() -> void;
  auto on_dateTimeEdit_dateChanged(const QDate date) -> void;
  auto on_itemBoxVeiculo_textChanged() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonAdicionarProduto_clicked() -> void;
  auto on_pushButtonAgendarColeta_clicked() -> void;
  auto on_pushButtonCancelarCarga_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_pushButtonMontarCarga_clicked() -> void;
  auto on_pushButtonRemoverProduto_clicked() -> void;
  auto on_tableEstoque_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableForn_selectionChanged() -> void;
  auto processRows(const QModelIndexList &list, const QDate dataPrevColeta) -> void;
  auto processRows(const QModelIndexList &list, const QDate dataReceb, const QString &recebidoPor) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
