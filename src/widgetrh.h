#pragma once

#include "cnab.h"
#include "sqltablemodel.h"

#include <QWidget>

namespace QXlsx { class Document; }

namespace Ui {
class WidgetRh;
}

class WidgetRh final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetRh(QWidget *parent);
  ~WidgetRh() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  QStack<int> blockingSignals;
  SqlTableModel modelFolhaPag;
  Ui::WidgetRh *ui;
  bool isSet = false;
  // methods
  auto montaFiltro() -> void;
  auto montarPagamento(const QModelIndexList &selection) -> QVector<CNAB::Pagamento>;
  auto on_groupBoxData_toggled(const bool enabled) -> void;
  auto on_pushButtonDarBaixa_clicked() -> void;
  auto on_pushButtonImportarFolhaPag_clicked() -> void;
  auto on_pushButtonRemessaItau_clicked() -> void;
  auto preencher(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto verificaCabecalho(QXlsx::Document &xlsx) -> void;
};
