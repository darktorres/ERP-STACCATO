#ifndef WIDGETESTOQUE_H
#define WIDGETESTOQUE_H

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public Widget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent = 0);
  ~WidgetEstoque();
  bool updateTables();
  void setHasError(const bool value);

private slots:
  void on_table_activated(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);
  void on_pushButtonRelatorio_clicked();

private:
  // attributes
  bool hasError = false;
  SqlQueryModel model;
  Ui::WidgetEstoque *ui;
  // methods
  bool setupTables();
  void montaFiltro();
};

#endif // WIDGETESTOQUE_H
