#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class PagamentosDia;
}

class PagamentosDia final : public QDialog {
  Q_OBJECT

public:
  explicit PagamentosDia(QWidget *parent);
  ~PagamentosDia();

  auto setFilter(const QDate date, const QString &idConta) -> void;

private:
  // attributes
  SqlTableModel modelFluxoCaixa;
  Ui::PagamentosDia *ui;
  // methods
  auto setupTables() -> void;
};
