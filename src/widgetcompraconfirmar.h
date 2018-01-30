#ifndef WIDGETCOMPRACONFIRMAR_H
#define WIDGETCOMPRACONFIRMAR_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraConfirmar;
}

class WidgetCompraConfirmar final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraConfirmar(QWidget *parent = nullptr);
  ~WidgetCompraConfirmar();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_checkBoxMostrarSul_toggled(bool checked);
  void on_pushButtonCancelarCompra_clicked();
  void on_pushButtonConfirmarCompra_clicked();
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  SqlRelationalTableModel modelResumo;
  Ui::WidgetCompraConfirmar *ui;
  // methods
  bool cancelar(const QModelIndexList &list);
  bool confirmarCompra(const QString &idCompra, const QDateTime &dataPrevista, const QDateTime &dataConf);
  void setupTables();
};

#endif // WIDGETCOMPRACONFIRMAR_H
