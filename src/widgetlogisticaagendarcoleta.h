#pragma once

#include "sqltablemodel.h"

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
  auto tableFornLogistica_clicked(const QString &fornecedor) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QString fornecedor;
  SqlTableModel modelEstoque;
  SqlTableModel modelTranspAtual;
  SqlTableModel modelTranspAgend;
  Ui::WidgetLogisticaAgendarColeta *ui;
  // methods
  auto adicionarProduto(const QModelIndexList &list) -> void;
  auto calcularPeso() -> void;
  auto montaFiltro() -> void;
  auto on_checkBoxEstoque_toggled() -> void;
  auto on_dateTimeEdit_dateChanged(const QDate &date) -> void;
  auto on_itemBoxVeiculo_textChanged(const QString &) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonAdicionarProduto_clicked() -> void;
  auto on_pushButtonAgendarColeta_clicked() -> void;
  auto on_pushButtonCancelarCarga_clicked() -> void;
  auto on_pushButtonDanfe_clicked() -> void;
  auto on_pushButtonMontarCarga_clicked() -> void;
  auto on_pushButtonRemoverProduto_clicked() -> void;
  auto on_pushButtonVenda_clicked() -> void;
  auto processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga = false) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
