#ifndef WIDGETLOGISTICACAMINHAO_H
#define WIDGETLOGISTICACAMINHAO_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaCaminhao;
}

class WidgetLogisticaCaminhao : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaCaminhao(QWidget *parent = 0);
  ~WidgetLogisticaCaminhao();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_table_clicked(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel modelCaminhao;
  SqlRelationalTableModel modelCarga;
  Ui::WidgetLogisticaCaminhao *ui;
  // methods
  void setupTables();
};

#endif // WIDGETLOGISTICACAMINHAO_H
