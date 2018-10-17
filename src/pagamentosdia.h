#ifndef PAGAMENTOSDIA_H
#define PAGAMENTOSDIA_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

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
  SqlRelationalTableModel modelViewFluxoCaixa;
  Ui::PagamentosDia *ui;
  //
  auto setupTables() -> void;
};

#endif // PAGAMENTOSDIA_H
