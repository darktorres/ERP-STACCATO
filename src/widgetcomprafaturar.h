#ifndef WIDGETCOMPRAFATURAR_H
#define WIDGETCOMPRAFATURAR_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraFaturar;
}

class WidgetCompraFaturar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraFaturar(QWidget *parent = nullptr);
  ~WidgetCompraFaturar();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  //  void on_checkBoxRepresentacao_toggled(bool checked);
  void on_pushButtonCancelarCompra_clicked();
  void on_pushButtonMarcarFaturado_clicked();
  void on_pushButtonReagendar_clicked();
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  SqlRelationalTableModel modelResumo;
  Ui::WidgetCompraFaturar *ui;
  // methods
  bool cancelar(const QModelIndexList &list);
  bool faturarCompra(const QDateTime &dataReal, const QStringList &idsCompra);
  void setupTables();
  void montaFiltro();
};

#endif // WIDGETCOMPRAFATURAR_H
