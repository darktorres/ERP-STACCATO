#pragma once

#include "sqlrelationaltablemodel.h"

#include <QDialog>

namespace Ui {
class PagamentosDia;
}

class PagamentosDia final : public QDialog {
  Q_OBJECT

public:
  explicit PagamentosDia(QWidget *parent = nullptr);
  ~PagamentosDia();
  auto setFilter(const QDate &date, const QString &idConta) -> bool;

private:
  // attributes
  SqlRelationalTableModel modelViewFluxoCaixa;
  Ui::PagamentosDia *ui;
  // methods
  auto setupTables() -> void;
};
