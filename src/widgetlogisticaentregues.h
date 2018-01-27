#ifndef WIDGETLOGISTICAENTREGUES_H
#define WIDGETLOGISTICAENTREGUES_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaEntregues;
}

class WidgetLogisticaEntregues final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaEntregues(QWidget *parent = 0);
  ~WidgetLogisticaEntregues();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_pushButtonCancelar_clicked();
  void on_tableVendas_clicked(const QModelIndex &index);

private:
  // attributes
  SqlQueryModel modelProdutos;
  SqlRelationalTableModel modelVendas;
  Ui::WidgetLogisticaEntregues *ui;
  // methods
  bool cancelar(const QModelIndexList &list);
  void montaFiltro();
  void setupTables();
};

#endif // WIDGETLOGISTICAENTREGUES_H
