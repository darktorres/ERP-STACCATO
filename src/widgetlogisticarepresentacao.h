#ifndef WIDGETLOGISTICAREPRESENTACAO_H
#define WIDGETLOGISTICAREPRESENTACAO_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaRepresentacao;
}

class WidgetLogisticaRepresentacao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRepresentacao(QWidget *parent = 0);
  ~WidgetLogisticaRepresentacao();
  bool updateTables();
  void tableFornLogistica_activated(const QString &fornecedor);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_lineEditBusca_textChanged(const QString &text);
  void on_pushButtonMarcarEntregue_clicked();
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  QString fornecedor;
  Ui::WidgetLogisticaRepresentacao *ui;
  // methods
  bool processRows(const QModelIndexList &list, const QDateTime &dataEntrega, const QString &recebeu);
  void setupTables();
};

#endif // WIDGETLOGISTICAREPRESENTACAO_H
