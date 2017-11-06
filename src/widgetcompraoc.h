#ifndef WIDGETCOMPRAOC_H
#define WIDGETCOMPRAOC_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraOC;
}

class WidgetCompraOC : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraOC(QWidget *parent = 0);
  ~WidgetCompraOC();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_lineEditBusca_textChanged(const QString &text);
  void on_pushButtonDanfe_clicked();
  void on_pushButtonDesfazerConsumo_clicked();
  void on_tableNFe_entered(const QModelIndex &);
  void on_tablePedido_clicked(const QModelIndex &index);
  void on_tablePedido_entered(const QModelIndex &);
  void on_tableProduto_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel modelNFe;
  SqlRelationalTableModel modelPedido;
  SqlRelationalTableModel modelProduto;
  Ui::WidgetCompraOC *ui;
  // methods
  bool desfazerConsumo(const QModelIndexList &list);
  void setupTables();
};

#endif // WIDGETCOMPRAOC_H
