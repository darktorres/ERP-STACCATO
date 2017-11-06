#ifndef WIDGETCOMPRAGERAR_H
#define WIDGETCOMPRAGERAR_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraGerar;
}

class WidgetCompraGerar : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraGerar(QWidget *parent = 0);
  ~WidgetCompraGerar();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void calcularPreco();
  void on_checkBoxMarcarTodos_clicked(const bool checked);
  void on_checkBoxMostrarSul_toggled(bool checked);
  void on_pushButtonCancelarCompra_clicked();
  void on_pushButtonGerarCompra_clicked();
  void on_tableResumo_activated(const QModelIndex &index);
  void on_tableProdutos_entered(const QModelIndex &);

private:
  // attributes
  int oc = 0;
  SqlRelationalTableModel modelResumo;
  SqlRelationalTableModel modelProdutos;
  Ui::WidgetCompraGerar *ui;
  // methods
  bool cancelar(const QModelIndexList &list);
  bool gerarCompra(const QList<int> &lista, const QDateTime &dataCompra, const QDateTime &dataPrevista);
  bool gerarExcel(const QList<int> &lista, QString &anexo, const bool isRepresentacao);
  void setupTables();
};

#endif // WIDGETCOMPRAGERAR_H
