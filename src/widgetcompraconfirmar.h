#ifndef WIDGETCOMPRACONFIRMAR_H
#define WIDGETCOMPRACONFIRMAR_H

#include <QWidget>

#include "sqltablemodel.h"

namespace Ui {
class WidgetCompraConfirmar;
}

class WidgetCompraConfirmar : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraConfirmar(QWidget *parent = 0);
  ~WidgetCompraConfirmar();
  bool updateTables();

signals:
  void errorSignal(const QString &error);

private slots:
  void on_pushButtonCancelarCompra_clicked();
  void on_pushButtonConfirmarCompra_clicked();
  void on_table_entered(const QModelIndex &);
  void on_checkBoxMostrarSul_toggled(bool checked);

private:
  // attributes
  QString error;
  SqlTableModel model;
  SqlTableModel modelResumo;
  Ui::WidgetCompraConfirmar *ui;
  // methods
  void setupTables();
  bool confirmarCompra(const QString &idCompra, const QDateTime &dataPrevista, const QDateTime &dataConf);
  bool cancelar(const QModelIndexList &list);
};

#endif // WIDGETCOMPRACONFIRMAR_H
