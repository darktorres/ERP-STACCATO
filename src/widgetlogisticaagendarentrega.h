#ifndef WIDGETLOGISTICAAGENDARENTREGA_H
#define WIDGETLOGISTICAAGENDARENTREGA_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

namespace Ui {
class WidgetLogisticaAgendarEntrega;
}

class WidgetLogisticaAgendarEntrega : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarEntrega(QWidget *parent = 0);
  ~WidgetLogisticaAgendarEntrega();
  bool updateTables();

signals:
  void errorSignal(const QString &error);

private slots:
  void on_dateTimeEdit_dateChanged(const QDate &date);
  void on_itemBoxVeiculo_textChanged(const QString &);
  void on_pushButtonAdicionarParcial_clicked();
  void on_pushButtonAdicionarProduto_clicked();
  void on_pushButtonAgendarCarga_clicked();
  void on_pushButtonReagendarPedido_clicked();
  void on_pushButtonRemoverProduto_clicked();
  void on_tableProdutos_entered(const QModelIndex &);
  void on_tableTransp2_entered(const QModelIndex &);
  void on_tableVendas_clicked(const QModelIndex &index);
  void on_tableVendas_doubleClicked(const QModelIndex &index);
  void on_tableVendas_entered(const QModelIndex &);

private:
  // attributes
  QString error;
  //  SqlTableModel modelConsumo;
  SqlTableModel modelProdutos;
  SqlTableModel modelTransp;
  SqlTableModel modelTransp2;
  SqlTableModel modelVendas;
  SqlQueryModel modelViewProdutos;
  Ui::WidgetLogisticaAgendarEntrega *ui;
  // methods
  bool adicionarProduto(const QModelIndexList &list);
  bool adicionarProdutoParcial(const int row);
  bool processRows();
  bool quebrarProduto(const int row, const int quantAgendar, const int quantTotal);
  void calcularDisponivel();
  void calcularPeso();
  void montaFiltro();
  void setupTables();
  bool reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao);
};

#endif // WIDGETLOGISTICAAGENDARENTREGA_H
