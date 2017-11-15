#ifndef WIDGETRELATORIO_H
#define WIDGETRELATORIO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetRelatorio;
}

class WidgetRelatorio final : public Widget {
  Q_OBJECT

public:
  explicit WidgetRelatorio(QWidget *parent = 0);
  ~WidgetRelatorio();
  bool updateTables();
  void setHasError(const bool value);

private slots:
  void on_dateEditMes_dateChanged(const QDate &);
  void on_pushButtonExcel_clicked();
  void on_tableRelatorio_entered(const QModelIndex &);
  void on_tableTotalLoja_entered(const QModelIndex &);
  void on_tableTotalVendedor_entered(const QModelIndex &);

private:
  // attributes
  bool hasError = false;
  SqlRelationalTableModel modelOrcamento;
  SqlRelationalTableModel modelRelatorio;
  SqlRelationalTableModel modelTotalLoja;
  SqlRelationalTableModel modelTotalVendedor;
  Ui::WidgetRelatorio *ui;
  // methods
  bool setupTables();
  void setFilterTotaisVendedor();
  void setFilterTotaisLoja();
  void calcularTotalGeral();
  void setFilterRelatorio();
};

#endif // WIDGETRELATORIO_H
