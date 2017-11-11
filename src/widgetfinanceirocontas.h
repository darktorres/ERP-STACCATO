#ifndef WIDGETFINANCEIROCONTAS_H
#define WIDGETFINANCEIROCONTAS_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetFinanceiroContas;
}

class WidgetFinanceiroContas final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Nulo, Receber, Pagar };
  explicit WidgetFinanceiroContas(QWidget *parent = 0);
  ~WidgetFinanceiroContas();
  bool updateTables();
  void setTipo(const Tipo &value);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_dateEditDe_dateChanged(const QDate &date);
  void on_doubleSpinBoxDe_valueChanged(const double value);
  void on_groupBoxData_toggled(const bool enabled);
  void on_pushButtonAdiantarRecebimento_clicked();
  void on_pushButtonExcluirLancamento_clicked();
  void on_pushButtonInserirLancamento_clicked();
  void on_pushButtonInserirTransferencia_clicked();
  void on_table_activated(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);
  void on_tableVencer_doubleClicked(const QModelIndex &index);
  void on_tableVencer_entered(const QModelIndex &);
  void on_tableVencidos_doubleClicked(const QModelIndex &index);
  void on_tableVencidos_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  SqlQueryModel modelVencidos;
  SqlQueryModel modelVencer;
  Tipo tipo = Tipo::Nulo;
  Ui::WidgetFinanceiroContas *ui;
  // methods
  void makeConnections();
  void montaFiltro();
  void setupTables();
};

#endif // WIDGETFINANCEIROCONTAS_H
