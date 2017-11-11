#ifndef WIDGETFINANCEIROFLUXOCAIXA_H
#define WIDGETFINANCEIROFLUXOCAIXA_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetFinanceiroFluxoCaixa;
}

class WidgetFinanceiroFluxoCaixa final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetFinanceiroFluxoCaixa(QWidget *parent = 0);
  ~WidgetFinanceiroFluxoCaixa();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_groupBoxCaixa1_toggled(const bool checked);
  void on_groupBoxCaixa2_toggled(const bool checked);
  void on_tableCaixa_activated(const QModelIndex &index);
  void on_tableCaixa_entered(const QModelIndex &);
  void on_tableCaixa2_activated(const QModelIndex &index);
  void on_tableCaixa2_entered(const QModelIndex &);

private:
  // attributes
  bool isReady = false;
  SqlQueryModel modelCaixa;
  SqlQueryModel modelCaixa2;
  SqlQueryModel modelFuturo;
  Ui::WidgetFinanceiroFluxoCaixa *ui;
  // methods
  void montaFiltro();
};

#endif // WIDGETFINANCEIROFLUXOCAIXA_H
