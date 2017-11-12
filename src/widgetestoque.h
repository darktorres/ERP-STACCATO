#ifndef WIDGETESTOQUE_H
#define WIDGETESTOQUE_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent = 0);
  ~WidgetEstoque();
  bool updateTables();
  void setHasError(const bool value);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

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
