#pragma once

#include "cnab.h"
#include "sqlquerymodel.h"
#include "xlsxdocument.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetFinanceiroContas;
}

class WidgetFinanceiroContas final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Nulo, Receber, Pagar };
  Q_ENUM(Tipo)

  explicit WidgetFinanceiroContas(QWidget *parent);
  ~WidgetFinanceiroContas();

  auto resetTables() -> void;
  auto setTipo(const Tipo &novoTipo) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlQueryModel model;
  SqlQueryModel modelVencidos;
  SqlQueryModel modelVencer;
  Tipo tipo = Tipo::Nulo;
  Ui::WidgetFinanceiroContas *ui;
  // methods
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto montarPagamento(const QModelIndexList &selection) -> QVector<CNAB::Pagamento>;
  auto on_dateEditVencimentoDe_dateChanged(const QDate &date) -> void;
  auto on_dateEditRealizadoDe_dateChanged(const QDate &date) -> void;
  auto on_doubleSpinBoxDe_valueChanged(const double value) -> void;
  auto on_groupBoxRealizado_toggled(const bool enabled) -> void;
  auto on_groupBoxVencimento_toggled(const bool enabled) -> void;
  auto on_pushButtonAdiantarRecebimento_clicked() -> void;
  auto on_pushButtonExcluirLancamento_clicked() -> void;
  auto on_pushButtonImportarFolhaPag_clicked() -> void;
  auto on_pushButtonInserirLancamento_clicked() -> void;
  auto on_pushButtonInserirTransferencia_clicked() -> void;
  auto on_pushButtonRemessaItau_clicked() -> void;
  auto on_pushButtonReverterPagamento_clicked() -> void;
  auto on_tableVencer_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableVencidos_doubleClicked(const QModelIndex &index) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto verificaCabecalho(QXlsx::Document &document) -> void;
};
