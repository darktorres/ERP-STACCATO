#ifndef WIDGETESTOQUE_H
#define WIDGETESTOQUE_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent = 0);
  ~WidgetEstoque();
  bool updateTables();

signals:
  void errorSignal(const QString &error);

private slots:
  void on_table_activated(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);
  void on_pushButtonRelatorio_clicked();

private:
  // attributes
  SqlQueryModel model;
  Ui::WidgetEstoque *ui;
  // methods
  void setupTables();
  void montaFiltro();
};

#endif // WIDGETESTOQUE_H
