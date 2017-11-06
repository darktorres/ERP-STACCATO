#ifndef WIDGETORCAMENTO_H
#define WIDGETORCAMENTO_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetOrcamento;
}

class WidgetOrcamento : public QWidget {
  Q_OBJECT

public:
  explicit WidgetOrcamento(QWidget *parent = 0);
  ~WidgetOrcamento();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void montaFiltro();
  void on_comboBoxLojas_currentIndexChanged(const int);
  void on_groupBoxStatus_toggled(const bool enabled);
  void on_pushButtonCriarOrc_clicked();
  void on_pushButtonFollowup_clicked();
  void on_table_activated(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::WidgetOrcamento *ui;
  // methods
  void setupTables();
  void setPermissions();
  void setupConnections();
};

#endif // WIDGETORCAMENTO_H
