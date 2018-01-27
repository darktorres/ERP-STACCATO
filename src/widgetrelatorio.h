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

private slots:
  void on_pushButtonExcel_clicked();
  void on_tableRelatorio_entered(const QModelIndex &);
  void on_tableTotalLoja_entered(const QModelIndex &);
  void on_tableTotalVendedor_entered(const QModelIndex &);

private:
  // attributes
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
  void dateEditMes_dateChanged(const QDate &);
};

#endif // WIDGETRELATORIO_H
