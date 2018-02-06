#ifndef WIDGETLOGISTICAAGENDARCOLETA_H
#define WIDGETLOGISTICAAGENDARCOLETA_H

#include <QStandardItemModel>

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaAgendarColeta;
}

class WidgetLogisticaAgendarColeta final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarColeta(QWidget *parent = nullptr);
  ~WidgetLogisticaAgendarColeta();
  auto updateTables() -> bool;
  auto tableFornLogistica_activated(const QString &fornecedor) -> void;

private:
  // attributes
  QString fornecedor; // REFAC: make this not class member
  SqlRelationalTableModel modelEstoque;
  SqlRelationalTableModel modelTransp;
  SqlRelationalTableModel modelTransp2;
  Ui::WidgetLogisticaAgendarColeta *ui;
  // methods
  auto adicionarProduto(const QModelIndexList &list) -> bool;
  auto calcularPeso() -> void;
  auto montaFiltro() -> void;
  auto on_checkBoxEstoque_toggled(bool checked) -> void;
  auto on_checkBoxSul_toggled(bool checked) -> void;
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
  auto on_tableEstoque_entered(const QModelIndex &) -> void;
  auto processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga = false) -> bool;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICAAGENDARCOLETA_H
